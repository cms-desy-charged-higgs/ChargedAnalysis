#include <ChargedAnalysis/Utility/include/extension.h>

std::map<std::string, std::vector<float>> Extension::HScore(TFile* file, const std::string& channel){
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    std::vector<std::string> branchNames;
    
    if(Utils::Find<std::string>(channel, "2FJ") == -1.) branchNames = {"ML_HTagFJ1"};
    else branchNames = {"ML_HTagFJ1", "ML_HTagFJ2"};   

    int nEntries = file->Get<TTree>(channel.c_str())->GetEntries();

    for(const std::string& branchName: branchNames){
        values[branchName] = std::vector<float>(nEntries, -999.);
    }

    for(int i = 0; i < branchNames.size(); i++){
        //Tagger
        torch::Device device(torch::kCPU);
        std::vector<std::shared_ptr<HTagger>> tagger(2, std::make_shared<HTagger>(7, 140, 1, 130, 57, 0.06, device));

        torch::load(tagger[0], std::string(std::getenv("CHDIR")) + "/DNN/Model/Even/htagger.pt");
        torch::load(tagger[1], std::string(std::getenv("CHDIR")) + "/DNN/Model/Odd/htagger.pt");

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
                values[branchNames[i]][evenIndex[j]] = evenPredict[j].item<float>();
            }

            for(int j = 0; j < oddTensors.size(); j++){
                values[branchNames[i]][oddIndex[j]] = oddPredict[j].item<float>();
            }
        }
    }

    return values;
}

std::map<std::string, std::vector<float>> Extension::DNNScore(TFile* file, const std::string& channel){
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

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
    evNr.SetFunction("EvNr");

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
                paramValues.push_back(functions[i].Get());
            }

            if(int(evNr.Get()) % 2 == 0){
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

std::map<std::string, std::vector<float>> Extension::HReconstruction(TFile* file, const std::string& channel){
    //Set values with default values
    std::map<std::string, std::vector<float>> values;
    std::vector<std::string> branchNames = {"W_Pt", "W_Eta", "W_Phi", "H1_Pt", "H1_Eta", "H1_Phi", "H1_Mass", "H2_Pt", "H2_Eta", "H2_Phi", "H2_Mass", "HPlus_Pt", "HPlus_Eta", "HPlus_Phi", "HPlus_Mass"};
        
    int nEntries = file->Get<TTree>(channel.c_str())->GetEntries();

    for(const std::string& branchName: branchNames){
        values[branchName] = std::vector<float>(nEntries, -999.);
    }
    
    std::string lepName = Utils::Find<std::string>(channel, "Muon") != 1. ? "mu" : "e";
    float lepMass = Utils::Find<std::string>(channel, "Muon") != - 1. ? 0.10565: 0.000510;

    TreeFunction LepPt(file, channel), LepEta(file, channel), LepPhi(file, channel);
    LepPt.SetP1(lepName, 1); LepEta.SetP1(lepName, 1); LepPhi.SetP1(lepName, 1);
    LepPt.SetFunction("Pt"); LepEta.SetFunction("Eta"); LepPhi.SetFunction("Phi");

    TreeFunction METPt(file, channel), METPhi(file, channel);
    METPt.SetP1("met"); METPhi.SetP1("met");
    METPt.SetFunction("Pt"); METPhi.SetFunction("Phi");
    
    TreeFunction nJet(file, channel), nFatJet(file, channel);
    nJet.SetP1("j"); nFatJet.SetP1("fj");
    nJet.SetFunction("N"); nFatJet.SetFunction("N");

    TreeFunction JetPt(file, channel), JetEta(file, channel), JetPhi(file, channel), JetMass(file, channel);
    JetPt.SetP1("j"); JetEta.SetP1("j"); JetPhi.SetP1("j"); JetMass.SetP1("j");
    JetPt.SetFunction("Pt"); JetEta.SetFunction("Eta"); JetPhi.SetFunction("Phi"); JetMass.SetFunction("Mass");

    TreeFunction FatJetPt(file, channel), FatJetEta(file, channel), FatJetPhi(file, channel), FatJetMass(file, channel);
    FatJetPt.SetP1("fj"); FatJetEta.SetP1("fj"); FatJetPhi.SetP1("fj"); FatJetMass.SetP1("fj");
    FatJetPt.SetFunction("Pt"); FatJetEta.SetFunction("Eta"); FatJetPhi.SetFunction("Phi"); FatJetMass.SetFunction("Mass");

    for(int i = 0; i < nEntries; i++){
        TreeFunction::SetEntry(i);

        ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> LepLV(LepPt.Get(), LepEta.Get(), LepPhi.Get(), lepMass);
        
        float pXNu = METPt.Get()*std::cos(METPhi.Get());
        float pYNu = METPt.Get()*std::sin(METPhi.Get());
        float mW = 80.399;
        
        //Analytic solutions to quadratic problem
        float pZNu1 = (-LepLV.E()*std::sqrt(-4*LepLV.E()*LepLV.E()*pXNu*pXNu - 4*LepLV.E()*LepLV.E()*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*LepLV.Px()*pXNu + 4*mW*mW*LepLV.Py()*pYNu + 4*LepLV.Px()*LepLV.Px()*pXNu*pXNu + 8*LepLV.Px()*pXNu*LepLV.Py()*pYNu + 4*pXNu*pXNu*LepLV.Pz()*LepLV.Pz() + 4*LepLV.Py()*LepLV.Py()*pYNu*pYNu + 4*pYNu*pYNu*LepLV.Pz()*LepLV.Pz())/2 + mW*mW*LepLV.Pz()/2 + LepLV.Px()*pXNu*LepLV.Pz() + LepLV.Py()*pYNu*LepLV.Pz())/(LepLV.E()*LepLV.E() - LepLV.Pz()*LepLV.Pz());

        float pZNu2 = (LepLV.E()*std::sqrt(-4*LepLV.E()*LepLV.E()*pXNu*pXNu - 4*LepLV.E()*LepLV.E()*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*LepLV.Px()*pXNu + 4*mW*mW*LepLV.Py()*pYNu + 4*LepLV.Px()*LepLV.Px()*pXNu*pXNu + 8*LepLV.Px()*pXNu*LepLV.Py()*pYNu + 4*pXNu*pXNu*LepLV.Pz()*LepLV.Pz() + 4*LepLV.Py()*LepLV.Py()*pYNu*pYNu + 4*pYNu*pYNu*LepLV.Pz()*LepLV.Pz())/2 + mW*mW*LepLV.Pz()/2 + LepLV.Px()*pXNu*LepLV.Pz() + LepLV.Py()*pYNu*LepLV.Pz())/(LepLV.E()*LepLV.E() - LepLV.Pz()*LepLV.Pz());

        //Neutrino candidates from solutions above
        ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>> v1(pXNu, pYNu, pZNu1, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu1*pZNu1));
        ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>> v2(pXNu, pYNu, pZNu2, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu2*pZNu2));

        //Take neutrino which gives physical reasonabel result
        ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>> W;
        W = abs((LepLV + v1).M() - mW) < abs((LepLV + v2).M() - mW) ? LepLV + v1 : LepLV + v2;

        values["W_Pt"][i] = W.Pt();
        values["W_Phi"][i] = W.Phi();
        values["W_Eta"][i] = W.Eta();


        std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> jets;
        std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> fatJets;

        for(unsigned int k = 0; k < nJet.Get(); k++){
            JetPt.SetP1("j", k+1); JetEta.SetP1("j", k+1); JetPhi.SetP1("j", k+1); JetMass.SetP1("j", k+1);
            jets.push_back(ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>(JetPt.Get(), JetEta.Get(), JetPhi.Get(), JetMass.Get()));
        }

        for(unsigned int k = 0; k < nFatJet.Get(); k++){
            FatJetPt.SetP1("fj", k+1); FatJetEta.SetP1("fj", k+1); FatJetPhi.SetP1("fj", k+1); FatJetMass.SetP1("fj", k+1);
            fatJets.push_back(ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>(FatJetPt.Get(), FatJetEta.Get(), FatJetPhi.Get(), FatJetMass.Get()));
        }

        //Intermediate step to save all possible combinations of two jets from jet collection
        std::vector<std::pair<int, int>> combi;
        
        for(unsigned int k = 0; k < jets.size(); k++){
            for(unsigned int j = 0; j < k; j++){
                combi.push_back({k, j});
            }
        }

        //Vector of candPairs
        typedef std::vector<std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>> hCandVec;
        hCandVec hCands; 
    
        //If 4 jets and no fat jets
        if(jets.size() >= 4){
            //Construct all pairs of possible jet pairs
            for(unsigned int k = 0; k < combi.size(); k++){
                for(unsigned int j = 0; j < k; j++){
                    //Check if not same jet in both pair
                    std::set<int> check = {combi[k].first, combi[k].second, combi[j].first, combi[j].second};
                   
                    if(check.size() == 4){
                        hCands.push_back({jets[combi[k].first], jets[combi[k].second], jets[combi[j].first], jets[combi[j].second]});
                    }
                }
            }
        }

        //If 2 jets and one fat jet
        else if(jets.size() >= 2 and fatJets.size() == 1){
            for(std::pair<int, int> jetIndex: combi){
                hCands.push_back({fatJets[0], jets[jetIndex.first], jets[jetIndex.second]});
            }
        }

        //If 2 fat jets
        else if(fatJets.size() == 2){
            hCands.push_back({fatJets[0], fatJets[1]});
        }

        //Sort candPairs for mass diff of jet pairs, where index 0 is the best pair
        std::function<bool(std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>, std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>)> sortFunc = [&](std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> cands1, std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> cands2){
            if(cands1.size()==4){
                return std::abs((cands1[0]+cands1[1]).M() - (cands1[2]+cands1[3]).M()) < std::abs((cands2[0]+cands2[1]).M() - (cands2[2]+cands2[3]).M());
            }

            else if(cands1.size()==3){
                return std::abs(cands1[0].M() - (cands1[1]+cands1[2]).M()) < std::abs(cands2[0].M() - (cands2[1]+cands2[2]).M());
            }

            else return true;
        };

        std::function<bool(ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>, ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>)> sortPt = [&](ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> p1, ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> p2){
            return p1.Pt() > p2.Pt();
        };

        std::sort(hCands.begin(), hCands.end(), sortFunc);

        ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> hCand1 = hCands[0].size()==4 ? hCands[0][0] + hCands[0][1]: hCands[0][0];

        ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> hCand2 = hCands[0].size()==4 ? hCands[0][2] + hCands[0][3]: hCands[0].size()==3 ? hCands[0][1] + hCands[0][2] : hCands[0][1];

        ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> Hc1 = hCand1; Hc1 += W;
        ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> Hc2 = hCand2; Hc2 += W;

        if(ROOT::Math::VectorUtil::DeltaPhi(Hc1, hCand2) > ROOT::Math::VectorUtil::DeltaPhi(Hc2, hCand1) and ROOT::Math::VectorUtil::DeltaPhi(Hc1, hCand1) < ROOT::Math::VectorUtil::DeltaPhi(Hc2, hCand2)){

            ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> h1 = hCands[0].size()==4 ? hCands[0][0]+hCands[0][1] : hCands[0][0];
            ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> h2 = hCands[0].size()==4 ? hCands[0][2]+hCands[0][3] : hCands[0].size()==3 ? hCands[0][1]+hCands[0][2] : hCands[0][1];   
    
            values["H1_Pt"][i] = h1.Pt();
            values["H1_Eta"][i] = h1.Eta();
            values["H1_Phi"][i] = h1.Phi();
            values["H1_Mass"][i] = h1.M();
            values["H2_Pt"][i] = h2.Pt();
            values["H2_Eta"][i] = h2.Eta();
            values["H2_Phi"][i] = h2.Phi();
            values["H2_Mass"][i] = h2.M();
            values["HPlus_Pt"][i] = Hc1.Pt();
            values["HPlus_Eta"][i] = Hc1.Eta();
            values["HPlus_Phi"][i] = Hc1.Phi();
            values["HPlus_Mass"][i] = Hc1.M();
        }

        else{
            ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> h1 = hCands[0].size()==4 ? hCands[0][2]+hCands[0][3] : hCands[0].size()==3 ? hCands[0][1]+hCands[0][2] : hCands[0][1];

            ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>> h2 = hCands[0].size()==4 ? hCands[0][0]+hCands[0][1] : hCands[0][0];

            values["H1_Pt"][i] = h1.Pt();
            values["H1_Eta"][i] = h1.Eta();
            values["H1_Phi"][i] = h1.Phi();
            values["H1_Mass"][i] = h1.M();
            values["H2_Pt"][i] = h2.Pt();
            values["H2_Eta"][i] = h2.Eta();
            values["H2_Phi"][i] = h2.Phi();
            values["H2_Mass"][i] = h2.M();
            values["HPlus_Pt"][i] = Hc2.Pt();
            values["HPlus_Eta"][i] = Hc2.Eta();
            values["HPlus_Phi"][i] = Hc2.Phi();
            values["HPlus_Mass"][i] = Hc2.M();
        }
    }

    return values;
}
