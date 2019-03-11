#include <ChargedHiggs/nano_skimming/interface/jetanalyzer.h>

JetAnalyzer::JetAnalyzer(const int &era, const float &ptCut, const float &etaCut, const int &minNJet):
    BaseAnalyzer(),    
    era(era),
    ptCut(ptCut),
    etaCut(etaCut),
    minNJet(minNJet)
    {
        bTagSF = {
                {2017, filePath + "/btagSF/DeepFlavour_94XSF_V1_B_F.csv"},
        };

        JMESF = {
                {2017, filePath + "/JME/Fall17_V3_MC_SF_AK4PFchs.txt"},
        };

        JMEPtReso = {
                {2017, filePath + "/JME/Fall17_V3_MC_PtResolution_AK4PFchs.txt"},
        };

        //https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation94
        bTagCuts = {
                {2017, {0.051, 0.3033, 0.7489}}, //Loose, Medium, Tight
        };
    }

//https://twiki.cern.ch/twiki/bin/viewauth/CMS/JetResolution#Smearing_procedures
//https://github.com/cms-sw/cmssw/blob/CMSSW_8_0_25/PhysicsTools/PatUtils/interface/SmearedJetProducerT.h#L203-L263
float JetAnalyzer::SmearEnergy(float &jetPt, float &jetPhi, float &jetEta, float &rho){
    //Configure jet SF reader
    jetParameter.setJetPt(jetPt).setJetEta(jetEta).setRho(rho);

    float dR;
    TLorentzVector matchedGenJet;
    float reso = resolution.getResolution(jetParameter);
    float resoSF = resolution_sf.getScaleFactor(jetParameter);
    float smearFac = 1.; 

    //Loop over all gen jets and find match
    for(unsigned int i = 0; i < genJetPt->GetSize(); i++){
        dR = std::sqrt(std::pow(jetPhi-genJetPhi->At(i), 2) + std::pow(jetEta-genJetEta->At(i), 2));

        //Check if jet and gen jet are matched
        if(dR < 0.4/2. and abs(jetPt - genJetPt->At(i)) < 3.*reso*jetPt){
            matchedGenJet.SetPtEtaPhiM(genJetPt->At(i), genJetEta->At(i), genJetPhi->At(i), 0);
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
        int jetIdx = jetGenIdx->At(i);

        validJet.genVec.SetPtEtaPhiM(genPt->At(jetIdx), genEta->At(jetIdx), genPhi->At(jetIdx), genMass->At(jetIdx));

        std::cout << genID->At(jetGenIdx->At(i)) << std::endl;
            //Check if gen jet from small higgs

            if(abs(genID->At(genMotherIdx->At(jetIdx))) == 25){
                int indexh = genMotherIdx->At(jetIdx);
        
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

void JetAnalyzer::BeginJob(TTreeReader &reader, TTree* tree, bool &isData){
    //Set data bool
    this->isData = isData;

    //Initiliaze TTreeReaderValues
    jetPt = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_pt");
    jetEta = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_eta");
    jetPhi = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_phi");
    jetMass = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_mass");
    jetRho = std::make_unique<TTreeReaderValue<float>>(reader, "fixedGridRhoFastjetAll");
    jetDeepBValue = std::make_unique<TTreeReaderArray<float>>(reader, "Jet_btagDeepFlavB");
    valueHT = std::make_unique<TTreeReaderValue<float>>(reader, "SoftActivityJetHT");
    
    metPhi = std::make_unique<TTreeReaderValue<float>>(reader, "MET_phi");
    metPt = std::make_unique<TTreeReaderValue<float>>(reader, "MET_pt");

    if(!this->isData){
        genJetPt = std::make_unique<TTreeReaderArray<float>>(reader, "GenJet_pt");
        genJetEta = std::make_unique<TTreeReaderArray<float>>(reader, "GenJet_eta");
        genJetPhi = std::make_unique<TTreeReaderArray<float>>(reader, "GenJet_phi");
        
        jetGenIdx = std::make_unique<TTreeReaderArray<int>>(reader, "Jet_genJetIdx");
    }

    //Set configuration for bTagSF reader  ##https://twiki.cern.ch/twiki/bin/view/CMS/BTagCalibration
    calib = BTagCalibration("DeepFlavourB", bTagSF[era]);
    looseReader = BTagCalibrationReader(BTagEntry::OP_LOOSE, "central", {"up", "down"});
    mediumReader = BTagCalibrationReader(BTagEntry::OP_MEDIUM, "central", {"up", "down"});  
    tightReader = BTagCalibrationReader(BTagEntry::OP_TIGHT, "central", {"up", "down"});  

    looseReader.load(calib,  BTagEntry::FLAV_B, "comb");  
    mediumReader.load(calib,  BTagEntry::FLAV_B, "comb");  
    tightReader.load(calib,  BTagEntry::FLAV_B, "comb");  

    //Set configuration for JER tools
    resolution = JME::JetResolution(JMEPtReso[era]);
    resolution_sf = JME::JetResolutionScaleFactor(JMESF[era]);
    
    //Set TTreeReader for genpart and trigger obj from baseanalyzer
    SetCollection(reader, this->isData);

    //Set Branches of output tree
    tree->Branch("jet", &validJets);
    tree->Branch("met", &met);
    tree->Branch("HT", &HT);
}


bool JetAnalyzer::Analyze(){
    //Clear jet vector
    validJets.clear();

    //JER smearing
    float smearFac = 1.;

    //MET values not correct for JER yet
    float metPx = *metPt->Get()*std::cos(*metPhi->Get());
    float metPy = *metPt->Get()*std::sin(*metPhi->Get());
    
    //Loop over all jets
    for(unsigned int i = 0; i < jetPt->GetSize(); i++){
        //Smear pt if not data
        if(!isData){
            smearFac = SmearEnergy(jetPt->At(i), jetPhi->At(i), jetEta->At(i), *jetRho->Get());

            //Correct met for JER
            metPx = metPx - std::cos(jetPhi->At(i))*jetPt->At(i)*(1 + smearFac);
            metPy = metPy - std::sin(jetPhi->At(i))*jetPt->At(i)*(1 + smearFac);
        }

        if(smearFac*jetPt->At(i) > ptCut and abs(jetEta->At(i)) < etaCut){
            Jet validJet;

            validJet.fourVec.SetPtEtaPhiM(smearFac*jetPt->At(i), jetEta->At(i), jetPhi->At(i), jetMass->At(i));

            //Check for btag
            validJet.isLooseB = bTagCuts[era][0] < jetDeepBValue->At(i);
            validJet.isMediumB = bTagCuts[era][1] < jetDeepBValue->At(i);
            validJet.isTightB = bTagCuts[era][2] < jetDeepBValue->At(i);

            if(!isData){
                //btag SF
                validJet.loosebTagSF = looseReader.eval_auto_bounds("central", BTagEntry::FLAV_B, abs(jetEta->At(i)), smearFac*jetPt->At(i));
                validJet.mediumbTagSF = mediumReader.eval_auto_bounds("central", BTagEntry::FLAV_B, abs(jetEta->At(i)), smearFac*jetPt->At(i));
                validJet.tightbTagSF = tightReader.eval_auto_bounds("central", BTagEntry::FLAV_B, abs(jetEta->At(i)), smearFac*jetPt->At(i));


                //Save gen particle information
                SetGenParticles(validJet, i);
            }

            validJets.push_back(validJet);
        } 
    }

    //Set HT
    HT = *valueHT->Get();

    //Set met
    met.SetPxPyPzE(metPx, metPy, 0 , 0);

    if(validJets.size() < minNJet){
        return false;
    }
    
    return true;
}


void JetAnalyzer::EndJob(TFile* file){
}
