#include <ChargedHiggs/analysis/interface/treereader.h>

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
        {"nJet", {30., 4., 8., nJet}}
    };
}

//Functions 

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
    return event.jets[0].fourVec.Pt();
};

std::function<float (TreeReader::Event)> TreeReader::Jet1Eta = [&](TreeReader::Event event)
{
    return event.jets[0].fourVec.Eta();
};

std::function<float (TreeReader::Event)> TreeReader::Jet1Phi = [&](TreeReader::Event event)
{
    return event.jets[0].fourVec.Phi();
};

std::function<float (TreeReader::Event)> TreeReader::nJet = [&](TreeReader::Event event)
{
    return event.jets.size();
};
