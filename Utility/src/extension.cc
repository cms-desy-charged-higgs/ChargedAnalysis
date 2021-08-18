/**
* @file extension.cc
* @brief Source file for Extension name space, see extension.h
*/

#include <ChargedAnalysis/Utility/include/extension.h>

std::map<std::string, std::vector<float>> Extension::HScore(std::shared_ptr<TFile>& file, const std::string& channel, const int& era){
    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    /*
    std::vector<std::string> branchNames;
    
    if(Utils::Find<std::string>(channel, "2FJ") == -1.) branchNames = {"ML_HTagFJ1"};
    else branchNames = {"HTag_FJ1", "HTag_FJ2"};   

    //Path with DNN infos
    std::string dnnPath = std::string(std::getenv("CHDIR")) + "/DNN"; 

    int nEntries = file->Get<TTree>(channel.c_str())->GetEntries();

    for(const std::string& branchName: branchNames){
        values[branchName] = std::vector<float>(nEntries, -999.);
    }

    for(int i = 0; i < branchNames.size(); i++){
        //Take time
        Utils::RunTime timer;

        std::unique_ptr<Frame> hyperParam = std::make_unique<Frame>(dnnPath + "/Tagger/HyperOpt/hyperparameter.csv");

        //Tagger
        torch::Device device(torch::kCPU);
        std::vector<std::shared_ptr<HTagger>> tagger(2, std::make_shared<HTagger>(4, hyperParam->Get("nHidden", 0), hyperParam->Get("nConvFilter", 0), hyperParam->Get("kernelSize", 0), hyperParam->Get("dropOut", 0), device));

        torch::load(tagger[0], StrUtil::Join("/", std::getenv("CHDIR"), "DNN/Tagger/Even", era, "htagger.pt"));
        torch::load(tagger[1], StrUtil::Join("/", std::getenv("CHDIR"), "DNN/Tagger/Odd", era, "htagger.pt"));

        tagger[0]->eval();
        tagger[1]->eval();
        torch::NoGradGuard no_grad;

        std::shared_ptr<TTree> tree(static_cast<TTree*>(file->Get<TTree>(channel.c_str())->Clone()));

        //Get data set
        HTagDataset data = HTagDataset(file, tree, {}, "", i, device, true);
        std::vector<int> entries;

        //Set tree parser and tree functions
        TreeParser parser;
        std::vector<TreeFunction> functions;
        TreeFunction evNr(file, channel);
        evNr.SetFunction<Axis::X>("EvNr");

        for(int i = 0; i < nEntries; i++){entries.push_back(i);}
        std::vector<int>::iterator entry = entries.begin();
        int batchSize = nEntries > 2500 ? 2500 : nEntries;
        int counter = 0;
        
        while(entry != entries.end()){
            //For right indexing
            std::vector<int> evenIndex, oddIndex;

            //Put signal + background in one vector and split by even or odd numbered event
            std::vector<HTensor> evenTensors;
            std::vector<HTensor> oddTensors;

            for(int k = 0; k < batchSize; k++){
                if(*entry % 1000 == 0 and *entry != 0){
                    std::cout << "Processed events: " << *entry << " (" << *entry/timer.Time() << " eve/s)" << std::endl;
                }

                HTensor tensor = data.get(*entry);

                if(int(evNr.Get<Axis::X>()) % 2 == 0){
                    evenTensors.push_back(tensor);
                    evenIndex.push_back(counter);
                }

                else{
                    oddTensors.push_back(tensor);
                    oddIndex.push_back(counter);
                }

                counter++;
                ++entry;
                if(entry == entries.end()) break; 
            }

            //Predictio
            HTensor even, odd; 
            torch::Tensor evenPredict, oddPredict;      

            if(evenTensors.size() != 0){
                even = HTagDataset::PadAndMerge(evenTensors);
                evenPredict = tagger[1]->forward(even.charged, even.neutral, even.SV);
            }  

            if(oddTensors.size() != 0){    
                odd = HTagDataset::PadAndMerge(oddTensors);
                oddPredict = tagger[0]->forward(odd.charged, odd.neutral, odd.SV);
            }     

            //Put all predictions back in order again
            for(int j = 0; j < evenTensors.size(); j++){
                values[branchNames[i]].at(evenIndex.at(j)) = evenTensors.size() != 1 ? evenPredict[j].item<float>() : evenPredict.item<float>();
            }

            for(int j = 0; j < oddTensors.size(); j++){
                values[branchNames[i]].at(oddIndex.at(j)) = oddTensors.size() != 1 ? oddPredict[j].item<float>() : oddPredict.item<float>();
            }
        }
    }
    */
    return values;
}

std::map<std::string, std::vector<float>> Extension::DNNScore(std::shared_ptr<TFile>& file, const std::string& channel, const int& era){
    //Set values with default values
    std::map<std::string, std::vector<float>> values;

    //Take time
    Utils::RunTime timer;
    std::shared_ptr<TTree> tree = RUtil::CloneSmart<TTree>(RUtil::Get<TTree>(file.get(), channel));
    int nEntries = tree->GetEntries();

    //Path with DNN infos
    std::string dnnPath = std::string(std::getenv("CHDIR")) + "/Results/DNN/";  

    //Set tree parser and tree functions
    Decoder parser;
    std::vector<NTupleReader> functions;
    TLeaf* evNr = RUtil::Get<TLeaf>(tree.get(), "Misc_eventNumber");

    //Read txt with parameter used in the trainind and set tree function
    std::ifstream params(StrUtil::Join("/", dnnPath, "Main", channel, era, "Even/parameter.txt")); 
    std::string parameter;

    while(getline(params, parameter)){
        NTupleReader func(tree, era);

        parser.GetParticle(parameter, func);
        parser.GetFunction(parameter, func);
        func.Compile();

        functions.push_back(func);
    }
    params.close();
    
    //Get classes
    std::ifstream clsFile(StrUtil::Join("/", dnnPath, "Main", channel, era, "Even/classes.txt")); 
    std::string cls;
    
    std::vector<std::string> classes;
    
    while(getline(clsFile, cls)){
        classes.push_back(cls);
    }
    classes.push_back("HPlus");
    
    clsFile.close();
    
        //Get classes
    std::ifstream massFile(StrUtil::Join("/", dnnPath, "Main", channel, era, "Even/masses.txt")); 
    std::string mass;
    
    std::vector<int> masses;
    
    while(getline(massFile, mass)){
        masses.push_back(std::atoi(mass.c_str()));
    }
    massFile.close();
    
    //Define branch names
    std::vector<std::string> branchNames;
    
    for(int& mass : masses){
        for(std::string& cls : classes){
            branchNames.push_back(StrUtil::Merge("DNN_", cls, mass));
            values[branchNames.back()] = std::vector<float>(nEntries, -999.);
        }
    }

    for(int& mass : masses){
        branchNames.push_back(StrUtil::Merge("DNN_Class", mass));
        values[branchNames.back()] = std::vector<float>(nEntries, -999.);
    }

    //Frame with optimized hyperparameter
    std::unique_ptr<Frame> hyperParam = std::make_unique<Frame>(StrUtil::Join("/", dnnPath, "HyperOpt", channel, era, "hyperparameter.csv"));

    //Get/load model and set to evaluation mode
    torch::Device device(torch::kCPU);
    std::vector<std::shared_ptr<DNNModel>> model(2, std::make_shared<DNNModel>(functions.size(), hyperParam->Get("n-nodes", 0), hyperParam->Get("n-layers", 0), hyperParam->Get("drop-out", 0), true, classes.size(), device));

    model[0]->eval();
    model[1]->eval();
    
    model[0]->Print();

    torch::load(model[0], StrUtil::Join("/", dnnPath, "Main", channel, era, "Even/model.pt"));
    torch::load(model[1], StrUtil::Join("/", dnnPath, "Main", channel, era, "Odd/model.pt"));

    torch::NoGradGuard no_grad;

    std::vector<int> entries;
    for(int i = 0; i < nEntries; i++){entries.push_back(i);}
    std::vector<int>::iterator entry = entries.begin();
    int batchSize = nEntries > 2500 ? 2500 : nEntries;
    int counter = 0;

    while(entry != entries.end()){
        //For right indexing
        std::vector<int> evenIndex, oddIndex;

        //Put signal + background in one vector and split by even or odd numbered event
        std::vector<torch::Tensor> evenTensors;
        std::vector<torch::Tensor> oddTensors;

        for(int j = 0; j < batchSize; j++){
            if(*entry % 1000 == 0 and *entry != 0){
                std::cout << "Processed events: " << *entry << " (" << *entry/timer.Time() << " eve/s)" << std::endl;
            }

            NTupleReader::SetEntry(*entry);

            //Fill event class with particle content
            std::vector<float> paramValues;

            for(int i=0; i < functions.size(); ++i){
                paramValues.push_back(functions[i].Get());
            }

            if(int(1/RUtil::GetEntry<float>(evNr, *entry)*10e10) % 2 == 0){
                evenTensors.push_back(torch::from_blob(paramValues.data(), {1, paramValues.size()}).clone().to(device));
                evenIndex.push_back(counter);
            }

            else{
                oddTensors.push_back(torch::from_blob(paramValues.data(), {1, paramValues.size()}).clone().to(device));
                oddIndex.push_back(counter);
            }

            counter++;
            ++entry;
            if(entry == entries.end()) break; 
        }

        for(int i = 0; i < masses.size(); ++i){
            //Prediction
            torch::Tensor even, odd; 
            torch::Tensor evenPredict, oddPredict;

            if(evenTensors.size() != 0){    
                even = torch::cat(evenTensors, 0);
                evenPredict = model[1]->forward(even, masses[i]*torch::ones({evenTensors.size(), 1}), true);
            }  

            if(oddTensors.size() != 0){    
                odd = torch::cat(oddTensors, 0);
                oddPredict = model[0]->forward(odd, masses[i]*torch::ones({oddTensors.size(), 1}), true);
            }

            //Put all predictions back in order again
            for(int j = 0; j < evenTensors.size(); ++j){
                for(int k = 0; k < classes.size(); ++k){
                    values[branchNames[k + i*classes.size()]].at(evenIndex.at(j)) = evenTensors.size() != 1 ? evenPredict.index({j, k}).item<float>() : evenPredict[k].item<float>();
                }

                values[StrUtil::Merge("DNN_Class", masses.at(i))].at(evenIndex.at(j)) = torch::argmax(evenPredict[j]).item<int>();
            }

            for(int j = 0; j < oddTensors.size(); ++j){
                for(int k = 0; k < classes.size(); ++k){
                    values[branchNames[k + i*classes.size()]].at(oddIndex.at(j)) = oddTensors.size() != 1 ? oddPredict.index({j, k}).item<float>() : oddPredict[k].item<float>();
                }

                values[StrUtil::Merge("DNN_Class", masses.at(i))].at(oddIndex.at(j)) = torch::argmax(oddPredict[j]).item<int>();
            }
        }
    }

    return values;
}

std::map<std::string, std::vector<float>> Extension::HReconstruction(std::shared_ptr<TFile>& file, const std::string& channel, const int& era){
    typedef ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> PolarLV;
    typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>> CartLV;
    typedef std::vector<std::pair<PolarLV, PolarLV>> hCandVec;

    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    std::vector<std::string> branchNames = {"W_Mass", "W_Pt", "W_Phi", "H1_Pt", "H1_Eta", "H1_Phi", "H1_Mass", "H2_Pt", "H2_Eta", "H2_Phi", "H2_Mass", "HPlus_Pt", "HPlus_Mass", "HPlus_Phi"};

    std::shared_ptr<TTree> tree = RUtil::CloneSmart<TTree>(RUtil::Get<TTree>(file.get(), channel));

    for(const std::string& branchName: branchNames){
        values[branchName] = std::vector<float>(tree->GetEntries(), -999.);
    }
    
    std::string lepName = !StrUtil::Find(channel, "Muon").empty() ? "mu" : "e";
    float lepMass = !StrUtil::Find(channel, "Muon").empty() ? 0.10565: 0.000510;

    NTupleReader LepPt(tree, era), LepEta(tree, era), LepPhi(tree, era);
    LepPt.AddParticle(lepName, 1, "medium"); LepEta.AddParticle(lepName, 1, "medium"); LepPhi.AddParticle(lepName, 1, "medium");
    LepPt.AddFunction("pt"); LepEta.AddFunction("eta"); LepPhi.AddFunction("phi");
    LepPt.Compile(); LepEta.Compile(); LepPhi.Compile();

    NTupleReader METPt(tree, era), METPhi(tree, era);
    METPt.AddParticle("met", 0, ""); METPhi.AddParticle("met", 0, "");
    METPt.AddFunction("pt"); METPhi.AddFunction("phi");
    METPt.Compile(); METPhi.Compile();

    NTupleReader JetPt(tree, era), JetEta(tree, era), JetPhi(tree, era), JetMass(tree, era);
    JetPt.AddParticle("j", 0, ""); JetEta.AddParticle("j", 0, ""); JetPhi.AddParticle("j", 0, ""); JetMass.AddParticle("j", 0, "");
    JetPt.AddFunction("pt"); JetEta.AddFunction("eta"); JetPhi.AddFunction("phi"); JetMass.AddFunction("m");
    JetPt.Compile(); JetEta.Compile(); JetPhi.Compile(); JetMass.Compile();

    NTupleReader FatJetPt(tree, era), FatJetEta(tree, era), FatJetPhi(tree, era), FatJetMass(tree, era);
    FatJetPt.AddParticle("fj", 0, ""); FatJetEta.AddParticle("fj", 0, ""); FatJetPhi.AddParticle("fj", 0, ""); FatJetMass.AddParticle("fj", 0, "");
    FatJetPt.AddFunction("pt"); FatJetEta.AddFunction("eta"); FatJetPhi.AddFunction("phi"); FatJetMass.AddFunction("m");
    FatJetPt.Compile(); FatJetEta.Compile(); FatJetPhi.Compile(); FatJetMass.Compile();

    float pXNu, pYNu, pXLep, pYLep, pZLep, mW;

    for(int i = 0; i < tree->GetEntries(); ++i){
        NTupleReader::SetEntry(i);

        PolarLV LepLV(LepPt.Get(), LepEta.Get(), LepPhi.Get(), lepMass);
        PolarLV W = LepLV + PolarLV(METPt.Get(), 0, METPhi.Get(), 0);

        if(LepLV.Pt() == -999.) continue;

        values["W_Mass"][i] = W.M();
        values["W_Pt"][i] = W.Pt();
        values["W_Phi"][i] = W.Phi();

        std::vector<PolarLV> jets;
        std::vector<PolarLV> fatJets;

        JetPt.Reset(); JetEta.Reset(); JetPhi.Reset(); JetMass.Reset();
        FatJetPt.Reset(); FatJetEta.Reset(); FatJetPhi.Reset(); FatJetMass.Reset();

        while(true){
            PolarLV jet = PolarLV(JetPt.Get(), JetEta.Get(), JetPhi.Get(), JetMass.Get());

            if(jet.Pt() != -999.){
                jets.push_back(jet);
                JetPt.Next(); JetEta.Next(); JetPhi.Next(); JetMass.Next();
            }

            else break;
        }

        while(true){
            PolarLV fatJet = PolarLV(FatJetPt.Get(), FatJetEta.Get(), FatJetPhi.Get(), FatJetMass.Get());

            if(fatJet.Pt() != -999.){
                fatJets.push_back(fatJet);
                FatJetPt.Next(); FatJetEta.Next(); FatJetPhi.Next(); FatJetMass.Next();
            }

            else break;
        }

        //Intermediate step to save all possible combinations of two jets from jet collection
        std::vector<std::pair<int, int>> combi;
        
        for(unsigned int k = 0; k < jets.size(); k++){
            for(unsigned int j = 0; j < k; j++){
                combi.push_back({k, j});
            }
        }

        //Vector of candPairs
        hCandVec hCands; 
    
        //If 4 jets and no fat jets
        if(jets.size() >= 4){
            //Construct all pairs of possible jet pairs
            for(unsigned int k = 0; k < combi.size(); ++k){
                for(unsigned int j = 0; j < k; ++j){
                    //Check if not same jet in both pair
                    std::set<int> check = {combi[k].first, combi[k].second, combi[j].first, combi[j].second};
                   
                    if(check.size() == 4){
                        hCands.push_back({jets[combi[k].first] + jets[combi[k].second], jets[combi[j].first] + jets[combi[j].second]});
                    }
                }
            }
        }

        //If 2 jets and one fat jet
        else if(jets.size() >= 2 and fatJets.size() == 1){
            for(std::pair<int, int> jetIndex: combi){
                hCands.push_back({fatJets[0], jets[jetIndex.first] + jets[jetIndex.second]});
            }
        }

        //If 2 fat jets
        else if(fatJets.size() == 2){
            hCands.push_back({fatJets[0], fatJets[1]});
        }

        //If not right jet configuration is given
        else continue;

        //Sort candPairs for mass diff of jet pairs, where index 0 is the best pair
        std::function<bool(std::pair<PolarLV, PolarLV>, std::pair<PolarLV, PolarLV>)> sortFunc = [&](std::pair<PolarLV, PolarLV> cands1, std::pair<PolarLV, PolarLV> cands2){  
            return std::abs(cands1.first.M() - cands1.second.M()) < std::abs(cands2.first.M() - cands2.second.M());
        };

        std::sort(hCands.begin(), hCands.end(), sortFunc);

        PolarLV Hc1 = hCands[0].first + W;
        PolarLV Hc2 = hCands[0].second + W;

        if(ROOT::Math::VectorUtil::DeltaPhi(hCands[0].first, Hc1) < ROOT::Math::VectorUtil::DeltaPhi(hCands[0].second, Hc2)){
            values["H1_Pt"][i] = hCands[0].first.Pt();
            values["H1_Eta"][i] = hCands[0].first.Eta();
            values["H1_Phi"][i] = hCands[0].first.Phi();
            values["H1_Mass"][i] = hCands[0].first.M();
            values["H2_Pt"][i] = hCands[0].second.Pt();
            values["H2_Eta"][i] = hCands[0].second.Eta();
            values["H2_Phi"][i] = hCands[0].second.Phi();
            values["H2_Mass"][i] = hCands[0].second.M();
            values["HPlus_Mass"][i] = Hc1.M();
            values["HPlus_Pt"][i] = Hc1.Pt();
            values["HPlus_Phi"][i] = Hc1.Phi();
        }

        else{
            values["H1_Pt"][i] = hCands[0].second.Pt();
            values["H1_Eta"][i] = hCands[0].second.Eta();
            values["H1_Phi"][i] = hCands[0].second.Phi();
            values["H1_Mass"][i] = hCands[0].second.M();
            values["H2_Pt"][i] = hCands[0].first.Pt();
            values["H2_Eta"][i] = hCands[0].first.Eta();
            values["H2_Phi"][i] = hCands[0].first.Phi();
            values["H2_Mass"][i] = hCands[0].first.M();
            values["HPlus_Mass"][i] = Hc2.M();
            values["HPlus_Phi"][i] = Hc2.Phi();
            values["HPlus_Pt"][i] = Hc2.Pt();
        }
    }

    return values;
}
