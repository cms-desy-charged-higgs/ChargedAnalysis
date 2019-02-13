#include <ChargedHiggs/analysis/interface/treereader.h>


void TreeReader::SetCutMap(){
    cutValues = {
         {"baseline", &TreeReader::Baseline},
         {"zerojet", &TreeReader::ZeroJet},
         {"fourjet", &TreeReader::FourJet},
    };
}

//Saves configuration of the histogram for wished quantity
void TreeReader::SetHistMap(){
    histValues = {
        {"W_mt", {30., 0., 600., "m_{T}(W) [GeV]", &TreeReader::WBosonMT}},
        {"W_phi", {30., -4., 4., "#phi(W) [rad]", &TreeReader::WBosonPhi}},
        {"W_pt", {30., 0., 600., "p_{T}(W) [GeV]", &TreeReader::WBosonPT}},
        {"e_pt", {40., 30., 400., "p_{T}(e) [GeV]", &TreeReader::ElectronPT}},
        {"e_phi", {30., -4., 4., "#phi(e) [rad]", &TreeReader::ElectronPhi}},
        {"e_eta", {30., -4., 4., "#eta(e) [rad]", &TreeReader::ElectronEta}},
        {"mu_pt", {30., 30., 400., "p_{T}(#mu) [GeV]", &TreeReader::MuonPT}},
        {"mu_phi", {30., -4., 4., "#phi(#mu) [rad]", &TreeReader::MuonPhi}},
        {"mu_eta", {30., -4., 4., "#eta(mu) [rad]", &TreeReader::MuonEta}},
        {"j1_pt", {30., 30., 400., "p_{T}(j_{1}) [GeV]", &TreeReader::Jet1PT}},
        {"j1_phi", {30., -4., 4., "#phi(j_{2}) [GeV]", &TreeReader::Jet1Phi}},
        {"j1_eta", {30., -4., 4., "#eta(j_{1}) [rad]", &TreeReader::Jet1Eta}},
        {"j2_pt", {30., 30., 400., "p_{T}(j_{2}) [GeV]", &TreeReader::Jet2PT}},
        {"j2_phi", {30., -4., 4., "#phi(j_{2}) [GeV]", &TreeReader::Jet2Phi}},
        {"j2_eta", {30., -4., 4., "#eta(j_{1}) [rad]", &TreeReader::Jet2Eta}},
        {"nJet", {10., 0., 10., "Jet multiplicity", &TreeReader::nJet}},
        {"nLooseBJet", {6., 0., 6., "Loose b-jet multiplicity", &TreeReader::nLooseBJet}},
        {"nMediumBJet", {6., 0., 6., "Medium b-jet multiplicity", &TreeReader::nMediumBJet}},
        {"nTightBJet", {6., 0., 6., "Tight b-jet multiplicity", &TreeReader::nTightBJet}},
        {"HT", {30., 0., 1000., "H_{T} [GeV]", &TreeReader::HT}},
        {"met", {30., 0., 500., "E_{T}^{miss} [GeV]", &TreeReader::MET}},
        {"met_phi", {30., -4., 4., "#phi(E_{T}^{miss}) [rad]", &TreeReader::METPhi}},
        {"h1_m", {30., 0., 300., "m(h_{1}) [GeV]", &TreeReader::higgs1Mass}},
        {"h2_m", {30., 0., 300., "m(h_{2}) [GeV]", &TreeReader::higgs2Mass}},
        {"h1_pt", {30., 0., 300., "p_{T}(h_{1}) [GeV]", &TreeReader::higgs1PT}},
        {"h2_pt", {30., 0., 300., "p_{T}(h_{2}) [GeV]", &TreeReader::higgs2PT}},
        {"Hc_pt", {30., 0., 300., "p_{T}(H^{#pm}) [GeV]", &TreeReader::cHiggsPT}},
        {"Hc_mt", {30., 0., 300., "m_{T}(H^{#pm}) [GeV]", &TreeReader::cHiggsMT}},
        {"dphih1h2", {30., 0., 4., "#Delta#phi(h_{1}, h_{2}) [rad]", &TreeReader::dPhih1h2}},
    };
}

//Functions for reconstruct mother final state particles

void TreeReader::WBoson(Event &event){
    if(event.electrons.size() != 0){
        event.W = event.electrons[0].fourVec + event.MET;
    }

    else{
        event.W = event.muons[0].fourVec + event.MET;
    }
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

    std::sort(candPairs.begin(), candPairs.end(), sortFunc);

    event.h1 = candPairs[0].first.Pt() > candPairs[0].first.Pt() ? candPairs[0].first: candPairs[0].second;
    event.h2 = candPairs[0].first.Pt() > candPairs[0].first.Pt() ? candPairs[0].second: candPairs[0].first;

    //Check if W Boson alread reconstructed
    if(event.W == TLorentzVector()) WBoson(event);

    //Reconstruct charge Higgs with smallest dR between h and W
    event.Hc = event.h1.DeltaPhi(event.W) < event.h2.DeltaPhi(event.W) ? event.h1 + event.W: event.h2 + event.W;

}


//Functions for checking if cut is passed

bool TreeReader::Baseline(Event &event){
    bool cut = true;
    
    if(event.electrons.size() != 0) cut *= (event.electrons.size() == 1);

    if(event.muons.size() != 0) cut *= (event.muons.size() == 1);

    for(const Muon &muon: event.muons){
        cut *= (muon.isMedium)*(muon.isLooseIso);
    }

    for(const Electron &electron: event.electrons){
        cut *= (electron.isMedium)*(electron.isolation < 0.1);
    }

    /*
    for(const Jet &jet: event.jets){
        //cut *= (jet.isMediumB);
    }
    */

    return cut;
}


bool TreeReader::ZeroJet(Event &event){
    return event.jets.size() == 0;
}

bool TreeReader::FourJet(Event &event){
    return event.jets.size() >= 4;
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
