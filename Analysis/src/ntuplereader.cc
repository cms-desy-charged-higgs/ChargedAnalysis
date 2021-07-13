#include <ChargedAnalysis/Analysis/include/ntuplereader.h>

bool Bigger(const float& v1, const float& v2){return v1 != -999 and v1 >= v2;}
bool BiggerAbs(const float& v1, const float& v2){return v1 != -999 and std::abs(v1) >= v2;}
bool Smaller(const float& v1, const float& v2){return v1 != -999 and v1 <= v2;}
bool SmallerAbs(const float& v1, const float& v2){return v1 != -999 and std::abs(v1) <= v2;}
bool Equal(const float& v1, const float& v2){return v1 != -999 and v1 == v2;}
bool EqualAbs(const float& v1, const float& v2){return v1 != -999 and std::abs(v1) == v2;}

//Constructor
NTupleReader::NTupleReader(){}

NTupleReader::NTupleReader(const std::shared_ptr<TTree>& inputTree, const int& era) : inputTree(inputTree), era(era) {
    pt::json_parser::read_json(StrUtil::Merge(std::getenv("CHDIR"), "/ChargedAnalysis/Analysis/data/particle.json"), partInfo);
    pt::json_parser::read_json(StrUtil::Merge(std::getenv("CHDIR"), "/ChargedAnalysis/Analysis/data/function.json"), funcInfo);
}

//Read out vector like branch at index idx
float NTupleReader::GetEntry(TLeaf* leaf, const int& idx){
    if(idx == -1) return RUtil::GetEntry<float>(leaf, entry);

    try{
        return RUtil::GetVecEntry<float>(leaf, entry).at(idx);
    }

    catch(...){
        return -999.;
    }
}

//Read out vector like branch at index idx with wished WP
float NTupleReader::GetEntryWithWP(TLeaf* leaf, std::vector<std::shared_ptr<CompiledFunc>>& cuts, const int& idx, const std::size_t& hash){
    const std::vector<float> data = RUtil::GetVecEntry<float>(leaf, entry);
    if(idx >= data.size()) return -999.;

    //Check if idx already calculated for this particle
    if(hash != 0){
        if(idxCache.count(hash) and idxCache.at(hash).count(idx)) return data.at(idxCache.at(hash).at(idx));
    }

    int wpIdx = -1, counter = -1;

    //Loop over all entries in collection
    for(std::size_t i = 0; i < data.size(); ++i){
        bool passed = true;

        //Check if all id cuts are passed
        for(std::shared_ptr<CompiledFunc>& cut : cuts){
            if(i == 0) cut->Reset();

            if(passed) passed = passed && cut->GetPassed();
            cut->Next();
        }

        if(passed) ++counter;

        if(counter == idx){
            wpIdx = i;
            break;
        }
    }

    if(wpIdx != -1){
        idxCache[hash][idx] = wpIdx;
        return data.at(wpIdx);
    }
  
    else return -999.;
}

std::shared_ptr<CompiledFunc> NTupleReader::compileBranchReading(pt::ptree& func, pt::ptree& part){
    std::shared_ptr<CompiledFunc> bindedFunc;
    
    //Replace 
    if(part.get_optional<std::string>("axis-name") and func.get_optional<std::string>("axis-name")){
        func.put("axis-name", StrUtil::Replace(func.get<std::string>("axis-name"), "[P]", part.get<std::string>("axis-name")));
        func.put("hist-name", StrUtil::Replace(func.get<std::string>("hist-name"), "[P]", part.get<std::string>("hist-name")));
    }

    if(part.get_optional<std::string>("name")){
        func.put("branch", StrUtil::Replace(func.get<std::string>("branch"), "[P]", part.get<std::string>("name")));
    }

    if(func.get_child_optional("values")){
        std::vector<std::string> values = NTupleReader::GetInfo(func.get_child("values"), false);

        for(const std::string& value : values){
            func.put("axis-name", StrUtil::Replace(func.get<std::string>("axis-name"), "[V]", value));
            func.put("hist-name", StrUtil::Replace(func.get<std::string>("hist-name"), "[V]", value));
            func.put("branch", StrUtil::Replace(func.get<std::string>("branch"), "[V]", value));
        }
    }

    std::string branchName = func.get<std::string>("branch");

    //Compile vector branch with WP
    if(part.get_optional<int>("index") && (part.get_child_optional("identification") or part.get_child_optional("requirements"))){
        std::vector<std::shared_ptr<CompiledFunc>> cuts;

        //Make part without requirements so functions can be used as iterators
        pt::ptree pNoWP = part; 
        if(pNoWP.get_optional<int>("index")) pNoWP.put("index", -1);
        pNoWP.put("hash", 0);
        pNoWP.erase("requirements");
        pNoWP.erase("identification");

        //Loop over all needed branches and create bool lambda checking the WP criteria
        if(part.get_child_optional("identification")){
            for(const std::string& idName : NTupleReader::GetInfo(part.get_child("identification"))){
                //Get ID branch as iterator
                pt::ptree funcID;
                funcID.put("branch", idName);

                std::shared_ptr<CompiledFunc> cutFunc = this->compileBranchReading(funcID, pNoWP);
                bool checkAbs = part.get_optional<bool>("identification." + idName + ".absolute-values") ? true : false;

                //Check which operation to do
                if(part.get<std::string>("identification." + idName + ".wp.compare") == "=="){
                    cutFunc->AddCut(checkAbs? &EqualAbs : &Equal, part.get<float>("identification." + idName + ".wp.value"));
                }

                else if(part.get<std::string>("identification." + idName + ".wp.compare") == ">="){
                    cutFunc->AddCut(checkAbs? &BiggerAbs : &Bigger, part.get<float>("identification." + idName + ".wp.value"));
                }

                else if(part.get<std::string>("identification." + idName + ".wp.compare") == "<="){
                    cutFunc->AddCut(checkAbs? &SmallerAbs : &Smaller, part.get<float>("identification." + idName + ".wp.value"));
                }

                else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown cut operator: '", part.get<std::string>("identification." + idName + ".wp.compare"), "'!"));

                cuts.push_back(cutFunc);
            }
        }

        //Loop over all other required cuts as bool lambda
        if(part.get_child_optional("requirements")){
            for(const std::string& fAlias : NTupleReader::GetInfo(part.get_child("requirements"))){
                std::shared_ptr<CompiledFunc> cutFunc;
                std::vector<pt::ptree> parts;

                std::string fName = NTupleReader::GetName(funcInfo, fAlias);

                if(fName == "") throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown function alias: '", fAlias, "'!"));

                pt::ptree fInfo = funcInfo.get_child(fName);

                //Loop over all particles needed by the cut
                for(const std::string& pAlias : NTupleReader::GetInfo(part.get_child("requirements." + fAlias + ".particles"))){
                    //Use particle given by the user
                    if(pAlias == "THIS") parts.push_back(pNoWP);

                    //Use external defined particle
                    else{
                        std::string pInfoPath;

                        //Check if there are channel specific particles
                        std::vector<std::string> chanPrefix = NTupleReader::GetInfo(part.get_child(StrUtil::Join(".", "requirements", fAlias, "particles", pAlias)));

                        if(chanPrefix.at(0) != ""){
                            for(const std::string& chan : chanPrefix){
                                if(!StrUtil::Find(inputTree->GetName(), chan).empty()) pInfoPath = StrUtil::Join(".", "requirements", fAlias, "particles", pAlias, chan);
                            }
                        }

                        else pInfoPath = StrUtil::Join(".", "requirements", fAlias, "particles", pAlias);

                        //Vector with partAlias, part index and part WP
                        std::vector<std::string> pInfo = NTupleReader::GetInfo(part.get_child(pInfoPath), false);
                        std::string pName = NTupleReader::GetName(partInfo, pInfo.at(0));

                        if(pName == "") throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown particle alias: '", pInfo.at(0), "'!"));

                        //Particle configuration
                        parts.push_back(partInfo.get_child(pName));
                        if(!parts.back().get_optional<bool>("non-vector")) parts.back().put("index", std::atoi(pInfo.at(1).c_str()) - 1);
                        parts.back().put("name", parts.back().get_optional<std::string>("branch-prefix") ? parts.back().get<std::string>("branch-prefix") : pName);
                        parts.back().put("hash", std::hash<std::string>()(parts.back().get<std::string>("alias") + pInfo.at(2)));

                        //Set proper cuts for WP of particles if needed
                        if(parts.back().get_child_optional("identification") and pInfo.at(2) != ""){
                            for(const std::string& idName : NTupleReader::GetInfo(parts.back().get_child("identification"))){
                                if(parts.back().get_optional<float>(StrUtil::Join(".", "identification", idName, "WP", pInfo.at(2), "value", era))){
                                    parts.back().put(StrUtil::Merge("identification.", idName, ".wp.value"), parts.back().get<float>(StrUtil::Join(".", "identification", idName, "WP", pInfo.at(2), "value", era)));
                                }

                                else{
                                    parts.back().put(StrUtil::Merge("identification.", idName, ".wp.value"), parts.back().get<float>(StrUtil::Join(".", "identification", idName, "WP", pInfo.at(2), "value")));
                                }

                                parts.back().put(StrUtil::Merge("identification.", idName, ".wp.compare"), parts.back().get<std::string>(StrUtil::Join(".", "identification", idName, "WP", pInfo.at(2), "compare")));
                                
                            }
                        }

                        else parts.back().erase("identification");
                    }
                }

                if(fInfo.get_child_optional("branch")){
                    cutFunc = this->compileBranchReading(fInfo, pNoWP);
                }

                else if(fInfo.get_child_optional("need")){
                    cutFunc = this->compileCustomFunction(fInfo, parts);
                }

                else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Function '", fAlias, "' has neither 'branch' nor 'need' configured!"));

               //Check which operation to do
                if(part.get<std::string>("requirements." + fAlias + ".compare") == "=="){
                    cutFunc->AddCut(&Equal, part.get<float>("requirements." + fAlias + ".cut"));
                }

                else if(part.get<std::string>("requirements." + fAlias + ".compare") == ">="){
                    cutFunc->AddCut(&Bigger, part.get<float>("requirements." + fAlias + ".cut"));
                }

                else if(part.get<std::string>("requirements." + fAlias + ".compare") == "<="){
                    cutFunc->AddCut(&Smaller, part.get<float>("requirements." + fAlias + ".cut"));
                }

                else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown cut operator: '", part.get<std::string>("requirements." + fAlias + ".compare"), "'!"));

                cuts.push_back(cutFunc);
            }
        }

        //Bind everything together
        bindedFunc = std::make_shared<CompiledFunc>(&NTupleReader::GetEntryWithWP, RUtil::Get<TLeaf>(inputTree.get(), branchName), cuts, part.get<int>("index"), part.get<std::size_t>("hash"), part.get<int>("index") == -1);

    }

    //Compile vector like branch
    else if(part.get_optional<int>("index")){
        bindedFunc = std::make_shared<CompiledFunc>(&NTupleReader::GetEntry, RUtil::Get<TLeaf>(inputTree.get(), branchName), part.get<int>("index"), part.get<int>("index") == -1);
    }

    //Compile non-vector like branch
    else{
        bindedFunc = std::make_shared<CompiledFunc>(&NTupleReader::GetEntry, RUtil::Get<TLeaf>(inputTree.get(), branchName), -1);
    }

    return bindedFunc;
}

std::shared_ptr<CompiledFunc> NTupleReader::compileCustomFunction(pt::ptree& func, std::vector<pt::ptree>& parts){
    std::shared_ptr<CompiledFunc> bindedFunc;
    std::map<std::string, std::shared_ptr<CompiledFunc>> subFuncs;

    //Get function values
    std::vector<std::string> values;
    if(func.get_child_optional("values")) values = NTupleReader::GetInfo(func.get_child("values"), false);

    //Label/axis name
    for(const pt::ptree part: parts){
        func.put("axis-name", StrUtil::Replace(func.get<std::string>("axis-name"), "[P]", part.get<std::string>("axis-name")));
        func.put("hist-name", StrUtil::Replace(func.get<std::string>("hist-name"), "[P]", part.get<std::string>("hist-name")));
    }

    for(const std::string value: values){
        func.put("axis-name", StrUtil::Replace(func.get<std::string>("axis-name"), "[V]", value));
        func.put("hist-name", StrUtil::Replace(func.get<std::string>("hist-name"), "[V]", value));
    }

    //Loop over all needed subfunctions/particles
    for(const std::string& fAlias: NTupleReader::GetInfo(func.get_child("need"))){
        //Check if sub function configured
        std::string fName = NTupleReader::GetName(funcInfo, fAlias);

        if(fName == "") throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown function alias: '", fAlias, "'!"));

        //Loop over all info collections with particles/values needed
        for(const std::pair<const std::string, pt::ptree> info : func.get_child("need." + fAlias)){
            pt::ptree subFunc = funcInfo.get_child(fName);
            std::string fKey = fAlias;
            pt::ptree part;

            std::vector<std::string> aliases = NTupleReader::GetInfo(info.second);
            std::vector<std::string> indeces = NTupleReader::GetInfo(info.second, false);

            for(int i = 0; i < aliases.size(); ++i){
                int idx = indeces.at(i) != "" ? std::atoi(indeces.at(i).c_str()) - 1 : 0;

                if(!StrUtil::Find(aliases.at(i), "P").empty()){
                    if(idx >= parts.size()){
                        throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Function '", func.get<std::string>("alias"), "' needs more than '", indeces.at(i), "' particles, but '", parts.size(), "' are given!"));    
                    }

                    fKey += "_" + aliases.at(i);

                    part = parts.at(idx);
                }

                if(!StrUtil::Find(aliases.at(i), "V").empty()){
                    if(idx >= values.size()){
                        throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Function '", func.get<std::string>("alias"), "' needs more than '", indeces.at(i), "' values, but '", values.size(), "' are given!"));    
                    }

                    fKey += "_" + aliases.at(i);

                    NTupleReader::PutVector(subFunc, "values", std::vector<std::string>{values.at(idx)});
                }
            }

            subFuncs[fKey] = this->compileBranchReading(subFunc, part);
        }
    }

    //Bind all sub function to the wished function
    bindedFunc = std::make_shared<CompiledCustomFunc>(Properties::properties.at(func.get<std::string>("alias")), subFuncs);

    return bindedFunc;
}

void NTupleReader::AddParticle(const std::string& pAlias, const int& idx, const std::string& wp, const std::experimental::source_location& location){
    std::string pName = NTupleReader::GetName(partInfo, pAlias);

    if(pName != ""){
        pt::ptree particle = partInfo.get_child(pName);

        //Set neccesary part infos
        if(!particle.get_optional<bool>("non-vector")) particle.put("index", idx - 1);
        particle.put("name", particle.get_optional<std::string>("branch-prefix") ? particle.get<std::string>("branch-prefix") : pName);

        particle.put("hist-name", StrUtil::Replace(particle.get<std::string>("hist-name"), "[WP]", wp));
        particle.put("axis-name", StrUtil::Replace(particle.get<std::string>("axis-name"), "[WP]", wp));
        particle.put("hash", std::hash<std::string>()(particle.get<std::string>("alias") + wp));

        if(idx == 0){
            particle.put("axis-name", StrUtil::Replace(particle.get<std::string>("axis-name"), "[I]", ""));
            particle.put("hist-name", StrUtil::Replace(particle.get<std::string>("hist-name"), "[I]", ""));
        }

        else{
            particle.put("axis-name", StrUtil::Replace(particle.get<std::string>("axis-name"), "[I]", idx));
            particle.put("hist-name", StrUtil::Replace(particle.get<std::string>("hist-name"), "[I]", idx));
        }

        //Set proper cuts for WP of particles if needed
        if(particle.get_child_optional("identification") and wp != ""){
            for(const std::string& idName : NTupleReader::GetInfo(particle.get_child("identification"))){
                if(particle.get_optional<float>(StrUtil::Join(".", "identification", idName, "WP", wp, "value", era))){
                    particle.put(StrUtil::Merge("identification.", idName, ".wp.value"), particle.get<float>(StrUtil::Join(".", "identification", idName, "WP", wp, "value", era)));
                }

                else{
                    particle.put(StrUtil::Merge("identification.", idName, ".wp.value"), particle.get<float>(StrUtil::Join(".", "identification", idName, "WP", wp, "value")));
                }

                particle.put(StrUtil::Merge("identification.", idName, ".wp.compare"), particle.get<std::string>(StrUtil::Join(".", "identification", idName, "WP", wp, "compare")));

            }
        }

        else particle.erase("identification");
  
        particles.push_back(particle);
    }

    else throw std::runtime_error(StrUtil::PrettyError(location, "Unknown particle alias: '", pAlias, "'!"));
};

void NTupleReader::AddFunction(const std::string& fAlias, const std::vector<std::string>& values, const std::experimental::source_location& location){
    std::string fName = NTupleReader::GetName(funcInfo, fAlias);

    //Check if function configured
    if(fName != ""){
        function.insert(function.end(), funcInfo.get_child(fName).begin(), funcInfo.get_child(fName).end());

        if(!values.empty()){
            NTupleReader::PutVector(function, "values", values);
        }
    }

    else throw std::runtime_error(StrUtil::PrettyError(location, "Unknown function alias: '", fAlias, "'!"));
};

void NTupleReader::AddFunctionByBranchName(const std::string& branchName, const std::experimental::source_location& location){
    //Check if function configured
    function.put("branch", branchName);
    function.put("axis-name", "");
    function.put("hist-name", "");
};

void NTupleReader::AddCut(const float& value, const std::string& op, const std::experimental::source_location& location){
    function.put("cut-op", op);
    function.put("cut-value", value);
}

void NTupleReader::AddWeighter(const std::shared_ptr<Weighter>& w, const std::experimental::source_location& location){
    weight = w;
}


void NTupleReader::Compile(const std::experimental::source_location& location){
    isCompiled = true;

    if(function.empty()){
        throw std::runtime_error(StrUtil::PrettyError(location, "No function information avaible! Call 'AddFunction' before compiling!"));
    }

    //Only read out branch value with particle
    if(function.get_child_optional("branch") and particles.size() > 0){
        func = this->compileBranchReading(function, particles.at(0));
    }

    //Only read out branch value without particle
    else if(function.get_child_optional("branch")){
        pt::ptree dummy;
        func = this->compileBranchReading(function, dummy);
    }

    //Compile external functions
    else if(function.get_child_optional("need")){
        func = this->compileCustomFunction(function, particles);
    }

    else{
        throw std::runtime_error(StrUtil::PrettyError(location, "Function '", function.get<std::string>("alias"), "' has neither 'branch' nor 'need' configured!"));
    }

    //Check if also cut should be compiled
    if(function.get_optional<std::string>("cut-value")){
        if(function.get<std::string>("cut-op") == "=="){
            func->AddCut(&Equal, function.get<float>("cut-value"));
        }

        else if(function.get<std::string>("cut-op") == ">="){
            func->AddCut(&Bigger, function.get<float>("cut-value"));
        }

        else if(function.get<std::string>("cut-op") == "<="){
            func->AddCut(&Smaller, function.get<float>("cut-value"));
        }

        else throw std::runtime_error(StrUtil::PrettyError(location, "Unknown cut operator: '", function.get<std::string>("cut-op"), "'!"));

        function.put("cut-name", StrUtil::Replace("[] [] []", "[]", function.get<std::string>("axis-name"), function.get<std::string>("cut-op"), function.get<std::string>("cut-value")));
    }
}

float NTupleReader::Get(){
    if(isCompiled) return func->Get();
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'Get' function!"));
}

bool NTupleReader::GetPassed(){
    if(isCompiled) return func->GetPassed();
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetPassed' function!"));
}

double NTupleReader::GetWeight(){
    if(weight) return weight->GetPartWeight(entry);
    else return 1.;
}

std::string NTupleReader::GetHistName(){
    if(isCompiled) return function.get<std::string>("hist-name");
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetHistName' function!"));
}

std::string NTupleReader::GetAxisLabel(){
    if(isCompiled) return function.get<std::string>("axis-name");
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetAxisLabel' function!"));
}

std::string NTupleReader::GetCutName(){
    if(isCompiled) return function.get<std::string>("cut-name");
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetCutName' function!"));
}

void NTupleReader::Next(){func->Next();}

void NTupleReader::Reset(){func->Reset();}


////// Custom functions of Properties namespace used by the NTupleReader

float Properties::HT(std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs){
    float sum = 0.;

    for(std::shared_ptr<CompiledFunc>& func : VUtil::MapValues(funcs)){
        func->Reset();

        while(true){
            const float& value = func->Get();
            if(value == -999.) break;

            func->Next();
            sum += value;
        }
    }

    return sum;
}

float Properties::dR(std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs){
    return std::sqrt(std::pow(funcs.at("eta_P1")->Get() - funcs.at("eta_P2")->Get(), 2) + std::pow(funcs.at("phi_P1")->Get() - funcs.at("phi_P2")->Get(), 2));
}

float Properties::dPhi(std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs){
    return std::acos(std::cos(funcs.at("phi_P1")->Get())*std::cos(funcs.at("phi_P2")->Get()) + std::sin(funcs.at("phi_P1")->Get())*std::sin(funcs.at("phi_P2")->Get()));
}

float Properties::diCharge(std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs){
    return funcs.at("charge_P1")->Get() * funcs.at("charge_P2")->Get();
}

float Properties::NParticles(std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs){
    int count = 0;

    funcs.at("pt_P")->Reset();

    while(true){
        if(funcs.at("pt_P")->Get() != -999.) ++count;
        else break;

        funcs.at("pt_P")->Next();
    }

    return count;
}

float Properties::DAK8C(std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs){
    int argMax = -1, counter = 0;
    float max = 0;

    for(std::shared_ptr<CompiledFunc>& func : VUtil::MapValues(funcs)){
        float score = func->Get();      

        if(score > max){
            max = score;
            argMax = counter;
        }

        ++counter;
    }

    return argMax;
}

float Properties::LP(std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs){
    ROOT::Math::PtEtaPhiMVector l(funcs.at("pt_P1")->Get(), 0, funcs.at("phi_P1")->Get(), 0);
    ROOT::Math::PtEtaPhiMVector MET(funcs.at("pt_P2")->Get(), 0, funcs.at("phi_P2")->Get(), 0);
    ROOT::Math::PtEtaPhiMVector W = l + MET;

	float dPhi = l.phi() - W.phi();
	while (dPhi >= TMath::Pi()) dPhi -= TMath::TwoPi();
	while (dPhi < -TMath::Pi()) dPhi += TMath::TwoPi();
    
    return l.pt()/W.pt()*std::cos(abs(dPhi));
}
