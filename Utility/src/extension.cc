/**
* @file extension.cc
* @brief Source file for Extension name space, see extension.h
*/

#include <ChargedAnalysis/Utility/include/extension.h>

std::map<std::string, std::vector<float>> Extension::HScore(std::shared_ptr<TFile>& file, const std::string& channel, const int& era){
    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    std::vector<std::string> branchNames;
    
    if(Utils::Find<std::string>(channel, "2FJ") == -1.) branchNames = {"ML_HTagFJ1"};
    else branchNames = {"ML_HTagFJ1", "ML_HTagFJ2"};   

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
        std::vector<std::shared_ptr<HTagger>> tagger(2, std::make_shared<HTagger>(7, hyperParam->Get("nHidden", 0), hyperParam->Get("nConvFilter", 0), hyperParam->Get("kernelSize", 0), hyperParam->Get("dropOut", 0), device));

        torch::load(tagger[0], std::string(std::getenv("CHDIR")) + "/DNN/Tagger/Even/htagger.pt");
        torch::load(tagger[1], std::string(std::getenv("CHDIR")) + "/DNN/Tagger/Odd/htagger.pt");

        tagger[0]->eval();
        tagger[1]->eval();
        torch::NoGradGuard no_grad;

        std::shared_ptr<TTree> tree(static_cast<TTree*>(file->Get<TTree>(channel.c_str())->Clone()));

        //Get data set
        HTagDataset data = HTagDataset(tree, i, device, true);
        std::vector<int> entries;

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

                if(tensor.isEven.item<bool>() == true){
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

    return values;
}

std::map<std::string, std::vector<float>> Extension::DNNScore(std::shared_ptr<TFile>& file, const std::string& channel, const int& era){
    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    std::vector<int> masses = {200, 300, 400, 500, 600};
    std::vector<std::string> branchNames = {"ML_DNN200", "ML_DNN300", "ML_DNN400", "ML_DNN500", "ML_DNN600"};

    //Take time
    Utils::RunTime timer;
        
    int nEntries = file->Get<TTree>(channel.c_str())->GetEntries();

    for(const std::string& branchName: branchNames){
        values[branchName] = std::vector<float>(nEntries, -999.);
    }

    //Path with DNN infos
    std::string dnnPath = std::string(std::getenv("CHDIR")) + "/DNN/Analysis/"; 

    //Set tree parser and tree functions
    TreeParser parser;
    std::vector<TreeFunction> functions;
    TreeFunction evNr(file, channel);
    evNr.SetFunction<Axis::X>("EvNr");

    //Read txt with parameter used in the trainind and set tree function
    std::ifstream params(StrUtil::Join("/", dnnPath, "Even", channel, era, "parameter.txt")); 
    std::string parameter;
  
    while(getline(params, parameter)){
        TreeFunction func(file, channel);

        parser.GetParticle(parameter, func);
        parser.GetFunction(parameter, func);

        functions.push_back(func);
    }
    params.close();

    //Frame with optimized hyperparameter
    std::unique_ptr<Frame> hyperParam = std::make_unique<Frame>(dnnPath + "/HyperOpt/" + channel + "/hyperparameter.csv");

    //Get/load model and set to evaluation mode
    torch::Device device(torch::kCPU);
    std::vector<std::shared_ptr<DNNModel>> model(2, std::make_shared<DNNModel>(functions.size(), hyperParam->Get("nNodes", 0), hyperParam->Get("nLayers", 0), hyperParam->Get("dropOut", 0), true, device));

    model[0]->eval();
    model[1]->eval();

    torch::load(model[0], StrUtil::Join("/", dnnPath, "Even", channel, era, "model.pt"));
    torch::load(model[1], StrUtil::Join("/", dnnPath, "Odd", channel, era, "model.pt"));

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

            TreeFunction::SetEntry(*entry);

            //Fill event class with particle content
            std::vector<float> paramValues;

            for(int i=0; i < functions.size(); i++){
                paramValues.push_back(functions[i].Get<Axis::X>());
            }

            if(int(evNr.Get<Axis::X>()) % 2 == 0){
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

        for(int i = 0; i < masses.size(); i++){
            //Prediction
            torch::Tensor even, odd; 
            torch::Tensor evenPredict, oddPredict;

            if(evenTensors.size() != 0){    
                even = torch::cat(evenTensors, 0);
                evenPredict = model[1]->forward(even, masses[i]*torch::ones({evenTensors.size(), 1}));
            }  

            if(oddTensors.size() != 0){    
                odd = torch::cat(oddTensors, 0);
                oddPredict = model[0]->forward(odd, masses[i]*torch::ones({oddTensors.size(), 1}));
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

    return values;
}

std::map<std::string, std::vector<float>> Extension::HReconstruction(std::shared_ptr<TFile>& file, const std::string& channel, const int& era){
    typedef ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> PolarLV;
    typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>> CartLV;
    typedef std::vector<std::pair<PolarLV, PolarLV>> hCandVec;

    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    std::vector<std::string> branchNames = {"W_Mass", "W_Pt", "W_Phi", "H1_Pt", "H1_Eta", "H1_Phi", "H1_Mass", "H2_Pt", "H2_Eta", "H2_Phi", "H2_Mass", "HPlus_Pt", "HPlus_Mass", "HPlus_Phi"};

    int nEntries = file->Get<TTree>(channel.c_str())->GetEntries();

    for(const std::string& branchName: branchNames){
        values[branchName] = std::vector<float>(nEntries, -999.);
    }
    
    std::string lepName = Utils::Find<std::string>(channel, "Muon") != -1. ? "mu" : "e";
    float lepMass = Utils::Find<std::string>(channel, "Muon") != - 1. ? 0.10565: 0.000510;

    TreeFunction LepPt(file, channel), LepEta(file, channel), LepPhi(file, channel);
    LepPt.SetP1<Axis::X>(lepName, 1); LepEta.SetP1<Axis::X>(lepName, 1); LepPhi.SetP1<Axis::X>(lepName, 1);
    LepPt.SetFunction<Axis::X>("Pt"); LepEta.SetFunction<Axis::X>("Eta"); LepPhi.SetFunction<Axis::X>("Phi");

    TreeFunction METPt(file, channel), METPhi(file, channel);
    METPt.SetP1<Axis::X>("met"); METPhi.SetP1<Axis::X>("met");
    METPt.SetFunction<Axis::X>("Pt"); METPhi.SetFunction<Axis::X>("Phi");
    
    TreeFunction nJet(file, channel), nFatJet(file, channel);
    nJet.SetP1<Axis::X>("j"); nFatJet.SetP1<Axis::X>("fj");
    nJet.SetFunction<Axis::X>("N"); nFatJet.SetFunction<Axis::X>("N");

    TreeFunction JetPt(file, channel), JetEta(file, channel), JetPhi(file, channel), JetMass(file, channel);
    JetPt.SetP1<Axis::X>("j"); JetEta.SetP1<Axis::X>("j"); JetPhi.SetP1<Axis::X>("j"); JetMass.SetP1<Axis::X>("j");
    JetPt.SetFunction<Axis::X>("Pt"); JetEta.SetFunction<Axis::X>("Eta"); JetPhi.SetFunction<Axis::X>("Phi"); JetMass.SetFunction<Axis::X>("Mass");

    TreeFunction FatJetPt(file, channel), FatJetEta(file, channel), FatJetPhi(file, channel), FatJetMass(file, channel);
    FatJetPt.SetP1<Axis::X>("fj"); FatJetEta.SetP1<Axis::X>("fj"); FatJetPhi.SetP1<Axis::X>("fj"); FatJetMass.SetP1<Axis::X>("fj");
    FatJetPt.SetFunction<Axis::X>("Pt"); FatJetEta.SetFunction<Axis::X>("Eta"); FatJetPhi.SetFunction<Axis::X>("Phi"); FatJetMass.SetFunction<Axis::X>("Mass");

    float pXNu, pYNu, pXLep, pYLep, pZLep, mW;

    for(int i = 0; i < nEntries; i++){
        TreeFunction::SetEntry(i);

        PolarLV LepLV(LepPt.Get<Axis::X>(), LepEta.Get<Axis::X>(), LepPhi.Get<Axis::X>(), lepMass);
        PolarLV W = LepLV + PolarLV(METPt.Get<Axis::X>(), 0, METPhi.Get<Axis::X>(), 0);

        values["W_Mass"][i] = W.M();
        values["W_Pt"][i] = W.Pt();
        values["W_Phi"][i] = W.Phi();

        std::vector<PolarLV> jets;
        std::vector<PolarLV> fatJets;

        for(unsigned int k = 0; k < nJet.Get<Axis::X>(); k++){
            JetPt.SetP1<Axis::X>("j", k+1); JetEta.SetP1<Axis::X>("j", k+1); JetPhi.SetP1<Axis::X>("j", k+1); JetMass.SetP1<Axis::X>("j", k+1);
            jets.push_back(PolarLV(JetPt.Get<Axis::X>(), JetEta.Get<Axis::X>(), JetPhi.Get<Axis::X>(), JetMass.Get<Axis::X>()));
        }

        for(unsigned int k = 0; k < nFatJet.Get<Axis::X>(); k++){
            FatJetPt.SetP1<Axis::X>("fj", k+1); FatJetEta.SetP1<Axis::X>("fj", k+1); FatJetPhi.SetP1<Axis::X>("fj", k+1); FatJetMass.SetP1<Axis::X>("fj", k+1);
            fatJets.push_back(PolarLV(FatJetPt.Get<Axis::X>(), FatJetEta.Get<Axis::X>(), FatJetPhi.Get<Axis::X>(), FatJetMass.Get<Axis::X>()));
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
            for(unsigned int k = 0; k < combi.size(); k++){
                for(unsigned int j = 0; j < k; j++){
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
            return ROOT::Math::VectorUtil::DeltaR(cands1.first, cands1.second) > ROOT::Math::VectorUtil::DeltaR(cands2.first, cands2.second);
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
