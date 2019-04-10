#include <ChargedHiggs/analysis/interface/treereader.h>


void TreeReader::SetTriggerMap(){
    trigNames = {
         {"HLT_ele35", {ELE35, "HLT_Ele35_WPTight_Gsf"}},
         {"HLT_ele30jet35", {ELE30JET35, "HLT_Ele30_eta2p1_WPTight_Gsf_CentralPFJet35_EleCleaned"}},
         {"HLT_ele28HT150", {ELE28HT150, "HLT_Ele28_eta2p1_WPTight_Gsf_HT150"}},
         {"HLT_met200", {MET200, "HLT_PFMET200_HBHECleaned"}},
         {"HLT_mu27", {MU27, "HLT_IsoMu27"}},
    };
}

void TreeReader::SetCutMap(){
    cutValues = {
         {"1ele",  &TreeReader::mediumSingleElectron},
         {"2ele",  &TreeReader::mediumDoubleElectron},
         {"1mu",  &TreeReader::mediumSingleMuon},
         {"2mu", &TreeReader::mediumDoubleMuon},
         {"nonisoele", &TreeReader::NonIsoElectron},
         {"0bjets", &TreeReader::ZeroBJets},
         {"1bjets", &TreeReader::OneBJets},
         {"2bjets", &TreeReader::TwoBJets},
         {"3bjets", &TreeReader::ThreeBJets},
         {"xbjets", &TreeReader::XBJets},
         {"2jets1fat", &TreeReader::TwoJetsOneFat},
         {"4jets", &TreeReader::FourJets},
         {"mass", &TreeReader::MassCut},
         {"antimass", &TreeReader::AntiMassCut},
         {"phi", &TreeReader::PhiCut},
    };
}

//Saves configuration of the histogram for wished quantity
void TreeReader::SetHistMap(){
    histValues = {
        {"HLT_ele35", {2., 0., 2., "HLT_Ele35_WPTight_Gsf", &TreeReader::HLTEle35}},
        {"HLT_ele28HT150", {2., 0., 2., "HLT_Ele28_eta2p1_WPTight_Gsf_HT150", &TreeReader::HLTEle28HT150}},
        {"HLT_ele30jet35", {2., 0., 2., "HLT_Ele30_eta2p1_WPTight_Gsf_CentralPFJet35_EleCleaned", &TreeReader::HLTEle30Jet35}},
        {"HLT_mu27", {2., 0., 2., "HLT_IsoMu27", &TreeReader::HLTMu27}},
        {"HLT_met200", {2., 0., 2., "HLT_met200", &TreeReader::HLTMet200}},
        {"HLT_mu27_and_met200", {2., 0., 2., "HLT_IsoMu27 && HLT_met200", &TreeReader::HLTMu27AndMet200}},
        {"HLT_ele35_and_met200", {2., 0., 2., "HLT_Ele35 && HLT_met200", &TreeReader::HLTEle35AndMet200}},
        {"HLT_ele30Jet35_and_met200", {2., 0., 2., "HLT_Ele30Jet35 && HLT_met200", &TreeReader::HLTEle30Jet35AndMet200}},
        {"HLT_ele28HT150_and_met200", {2., 0., 2., "HLT_Ele28HT150 && HLT_met200", &TreeReader::HLTEle28HT150AndMet200}},
        {"HLT_eleAll_and_met200", {2., 0., 2., "HLT_Ele35 || HLT_Ele30Jet35 || HLT_Ele28HT150 && HLT_met200", &TreeReader::HLTEleAllAndMet200}},
        {"W_mt", {30., 0., 600., "m_{T}(W) [GeV]", &TreeReader::WBosonMT}},
        {"W_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(W) [rad]", &TreeReader::WBosonPhi}},
        {"W_pt", {30., 0., 600., "p_{T}(W) [GeV]", &TreeReader::WBosonPT}},
        {"nElectron", {4., 0., 4., "Electron multiplicity", &TreeReader::nElectron}},
        {"nMuon", {4., 0., 4., "Muon multiplicity", &TreeReader::nMuon}},
        {"M_ee", {30., 50., 140., "m(e, e) [GeV]", &TreeReader::DiEleMass}},
        {"e_pt", {20., 0., 300., "p_{T}(e) [GeV]", &TreeReader::ElectronPT}},
        {"e_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(e) [rad]", &TreeReader::ElectronPhi}},
        {"e_eta", {30., -2.4, 2.4, "#eta(e) [rad]", &TreeReader::ElectronEta}},
        {"mu_pt", {40., 20., 150., "p_{T}(#mu) [GeV]", &TreeReader::MuonPT}},
        {"mu_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(#mu) [rad]", &TreeReader::MuonPhi}},
        {"mu_eta", {30., -2.4, 2.4, "#eta(#mu) [rad]", &TreeReader::MuonEta}},
        {"j1_pt", {30., 30., 400., "p_{T}(j_{1}) [GeV]", &TreeReader::Jet1PT}},
        {"j1_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(j_{2}) [GeV]", &TreeReader::Jet1Phi}},
        {"j1_eta", {30., -2.4, 2.4, "#eta(j_{1}) [rad]", &TreeReader::Jet1Eta}},
        {"j2_pt", {30., 30., 400., "p_{T}(j_{2}) [GeV]", &TreeReader::Jet2PT}},
        {"j2_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(j_{2}) [GeV]", &TreeReader::Jet2Phi}},
        {"j2_eta", {30., -2.4, 2.4, "#eta(j_{1}) [rad]", &TreeReader::Jet2Eta}},
        {"nJet", {10., 0., 10., "Jet multiplicity", &TreeReader::nJet}},
        {"nFatJet", {3., 0., 3., "Fat jet multiplicity", &TreeReader::nFatJet}},
        {"nLooseBJet", {6., 0., 6., "Loose b-jet multiplicity", &TreeReader::nLooseBJet}},
        {"nMediumBJet", {6., 0., 6., "Medium b-jet multiplicity", &TreeReader::nMediumBJet}},
        {"nTightBJet", {6., 0., 6., "Tight b-jet multiplicity", &TreeReader::nTightBJet}},
        {"HT", {30., 0., 1000., "H_{T} [GeV]", &TreeReader::HT}},
        {"met", {30., 0., 500., "E_{T}^{miss} [GeV]", &TreeReader::MET}},
        {"met_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(E_{T}^{miss}) [rad]", &TreeReader::METPhi}},
        {"h1_m", {30., 0., 300., "m(h_{1}) [GeV]", &TreeReader::higgs1Mass}},
        {"h2_m", {30., 0., 300., "m(h_{2}) [GeV]", &TreeReader::higgs2Mass}},
        {"h1_pt", {30., 0., 300., "p_{T}(h_{1}) [GeV]", &TreeReader::higgs1PT}},
        {"h2_pt", {30., 0., 300., "p_{T}(h_{2}) [GeV]", &TreeReader::higgs2PT}},
        {"h1_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(h_{1}) [rad]", &TreeReader::higgs1Phi}},
        {"h2_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(h_{2}) [rad]", &TreeReader::higgs2Phi}},
        {"Hc_pt", {30., 0., 300., "p_{T}(H^{#pm}) [GeV]", &TreeReader::cHiggsPT}},
        {"Hc_mt", {30., 0., 300., "m_{T}(H^{#pm}) [GeV]", &TreeReader::cHiggsMT}},
        {"Hc_m", {20., 100., 1000., "m(H^{#pm}) [GeV]", &TreeReader::cHiggsM}},
        {"dphih1h2", {30., 0., TMath::Pi(), "#Delta#phi(h_{1}, h_{2}) [rad]", &TreeReader::dPhih1h2}},
        {"dRh1W", {30., 0., 2*TMath::Pi(), "#DeltaR(h_{1}, W^{#pm}) [rad]", &TreeReader::dRh1W}},
        {"dRh2W", {30., 0., 2*TMath::Pi(), "#DeltaR(h_{2}, W^{#pm}) [rad]", &TreeReader::dRh2W}},
        {"dRh1Hc", {30., 0., 2*TMath::Pi(), "#DeltaR(h_{1}, H^{#pm}) [rad]", &TreeReader::dRh1Hc}},
        {"dRh2Hc", {30., 0., 2*TMath::Pi(), "#DeltaR(h_{2}, H^{#pm}) [rad]", &TreeReader::dRh2Hc}},
        {"dPhih1W", {30., 0., TMath::Pi(), "#Delta#phi(h_{1}, W^{#pm}) [rad]", &TreeReader::dPhih1W}},
        {"dPhih2W", {30., 0., TMath::Pi(), "#Delta#phi(h_{2}, W^{#pm}) [rad]", &TreeReader::dPhih2W}},
        {"dPhih1Hc", {30., 0., TMath::Pi(), "#Delta#phi(h_{1}, H^{#pm}) [rad]", &TreeReader::dPhih1Hc}},
        {"dPhih2Hc", {30., 0., TMath::Pi(), "#Delta#phi(h_{2}, H^{#pm}) [rad]", &TreeReader::dPhih2Hc}},
        {"top1_m", {30., 0., 400., "m(t_{1}) [GeV]", &TreeReader::top1Mass}},
        {"top2_m", {30., 0., 400., "m(t_{2}) [GeV]", &TreeReader::top2Mass}},
        {"top1_pt", {30., 0., 300., "p_{T}(t_{1}) [GeV]", &TreeReader::top1PT}},
        {"top2_pt", {30., 0., 300., "p_{T}(t_{2}) [GeV]", &TreeReader::top2PT}},
        {"dPhitop1top2", {30., 0., TMath::Pi(), "#Delta#phi(t_{1}, t_{2}) [rad]", &TreeReader::dPhitop1top2}},
        {"dRtop1top2", {30., 0., 2*TMath::Pi(), "#DeltaR(t_{1}, t_{2}) [rad]", &TreeReader::dRtop1top2}},
        {"dMtop1top2", {30, -50., 50., "#DeltaM(t_{1}, t_{2}) [GeV]", &TreeReader::dMtop1top2}},
        {"dMh1h2", {30, -50., 50., "#DeltaM(h_{1}, h_{2}) [GeV]", &TreeReader::dMh1h2}},
        {"dPhitop1W", {30., 0., TMath::Pi(), "#Delta#phi(t_{1}, W^{#pm}) [rad]", &TreeReader::dPhitop1W}},
        {"dPhitop2W", {30., 0., TMath::Pi(), "#Delta#phi(t_{2}, W^{#pm}) [rad]", &TreeReader::dPhitop2W}},
        {"sumljet", {30., 0., 600, "p_{T}(l) + #sum p_{T}(j)", &TreeReader::SumLepJet}},
        {"vecsumljet", {30., 0., 600, "(p^{#mu}(l) + #sum p^{#mu}(j))_{T}", &TreeReader::VecSumLepJet}},
        {"dPth2Hc", {30., 0., 600, "(p^{#mu}(H^{#pm}) + p^{#mu}(h_{2}))_{T}", &TreeReader::dPth2Hc}},
        {"RMHcMTop", {30., 0., 1., "m(H^{#pm})/m(t_{1})", &TreeReader::RMHcMTop}},
    };
}

//Functions for reconstruct mother final state particles

void TreeReader::WBoson(Event &event){
    TLorentzVector lep = event.muons.size() != 0 ? event.muons[0].fourVec : event.electrons[0].fourVec;

    float pXNu = event.MET.Pt()*std::cos(event.MET.Phi());
    float pYNu = event.MET.Pt()*std::sin(event.MET.Phi());
    float mW = 80.399;
    
    float pZNu1 = (-lep.E()*std::sqrt(-4*lep.E()*lep.E()*pXNu*pXNu - 4*lep.E()*lep.E()*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*lep.Px()*pXNu + 4*mW*mW*lep.Py()*pYNu + 4*lep.Px()*lep.Px()*pXNu*pXNu + 8*lep.Px()*pXNu*lep.Py()*pYNu + 4*pXNu*pXNu*lep.Pz()*lep.Pz() + 4*lep.Py()*lep.Py()*pYNu*pYNu + 4*pYNu*pYNu*lep.Pz()*lep.Pz())/2 + mW*mW*lep.Pz()/2 + lep.Px()*pXNu*lep.Pz() + lep.Py()*pYNu*lep.Pz())/(lep.E()*lep.E() - lep.Pz()*lep.Pz());

    float pZNu2 = (lep.E()*std::sqrt(-4*lep.E()*lep.E()*pXNu*pXNu - 4*lep.E()*lep.E()*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*lep.Px()*pXNu + 4*mW*mW*lep.Py()*pYNu + 4*lep.Px()*lep.Px()*pXNu*pXNu + 8*lep.Px()*pXNu*lep.Py()*pYNu + 4*pXNu*pXNu*lep.Pz()*lep.Pz() + 4*lep.Py()*lep.Py()*pYNu*pYNu + 4*pYNu*pYNu*lep.Pz()*lep.Pz())/2 + mW*mW*lep.Pz()/2 + lep.Px()*pXNu*lep.Pz() + lep.Py()*pYNu*lep.Pz())/(lep.E()*lep.E() - lep.Pz()*lep.Pz());

    TLorentzVector v1;
    v1.SetPxPyPzE(pXNu, pYNu, pZNu1, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu1*pZNu1));

    TLorentzVector v2;
    v2.SetPxPyPzE(pXNu, pYNu, pZNu2, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu2*pZNu2));

    event.W = abs((lep + v1).M() - mW) < abs((lep + v2).M() - mW) ? lep + v1 : lep + v2;
}


void TreeReader::Top(Event &event){
    //Vector of candPairs
    typedef std::pair<TLorentzVector, TLorentzVector> Pair;
    typedef std::pair<TLorentzVector, std::vector<TLorentzVector>> WPair;
    std::vector<Pair> topPairs; 
    std::vector<WPair> Wcands; 

    //Check if W Boson already reconstructed
    if(event.W == TLorentzVector()) WBoson(event);

    //Reconstruct other W Boson
    float mW = 80.399;

    std::function<bool(WPair, WPair)> sortW = [&](WPair W1, WPair W2){return abs(W1.first.M() - mW) < abs(W2.first.M() - mW);};

    Wcands.push_back({event.jets[0].fourVec + event.jets[1].fourVec, {event.jets[2].fourVec, event.jets[3].fourVec}});
    Wcands.push_back({event.jets[0].fourVec + event.jets[2].fourVec, {event.jets[1].fourVec, event.jets[3].fourVec}});
    Wcands.push_back({event.jets[0].fourVec + event.jets[3].fourVec, {event.jets[2].fourVec, event.jets[1].fourVec}});
    Wcands.push_back({event.jets[1].fourVec + event.jets[2].fourVec, {event.jets[0].fourVec, event.jets[3].fourVec}});
    Wcands.push_back({event.jets[1].fourVec + event.jets[3].fourVec, {event.jets[0].fourVec, event.jets[2].fourVec}});
    Wcands.push_back({event.jets[2].fourVec + event.jets[3].fourVec, {event.jets[0].fourVec, event.jets[1].fourVec}});

    std::sort(Wcands.begin(), Wcands.end(), sortW);

    //Push back each combination of top pair canditates
    topPairs.push_back({event.W + Wcands[0].second[0], Wcands[0].first + Wcands[0].second[1]});
    topPairs.push_back({event.W + Wcands[0].second[1], Wcands[0].first + Wcands[0].second[0]});

    //Sort candPairs for mass diff of top pairs, where index 0 is the best pair
    std::function<bool(Pair, Pair)> sortFunc = [&](Pair pair1, Pair pair2){return std::abs(pair1.first.M() -pair1.second.M()) < std::abs(pair2.first.M() - pair2.second.M());};

    std::sort(topPairs.begin(), topPairs.end(), sortFunc);

    event.top1 = topPairs[0].first;
    event.top2 = topPairs[0].second;
}


void TreeReader::Higgs(Event &event){
    //Vector of candPairs
    typedef std::pair<TLorentzVector, TLorentzVector> hPair;
    std::vector<hPair> candPairs; 
    
    //Push back each combination of jet pair which could originate from Higgs
    candPairs.push_back({event.jets[0].fourVec + event.jets[1].fourVec,  event.jets[2].fourVec + event.jets[3].fourVec});

    candPairs.push_back({event.jets[0].fourVec + event.jets[2].fourVec,  event.jets[1].fourVec + event.jets[3].fourVec});

    candPairs.push_back({event.jets[0].fourVec + event.jets[3].fourVec,  event.jets[1].fourVec + event.jets[2].fourVec});

    //Sort candPairs for mass diff of jet pairs, where index 0 is the best pair
    std::function<bool(hPair, hPair)> sortFunc = [&](hPair pair1, hPair pair2){return std::abs(pair1.first.M() -pair1.second.M()) < std::abs(pair2.first.M() - pair2.second.M());};

    std::sort(candPairs.begin(), candPairs.end(), sortFunc);

    //Check if W Boson alread reconstructed
    if(event.W == TLorentzVector()) WBoson(event);

    //Reconstruct charge Higgs with smallest dR between h and W
    TLorentzVector Hc1 = event.W + candPairs[0].first;
    TLorentzVector Hc2 = event.W + candPairs[0].second;

    if(Hc1.DeltaPhi(candPairs[0].second) > Hc2.DeltaPhi(candPairs[0].first) and Hc1.DeltaPhi(candPairs[0].first) < Hc2.DeltaPhi(candPairs[0].second)){
        event.Hc = Hc1;
        event.h1 = candPairs[0].first;
        event.h2 = candPairs[0].second;
    }

    else{
        event.Hc = Hc2;
        event.h1 = candPairs[0].second;
        event.h2 = candPairs[0].first;
    }

}



//Functions for checking if cut is passed
bool TreeReader::NonIsoElectron(Event &event){
    bool cut = true;
   
    for(const Electron &electron: event.electrons){
        cut = cut && (electron.isolation > 0.1);
        event.weight *= electron.recoSF;
    }

    return cut;
}

//Functions for checking if cut is passed
bool TreeReader::mediumSingleElectron(Event &event){
    bool cut = true;
    
    cut = cut && (event.electrons.size() == 1);
    cut *= (event.muons.size() == 0);

    for(const Electron &electron: event.electrons){
        cut = cut && (electron.isTriggerMatched) && (electron.isMedium) && (electron.isolation < 0.1);
        event.weight *= electron.recoSF*electron.mediumMvaSF;
    }

    return cut;
}

bool TreeReader::mediumDoubleElectron(Event &event){
    bool cut = true;
    
    cut = cut && (event.electrons.size() == 2);

    for(const Electron &electron: event.electrons){
        cut = cut && (electron.isMedium) && (electron.isolation < 0.1);
        event.weight *= electron.recoSF*electron.mediumMvaSF;
    }

    return cut;
}

bool TreeReader::mediumSingleMuon(Event &event){
    bool cut = true;
    
    cut = cut && (event.muons.size() == 1);
    cut = cut && (event.electrons.size() == 0);

    for(const Muon &muon: event.muons){
        cut = cut && (muon.isTriggerMatched) && (muon.isMedium) && (muon.isLooseIso);
        event.weight *= muon.triggerSF*muon.mediumSF*muon.looseIsoMediumSF;
    }

    return cut;
}

bool TreeReader::mediumDoubleMuon(Event &event){
    bool cut = true;
    
    cut = cut && (event.muons.size() == 2);

    for(const Muon &muon: event.muons){
        cut = cut && (muon.isMedium) && (muon.isLooseIso);
        event.weight *= muon.triggerSF*muon.mediumSF*muon.looseIsoMediumSF;
    }

    return cut;
}

bool TreeReader::ZeroBJets(Event &event){
    return (this->nTightBJet(event) == 0);
}


bool TreeReader::OneBJets(Event &event){
    bool cut = true;

    cut *= (nTightBJet(event) == 1);

    for(const Jet &jet: event.jets){
        event.weight *= jet.tightbTagSF;
    }

    return cut;
}

bool TreeReader::TwoBJets(Event &event){
    bool cut = true;
    
    cut *= (this->nTightBJet(event) == 2);

    for(const Jet &jet: event.jets){
        event.weight *= jet.tightbTagSF;
    }

    return cut;
}

bool TreeReader::ThreeBJets(Event &event){
    bool cut = true;
    
    cut *= (this->nTightBJet(event) == 3);

    for(const Jet &jet: event.jets){
        event.weight *= jet.tightbTagSF;
    }

    return cut;
}

bool TreeReader::XBJets(Event &event){
    bool cut = true;
    
    cut *= (this->nTightBJet(event) > 3);

    for(const Jet &jet: event.jets){
        event.weight *= jet.tightbTagSF;
    }

    return cut;
}

bool TreeReader::TwoJetsOneFat(Event &event){
    bool cut = true;
    
    cut *= (nJet(event) >= 2 and nFatJet(event) >= 1);

    return cut;
}

bool TreeReader::FourJets(Event &event){
    bool cut = true;
    
    cut *= (nJet(event) >= 4 and nFatJet(event) == 0);

    return cut;
}


bool TreeReader::MassCut(Event &event){
    return (higgs1Mass(event) < 150) && (higgs2Mass(event) < 150);
}

bool TreeReader::AntiMassCut(Event &event){
    return (higgs1Mass(event) > 150) && (higgs2Mass(event) > 150);
}

bool TreeReader::PhiCut(Event &event){
    return (dPhih2Hc(event) > 2.5);
}

//Functions calculating value of quantities

float TreeReader::HLTMu27(Event &event){
    if(event.trigger.find(MU27) == event.trigger.end()){
        return -999.;
    }

    else{
        return event.trigger[MU27];
    }
}

float TreeReader::HLTEle35(Event &event){
    if(event.trigger.find(ELE35) == event.trigger.end()){
        return -999.;
    }

    else{
        return event.trigger[ELE35];
    }
}

float TreeReader::HLTEle30Jet35(Event &event){
    if(event.trigger.find(ELE30JET35) == event.trigger.end()){
        return -999.;
    }

    else{
        return event.trigger[ELE30JET35];
    }
}

float TreeReader::HLTEle28HT150(Event &event){
    if(event.trigger.find(ELE28HT150) == event.trigger.end()){
        return -999.;
    }

    else{
        return event.trigger[ELE28HT150];
    }
}

float TreeReader::HLTMet200(Event &event){
    if(event.trigger.find(MET200) == event.trigger.end()){
        return -999.;
    }

    else{
        return event.trigger[MET200];
    }
}

float TreeReader::HLTEle35AndMet200(Event &event){
    return HLTEle35(event)*HLTMet200(event);
}

float TreeReader::HLTEle30Jet35AndMet200(Event &event){
    return HLTEle30Jet35(event)*HLTMet200(event);
}

float TreeReader::HLTEle28HT150AndMet200(Event &event){
    return HLTEle28HT150(event)*HLTMet200(event);
}

float TreeReader::HLTEleAllAndMet200(Event &event){
    return (HLTEle35(event) || HLTEle28HT150(event) || HLTEle30Jet35(event))*HLTMet200(event);
}

float TreeReader::HLTMu27AndMet200(Event &event){
    return HLTMu27(event)*HLTMet200(event);
}


float TreeReader::WBosonMT(Event &event){
    if(event.W != TLorentzVector()){
        return event.W.Mt();
    }

    else{
        WBoson(event);
        return event.W.Mt();
    }
}

float TreeReader::WBosonPhi(Event &event){
    if(event.W != TLorentzVector()){
        return event.W.Phi();
    }

    else{
        WBoson(event);
        return event.W.Phi();
    }
}


float TreeReader::WBosonPT(Event &event){
    if(event.W != TLorentzVector()){
        return event.W.Pt();
    }

    else{
        WBoson(event);
        return event.W.Pt();
    }
}


float TreeReader::nElectron(Event &event){
    return event.electrons.size();
}

float TreeReader::nMuon(Event &event){
    return event.muons.size();
}

float TreeReader::DiEleMass(Event &event){
    if(event.electrons.size() < 2){
        return -999.;
    } 

    else{
        return (event.electrons[0].fourVec + event.electrons[1].fourVec).M();
    }
}

float TreeReader::ElectronPT(Event &event){
    return (event.electrons.size() > 0) ? event.electrons[0].fourVec.Pt() : -999;
}

float TreeReader::ElectronPhi(Event &event){
    return (event.electrons.size() > 0) ? event.electrons[0].fourVec.Phi() : -999;
}

float TreeReader::ElectronEta(Event &event){
    return (event.electrons.size() > 0) ? event.electrons[0].fourVec.Eta() : -999;
}

float TreeReader::MuonPT(Event &event){
    return (event.muons.size() > 0) ? event.muons[0].fourVec.Pt() : -999;
}

float TreeReader::MuonPhi(Event &event){
    return (event.muons.size() > 0) ? event.muons[0].fourVec.Phi() : -999;
}

float TreeReader::MuonEta(Event &event){
    return (event.muons.size() > 0) ? event.muons[0].fourVec.Eta() : -999;
}


float TreeReader::Jet1PT(Event &event){
    return (event.jets.size() > 0) ? event.jets[0].fourVec.Pt(): -999;
}

float TreeReader::Jet1Eta(Event &event){
    return (event.jets.size() > 0)  ? event.jets[0].fourVec.Eta(): -999;
}

float TreeReader::Jet1Phi(Event &event){
    return (event.jets.size() > 0) ? event.jets[0].fourVec.Phi(): -999;
}

float TreeReader::Jet2PT(Event &event){
    return (event.jets.size() > 1)  ? event.jets[1].fourVec.Pt(): -999;
}

float TreeReader::Jet2Eta(Event &event){
    return (event.jets.size() > 1)  ? event.jets[1].fourVec.Eta(): -999;
}

float TreeReader::Jet2Phi(Event &event){
    return (event.jets.size() > 1)  ? event.jets[1].fourVec.Phi(): -999;
}

float TreeReader::nJet(Event &event){
    return event.jets.size();
}


float TreeReader::nFatJet(Event &event){
    return event.fatjets.size();
}

float TreeReader::nLooseBJet(Event &event){
    float nbJets = 0.;
    
    for(const Jet &jet: event.jets){
        if(jet.isLooseB){
            nbJets+=1;
        }        
    }

    return nbJets;
}

float TreeReader::nMediumBJet(Event &event){
    float nbJets = 0.;
    
    for(const Jet &jet: event.jets){
        if(jet.isMediumB){
            nbJets+=1;
        }        
    }

    return nbJets;
}

float TreeReader::nTightBJet(Event &event){
    float nbJets = 0.;
    
    for(const Jet &jet: event.jets){
        if(jet.isTightB){
            nbJets+=1;
        }        
    }

    return nbJets;
}

float TreeReader::HT(Event &event){
    return event.HT;
}

float TreeReader::MET(Event &event){
    return event.MET.Pt();
}

float TreeReader::METPhi(Event &event){
    return event.MET.Phi();
}

float TreeReader::higgs1Mass(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector()){
            return event.h1.M();
        }

        else{
            Higgs(event);
            return event.h1.M();
        }
    }
}

float TreeReader::higgs2Mass(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h2 != TLorentzVector()){
            return event.h2.M();
        }

        else{
            Higgs(event);
            return event.h2.M();
        }
    }
}

float TreeReader::higgs1PT(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector()){
            return event.h1.Pt();
        }

        else{
            Higgs(event);
            return event.h1.Pt();
        }
    }
}

float TreeReader::higgs2PT(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h2 != TLorentzVector()){
            return event.h2.Pt();
        }

        else{
            Higgs(event);
            return event.h2.Pt();
        }
    }
}

float TreeReader::higgs1Phi(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h2 != TLorentzVector()){
            return event.h1.Phi();
        }

        else{
            Higgs(event);
            return event.h1.Phi();
        }
    }
}

float TreeReader::higgs2Phi(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h2 != TLorentzVector()){
            return event.h2.Phi();
        }

        else{
            Higgs(event);
            return event.h2.Phi();
        }
    }
}

float TreeReader::dPhih1h2(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h1.DeltaPhi(event.h2);
        }

        else{
            Higgs(event);
            return event.h1.DeltaPhi(event.h2);
        }
    }
}


float TreeReader::dRh1W(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h1.DeltaR(event.W);
        }

        else{
            Higgs(event);
            return event.h1.DeltaR(event.W);
        }
    }
}

float TreeReader::dRh2W(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h2.DeltaR(event.W);
        }

        else{
            Higgs(event);
            return event.h2.DeltaR(event.W);
        }
    }
}

float TreeReader::dRh1Hc(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h1.DeltaR(event.Hc);
        }

        else{
            Higgs(event);
            return event.h1.DeltaR(event.Hc);
        }
    }
}

float TreeReader::dRh2Hc(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h2.DeltaR(event.Hc);
        }

        else{
            Higgs(event);
            return event.h2.DeltaR(event.Hc);
        }
    }
}

float TreeReader::dPhih1W(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h1.DeltaPhi(event.W);
        }

        else{
            Higgs(event);
            return event.h1.DeltaPhi(event.W);
        }
    }
}

float TreeReader::dPhih2W(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h2.DeltaPhi(event.W);
        }

        else{
            Higgs(event);
            return event.h2.DeltaPhi(event.W);
        }
    }
}

float TreeReader::dPhih1Hc(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h1.DeltaPhi(event.Hc);
        }

        else{
            Higgs(event);
            return event.h1.DeltaPhi(event.Hc);
        }
    }
}

float TreeReader::dPhih2Hc(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h2.DeltaPhi(event.Hc);
        }

        else{
            Higgs(event);
            return event.h2.DeltaPhi(event.Hc);
        }
    }
}

float TreeReader::cHiggsPT(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.Hc != TLorentzVector()){
            return event.Hc.Pt();
        }

        else{
            Higgs(event);
            return event.Hc.Pt();
        }
    }
}

float TreeReader::cHiggsMT(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.Hc != TLorentzVector()){
            return event.Hc.Pt();
        }

        else{
            Higgs(event);
            return event.Hc.Pt();
        }
    }
}

float TreeReader::cHiggsM(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.Hc != TLorentzVector()){
            return event.Hc.M();
        }

        else{
            Higgs(event);
            return event.Hc.M();
        }
    }
}

float TreeReader::dMh1h2(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return event.h1.M() - event.h2.M();
        }

        else{
            Higgs(event);
            return event.h1.M() - event.h2.M();
        }
    }
}

float TreeReader::top1Mass(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.top1 != TLorentzVector()){
            return event.top1.M();
        }

        else{
            Top(event);
            return event.top1.M();
        }
    }
}

float TreeReader::top2Mass(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.top2 != TLorentzVector()){
            return event.top2.M();
        }

        else{
            Top(event);
            return event.top2.M();
        }
    }
}

float TreeReader::top1PT(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h1 != TLorentzVector()){
            return event.top1.Pt();
        }

        else{
            Top(event);
            return event.top1.Pt();
        }
    }
}

float TreeReader::top2PT(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.h2 != TLorentzVector()){
            return event.top2.Pt();
        }

        else{
            Top(event);
            return event.top2.Pt();
        }
    }
}

float TreeReader::dPhitop1top2(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.top1 != TLorentzVector() and event.top2 != TLorentzVector()){
            return event.top2.DeltaPhi(event.top1);
        }

        else{
            Top(event);
            return event.top2.DeltaPhi(event.top1);
        }
    }
}

float TreeReader::dRtop1top2(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.top1 != TLorentzVector() and event.top2 != TLorentzVector()){
            return event.top2.DeltaR(event.top1);
        }

        else{
            Top(event);
            return event.top2.DeltaR(event.top1);
        }
    }
}

float TreeReader::dMtop1top2(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.top1 != TLorentzVector() and event.top2 != TLorentzVector()){
            return event.top1.M() - event.top2.M();
        }

        else{
            Top(event);
            return event.top1.M() - event.top2.M();
        }
    }
}

float TreeReader::dPhitop1W(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.top1 != TLorentzVector() and event.top2 != TLorentzVector()){
            return event.top1.DeltaPhi(event.W);
        }

        else{
            Top(event);
            return event.top1.DeltaPhi(event.W);
        }
    }
}

float TreeReader::dPhitop2W(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.top1 != TLorentzVector() and event.top2 != TLorentzVector()){
            return event.top2.DeltaPhi(event.W);
        }

        else{
            Top(event);
            return event.top2.DeltaPhi(event.W);
        }
    }
}

float TreeReader::SumLepJet(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.electrons.size() == 1){
            return event.electrons[0].fourVec.Pt() + event.jets[0].fourVec.Pt() + event.jets[1].fourVec.Pt() + event.jets[2].fourVec.Pt() + event.jets[3].fourVec.Pt();
        }

        else if(event.muons.size() == 1){
            return event.muons[0].fourVec.Pt() + event.jets[0].fourVec.Pt() + event.jets[1].fourVec.Pt() + event.jets[2].fourVec.Pt() + event.jets[3].fourVec.Pt();
        }

        else{
            return -999.;
        }
    }
}

float TreeReader::VecSumLepJet(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }

    else{
        if(event.electrons.size() == 1){
            return (event.electrons[0].fourVec + event.jets[0].fourVec + event.jets[1].fourVec + event.jets[2].fourVec + event.jets[3].fourVec).Pt();
        }

        else if(event.muons.size() == 1){
            return (event.electrons[0].fourVec + event.jets[0].fourVec + event.jets[1].fourVec + event.jets[2].fourVec + event.jets[3].fourVec).Pt();
        }

        else{
            return -999.;
        }
    }
}

float TreeReader::dPth2Hc(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }


    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector()){
            return (event.Hc + event.h2).Pt();
        }

        else{
            Higgs(event);
            return (event.Hc + event.h2).Pt();
        }
    }
}

float TreeReader::RMHcMTop(Event &event){
    if(event.jets.size() < 4){
        return -999.;
    }


    else{
        if(event.h1 != TLorentzVector() and event.h2 != TLorentzVector() and event.top1 != TLorentzVector()){
            return (event.top1.M() - event.top2.M())/(event.top1.M() + event.top2.M());
        }

        else{
            Higgs(event);
            Top(event);
            return (event.top1.M() - event.top2.M())/(event.top1.M() + event.top2.M());
        }
    }
}
