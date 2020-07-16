#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <filesystem>

#include <TH1F.h>
#include <TFile.h>
#include <TTree.h>
#include <TLeaf.h>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/plotter.h>
#include <ChargedAnalysis/Utility/include/mathutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

struct Particle{
    int partID;
    int motherID;
    int index = -1;
};

struct Hist{
    std::shared_ptr<TH1F> hist; 
    std::string xLabel;
    std::vector<TLeaf*> leaves;
    std::vector<std::string> parts;
};


int main(int argc, char* argv[]){
    //Extract informations of command line
    Parser parser(argc, argv);

    std::string fileName = parser.GetValue<std::string>("file-name");
    std::string outPath = parser.GetValue<std::string>("out-path");
   
    std::shared_ptr<TFile> file(TFile::Open(fileName.c_str(), "READ"));
    std::shared_ptr<TTree> tree(file->TFile::Get<TTree>("MuonIncl"));

    TLeaf* pt = tree->GetLeaf("GenParticle_Pt");
    TLeaf* eta = tree->GetLeaf("GenParticle_Eta");
    TLeaf* phi = tree->GetLeaf("GenParticle_Phi");
    TLeaf* mass = tree->GetLeaf("GenParticle_Mass");
    TLeaf* ID = tree->GetLeaf("GenParticle_ParticleID");
    TLeaf* motherID = tree->GetLeaf("GenParticle_MotherID");

    Plotter::SetStyle();

    std::unique_ptr<TCanvas> canvas = std::make_unique<TCanvas>("canvas",  "canvas", 1000, 1000);
    Plotter::SetPad(canvas.get());

    std::vector<std::string> partNames = {"W", "HPlus", "H1", "H2", "mu", "vmu"};
    std::map<std::string, Particle> parts = {
        {"b", {5, 25, -1}}, 
        {"mu", {13, 24, -1}}, 
        {"vmu", {14, 24, -1}}, 
        {"W", {24, 37, -1}}, 
        {"HPlus", {37, 24, -1}}, 
        {"H1", {25, 37, -1}}, 
        {"H2", {25, 24, -1}}, 
    };

    std::vector<Hist> hists = {
        {std::make_shared<TH1F>("WMass", "WMass", 30, 60, 100), "m(W^{#pm})", {mass}, {"W"}},
        {std::make_shared<TH1F>("HPlusMass", "HPlusMass", 30, 150, 250), "m(H^{#pm})", {mass}, {"HPlus"}},
        {std::make_shared<TH1F>("H1Mass", "H1Mass", 30, 80, 120), "m(h)", {mass}, {"H1"}},
        {std::make_shared<TH1F>("H2Mass", "H2Mass", 30, 80, 120), "m(h)", {mass}, {"H2"}},
        {std::make_shared<TH1F>("MuPt", "MuPt", 30, 0, 400), "p_{T}(#mu)", {pt}, {"mu"}},
        {std::make_shared<TH1F>("BPt", "BPt", 30, 0, 400), "p_{T}(b)", {pt}, {"b"}},
        {std::make_shared<TH1F>("NuMuPt", "NuMuPt", 30, 0, 400), "p_{T}(#nu_{#mu})", {pt}, {"vmu"}},
        {std::make_shared<TH1F>("WPt", "WPt", 30, 0, 400), "p_{T}(W^{#pm})", {pt}, {"W"}},
        {std::make_shared<TH1F>("HPlusPt", "HPlusPt", 30, 0, 400), "p_{T}(H^{#pm})", {pt}, {"HPlus"}},
        {std::make_shared<TH1F>("H1Pt", "H1Pt", 30, 0, 400), "p_{T}(h)", {pt}, {"H1"}},
        {std::make_shared<TH1F>("H2Pt", "H2Pt", 30, 0, 400), "p_{T}(h)", {pt}, {"H2"}},
        {std::make_shared<TH1F>("DeltaRHPlusH1", "DeltaRHPlusH1", 30, 0, 6), "#Delta R(H^{#pm}, h)", {phi, eta}, {"HPlus", "H1"}},
        {std::make_shared<TH1F>("DeltaRHPlusH2", "DeltaRHPlusH2", 30, 0, 6), "#Delta R(H^{#pm}, h)", {phi, eta}, {"HPlus", "H2"}},
        {std::make_shared<TH1F>("DeltaPhiHPlusH1", "DeltaPhiHPlusH1", 30, 0, 3.14), "#Delta#phi(H^{#pm}, h)", {phi}, {"HPlus", "H1"}},
        {std::make_shared<TH1F>("DeltaPhiHPlusH2", "DeltaPhiHPlusH2", 30, 0, 3.14), "#Delta#phi(H^{#pm}, h)", {phi}, {"HPlus", "H2"}},
        {std::make_shared<TH1F>("DeltaPhiHPlusW", "DeltaPhiHPlusW", 30, 0, 3.14), "#Delta#phi(H^{#pm}, W)", {phi}, {"HPlus", "W"}},
        {std::make_shared<TH1F>("DeltaPhiH1W", "DeltaPhiH1W", 30, 0, 3.14), "#Delta#phi(W^{#pm}, h)", {phi}, {"W", "H1"}},
        {std::make_shared<TH1F>("DeltaPhiH2W", "DeltaPhiH2W", 30, 0, 3.14), "#Delta#phi(W^{#pm}, h)", {phi}, {"W", "H2"}},
        {std::make_shared<TH1F>("DeltaPhiH1H2", "DeltaPhiH1H2", 30, 0, 3.14), "#Delta#phi(h, h)", {phi}, {"H1", "H2"}},
    };

    std::function<float(std::vector<TLeaf*>, std::vector<int>)> returner = [&](std::vector<TLeaf*> leaves, std::vector<int> indeces){
        if(std::find(indeces.begin(), indeces.end(), -1) != indeces.end()) return float(-999.);

        if(leaves[0] == phi and leaves[1] == eta){
            std::vector<float>* phi = static_cast<std::vector<float>*>(leaves[0]->GetValuePointer());
            std::vector<float>* eta = static_cast<std::vector<float>*>(leaves[1]->GetValuePointer());

            return MUtil::DeltaR(eta->at(indeces[0]), phi->at(indeces[0]), eta->at(indeces[1]), phi->at(indeces[1]));
        }

        else if(leaves[0] == phi and indeces.size() == 2){
            std::vector<float>* phi = static_cast<std::vector<float>*>(leaves[0]->GetValuePointer());

            return MUtil::DeltaPhi(phi->at(indeces[0]), phi->at(indeces[1]));
        }
    
        else{
            std::vector<float>* quan = static_cast<std::vector<float>*>(leaves[0]->GetValuePointer());

            return quan->at(indeces[0]);
        }
    };

    canvas->Draw();

    for(int i = 0; i < tree->GetEntries(); i++){
        for(const TLeaf* l : {mass, pt, eta, phi, ID, motherID}) l->GetBranch()->GetEntry(i);

        std::vector<char>* IDs = static_cast<std::vector<char>*>(ID->GetValuePointer());
        std::vector<char>* motherIDs = static_cast<std::vector<char>*>(motherID->GetValuePointer());

        for(const std::string& p: partNames){
            parts.at(p).index = -1;

            for(int j = 0; j < IDs->size(); j++){
                if(parts.at(p).partID == IDs->at(j) and parts.at(p).motherID == motherIDs->at(j)){
                    parts.at(p).index = j;  
                    break;
                }
            }               
        }

        for(const Hist& hist: hists){
            hist.hist->Fill(returner(hist.leaves, VUtil::Transform<int>(hist.parts, [&](std::string p){return parts.at(p).index;})));
        } 
    }

    for(const Hist& hist: hists){
        canvas->Clear();
        Plotter::SetHist(canvas.get(), hist.hist.get(), "Events");

        hist.hist->GetXaxis()->SetTitle(hist.xLabel.c_str());
        hist.hist->SetLineColor(kBlack);
        hist.hist->SetLineWidth(5);

        hist.hist->Draw();
        Plotter::DrawHeader(canvas.get(), "Simulation", "Work in progress");
        std::filesystem::create_directory(outPath);
        canvas->SaveAs(StrUtil::Merge(outPath, "/", hist.hist->GetName(), ".pdf").c_str());  
        canvas->SaveAs(StrUtil::Merge(outPath, "/", hist.hist->GetName(), ".png").c_str());  
    }   
}
