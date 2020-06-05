#include <string>

#include <TFile.h>
#include <TCanvas.h>
#include <TGraph.h>

#include <ChargedAnalysis/Analysis/include/plotter.h>
#include <ChargedAnalysis/Analysis/include/treefunction.h>
#include <ChargedAnalysis/Utility/include/utils.h>

int main(){
    for(const std::string& channel: {"Ele2J1FJ", "Muon2J1FJ", "Ele2FJ", "Muon2FJ"}){
        TFile* bkgFile = TFile::Open((std::string(std::getenv("CHDIR")) + Utils::Format("@", "/Skim/Channels/@/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8/merged/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8.root", channel)).c_str(), "READ");

        std::vector<float> predDeepAK;
        std::vector<float> predHTagAK;
        std::vector<int> truth;

        TreeFunction bkgDeepAK(bkgFile, channel);
        TreeFunction bkgHTag(bkgFile, channel);
        bkgDeepAK.SetP1("fj", 1, "");
        bkgDeepAK.SetFunction("DAK8");
        bkgHTag.SetP1("fj", 1, "");
        bkgHTag.SetFunction("HTag");
        
        for(int i = 0; i < bkgFile->Get<TTree>(channel.c_str())->GetEntries(); i++){
            TreeFunction::SetEntry(i);
            predDeepAK.push_back(bkgDeepAK.Get());
            predHTagAK.push_back(bkgHTag.Get());
            truth.push_back(0);
        }

        for(const std::string& mass : {"200", "400", "600"}){
            TFile* sigFile = TFile::Open((std::string(std::getenv("CHDIR")) + Utils::Format("@", "/Skim/Channels/@/HPlusAndH_ToWHH_ToL4B_" + mass + "_100/merged/HPlusAndH_ToWHH_ToL4B_" + mass + "_100.root", channel)).c_str(), "READ");

            TreeFunction sigDeepAK(sigFile, channel);
            TreeFunction sigHTag(sigFile, channel);
            sigDeepAK.SetP1("fj", 1, "");
            sigDeepAK.SetFunction("DAK8");
            sigHTag.SetP1("fj", 1, "");
            sigHTag.SetFunction("HTag");
            
            for(int i = 0; i < sigFile->Get<TTree>(channel.c_str())->GetEntries(); i++){
                TreeFunction::SetEntry(i);
                predDeepAK.push_back(sigDeepAK.Get());
                predHTagAK.push_back(sigHTag.Get());
                truth.push_back(1);
            }

            Plotter::SetStyle();

            TCanvas* canvas = new TCanvas("canvas",  "canvas", 1000, 1000);
            TGraph* deepAKROC = Utils::GetROC(predDeepAK, truth);
            TGraph* HTagROC = Utils::GetROC(predHTagAK, truth);

            TLegend* legend = new TLegend(0., 0., 1, 1);
            legend->AddEntry(deepAKROC, "DeepAK8", "P");
            legend->AddEntry(HTagROC, "Higgs Tagger", "P");

            deepAKROC->SetMarkerStyle(21);
            deepAKROC->SetMarkerColor(kBlack);
            HTagROC->SetMarkerStyle(22);
            HTagROC->SetMarkerColor(kViolet);

            Plotter::SetPad(canvas);
            Plotter::SetHist(canvas, deepAKROC->GetHistogram(), "p(True positive)");
            Plotter::SetHist(canvas, deepAKROC->GetHistogram(), "p(True positive)");
            deepAKROC->GetHistogram()->GetXaxis()->SetTitle("p(False positive)");
            deepAKROC->GetHistogram()->SetMaximum(1.2);

            deepAKROC->Draw("AP");
            HTagROC->Draw("P SAME");

            Plotter::DrawHeader(canvas, "", "Work in progress");
            Plotter::DrawLegend(legend, 2);

            canvas->SaveAs(Utils::Format("@", "DeepAKvsHTag_"  + mass + "_@.pdf", channel).c_str());

            delete deepAKROC; delete HTagROC; delete legend; delete canvas;

            predDeepAK.erase(predDeepAK.begin() + bkgFile->Get<TTree>(channel.c_str())->GetEntries());
            predHTagAK.erase(predHTagAK.begin() + bkgFile->Get<TTree>(channel.c_str())->GetEntries());
            truth.erase(truth.begin() + bkgFile->Get<TTree>(channel.c_str())->GetEntries());
        }
    }
}
