#include <ChargedHiggs/analysis/interface/treereader.h>

void TreeReader::SetCutMap(){
    cutValues = {
         {"baseline", Baseline},
    };
}

void TreeReader::SetHistMap(){
    histValues = {
        {"W_mT", {30., 0., 600., WBosonMT}},
        {"W_phi", {30., -4., 4., WBosonPhi}},
        {"W_pt", {30., 0., 600., WBosonPT}},
        {"e_pt", {30., 0., 400., ElectronPT}},
        {"e_phi", {30., -4., 4., ElectronPhi}},
        {"e_eta", {30., -4., 4., ElectronEta}},
        {"j1_pt", {30., 0., 400., Jet1PT}},
        {"j1_phi", {30., -4., 4., Jet1Phi}},
        {"j1_eta", {30., -4., 4., Jet1Eta}},
        {"j2_pt", {30., 0., 400., Jet2PT}},
        {"j2_phi", {30., -4., 4., Jet2Phi}},
        {"j2_eta", {30., -4., 4., Jet2Eta}},
        {"nJet", {10., 0., 10., nJet}},
        {"nLooseBJet", {6., 0., 6., nLooseBJet}},
        {"nMediumBJet", {6., 0., 6., nMediumBJet}},
        {"nTightBJet", {6., 0., 6., nTightBJet}},
        {"HT", {30., 0., 1000., HT}},
        {"met", {30., 0., 500., MET}},
    };
}

//Cut functions

std::function<bool (TreeReader::Event)> TreeReader::Baseline = [&](TreeReader::Event event)
{
    bool cut = true;
    cut *= (event.leptons.size() == 1)*(event.quantities.HT > 100.); //*(event.jets.size() == 4);

    for(Lepton electron: event.leptons){
        cut *= (electron.isMedium)*(electron.isolation < 0.1);
    }

    for(Jet jet: event.jets){
        //cut *= (jet.isMediumB);
    }

    return cut;
};


//Hist functions

std::function<float (TreeReader::Event)> TreeReader::WBosonMT = [&](TreeReader::Event event)
{
    return (event.leptons[0].fourVec + event.MET).Mt();
};

std::function<float (TreeReader::Event)> TreeReader::WBosonPhi = [&](TreeReader::Event event)
{
    return (event.leptons[0].fourVec + event.MET).Phi();
};

std::function<float (TreeReader::Event)> TreeReader::WBosonPT = [&](TreeReader::Event event)
{
    return (event.leptons[0].fourVec + event.MET).Pt();
};

std::function<float (TreeReader::Event)> TreeReader::ElectronPT = [&](TreeReader::Event event)
{
    return event.leptons[0].fourVec.Pt();
};

std::function<float (TreeReader::Event)> TreeReader::ElectronPhi = [&](TreeReader::Event event)
{
    return event.leptons[0].fourVec.Phi();
};

std::function<float (TreeReader::Event)> TreeReader::ElectronEta = [&](TreeReader::Event event)
{
    return event.leptons[0].fourVec.Eta();
};

std::function<float (TreeReader::Event)> TreeReader::Jet1PT = [&](TreeReader::Event event)
{
    return (event.jets.size() > 0) ? event.jets[0].fourVec.Pt(): -999;
};

std::function<float (TreeReader::Event)> TreeReader::Jet1Eta = [&](TreeReader::Event event)
{
    return (event.jets.size() > 0)  ? event.jets[0].fourVec.Eta(): -999;
};

std::function<float (TreeReader::Event)> TreeReader::Jet1Phi = [&](TreeReader::Event event)
{
    return (event.jets.size() > 0) ? event.jets[0].fourVec.Phi(): -999;
};

std::function<float (TreeReader::Event)> TreeReader::Jet2PT = [&](TreeReader::Event event)
{
    return (event.jets.size() > 1)  ? event.jets[1].fourVec.Pt(): -999;
};

std::function<float (TreeReader::Event)> TreeReader::Jet2Eta = [&](TreeReader::Event event)
{
    return (event.jets.size() > 1)  ? event.jets[1].fourVec.Eta(): -999;
};

std::function<float (TreeReader::Event)> TreeReader::Jet2Phi = [&](TreeReader::Event event)
{
    return (event.jets.size() > 1)  ? event.jets[1].fourVec.Phi(): -999;
};

std::function<float (TreeReader::Event)> TreeReader::nJet = [&](TreeReader::Event event)
{
    return event.jets.size();
};

std::function<float (TreeReader::Event)> TreeReader::nLooseBJet = [&](TreeReader::Event event)
{
    float nbJets = 0.;
    
    for(Jet jet: event.jets){
        if(jet.isLooseB){
            nbJets+=1;
        }        
    }

    return nbJets;
};

std::function<float (TreeReader::Event)> TreeReader::nMediumBJet = [&](TreeReader::Event event)
{
    float nbJets = 0.;
    
    for(Jet jet: event.jets){
        if(jet.isMediumB){
            nbJets+=1;
        }        
    }

    return nbJets;
};

std::function<float (TreeReader::Event)> TreeReader::nTightBJet = [&](TreeReader::Event event)
{
    float nbJets = 0.;
    
    for(Jet jet: event.jets){
        if(jet.isTightB){
            nbJets+=1;
        }        
    }

    return nbJets;
};

std::function<float (TreeReader::Event)> TreeReader::HT = [&](TreeReader::Event event)
{
    return event.quantities.HT;
};

std::function<float (TreeReader::Event)> TreeReader::MET = [&](TreeReader::Event event)
{
    return event.MET.Pt();
};
