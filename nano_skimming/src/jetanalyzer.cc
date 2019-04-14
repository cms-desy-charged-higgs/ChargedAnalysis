#include <ChargedHiggs/nano_skimming/interface/jetanalyzer.h>

JetAnalyzer::JetAnalyzer(const int &era, const float &ptCut, const float &etaCut, const std::vector<std::pair<unsigned int, unsigned int>> minNJet):
    BaseAnalyzer(),    
    era(era),
    ptCut(ptCut),
    etaCut(etaCut),
    minNJet(minNJet)
    {
        bTagSF = {
                {AK4, {
                        {2017, filePath + "/btagSF/DeepFlavour_94XSF_V1_B_F.csv"},
                    }
                },

                {AK8, {
                        {2017, filePath + "/btagSF/subjet_DeepCSV_94XSF_V4_B_F.csv"},
                    }
                },
        };

        JMESF = {
                {AK4, {
                        {2017, filePath + "/JME/Fall17_V3_MC_SF_AK4PFchs.txt"},
                    }
                },       

                {AK8, {
                        {2017, filePath + "/JME/Fall17_V3_MC_SF_AK8PFchs.txt"},
                    }
                },             
        };

        JMEPtReso = {
                {AK4, {
                        {2017, filePath + "/JME/Fall17_V3_MC_PtResolution_AK4PFchs.txt"},
                    }
                },       

                {AK8, {
                        {2017, filePath + "/JME/Fall17_V3_MC_PtResolution_AK8PFchs.txt"},
                    }
                },
        };

        //https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation94
        bTagCuts = {
                {AK4, {
                        {2017, {0.051, 0.3033, 0.7489}}, //Loose, Medium, Tight
                    }
                },

                {AK8, {
                        {2017, {0.1522, 0.4941}}, //Loose, Medium, Tight
                    }
                },
        };
    }

//https://twiki.cern.ch/twiki/bin/viewauth/CMS/JetResolution#Smearing_procedures
//https://github.com/cms-sw/cmssw/blob/CMSSW_8_0_25/PhysicsTools/PatUtils/interface/SmearedJetProducerT.h#L203-L263
float JetAnalyzer::SmearEnergy(float &jetPt, float &jetPhi, float &jetEta, float &rho, const float &coneSize, const JetType &type){
    //Configure jet SF reader
    jetParameter.setJetPt(jetPt).setJetEta(jetEta).setRho(rho);

    float dR;
    TLorentzVector matchedGenJet;
    float reso = resolution[type].getResolution(jetParameter);
    float resoSF = resolution_sf[type].getScaleFactor(jetParameter);
    float smearFac = 1.; 

    float genPt, genPhi, genEta, genMass;
    unsigned int size = (type == AK4) ? genJetPt->GetSize(): genFatJetPt->GetSize();

    //Loop over all gen jets and find match
    for(unsigned int i = 0; i < size; i++){
        genPt = (type == AK4) ? genJetPt->At(i): genFatJetPt->At(i);
        genPhi = (type == AK4) ? genJetPhi->At(i): genFatJetPhi->At(i);
        genEta = (type == AK4) ? genJetEta->At(i): genFatJetEta->At(i);
        genMass = (type == AK4) ? genJetMass->At(i): genFatJetMass->At(i);

        dR = std::sqrt(std::pow(jetPhi-genPhi, 2) + std::pow(jetEta-genEta, 2));

        //Check if jet and gen jet are matched
        if(dR < coneSize/2. and abs(jetPt - genPt) < 3.*reso*jetPt){
            matchedGenJet.SetPtEtaPhiM(genPt, genEta, genPhi, genMass);
            break;
        }
    }  

    //If you found gen matched 
    if(matchedGenJet != TLorentzVector()){
        smearFac = 1.+(resoSF-1)*(jetPt - matchedGenJet.Pt())/jetPt; 
    }

    //If no match, smear with gaussian pdf
    else if(resoSF > 1.){
        std::default_random_engine generator;
        std::normal_distribution<> gaus(0, reso * std::sqrt(resoSF * resoSF - 1));

        smearFac = 1. + gaus(generator);
    }


    //Check if direction of jet not changed
    if(jetPt*smearFac < 1e-2){
        smearFac = 1e-2/jetPt;
    }

    return smearFac;
}

void JetAnalyzer::SetGenParticles(Jet &validJet, const int &i){
    //Check if gen matched particle exist
    if(jetGenIdx->At(i) != -1){
        int genPartIdx = -1;
        float dR;
        
        //Find Gen particle to gen Jet
        for(unsigned int index=0; index < genPt->GetSize(); index++){
            dR = std::sqrt(std::pow(genPhi->At(index) -genJetPhi->At(jetGenIdx->At(i)), 2) + std::pow(genEta->At(index)-genJetEta->At(jetGenIdx->At(i)), 2));

            if(dR < 0.05 and abs(genPt->At(index) - genJetPt->At(jetGenIdx->At(i)))/genPt->At(index) < 0.1 and abs(genID->At(index)) < 22){
                genPartIdx = index;
            }
        }

        if(genPartIdx != -1){
            validJet.genVec.SetPtEtaPhiM(genPt->At(genPartIdx), genEta->At(genPartIdx), genPhi->At(genPartIdx), genMass->At(genPartIdx));

            while(genID->At(genMotherIdx->At(genPartIdx)) == genID->At(genPartIdx)){
                genPartIdx = genMotherIdx->At(genPartIdx);
            }

            //Check if gen jet from small higgs
            if(abs(genID->At(genMotherIdx->At(genPartIdx))) == 25){
                int indexh = genMotherIdx->At(genPartIdx);

                while(genID->At(genMotherIdx->At(indexh)) == genID->At(indexh)){
                    indexh = genMotherIdx->At(indexh);
                }

                if(abs(genID->At(genMotherIdx->At(indexh))) == 37){
                    validJet.isFromh1 = true;
                    validJet.isFromh2 = false;
                }

                else{
                    validJet.isFromh1 = false;
                    validJet.isFromh2 = true;
                }
            }
        }
    }
}

void JetAnalyzer::BeginJob(TTreeReader &reader, TTree* tree, bool &isData){
    //Set data bool
    this->isData = isData;

    //Initiliaze TTreeReaderValues
    fatJetPt = std::make_unique<TTreeReaderArray<float>>(reader, "FatJet_pt");
    fatJetEta = std::make_unique<TTreeReaderArray<float>>(reader, "FatJet_eta");
    fatJetPhi = std::make_unique<TTreeReaderArray<float>>(reader, "FatJet_phi");
    fatJetMass = std::make_unique<TTreeReaderArray<float>>(reader, "FatJet_mass");
    fatJetCSV = std::make_unique<TTreeReaderArray<float>>(reader, "FatJet_btagDeepB");
    fatJetTau2 = std::make_unique<TTreeReaderArray<float>>(reader, "FatJet_tau2");
    fatJetTau3 = std::make_unique<TTreeReaderArray<float>>(reader, "FatJet_tau3");

    jetPt = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_pt");
    jetEta = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_eta");
    jetPhi = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_phi");
    jetMass = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_mass");
    jetRho = std::make_unique<TTreeReaderValue<float>>(reader, "fixedGridRhoFastjetAll");
    jetDeepBValue = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_btagDeepFlavB");
    valueHT = std::make_unique<TTreeReaderValue<float>>(reader, "SoftActivityJetHT");

    eleIdx = std::make_unique<TTreeReaderArray<int>>(reader, "Jet_electronIdx1");
    elePt = std::make_unique<TTreeReaderArray<float>>(reader, "Electron_pt");
    eleEta = std::make_unique<TTreeReaderArray<float>>(reader, "Electron_eta");
    muonIdx = std::make_unique<TTreeReaderArray<int>>(reader, "Jet_muonIdx1");
    muonPt = std::make_unique<TTreeReaderArray<float>>(reader, "Muon_pt");
    muonEta = std::make_unique<TTreeReaderArray<float>>(reader, "Muon_eta");
    
    metPhi = std::make_unique<TTreeReaderValue<float>>(reader, "MET_phi");
    metPt = std::make_unique<TTreeReaderValue<float>>(reader, "MET_pt");

    if(!this->isData){
        genJetPt = std::make_unique<TTreeReaderArray<float>>(reader, "GenJet_pt");
        genJetEta = std::make_unique<TTreeReaderArray<float>>(reader, "GenJet_eta");
        genJetPhi = std::make_unique<TTreeReaderArray<float>>(reader, "GenJet_phi");
        genJetMass = std::make_unique<TTreeReaderArray<float>>(reader, "GenJet_mass");

        genFatJetPt = std::make_unique<TTreeReaderArray<float>>(reader, "GenJetAK8_pt");
        genFatJetEta = std::make_unique<TTreeReaderArray<float>>(reader, "GenJetAK8_eta");
        genFatJetPhi = std::make_unique<TTreeReaderArray<float>>(reader, "GenJetAK8_phi");
        genFatJetMass = std::make_unique<TTreeReaderArray<float>>(reader, "GenJetAK8_mass");
        
        jetGenIdx = std::make_unique<TTreeReaderArray<int>>(reader, "Jet_genJetIdx");
    }

    for(JetType type: {AK4, AK8}){
        //Set configuration for bTagSF reader  ##https://twiki.cern.ch/twiki/bin/view/CMS/BTagCalibration
        calib[type] = BTagCalibration(std::to_string(type), bTagSF[type][era]);

        looseReader[type] = BTagCalibrationReader(BTagEntry::OP_LOOSE, "central", {"up", "down"});
        mediumReader[type] = BTagCalibrationReader(BTagEntry::OP_MEDIUM, "central", {"up", "down"});  
        tightReader[type] = BTagCalibrationReader(BTagEntry::OP_TIGHT, "central", {"up", "down"});  

        looseReader[type].load(calib[type],  BTagEntry::FLAV_B, (type==AK4) ? "comb": "lt");  
        mediumReader[type].load(calib[type],  BTagEntry::FLAV_B, (type==AK4) ? "comb": "lt");  
        
        if(type!=AK8){
            tightReader[type].load(calib[type],  BTagEntry::FLAV_B, (type==AK4) ? "comb": "lt");  
        }
    
        //Set configuration for JER tools
        resolution[type] = JME::JetResolution(JMEPtReso[type][era]);
        resolution_sf[type] = JME::JetResolutionScaleFactor(JMESF[type][era]);
    }

    //Set TTreeReader for genpart and trigger obj from baseanalyzer
    SetCollection(reader, this->isData);

    //Set Branches of output tree
    tree->Branch("jet", &validJets);
    tree->Branch("fatjet", &fatJets);
    tree->Branch("met", &met);
    tree->Branch("HT", &HT);
}


bool JetAnalyzer::Analyze(){
    //Clear jet vector
    validJets.clear();
    fatJets.clear();

    //JER smearing
    float smearFac = 1.;

    //MET values not correct for JER yet
    float metPx = *metPt->Get()*std::cos(*metPhi->Get());
    float metPy = *metPt->Get()*std::sin(*metPhi->Get());
    
    //Loop over all fat jets
    for(unsigned int i = 0; i < fatJetPt->GetSize(); i++){
        //Define fat jet
        Jet fatJet;
        fatJet.fourVec.SetPtEtaPhiM(fatJetPt->At(i), fatJetEta->At(i), fatJetPhi->At(i), fatJetMass->At(i));
    
        //Smear pt if not data
        if(!isData){
            smearFac = SmearEnergy(fatJetPt->At(i), fatJetPhi->At(i), fatJetEta->At(i), *jetRho->Get(), 0.8, AK8);
            fatJet.fourVec *= smearFac;

            //Correct met
            metPx+= std::cos(fatJetPhi->At(i))*fatJetPt->At(i) - std::cos(fatJet.fourVec.Phi())*fatJet.fourVec.Pt();
            metPy+= std::sin(fatJetPhi->At(i))*fatJetPt->At(i) - std::sin(fatJet.fourVec.Phi())*fatJet.fourVec.Pt();
        }

        if(fatJet.fourVec.M() > 50. and abs(fatJet.fourVec.Eta()) < etaCut){
            //Check for btag
            fatJet.isLooseB = bTagCuts[AK8][era][0] < fatJetCSV->At(i);
            fatJet.isMediumB = bTagCuts[AK8][era][1] < fatJetCSV->At(i);

            //Nsubjettiness
            fatJet.twoSubJettiness = fatJetTau2->At(i);
            fatJet.threeSubJettiness = fatJetTau3->At(i);

            if(!isData){
                //btag SF
                fatJet.loosebTagSF = looseReader[AK8].eval_auto_bounds("central", BTagEntry::FLAV_B, abs(fatJet.fourVec.Eta()), fatJet.fourVec.Pt());
                fatJet.mediumbTagSF = mediumReader[AK8].eval_auto_bounds("central", BTagEntry::FLAV_B, abs(fatJet.fourVec.Eta()), fatJet.fourVec.Pt());
            }

            fatJets.push_back(fatJet);
        }
    }

    //Loop over all jets
    for(unsigned int i = 0; i < jetPt->GetSize(); i++){
        //Clean jets from electron and muon
        if(eleIdx->At(i) != -1){
            if(elePt->At(eleIdx->At(i)) > 20. and abs(eleEta->At(eleIdx->At(i))) < 2.4){
                continue;
            }
        }

        if(muonIdx->At(i) != -1){
            if(muonPt->At(muonIdx->At(i)) > 20. and abs(muonEta->At(muonIdx->At(i))) < 2.4){
                continue;
            }
        }

        //Define here already jet, because of smearing of 4-vec
        Jet jetCand;
        jetCand.fourVec.SetPtEtaPhiM(jetPt->At(i), jetEta->At(i), jetPhi->At(i), jetMass->At(i));

        //Smear pt if not data
        if(!isData){
            smearFac = SmearEnergy(jetPt->At(i), jetPhi->At(i), jetEta->At(i), *jetRho->Get(), 0.4, AK4);
            jetCand.fourVec *= smearFac;
        }

        if(jetCand.fourVec.Pt() > ptCut and abs(jetCand.fourVec.Eta()) < etaCut){
            //bool for cleaning fatjets from AK4 jets
            bool isCleaned = true;

            //Check overlap with AK4 valid jets
            for(Jet fatJet: fatJets){
                if(fatJet.fourVec.DeltaR(jetCand.fourVec) < 0.6){
                    isCleaned = false;
                    break;
                }
            }        

            if(!isData){
                //Correct met
                metPx+= std::cos(jetPhi->At(i))*jetPt->At(i) - std::cos(jetCand.fourVec.Phi())*jetCand.fourVec.Pt();
                metPy+= std::sin(jetPhi->At(i))*jetPt->At(i) - std::sin(jetCand.fourVec.Phi())*jetCand.fourVec.Pt();
            }

            //If not cleaned dont use this fatjet
            if(!isCleaned){
                continue;
            }


            //Check for btag
            jetCand.isLooseB = bTagCuts[AK4][era][0] < jetDeepBValue->At(i);
            jetCand.isMediumB = bTagCuts[AK4][era][1] < jetDeepBValue->At(i);
            jetCand.isTightB = bTagCuts[AK4][era][2] < jetDeepBValue->At(i);

            if(!isData){
                //btag SF
                jetCand.loosebTagSF = looseReader[AK4].eval_auto_bounds("central", BTagEntry::FLAV_B, abs(jetCand.fourVec.Eta()), jetCand.fourVec.Pt());
                jetCand.mediumbTagSF = mediumReader[AK4].eval_auto_bounds("central", BTagEntry::FLAV_B, abs(jetCand.fourVec.Eta()), jetCand.fourVec.Pt());
                jetCand.tightbTagSF = tightReader[AK4].eval_auto_bounds("central", BTagEntry::FLAV_B, abs(jetCand.fourVec.Eta()), jetCand.fourVec.Pt());


                //Save gen particle information
                SetGenParticles(jetCand, i);
            }

            validJets.push_back(jetCand);
        } 
    }


    //Set HT
    HT = *valueHT->Get();

    //Set met
    met.SetPxPyPzE(metPx, metPy, 0 , 0);

    //Check if one combination of jet and fatjet number is fullfilled

    for(std::pair<unsigned int, unsigned int> minN: minNJet){
        if(validJets.size() >= minN.first && fatJets.size() >= minN.second){
            return true;
        }
    }
    
    return false;
}


void JetAnalyzer::EndJob(TFile* file){
}
