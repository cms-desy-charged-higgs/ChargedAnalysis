#include <ChargedAnalysis/Analysis/include/ntuplereader.h>

Cut NTupleReader::CreateCut(const std::string& op, const float& compV){
    //Check which operation to do
    if(op == "=="){
        return [=](const float& v){return v == compV or v == -999.;};
    }

    else if(op == "|==|"){
        return [=](const float& v){return std::abs(v) == compV or v == -999.;};
    }

    else if(op == ">="){
        return [=](const float& v){return v >= compV or v == -999.;};
    }

    else if(op == "|>=|"){
        return [=](const float& v){return std::abs(v) >= compV or v == -999.;};
    }

    else if(op == "<="){
        return [=](const float& v){return v <= compV or v == -999.;};
    }

    else if(op == "|<=|"){
        return [=](const float& v){return std::abs(v) <= compV or v == -999.;};
    }

    else if(op == "%2"){
        return [=](const float& v){return int(v) % 2 == compV or v == -999.;};
    }

    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown cut operator: '", op, "'!"));
}

Cut NTupleReader::CreateCut(const std::string& op, const std::vector<float>& compV){
    if(op == "<=OR>="){
        return [=](const float& v){return (v <= compV.at(0) or v >= compV.at(1)) or v == -999.;};
    }

    else if(op == ">=AND<="){
        return [=](const float& v){return (v >= compV.at(0) and v <= compV.at(1)) or v == -999.;};
    }

    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown cut operator: '", op, "'!"));
}

Func NTupleReader::CreateFunc(const std::string& branchName, const pt::ptree& part){
    std::string bName = branchName;

    //Dummy particle, branch without particle (like eventNumber)
    if(part.empty()){
        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, bName);
        return [=](const int& entry, const int& idx){return RUtil::GetEntry<float>(leaf, entry, 0);};
    }

    else{
        bName = StrUtil::Replace(bName, "[P]", part.get_child_optional("branch-prefix") ? part.get<std::string>("branch-prefix") : part.get<std::string>("name"));

        TLeaf* leaf = RUtil::Get<TLeaf>(inputTree, bName);

        std::size_t partHash = part.get<std::size_t>("hash");

        return [=, this](const int& entry, const int& idx)
                 {return RUtil::GetEntry<float>(leaf, entry, GetWPIndex(entry, partHash, idx));};
    }
}

Func NTupleReader::CreateCustomFunc(const std::string& customFunc, const std::vector<pt::ptree>& parts){
    std::vector<std::string> neededBranches;

    for(const std::string& fAlias: NTupleReader::GetVector(funcInfo.get_child(NTupleReader::GetName(funcInfo, customFunc) + ".need"))){
        //Check if sub function configured
        std::string fName = NTupleReader::GetName(funcInfo, fAlias);

        for(const pt::ptree& part : parts){
            std::string needBranch = funcInfo.get<std::string>(fName + "." + "branch");
            needBranch = StrUtil::Replace(needBranch, "[P]", part.get_child_optional("branch-prefix") ? part.get<std::string>("branch-prefix") : part.get<std::string>("name"));
            neededBranches.push_back(std::move(needBranch));
        }

        if(parts.size() == 0){
            neededBranches.push_back(funcInfo.get<std::string>(fName + "." + "branch"));
        }
    }

    if(customFunc == "N"){
        std::size_t pHash = parts.at(0).get<std::size_t>("hash");
        TLeaf* nPart = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::NParticles(entry, pHash, nPart);};
    }

    else if(customFunc == "HT"){
        std::size_t jHash = parts.at(0).get<std::size_t>("hash");
        TLeaf* jBranch = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::HT(entry, jHash, jBranch);};
    }

    else if(customFunc == "dphi"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash"), p2Hash = parts.at(1).get<std::size_t>("hash");
        std::size_t p2Idx = parts.at(1).get<std::size_t>("idx");

        TLeaf* phi1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));
        TLeaf* phi2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(1));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::dPhi(entry, idx, p2Idx, p1Hash, p2Hash, phi1, phi2);};
    }

    else if(customFunc == "dicharge"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash"), p2Hash = parts.at(1).get<std::size_t>("hash");
        std::size_t p2Idx = parts.at(1).get<std::size_t>("idx");

        TLeaf* c1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));
        TLeaf* c2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(1));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::diCharge(entry, idx, p2Idx, p1Hash, p2Hash, c1, c2);};
    }

    else if(customFunc == "dR"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash"), p2Hash = parts.at(1).get<std::size_t>("hash");
        std::size_t p2Idx = parts.at(1).get<std::size_t>("idx");

        TLeaf* phi1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));
        TLeaf* phi2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(1));
        TLeaf* eta1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(2));
        TLeaf* eta2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(3));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::dR(entry, idx, p2Idx, p1Hash, p2Hash, phi1, phi2, eta1, eta2);};
    }

    else if(customFunc == "dR3"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash"), p2Hash = parts.at(1).get<std::size_t>("hash"), p3Hash = parts.at(2).get<std::size_t>("hash");
        std::size_t p2Idx = parts.at(1).get<std::size_t>("idx"), p3Idx = parts.at(2).get<std::size_t>("idx");

        TLeaf* phi1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));
        TLeaf* phi2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(1));
        TLeaf* phi3 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(2));
        TLeaf* eta1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(3));
        TLeaf* eta2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(4));
        TLeaf* eta3 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(5));
        TLeaf* pt2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(7));
        TLeaf* pt3 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(8));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::dR3(entry, idx, p2Idx, p3Idx, p1Hash, p2Hash, p3Hash, phi1, eta1, pt2, phi2, eta2, phi2, pt3, eta3);};
    }


    else if(customFunc == "dphi3"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash"), p2Hash = parts.at(1).get<std::size_t>("hash"), p3Hash = parts.at(2).get<std::size_t>("hash");
        std::size_t p2Idx = parts.at(1).get<std::size_t>("idx"), p3Idx = parts.at(2).get<std::size_t>("idx");

        TLeaf* phi1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));
        TLeaf* phi2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(1));
        TLeaf* phi3 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(2));
        TLeaf* eta1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(3));
        TLeaf* eta2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(4));
        TLeaf* eta3 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(5));
        TLeaf* pt2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(7));
        TLeaf* pt3 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(8));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::dPhi3(entry, idx, p2Idx, p3Idx, p1Hash, p2Hash, p3Hash, phi1, pt2, phi2, eta2, phi2, pt3, eta3);};
    }

    else if(customFunc == "diM"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash"), p2Hash = parts.at(1).get<std::size_t>("hash");
        std::size_t p2Idx = parts.at(1).get<std::size_t>("idx");

        TLeaf* pt1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));
        TLeaf* pt2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(1));
        TLeaf* phi1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(2));
        TLeaf* phi2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(3));
        TLeaf* eta1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(4));
        TLeaf* eta2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(5));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::diMass(entry, idx, p2Idx, p1Hash, p2Hash, pt1, pt2, phi1, phi2, eta1, eta2);};
    }

    else if(customFunc == "diMT"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash"), p2Hash = parts.at(1).get<std::size_t>("hash");
        std::size_t p2Idx = parts.at(1).get<std::size_t>("idx");

        TLeaf* pt1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));
        TLeaf* pt2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(1));
        TLeaf* phi1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(2));
        TLeaf* phi2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(3));

        return [=, this](const int& entry, const int& idx)
               {return diMT(entry, idx, p2Idx, p1Hash, p2Hash, pt1, pt2, phi1, phi2);};
    }

    else if(customFunc == "LP"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash");
        std::size_t p1Idx = parts.at(0).get<std::size_t>("idx");

        TLeaf* pt1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));
        TLeaf* pt2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(1));
        TLeaf* phi1 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(2));
        TLeaf* phi2 = RUtil::Get<TLeaf>(inputTree, neededBranches.at(3));

        return [=, this](const int& entry, const int& idx)
               {return LP(entry, idx, p1Hash, pt1, pt2, phi1, phi2);};
    }

    else if(customFunc == "mEvNr"){
        TLeaf* evNr = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::ModifiedEntry(entry, evNr);};
    }

    else if(customFunc == "gM"){
        std::size_t p1Hash = parts.at(0).get<std::size_t>("hash");

        TLeaf* gID = RUtil::Get<TLeaf>(inputTree, neededBranches.at(0));

        return [=, this](const int& entry, const int& idx)
               {return NTupleReader::isGenMatched(entry, idx, p1Hash, gID);};
    }

    else{
        throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown custom function alias '", customFunc, "'!")); 
    }
}

std::size_t NTupleReader::GetWPIndex(const std::size_t& entry, const std::size_t& partHash, const std::size_t& idx){
    //Check if WP idx is needed
    if(!partRequirements.count(partHash)) return idx;

    //Check if idx already calculated for this particle
    try{
        return idxCache.at({partHash, idx});
    }

    catch(...){}

    //Get TLeaf with size
    std::size_t size = RUtil::GetEntry<std::size_t>(partSize.at(partHash), entry);
    if(idx >= size) return std::numeric_limits<std::size_t>::max();

    //Check if prevouis index alrealdy exist    
    std::size_t start = idxCache.count({partHash, idx - 1}) ? idxCache.at({partHash, idx - 1}) + 1 : 0;
    std::size_t counter = start == 0 ? 0 : idx;

    std::size_t wpIdx = std::numeric_limits<std::size_t>::max();

    //Loop over all entries in collection
    for(std::size_t i = start; i < size; ++i){
        bool passed = true;

        //Check if all id cuts are passed
        for(std::pair<Cut, Func>& partCriteria : partRequirements.at(partHash)){
            Cut& cut = partCriteria.first;
            Func& func = partCriteria.second;

            passed = passed && cut(func(entry, i));
            if(!passed) break;
        }

        if(passed){
            if(counter == idx){
                wpIdx = i;
                break;
            }

            ++counter;
        }
    }

    if(wpIdx != std::numeric_limits<std::size_t>::max()){
        idxCache[{partHash, idx}] = wpIdx;
    }
  
    return wpIdx;
}

pt::ptree NTupleReader::SetPartWP(const pt::ptree& part, const std::string& wp, const std::size_t& idx){
    pt::ptree setPart = part;

    //Set particle stuff
    setPart.put("name", NTupleReader::GetName(partInfo, setPart.get<std::string>("alias")));
    setPart.put("hash", std::hash<std::string>{}(wp + setPart.get<std::string>("name") + (part.get_child_optional("requirements") ? "req" : "") + + (part.get_child_optional("identification") ? "id" : "")));
    setPart.put("wp", wp);
    setPart.put("idx", idx > 0 ? idx - 1 : idx);

    //Set proper cuts for WP of particles if needed
    if(part.get_child_optional("identification") and wp != ""){
        //Loop over all identification criteria
        for(const std::string& idName : NTupleReader::GetKeys(part.get_child("identification"))){
            //WP value with era
            if(part.get_optional<float>(StrUtil::Join(".", "identification", idName, "WP", wp, "value", era))){
                setPart.put(StrUtil::Merge("identification.", idName, ".value"), 
                            part.get<float>(StrUtil::Join(".", "identification", idName, "WP", wp, "value", era)));
            }

            //WP value with era
            else if(part.get_optional<float>(StrUtil::Join(".", "identification", idName, "WP", wp, "value"))){
                setPart.put(StrUtil::Merge("identification.", idName, ".value"), 
                            part.get<float>(StrUtil::Join(".", "identification", idName, "WP", wp, "value")));
            }

            //No WP for this particular case
            else{
                setPart.get_child("identification").erase(idName);
                continue;
            }

            setPart.put(StrUtil::Merge("identification.", idName, ".compare"), 
                        part.get<std::string>(StrUtil::Join(".", "identification", idName, "WP", wp, "compare")));
            setPart.get_child("identification." + idName).erase("WP");
        }
    }

    else setPart.erase("identification");

    return setPart;
}

void NTupleReader::RegisterParticle(const pt::ptree& part){
    //Already registered, just return
    if(partRequirements.count(part.get<std::size_t>("hash"))) return;

    std::vector<std::pair<Cut, Func>> cuts;

    //Self with no requirements (for branch reading)
    pt::ptree self = partInfo.get_child(NTupleReader::GetName(partInfo, part.get<std::string>("alias")));
    self.erase("identification");
    self.erase("requirements");
    self = SetPartWP(self, "", 0);

    if(part.get_child_optional("identification")){
        //Loop over all basic idenfication requirements there only branch reading is needed
        for(const std::string& idName : NTupleReader::GetKeys(part.get_child("identification"))){
            Func func = CreateFunc(idName, self);
            Cut cut = CreateCut(part.get<std::string>("identification." + idName + ".compare"), 
                                part.get<float>("identification." + idName + ".value"));

            cuts.push_back(std::make_pair(std::move(cut), std::move(func)));
        }
    }

    if(part.get_child_optional("requirements")){
        //Loop over all other requirements with other particles involved and custom functions
        for(const std::string& fAlias : NTupleReader::GetKeys(part.get_child("requirements"))){
            std::string fName = NTupleReader::GetName(funcInfo, fAlias);

            //Make cut function
            Cut cut = CreateCut(part.get<std::string>("requirements." + fAlias + ".compare"),
                                part.get<float>("requirements." + fAlias + ".value"));
    
            Func func;

            std::vector<pt::ptree> reqParts;
            reqParts.push_back(self);

            //Loop over all additional particles
            if(part.get_child_optional("requirements." + fAlias + ".particles")){
                pt::ptree requirements = part.get_child("requirements." + fAlias + ".particles");

                for(const std::string& pAlias : NTupleReader::GetKeys(requirements)){
                    pt::ptree reqPart;

                    //Set idx, working point and hash for channel dependent part
                    if(requirements.get_child_optional(pAlias + "." + chanPrefix)){
                        reqPart = partInfo.get_child(NTupleReader::GetName(partInfo, requirements.get<std::string>(pAlias + "." + chanPrefix + ".partName")));

                        reqPart = SetPartWP(reqPart, 
                                            requirements.get<std::string>(pAlias + "." + chanPrefix + ".wp"),
                                            requirements.get<std::size_t>(pAlias + "." + chanPrefix + ".idx"));
                    }

                    //Not channel dependent
                    else{
                        reqPart = partInfo.get_child(requirements.get<std::string>(pAlias + ".partName"));

                        reqPart = SetPartWP(reqPart, 
                                            requirements.get<std::string>(pAlias + ".wp"),
                                            requirements.get<std::size_t>(pAlias + ".idx"));
                    }

                    //Register needed particle if not already registered
                    RegisterParticle(reqPart); 

                    reqParts.push_back(std::move(reqPart));
                } 

                func = CreateCustomFunc(fAlias, reqParts);
            }

            //No other particles are needed
            else{
                if(funcInfo.get_child_optional(fName + "." + "need")){
                    func = CreateCustomFunc(fAlias, reqParts);
                }

                else{
                    std::string reqBranch = funcInfo.get<std::string>(fName + "." + "branch");
                    func = CreateFunc(reqBranch, self);
                }
            }

            cuts.push_back(std::make_pair(std::move(cut), std::move(func)));
        }
    }
    
    //Add to register
    if(!cuts.empty()){
        partSize[part.get<std::size_t>("hash")] = RUtil::Get<TLeaf>(inputTree, part.get<std::string>("size"));
        partRequirements[part.get<std::size_t>("hash")] = std::move(cuts);
    }
}

void NTupleFunction::AddParticle(const std::string& pAlias, const std::size_t& idx, const std::string& wp, const std::experimental::source_location& location){
    std::string pName = NTupleReader::GetName(reader->partInfo, pAlias);

    if(pName != ""){
        pt::ptree particle = reader->partInfo.get_child(pName);

        //Set neccesary part infos
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

        particle = reader->SetPartWP(particle, wp, idx);
        reader->RegisterParticle(particle);

        

        particles.push_back(std::move(particle));
    }

    else throw std::runtime_error(StrUtil::PrettyError(location, "Unknown particle alias: '", pAlias, "'!"));
};

void NTupleFunction::AddFunction(const std::string& fAlias, const std::vector<std::string>& values, const std::experimental::source_location& location){
    std::string fName = NTupleReader::GetName(reader->funcInfo, fAlias);

    //Check if function configured
    if(fName != ""){
        function.insert(function.end(), reader->funcInfo.get_child(fName).begin(), reader->funcInfo.get_child(fName).end());

        if(!values.empty()){
            NTupleReader::PutVector(function, "values", values);
        }
    }

    else throw std::runtime_error(StrUtil::PrettyError(location, "Unknown function alias: '", fAlias, "'!"));
};

void NTupleFunction::AddFunctionByBranchName(const std::string& branchName, const std::experimental::source_location& location){
    //Check if function configured
    function.put("branch", branchName);
    function.put("axis-name", branchName);
    function.put("hist-name", branchName);
};

void NTupleFunction::AddCut(const float& value, const std::string& op, const std::experimental::source_location& location){
    function.put("cut-op", op);
    function.put("cut-value", value);
}

void NTupleFunction::AddCut(const std::vector<float>& values, const std::string& op, const std::experimental::source_location& location){
    function.put("cut-op", op);
    NTupleReader::PutVector(function, "cut-values", values);
}

Func NTupleFunction::compileBranchReading(const pt::ptree& part){
    //Replace key holder for particle/value
    if(!part.empty()){
        function.put("axis-name", StrUtil::Replace(function.get<std::string>("axis-name"), "[P]", part.get<std::string>("axis-name")));
        function.put("hist-name", StrUtil::Replace(function.get<std::string>("hist-name"), "[P]", part.get<std::string>("hist-name")));
        function.put("branch", StrUtil::Replace(function.get<std::string>("branch"), "[P]", part.get_child_optional("branch-prefix") ? part.get<std::string>("branch-prefix") : part.get<std::string>("name")));
    }

    if(function.get_child_optional("values")){
        std::vector<std::string> values = NTupleReader::GetVector(function.get_child("values"));

        for(const std::string& value : values){
            function.put("axis-name", StrUtil::Replace(function.get<std::string>("axis-name"), "[V]", value));
            function.put("hist-name", StrUtil::Replace(function.get<std::string>("hist-name"), "[V]", value));
            function.put("branch", StrUtil::Replace(function.get<std::string>("branch"), "[V]", value));
        }
    }

    return reader->CreateFunc(function.get<std::string>("branch"), part);
}

Func NTupleFunction::compileCustomFunction(const std::vector<pt::ptree>& parts){
    //Get function values
    std::vector<std::string> values;
    if(function.get_child_optional("values")) values = NTupleReader::GetVector(function.get_child("values"));

    //Label/axis name
    for(const pt::ptree part: parts){
        function.put("axis-name", StrUtil::Replace(function.get<std::string>("axis-name"), "[P]", part.get<std::string>("axis-name")));
        function.put("hist-name", StrUtil::Replace(function.get<std::string>("hist-name"), "[P]", part.get<std::string>("hist-name")));
    }

    for(const std::string value: values){
        function.put("axis-name", StrUtil::Replace(function.get<std::string>("axis-name"), "[V]", value));
        function.put("hist-name", StrUtil::Replace(function.get<std::string>("hist-name"), "[V]", value));
    }

    return reader->CreateCustomFunc(function.get<std::string>("alias"), parts);
}

void NTupleFunction::Compile(const std::experimental::source_location& location){
    isCompiled = true;

    if(function.empty()){
        throw std::runtime_error(StrUtil::PrettyError(location, "No function information avaible! Call 'AddFunction' before compiling!"));
    }

    //Only read out branch value with particle
    if(function.get_child_optional("branch") and particles.size() > 0){
        mainIdx = particles.at(0).get<std::size_t>("idx");
        func = compileBranchReading(particles.at(0));
    }

    //Only read out branch value without particle
    else if(function.get_child_optional("branch")){
        mainIdx = 0;
        pt::ptree dummy;
        func = compileBranchReading(dummy);
    }

    //Compile external functions with particles
    else if(function.get_child_optional("need") and particles.size() > 0){
        mainIdx = particles.at(0).get<std::size_t>("idx");
        func = compileCustomFunction(particles);
    }

    //Compile external functions without particles
    else if(function.get_child_optional("need")){
        mainIdx = 0;
        func = compileCustomFunction(particles);
    }

    else{
        throw std::runtime_error(StrUtil::PrettyError(location, "Function '", function.get<std::string>("alias"), "' has neither 'branch' nor 'need' configured!"));
    }

    //Check if also cut should be compiled
    if(function.get_optional<std::string>("cut-value")){
        cut = reader->CreateCut(function.get<std::string>("cut-op"), function.get<float>("cut-value"));

        function.put("cut-name", StrUtil::Replace("[] [] []", "[]", function.get<std::string>("axis-name"), function.get<std::string>("cut-op"), function.get<std::string>("cut-value")));
    }

    if(function.get_optional<std::string>("cut-values")){
        std::vector<std::string> cutOps;
        if(!StrUtil::Find(function.get<std::string>("cut-op"), "OR").empty()){
            cutOps = StrUtil::Split(function.get<std::string>("cut-op"), "OR");
        }

        else cutOps = StrUtil::Split(function.get<std::string>("cut-op"), "AND");

        std::vector<float> cutValues = NTupleReader::GetVector<float>(function.get_child("cut-values"));
        cut = reader->CreateCut(function.get<std::string>("cut-op"), cutValues);

        function.put("cut-name", StrUtil::Replace("[] [] [] || [] [] []", "[]", function.get<std::string>("axis-name"), cutOps.at(0), cutValues.at(0), function.get<std::string>("axis-name"), cutOps.at(1), cutValues.at(1)));
    }
}

float NTupleFunction::Get(){
    if(isCompiled) return func(reader->entry, mainIdx);
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'Get' function!"));
}

float NTupleFunction::Get(const std::size_t& index){
    if(isCompiled) return func(reader->entry, index);
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'Get' function!"));
}

bool NTupleFunction::GetPassed(){
    if(isCompiled) return cut(func(reader->entry, mainIdx));
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetPassed' function!"));
}

bool NTupleFunction::GetPassed(const std::size_t& index){
    if(isCompiled) return cut(func(reader->entry, index));
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetPassed' function!"));
}

std::string NTupleFunction::GetHistName(){
    if(isCompiled) return function.get<std::string>("hist-name");
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetHistName' function!"));
}

std::string NTupleFunction::GetAxisLabel(){
    if(isCompiled) return function.get<std::string>("axis-name");
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetAxisLabel' function!"));
}

std::string NTupleFunction::GetCutName(){
    if(isCompiled) return function.get<std::string>("cut-name");
    else throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Use the 'Compile' function before calling the 'GetCutName' function!"));
}

float NTupleReader::NParticles(const std::size_t& entry, const std::size_t& pHash, TLeaf* pSize){
    std::size_t nPart = 0;
    std::size_t nInitPart = RUtil::GetEntry<std::size_t>(pSize, entry);
   
    for(std::size_t i = 0; i < nInitPart; ++i){
        if(GetWPIndex(entry, pHash, i) != std::numeric_limits<std::size_t>::max()) ++nPart;
    }

    return nPart;
}

float NTupleReader::HT(const std::size_t& entry, const std::size_t& jetHash, TLeaf* jetPt){
    std::size_t nJets = RUtil::GetLen(jetPt, entry);

    float sumPt = 0.;

    for(std::size_t i = 0; i < nJets; ++i){
        std::size_t idx = GetWPIndex(entry, jetHash, i);
    
        if(idx != std::numeric_limits<std::size_t>::max()) sumPt += RUtil::GetEntry<float>(jetPt, entry, idx);
    }

    return sumPt;
}

float NTupleReader::dR(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* phi1, TLeaf* phi2, TLeaf* eta1, TLeaf* eta2){
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);
    std::size_t p2WpIdx = GetWPIndex(entry, p2Hash, p2Idx);

    if(p1WpIdx != std::numeric_limits<std::size_t>::max() and p2WpIdx != std::numeric_limits<std::size_t>::max()){
        return std::sqrt(std::pow(RUtil::GetEntry<float>(phi1, entry, p1WpIdx) - RUtil::GetEntry<float>(phi2, entry, p2WpIdx), 2) +
                         std::pow(RUtil::GetEntry<float>(eta1, entry, p1WpIdx) - RUtil::GetEntry<float>(eta2, entry, p2WpIdx), 2));
    }

    else return -999.;
}


float NTupleReader::diCharge(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* ch1, TLeaf* ch2){    
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);
    std::size_t p2WpIdx = GetWPIndex(entry, p2Hash, p2Idx);

    if(p1WpIdx != std::numeric_limits<std::size_t>::max() and p2WpIdx != std::numeric_limits<std::size_t>::max()){
        return RUtil::GetEntry<float>(ch1, entry, p1WpIdx) * RUtil::GetEntry<float>(ch2, entry, p2Idx);
    }

    else return -999.;
}

float NTupleReader::dPhi(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* phi1, TLeaf* phi2){
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);
    std::size_t p2WpIdx = GetWPIndex(entry, p2Hash, p2Idx);

    if(p1WpIdx != std::numeric_limits<std::size_t>::max() and p2WpIdx != std::numeric_limits<std::size_t>::max()){
        return std::acos(std::cos(RUtil::GetEntry<float>(phi1, entry, p1WpIdx))*std::cos(RUtil::GetEntry<float>(phi2, entry, p2WpIdx)) 
                       + std::sin(RUtil::GetEntry<float>(phi1, entry, p1WpIdx))*std::sin(RUtil::GetEntry<float>(phi2, entry, p2WpIdx)));
    }

    else return -999.;
}

float NTupleReader::dPhi3(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p3Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, const std::size_t& p3Hash, TLeaf* phi1, TLeaf* pt2, TLeaf* phi2, TLeaf* eta2, TLeaf* pt3, TLeaf* phi3, TLeaf* eta3){
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);
    std::size_t p2WpIdx = GetWPIndex(entry, p2Hash, p2Idx);
    std::size_t p3WpIdx = GetWPIndex(entry, p3Hash, p3Idx);

    if(p1WpIdx != std::numeric_limits<std::size_t>::max() and p2WpIdx != std::numeric_limits<std::size_t>::max() and p3WpIdx != std::numeric_limits<std::size_t>::max()){
        ROOT::Math::PtEtaPhiMVector newP = ROOT::Math::PtEtaPhiMVector(
                                                RUtil::GetEntry<float>(pt2, entry, p2WpIdx),
                                                RUtil::GetEntry<float>(eta2, entry, p2WpIdx),
                                                RUtil::GetEntry<float>(phi2, entry, p2WpIdx), 0.) +

                                           ROOT::Math::PtEtaPhiMVector(
                                                RUtil::GetEntry<float>(pt3, entry, p3WpIdx),
                                                RUtil::GetEntry<float>(eta3, entry, p3WpIdx),
                                                RUtil::GetEntry<float>(phi3, entry, p3WpIdx), 0.);

        return std::acos(std::cos(newP.Phi())*std::cos(RUtil::GetEntry<float>(phi1, entry, p1WpIdx)) 
                       + std::sin(newP.Phi())*std::sin(RUtil::GetEntry<float>(phi1, entry, p1WpIdx)));

    }

    else return -999.;
}

float NTupleReader::dR3(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p3Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, const std::size_t& p3Hash, TLeaf* phi1, TLeaf* eta1, TLeaf* pt2, TLeaf* phi2, TLeaf* eta2, TLeaf* pt3, TLeaf* phi3, TLeaf* eta3){    
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);
    std::size_t p2WpIdx = GetWPIndex(entry, p2Hash, p2Idx);
    std::size_t p3WpIdx = GetWPIndex(entry, p3Hash, p3Idx);

    if(p1WpIdx != std::numeric_limits<std::size_t>::max() and p2WpIdx != std::numeric_limits<std::size_t>::max() and p3WpIdx != std::numeric_limits<std::size_t>::max()){
        ROOT::Math::PtEtaPhiMVector newP = ROOT::Math::PtEtaPhiMVector(
                                                RUtil::GetEntry<float>(pt2, entry, p2WpIdx),
                                                RUtil::GetEntry<float>(eta2, entry, p2WpIdx),
                                                RUtil::GetEntry<float>(phi2, entry, p2WpIdx), 0.) +

                                           ROOT::Math::PtEtaPhiMVector(
                                                RUtil::GetEntry<float>(pt3, entry, p3WpIdx),
                                                RUtil::GetEntry<float>(eta3, entry, p3WpIdx),
                                                RUtil::GetEntry<float>(phi3, entry, p3WpIdx), 0.);

        return std::sqrt(std::pow(RUtil::GetEntry<float>(phi1, entry, p1WpIdx) - newP.Phi(), 2) +
                         std::pow(RUtil::GetEntry<float>(eta1, entry, p1WpIdx) - newP.Eta(), 2));

    }

    else return -999.;
}

float NTupleReader::diMass(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* pt1, TLeaf* pt2, TLeaf* phi1, TLeaf* phi2, TLeaf* eta1, TLeaf* eta2){
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);
    std::size_t p2WpIdx = GetWPIndex(entry, p2Hash, p2Idx);

    if(p1WpIdx != std::numeric_limits<std::size_t>::max() and p2WpIdx != std::numeric_limits<std::size_t>::max()){
        ROOT::Math::PtEtaPhiMVector p1(RUtil::GetEntry<float>(pt1, entry, p1WpIdx), 
                                       RUtil::GetEntry<float>(eta1, entry, p1WpIdx),
                                       RUtil::GetEntry<float>(phi1, entry, p1WpIdx), 0.);
        ROOT::Math::PtEtaPhiMVector p2(RUtil::GetEntry<float>(pt2, entry, p2WpIdx), 
                                       RUtil::GetEntry<float>(eta2, entry, p2WpIdx),
                                       RUtil::GetEntry<float>(phi2, entry, p2WpIdx), 0.);


        return (p1 + p2).M();
    }
    
    else return -999.;
}

float NTupleReader::diMT(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* pt1, TLeaf* pt2, TLeaf* phi1, TLeaf* phi2){
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);
    std::size_t p2WpIdx = GetWPIndex(entry, p2Hash, p2Idx);

    if(p1WpIdx != std::numeric_limits<std::size_t>::max() and p2WpIdx != std::numeric_limits<std::size_t>::max()){
        return std::sqrt(2*RUtil::GetEntry<float>(pt1, entry, p1WpIdx)*RUtil::GetEntry<float>(pt2, entry, p2WpIdx)*(
               1 - std::cos(dPhi(entry, p1Idx, p2Idx, p1Hash, p2Hash, phi1, phi2))));
    }
    
    else return -999.;
}


float NTupleReader::LP(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p1Hash, TLeaf* pt1, TLeaf* ptMet, TLeaf* phi1, TLeaf* phiMet){
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);

    if(p1WpIdx != std::numeric_limits<std::size_t>::max()){
        ROOT::Math::PtEtaPhiMVector l(RUtil::GetEntry<float>(pt1, entry, p1WpIdx), 0, RUtil::GetEntry<float>(phi1, entry, p1WpIdx), 0);
        ROOT::Math::PtEtaPhiMVector MET(RUtil::GetEntry<float>(ptMet, entry, 0), 0, RUtil::GetEntry<float>(phiMet, entry, 0), 0);
        ROOT::Math::PtEtaPhiMVector W = l + MET;

        float dPhi = l.phi() - W.phi();
        while (dPhi >= TMath::Pi()) dPhi -= TMath::TwoPi();
        while (dPhi < -TMath::Pi()) dPhi += TMath::TwoPi();

        return l.pt()/W.pt()*std::cos(abs(dPhi));
    }

    else return -999.;
}


float NTupleReader::ModifiedEntry(const int& entry, TLeaf* evNr){
    return 1./RUtil::GetEntry<float>(evNr, entry)*10e10;
}

float NTupleReader::isGenMatched(const int& entry, const std::size_t& p1Idx, const std::size_t& p1Hash, TLeaf* gID){
    std::size_t p1WpIdx = GetWPIndex(entry, p1Hash, p1Idx);

    return RUtil::GetEntry<float>(gID, entry, p1WpIdx) > -20.;
}
