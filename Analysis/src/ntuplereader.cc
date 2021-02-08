#include <ChargedAnalysis/Analysis/include/ntuplereader.h>

//Constructor
NTupleReader::NTupleReader(){}

NTupleReader::NTupleReader(const std::shared_ptr<TTree>& inputTree) : inputTree(inputTree) {
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
float NTupleReader::GetEntryWithWP(TLeaf* leaf, std::vector<CompiledCut>& cuts, const int& idx){
    int wpIdx = -1, counter = -1;

    const std::vector<float> data = RUtil::GetVecEntry<float>(leaf, entry);
    if(idx >= data.size()) return -999.;

    //Loop over all entries in collection
    for(std::size_t i = 0; i < data.size(); ++i){
        bool passed = true;

        //Check if all id cuts are passed
        for(CompiledCut& cut : cuts){
            if(i == 0) cut.Reset();

            if(passed) passed = passed && cut.GetPassed();
            cut.Next();
        }

        if(passed) ++counter;

        if(counter == idx){
            wpIdx = i;
            break;
        }
    }

    try{
        return data.at(wpIdx);
    }
    catch(...){
        return -999.;
    }
}

std::shared_ptr<CompiledFunc> NTupleReader::compileBranchReading(const pt::ptree& func, const pt::ptree& part){
    std::string branchName = StrUtil::Replace(func.get<std::string>("branch"), "[P]", part.get<std::string>("name"));
    std::shared_ptr<CompiledFunc> bindedFunc;

    //Labels names
    if(part.get_optional<int>("index")){
        function.put("axis-name", StrUtil::Replace(function.get<std::string>("axis-name"), "[P]", particles.at(0).get<std::string>("axis-name")));
        function.put("hist-name", StrUtil::Replace(function.get<std::string>("hist-name"), "[P]", particles.at(0).get<std::string>("hist-name")));
    }

    //Compile vector branch with WP
    if(part.get_optional<int>("index") && (part.get_child_optional("identification") or part.get_child_optional("requirements"))){
        std::vector<CompiledCut> cuts;

        //Make part without requirements so functions can be used as iterators
        pt::ptree pNoWP = part;
        pNoWP.put("index", -1);
        pNoWP.erase("requirements");
        pNoWP.erase("identification");

        //Loop over all needed branches and create bool lambda checking the WP criteria
        if(part.get_child_optional("identification")){
            for(const std::string& idName : NTupleReader::GetInfo(part.get_child("identification"))){
                //Get ID branch as iterator
                pt::ptree funcID;
                funcID.put("branch", idName);

                std::shared_ptr<CompiledFunc> cutFunc = this->compileBranchReading(funcID, pNoWP);
                CompiledCut cut;

                //Check which operation to do
                if(part.get<std::string>("identification." + idName + ".compare") == "=="){
                    cut = CompiledCut(cutFunc, &Equal, part.get<float>("identification." + idName + ".wp"));
                }

                else if(part.get<std::string>("identification." + idName + ".compare") == ">="){
                    cut = CompiledCut(cutFunc, &Bigger, part.get<float>("identification." + idName + ".wp"));
                }

                else if(part.get<std::string>("identification." + idName + ".compare") == "<="){
                    cut = CompiledCut(cutFunc, &Smaller, part.get<float>("identification." + idName + ".wp"));
                }

                else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown cut operator: '", part.get<std::string>("identification." + idName + ".compare"), "'!"));

                cuts.push_back(cut);
            }
        }

        //Loop over all other required cuts as bool lambda
        if(part.get_child_optional("requirements")){
            for(const std::string& fAlias : NTupleReader::GetInfo(part.get_child("requirements"))){
                std::shared_ptr<CompiledFunc> cutFunc;
                CompiledCut cut;
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
                        //Vector with partAlias, part index and part WP
                        std::vector<std::string> pInfo = NTupleReader::GetInfo(part.get_child(StrUtil::Join(".", "requirements", fAlias, "particles", pAlias)), false);
                        std::string pName = NTupleReader::GetName(partInfo, pInfo.at(0));

                        if(pName == "") throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown particle alias: '", pInfo.at(0), "'!"));

                        parts.push_back(partInfo.get_child(pName));
                        parts.back().put("index", std::atoi(pInfo.at(1).c_str()) - 1);
                        parts.back().put("name", parts.back().get_optional<std::string>("branch-prefix") ? parts.back().get<std::string>("branch-prefix") : pName);

                        //Set proper cuts for WP of particles if needed
                        if(parts.back().get_child_optional("identification") and pInfo.at(2) != ""){
                            for(const std::string& idName : NTupleReader::GetInfo(parts.back().get_child("identification"))){
                                parts.back().put(StrUtil::Merge("identification.", idName, ".wp"), parts.back().get<float>(StrUtil::Merge("identification.", idName, ".WP.", pInfo.at(2))));
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
                    cut = CompiledCut(cutFunc, &Equal, part.get<float>("requirements." + fAlias + ".cut"));
                }

                else if(part.get<std::string>("requirements." + fAlias + ".compare") == ">="){
                    cut = CompiledCut(cutFunc, &Bigger, part.get<float>("requirements." + fAlias + ".cut"));
                }

                else if(part.get<std::string>("requirements." + fAlias + ".compare") == "<="){
                    cut = CompiledCut(cutFunc, &Smaller, part.get<float>("requirements." + fAlias + ".cut"));
                }

                else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown cut operator: '", part.get<std::string>("requirements." + fAlias + ".compare"), "'!"));

                cuts.push_back(cut);
            }
        }

        //Bind everything together
        bindedFunc = std::make_shared<CompiledFunc>(&NTupleReader::GetEntryWithWP, RUtil::Get<TLeaf>(inputTree.get(), branchName), cuts, part.get<int>("index"), part.get<int>("index") == -1);

    }

    //Compile vector like branch
    else if(part.get_optional<int>("index")){
        bindedFunc = std::make_shared<CompiledFunc>(&NTupleReader::GetEntry, RUtil::Get<TLeaf>(inputTree.get(), branchName), part.get<int>("index"), part.get<int>("index") == -1);
    }

    //Compile non-vector like branch
    else{
        bindedFunc = std::make_shared<CompiledFunc>(&NTupleReader::GetEntry, RUtil::Get<TLeaf>(inputTree.get(), branchName), 0);
    }

    return bindedFunc;
}

std::shared_ptr<CompiledFunc> NTupleReader::compileCustomFunction(const pt::ptree& func, const std::vector<pt::ptree>& parts){
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
            function.put("axis-name", StrUtil::Replace(func.get<std::string>("axis-name"), "[" + pAliases.at(i) + "]", parts.at(i).get<std::string>("axis-name")));
            function.put("hist-name", StrUtil::Replace(func.get<std::string>("hist-name"), "[" + pAliases.at(i) + "]", parts.at(i).get<std::string>("hist-name")));

            partValues[{pAliases.at(i), fAlias}] = this->compileBranchReading(funcInfo.get_child(fName), parts.at(i));   
        }
    }

    //Bind all sub function to the wished function
    bindedFunc = std::make_shared<CompiledCustomFunc>(Properties::properties.at(func.get<std::string>("alias")), partValues);

    return bindedFunc;
}

void NTupleReader::AddParticle(const std::string& pAlias, const std::size_t& idx, const std::string& wp, const std::experimental::source_location& location){
    std::string pName = NTupleReader::GetName(partInfo, pAlias);

    if(pName != ""){
        pt::ptree particle = partInfo.get_child(pName);

        //Set neccesary part infos
        particle.put("index", idx == 0 ?  -1. : idx - 1);
        particle.put("name", particle.get_optional<std::string>("branch-prefix") ? particle.get<std::string>("branch-prefix") : pName);

        particle.put("hist-name", StrUtil::Replace(particle.get<std::string>("hist-name"), "[WP]", wp));
        particle.put("axis-name", StrUtil::Replace(particle.get<std::string>("axis-name"), "[WP]", wp));

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
                particle.put(StrUtil::Merge("identification.", idName, ".wp"), particle.get<float>(StrUtil::Merge("identification.", idName, ".WP.", wp)));
            }
        }

        else particle.erase("identification");
           
        particles.push_back(particle);
    }

    else throw std::runtime_error(StrUtil::PrettyError(location, "Unknown particle alias: '", pAlias, "'!"));
};

void NTupleReader::AddFunction(const std::string& fAlias, const std::experimental::source_location& location){
    std::string fName = NTupleReader::GetName(funcInfo, fAlias);

    //Check if function configured
    if(fName != ""){
        function.insert(function.end(), funcInfo.get_child(fName).begin(), funcInfo.get_child(fName).end());
    }

    else throw std::runtime_error(StrUtil::PrettyError(location, "Unknown function alias: '", fName, "'!"));
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
        func = this->compileBranchReading(function, pt::ptree());
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
            cut = std::make_shared<CompiledCut>(func, &Equal, function.get<float>("cut-value"));
        }

        if(function.get<std::string>("cut-op") == ">="){
            cut = std::make_shared<CompiledCut>(func, &Bigger, function.get<float>("cut-value"));
        }

        if(function.get<std::string>("cut-op") == "<="){
            cut = std::make_shared<CompiledCut>(func, &Smaller, function.get<float>("cut-value"));
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
    if(isCompiled) return cut->GetPassed();
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


////// Custom functions of Properties namespace used by the NTupleReader

float Properties::HT(Particles& parts){
    float sum = 0.; int idx = 0;

    parts.at({"P", "pt"})->Reset();

    while(true){
        const float& value = parts.at({"P", "pt"})->Get();
        if(value != -999.) ++idx;
        else break;

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
        const float& value = parts.at({"P", "pt"})->Get();
        if(value != -999.) ++count;
        else break;

        parts.at({"P", "pt"})->Next();
    }

    return count;
}
