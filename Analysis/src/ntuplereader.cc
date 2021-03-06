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
    if(idxCache.count(hash) and idxCache.at(hash).count(idx)) return data.at(idxCache.at(hash).at(idx));

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

    //Labels names
    if(func.get_optional<std::string>("axis-name") and part.get_optional<std::string>("axis-name")){
        func.put("axis-name", StrUtil::Replace(func.get<std::string>("axis-name"), "[P]", particles.at(0).get<std::string>("axis-name")));
        func.put("hist-name", StrUtil::Replace(func.get<std::string>("hist-name"), "[P]", particles.at(0).get<std::string>("hist-name")));
    }

    std::string branchName = func.get<std::string>("branch");

    if(part.get_optional<std::string>("name")){
        branchName = StrUtil::Replace(branchName, "[P]", part.get<std::string>("name"));
    }

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
                if(part.get<std::string>("identification." + idName + ".compare") == "=="){
                    cutFunc->AddCut(checkAbs? &EqualAbs : &Equal, part.get<float>("identification." + idName + ".wp"));
                }

                else if(part.get<std::string>("identification." + idName + ".compare") == ">="){
                    cutFunc->AddCut(checkAbs? &BiggerAbs : &Bigger, part.get<float>("identification." + idName + ".wp"));
                }

                else if(part.get<std::string>("identification." + idName + ".compare") == "<="){
                    cutFunc->AddCut(checkAbs? &SmallerAbs : &Smaller, part.get<float>("identification." + idName + ".wp"));
                }

                else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown cut operator: '", part.get<std::string>("identification." + idName + ".compare"), "'!"));

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
                                std::string wpPath = StrUtil::Join(".", "identification", idName, "WP", pInfo.at(2));
                                std::string wpPathWithEra = StrUtil::Join(".", "identification", idName, "WP", pInfo.at(2), era);

                                parts.back().put(StrUtil::Merge("identification.", idName, ".wp"), parts.back().get_optional<float>(wpPathWithEra) ? parts.back().get<float>(wpPathWithEra) : parts.back().get<float>(wpPath));
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
    Particles partValues;

    //Loop over all needed subfunctions/particles
    for(const std::string& fAlias: NTupleReader::GetInfo(func.get_child("need"))){
        //Check if sub function configured
        std::string fName = NTupleReader::GetName(funcInfo, fAlias);

        if(fName == "") throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown function alias: '", fAlias, "'!"));

        //Loop over all needed particles
        std::vector<std::string> pAliases = NTupleReader::GetInfo(func.get_child("need." + fAlias), false);

        if(pAliases.size() > parts.size()){
            throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Function '", func.get<std::string>("alias"), "' needs '", pAliases.size(), "' particles, but '", parts.size(), "' are given!"));    
        }   

        for(int i = 0; i < pAliases.size(); ++i){
            func.put("axis-name", StrUtil::Replace(func.get<std::string>("axis-name"), "[" + pAliases.at(i) + "]", parts.at(i).get<std::string>("axis-name")));
            func.put("hist-name", StrUtil::Replace(func.get<std::string>("hist-name"), "[" + pAliases.at(i) + "]", parts.at(i).get<std::string>("hist-name")));

            partValues[{pAliases.at(i), fAlias}] = this->compileBranchReading(funcInfo.get_child(fName), parts.at(i));   
        }
    }

    //Bind all sub function to the wished function
    bindedFunc = std::make_shared<CompiledCustomFunc>(Properties::properties.at(func.get<std::string>("alias")), partValues);

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
                std::string wpPath = StrUtil::Join(".", "identification", idName, "WP", wp);
                std::string wpPathWithEra = StrUtil::Join(".", "identification", idName, "WP", wp, era);

                particle.put(StrUtil::Merge("identification.", idName, ".wp"), particle.get_optional<float>(wpPathWithEra) ? particle.get<float>(wpPathWithEra) : particle.get<float>(wpPath));
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

        for(const std::string& value : values){
            function.put("axis-name", StrUtil::Replace(function.get<std::string>("axis-name"), "[V]", value));
            function.put("hist-name", StrUtil::Replace(function.get<std::string>("hist-name"), "[V]", value));
            function.put("branch", StrUtil::Replace(function.get<std::string>("branch"), "[V]", value));
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

float Properties::HT(Particles& parts){
    float sum = 0.;

    parts.at({"P", "pt"})->Reset();

    while(true){
        const float& value = parts.at({"P", "pt"})->Get();
        if(value == -999.) break;

        parts.at({"P", "pt"})->Next();
        sum += value;
    }

    return sum;
}

float Properties::dR(Particles& parts){
    return std::sqrt(std::pow(parts.at({"P1", "eta"})->Get() - parts.at({"P2", "eta"})->Get(), 2) + std::pow(parts.at({"P1", "phi"})->Get() - parts.at({"P2", "phi"})->Get(), 2));
}

float Properties::dPhi(Particles& parts){
    return std::acos(std::cos(parts.at({"P1", "phi"})->Get())*std::cos(parts.at({"P2", "phi"})->Get()) + std::sin(parts.at({"P1", "phi"})->Get())*std::sin(parts.at({"P2", "phi"})->Get()));
}

float Properties::diCharge(Particles& parts){
    return parts.at({"P1", "charge"})->Get() * parts.at({"P2", "charge"})->Get();
}

float Properties::NParticles(Particles& parts){
    int count = 0;

    parts.at({"P", "pt"})->Reset();

    while(true){
        if(parts.at({"P", "pt"})->Get() != -999.) ++count;
        else break;

        parts.at({"P", "pt"})->Next();
    }

    return count;
}
