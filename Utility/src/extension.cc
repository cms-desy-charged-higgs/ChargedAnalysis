/**
* @file extension.cc
* @brief Source file for Extension name space, see extension.h
*/

#include <ChargedAnalysis/Utility/include/extension.h>

std::map<std::string, std::vector<float>> Extension::HScore(std::shared_ptr<TFile>& file, const std::string& channel){
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
        std::unique_ptr<Frame> hyperParam = std::make_unique<Frame>(dnnPath + "/Tagger/HyperOpt/hyperparameter.csv");

        //Tagger
        torch::Device device(torch::kCPU);
        std::vector<std::shared_ptr<HTagger>> tagger(2, std::make_shared<HTagger>(7, hyperParam->Get("nHidden", 0), hyperParam->Get("nConvFilter", 0), hyperParam->Get("kernelSize", 0), hyperParam->Get("dropOut", 0), device));

        torch::load(tagger[0], std::string(std::getenv("CHDIR")) + "/DNN/Tagger/Even/htagger.pt");
        torch::load(tagger[1], std::string(std::getenv("CHDIR")) + "/DNN/Tagger/Odd/htagger.pt");

        tagger[0]->eval();
        tagger[1]->eval();
        torch::NoGradGuard no_grad;

        //Get data set
        HTagDataset data = HTagDataset({std::string(file->GetName()) + "/"  + channel}, i, device, true);
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

std::map<std::string, std::vector<float>> Extension::DNNScore(std::shared_ptr<TFile>& file, const std::string& channel){
    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    std::vector<int> masses = {200, 300, 400, 500, 600};
    std::vector<std::string> branchNames = {"ML_DNN200", "ML_DNN300", "ML_DNN400", "ML_DNN500", "ML_DNN600"};
        
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
    std::ifstream params(dnnPath + "/Even/" + channel + "/parameter.txt"); 
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

    torch::load(model[0], dnnPath + "/Even/" + channel + "/model.pt");
    torch::load(model[1], dnnPath + "/Odd/" + channel + "/model.pt");

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

std::map<std::string, std::vector<float>> Extension::HReconstruction(std::shared_ptr<TFile>& file, const std::string& channel){
    typedef ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> PolarLV;
    typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>> CartLV;
    typedef std::vector<std::pair<PolarLV, PolarLV>> hCandVec;

    std::function<float(const float, const float)> smear = [&](const float& nominal, const float& sigma){
        std::random_device rd;
        std::default_random_engine generator(rd());
        std::normal_distribution<double> distribution(nominal, sigma);

        return distribution(generator);
    };

    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    std::vector<std::string> branchNames = {"W_Mass", "W_Pt", "W_Eta", "W_Phi", "H1_Pt", "H1_Eta", "H1_Phi", "H1_Mass", "H2_Pt", "H2_Eta", "H2_Phi", "H2_Mass", "HPlus_Pt", "HPlus_Eta", "HPlus_Phi", "HPlus_Mass"};

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

    for(int i = 0; i < nEntries; i++){
        TreeFunction::SetEntry(i);

        PolarLV LepLV(LepPt.Get<Axis::X>(), LepEta.Get<Axis::X>(), LepPhi.Get<Axis::X>(), lepMass);

        float pZNu1, pZNu2, pXNu, pYNu, pXLep, pYLep, pZLep, mW;
        int counter = 0;

        while(true){
            counter++;

            pXNu = smear(METPt.Get<Axis::X>()*std::cos(METPhi.Get<Axis::X>()), 1. + counter*2);
            pYNu = smear(METPt.Get<Axis::X>()*std::sin(METPhi.Get<Axis::X>()), 1. + counter*2);

            pXLep = smear(LepLV.Px(), 1. + counter);
            pYLep = smear(LepLV.Py(), 1. + counter);
            pZLep = smear(LepLV.Pz(), 1. + counter);

            mW = smear(80.399, 10.);
        
        //Analytic solutions to quadratic problem
      //  float pZNu1 = pZLep*sqrt(std::pow(pXNu, 2) + std::pow(pYNu, 2))/sqrt(std::pow(lepMass, 2) + std::pow(pXLep, 2) + std::pow(pYLep, 2));
      //  float pZNu2 = - pZLep*sqrt(std::pow(pXNu, 2) + std::pow(pYNu, 2))/sqrt(std::pow(lepMass, 2) + std::pow(pXLep, 2) + std::pow(pYLep, 2));

            pZNu1 = pZLep*(std::pow(mW, 2) - std::pow(lepMass, 2) + 2*pXLep*pXNu + 2*pYLep*pYNu)/(2*(std::pow(lepMass, 2) + std::pow(pXLep, 2) + std::pow(pYLep, 2))) - sqrt(std::pow(lepMass, 2) + std::pow(pXLep, 2) + std::pow(pYLep, 2) + std::pow(pZLep, 2))*sqrt(std::pow(mW, 4) - 2*std::pow(mW, 2)*std::pow(lepMass, 2) + 4*std::pow(mW, 2)*pXLep*pXNu + 4*std::pow(mW, 2)*pYLep*pYNu + std::pow(lepMass, 4) - 4*std::pow(lepMass, 2)*pXLep*pXNu - 4*std::pow(lepMass, 2)*std::pow(pXNu, 2) - 4*std::pow(lepMass, 2)*pYLep*pYNu - 4*std::pow(lepMass, 2)*std::pow(pYNu, 2) - 4*std::pow(pXLep, 2)*std::pow(pYNu, 2) + 8*pXLep*pXNu*pYLep*pYNu - 4*std::pow(pXNu, 2)*std::pow(pYLep, 2))/(2*(std::pow(lepMass, 2) + std::pow(pXLep, 2) + std::pow(pYLep, 2)));
            pZNu2 = pZLep*(std::pow(mW, 2) - std::pow(lepMass, 2) + 2*pXLep*pXNu + 2*pYLep*pYNu)/(2*(std::pow(lepMass, 2) + std::pow(pXLep, 2) + std::pow(pYLep, 2))) + sqrt(std::pow(lepMass, 2) + std::pow(pXLep, 2) + std::pow(pYLep, 2) + std::pow(pZLep, 2))*sqrt(std::pow(mW, 4) - 2*std::pow(mW, 2)*std::pow(lepMass, 2) + 4*std::pow(mW, 2)*pXLep*pXNu + 4*std::pow(mW, 2)*pYLep*pYNu + std::pow(lepMass, 4) - 4*std::pow(lepMass, 2)*pXLep*pXNu - 4*std::pow(lepMass, 2)*std::pow(pXNu, 2) - 4*std::pow(lepMass, 2)*pYLep*pYNu - 4*std::pow(lepMass, 2)*std::pow(pYNu, 2) - 4*std::pow(pXLep, 2)*std::pow(pYNu, 2) + 8*pXLep*pXNu*pYLep*pYNu - 4*std::pow(pXNu, 2)*std::pow(pYLep, 2))/(2*(std::pow(lepMass, 2) + std::pow(pXLep, 2) + std::pow(pYLep, 2)));

            if(!std::isnan(pZNu1)) break;
        }

        //Neutrino candidates from solutions above
        CartLV v1(pXNu, pYNu, pZNu1, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu1*pZNu1));
        CartLV v2(pXNu, pYNu, pZNu2, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu2*pZNu2));

        //Take neutrino which gives physical reasonabel result
        CartLV smearLep(pXLep, pYLep, pZLep, std::sqrt(pXLep*pXLep + pYLep*pYLep + pZLep*pZLep + lepMass*lepMass));
        CartLV W1 = smearLep + v1, W2 = smearLep + v2;
        CartLV W = std::pow(W1.M() - mW, 2) < std::pow(W2.M() - mW, 2) ? W1 : W2;

        values["W_Mass"][i] = W.M();
        values["W_Pt"][i] = W.Pt();
        values["W_Phi"][i] = W.Phi();
        values["W_Eta"][i] = W.Eta();

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
            int goodFeat1 = 0, goodFeat2 = 0;

            if(ROOT::Math::VectorUtil::DeltaPhi(cands1.first, cands1.second) > ROOT::Math::VectorUtil::DeltaPhi(cands2.first, cands2.second)) goodFeat1++;
            else goodFeat2++;

            if(std::pow(cands1.first.M() - cands1.second.M(), 2)  < std::pow(cands2.first.M() - cands2.second.M(), 2)) goodFeat1++;
            else goodFeat2++;

            if(cands1.first.M() < 150) goodFeat1++;
            if(cands1.second.M() < 150) goodFeat1++;
            if(cands2.first.M() < 150) goodFeat2++;
            if(cands2.second.M() < 150) goodFeat2++;

            return goodFeat1 > goodFeat2;
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
            values["HPlus_Pt"][i] = Hc1.Pt();
            values["HPlus_Phi"][i] = Hc1.Phi();
            values["HPlus_Eta"][i] = Hc1.Eta();
            values["HPlus_Mass"][i] = Hc1.M();
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
            values["HPlus_Pt"][i] = Hc2.Pt();
            values["HPlus_Phi"][i] = Hc2.Phi();
            values["HPlus_Eta"][i] = Hc2.Eta();
            values["HPlus_Mass"][i] = Hc2.M();
        }
    }

    return values;
}
