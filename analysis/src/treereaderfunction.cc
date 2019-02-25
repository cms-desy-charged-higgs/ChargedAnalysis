#include <ChargedHiggs/analysis/interface/treereader.h>


void TreeReader::SetCutMap(){
    cutValues = {
         {"1ele",  &TreeReader::mediumSingleElectron},
         {"1mu",  &TreeReader::mediumSingleMuon},
         {"0bjets", &TreeReader::ZeroBJets},
         {"1bjets", &TreeReader::OneBJets},
         {"2bjets", &TreeReader::TwoBJets},
    };
}

//Saves configuration of the histogram for wished quantity
void TreeReader::SetHistMap(){
    histValues = {
        {"W_mt", {30., 0., 600., "m_{T}(W) [GeV]", &TreeReader::WBosonMT}},
        {"W_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(W) [rad]", &TreeReader::WBosonPhi}},
        {"W_pt", {30., 0., 600., "p_{T}(W) [GeV]", &TreeReader::WBosonPT}},
        {"nElectron", {4., 0., 4., "Electron multiplicity", &TreeReader::nElectron}},
        {"e_pt", {40., 30., 400., "p_{T}(e) [GeV]", &TreeReader::ElectronPT}},
        {"e_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(e) [rad]", &TreeReader::ElectronPhi}},
        {"e_eta", {30., -2.4, 2.4, "#eta(e) [rad]", &TreeReader::ElectronEta}},
        {"mu_pt", {30., 30., 400., "p_{T}(#mu) [GeV]", &TreeReader::MuonPT}},
        {"mu_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(#mu) [rad]", &TreeReader::MuonPhi}},
        {"mu_eta", {30., -2.4, 2.4, "#eta(#mu) [rad]", &TreeReader::MuonEta}},
        {"j1_pt", {30., 30., 400., "p_{T}(j_{1}) [GeV]", &TreeReader::Jet1PT}},
        {"j1_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(j_{2}) [GeV]", &TreeReader::Jet1Phi}},
        {"j1_eta", {30., -2.4, 2.4, "#eta(j_{1}) [rad]", &TreeReader::Jet1Eta}},
        {"j2_pt", {30., 30., 400., "p_{T}(j_{2}) [GeV]", &TreeReader::Jet2PT}},
        {"j2_phi", {30., -TMath::Pi(), TMath::Pi(), "#phi(j_{2}) [GeV]", &TreeReader::Jet2Phi}},
        {"j2_eta", {30., -2.4, 2.4, "#eta(j_{1}) [rad]", &TreeReader::Jet2Eta}},
        {"nJet", {10., 0., 10., "Jet multiplicity", &TreeReader::nJet}},
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
        {"Hc_m", {50., 0., 800., "m(H^{#pm}) [GeV]", &TreeReader::cHiggsM}},
        {"dphih1h2", {30., 0., TMath::Pi(), "#Delta#phi(h_{1}, h_{2}) [rad]", &TreeReader::dPhih1h2}},
        {"dRh1W", {30., 0., 2*TMath::Pi(), "#DeltaR(h_{1}, W^{#pm}) [rad]", &TreeReader::dRh1W}},
        {"dRh2W", {30., 0., 2*TMath::Pi(), "#DeltaR(h_{2}, W^{#pm}) [rad]", &TreeReader::dRh2W}},
        {"dRh1Hc", {30., 0., 2*TMath::Pi(), "#DeltaR(h_{1}, H^{#pm}) [rad]", &TreeReader::dRh1Hc}},
        {"dRh2Hc", {30., 0., 2*TMath::Pi(), "#DeltaR(h_{2}, H^{#pm}) [rad]", &TreeReader::dRh2Hc}},
        {"dPhih1W", {30., 0., TMath::Pi(), "#Delta#phi(h_{1}, W^{#pm}) [rad]", &TreeReader::dPhih1W}},
        {"dPhih2W", {30., 0., TMath::Pi(), "#Delta#phi(h_{2}, W^{#pm}) [rad]", &TreeReader::dPhih2W}},
        {"dPhih1Hc", {30., 0., TMath::Pi(), "#Delta#phi(h_{1}, H^{#pm}) [rad]", &TreeReader::dPhih1Hc}},
        {"dPhih2Hc", {30., 0., TMath::Pi(), "#Delta#phi(h_{2}, H^{#pm}) [rad]", &TreeReader::dPhih2Hc}},
    };
}

//Functions for reconstruct mother final state particles

void TreeReader::WBoson(Event &event){
    //Lepton momenta 
    float pXL = event.muons[0].fourVec.Px(); 
    float pYL = event.muons[0].fourVec.Py();
    float pZL = event.muons[0].fourVec.Pz();

    float pXNu = event.MET.Pt()*std::cos(event.MET.Phi());
    float pYNu = event.MET.Pt()*std::sin(event.MET.Phi());
    float EL = event.muons[0].fourVec.E();
    float mW = 80.399;
    
    float pZNu1 = (-EL*std::sqrt(-4*EL*EL*pXNu*pXNu - 4*EL*EL*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*pXL*pXNu + 4*mW*mW*pYL*pYNu + 4*pXL*pXL*pXNu*pXNu + 8*pXL*pXNu*pYL*pYNu + 4*pXNu*pXNu*pZL*pZL + 4*pYL*pYL*pYNu*pYNu + 4*pYNu*pYNu*pZL*pZL)/2 + mW*mW*pZL/2 + pXL*pXNu*pZL + pYL*pYNu*pZL)/(EL*EL - pZL*pZL);

    float pZNu2 = (EL*std::sqrt(-4*EL*EL*pXNu*pXNu - 4*EL*EL*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*pXL*pXNu + 4*mW*mW*pYL*pYNu + 4*pXL*pXL*pXNu*pXNu + 8*pXL*pXNu*pYL*pYNu + 4*pXNu*pXNu*pZL*pZL + 4*pYL*pYL*pYNu*pYNu + 4*pYNu*pYNu*pZL*pZL)/2 + mW*mW*pZL/2 + pXL*pXNu*pZL + pYL*pYNu*pZL)/(EL*EL - pZL*pZL);

    TLorentzVector v1;
    v1.SetPxPyPzE(pXNu, pYNu, pZNu1, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu1*pZNu1));

    TLorentzVector v2;
    v2.SetPxPyPzE(pXNu, pYNu, pZNu2, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu2*pZNu2));

    event.W = abs((event.muons[0].fourVec + v1).M() - mW) < abs((event.muons[0].fourVec + v2).M() - mW) ? event.muons[0].fourVec + v1 : event.muons[0].fourVec + v2;
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
    std::function<bool(hPair, hPair)> sortFunc = [](hPair pair1, hPair pair2){return std::abs(pair1.first.M() -pair1.second.M()) < std::abs(pair2.first.M() - pair2.second.M());};

    std::sort(candPairs.begin(), candPairs.end(), sortFunc);;

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
bool TreeReader::mediumSingleElectron(Event &event){
    bool cut = true;
    
    cut *= (event.electrons.size() == 1);
    cut *= (event.muons.size() == 0);

    for(const Electron &electron: event.electrons){
        cut *= (electron.isMedium)*(electron.isolation < 0.1);
        event.weight *= electron.recoSF*electron.mediumMvaSF;
    }

    return cut;
}

bool TreeReader::mediumSingleMuon(Event &event){
    bool cut = true;
    
    cut *= (event.muons.size() == 1);
    cut *= (event.electrons.size() == 0);

    for(const Muon &muon: event.muons){
        cut *= (muon.isMedium)*(muon.isLooseIso);
        event.weight *= muon.triggerSF*muon.mediumSF*muon.looseIsoMediumSF;
    }

    return cut;
}

bool TreeReader::ZeroBJets(Event &event){
    return (this->nTightBJet(event) == 0);
}


bool TreeReader::OneBJets(Event &event){
    bool cut = true;
    
    cut *= (this->nTightBJet(event) == 1);

    for(const Jet &jet: event.jets){
        event.weight *= jet.bTagSF;
    }

    return cut;
}

bool TreeReader::TwoBJets(Event &event){
    bool cut = true;
    
    cut *= (this->nTightBJet(event) >= 2);

    for(const Jet &jet: event.jets){
        event.weight *= jet.bTagSF;
    }

    return cut;
}


//Functions calculating value of quantities

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

float TreeReader::ElectronPT(Event &event){
    return event.electrons[0].fourVec.Pt();
}

float TreeReader::ElectronPhi(Event &event){
    return event.electrons[0].fourVec.Phi();
}

float TreeReader::ElectronEta(Event &event){
    return event.electrons[0].fourVec.Eta();
}

float TreeReader::MuonPT(Event &event){
    return event.muons[0].fourVec.Pt();
}

float TreeReader::MuonPhi(Event &event){
    return event.muons[0].fourVec.Phi();
}

float TreeReader::MuonEta(Event &event){
    return event.muons[0].fourVec.Eta();
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

float TreeReader::nLooseBJet(Event &event){
    float nbJets = 0.;
    
    for(const Jet &jet: event.jets){
        if(jet.isLooseB){
            nbJets+=jet.bTagSF;
        }        
    }

    return nbJets;
}

float TreeReader::nMediumBJet(Event &event){
    float nbJets = 0.;
    
    for(const Jet &jet: event.jets){
        if(jet.isMediumB){
            nbJets+=jet.bTagSF;
        }        
    }

    return nbJets;
}

float TreeReader::nTightBJet(Event &event){
    float nbJets = 0.;
    
    for(const Jet &jet: event.jets){
        if(jet.isTightB){
            nbJets+=jet.bTagSF;
        }        
    }

    return nbJets;
}

float TreeReader::HT(Event &event){
    return event.quantities.HT;
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
