#include <string>

#include <TFile.h>
#include <TCanvas.h>
#include <TGraph.h>

#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Analysis/include/treefunction.h>
#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

int main(){
    for(const std::string& channel: {"Ele2J1FJ", "Muon2J1FJ", "Ele2FJ", "Muon2FJ"}){
        std::shared_ptr<TFile> bkgFile = std::make_shared<TFile>(StrUtil::Replace("@/Skim/Channels/@/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8/merged/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8.root", "@", std::getenv("CHDIR"), channel).c_str(), "READ");

        std::vector<float> predDeepAK;
        std::vector<float> predHTagAK;
        std::vector<int> truth;

        TreeFunction bkgDeepAK(bkgFile, channel);
        TreeFunction bkgHTag(bkgFile, channel);
        bkgDeepAK.SetP1<Axis::X>("fj", 1, "");
        bkgDeepAK.SetFunction<Axis::X>("DAK8", "top");
        bkgHTag.SetP1<Axis::X>("fj", 1, "");
        bkgHTag.SetFunction<Axis::X>("HTag");
        
        for(int i = 0; i < bkgFile->Get<TTree>(channel.c_str())->GetEntries(); i++){
            TreeFunction::SetEntry(i);
            predDeepAK.push_back(bkgDeepAK.Get<Axis::X>());
            predHTagAK.push_back(bkgHTag.Get<Axis::X>());
            truth.push_back(0);
        }

        for(const std::string& mass : {"200", "400", "600"}){
            std::shared_ptr<TFile> sigFile = std::make_shared<TFile>(StrUtil::Replace("@/Skim/Channels/@/HPlusAndH_ToWHH_ToL4B_@_100/merged/HPlusAndH_ToWHH_ToL4B_@_100.root", "@", std::getenv("CHDIR"), channel, mass, mass).c_str(), "READ");

            TreeFunction sigDeepAK(sigFile, channel);
            TreeFunction sigHTag(sigFile, channel);
            sigDeepAK.SetP1<Axis::X>("fj", 1, "");
            sigDeepAK.SetFunction<Axis::X>("DAK8", "top");
            sigHTag.SetP1<Axis::X>("fj", 1, "");
            sigHTag.SetFunction<Axis::X>("HTag");
            
            for(int i = 0; i < sigFile->Get<TTree>(channel.c_str())->GetEntries(); i++){
                TreeFunction::SetEntry(i);
                predDeepAK.push_back(sigDeepAK.Get<Axis::X>());
                predHTagAK.push_back(sigHTag.Get<Axis::X>());
                truth.push_back(1);
            }

            PUtil::SetStyle();

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

            PUtil::SetPad(canvas);
            PUtil::SetHist(canvas, deepAKROC->GetHistogram(), "p(True positive)");
            deepAKROC->GetHistogram()->GetXaxis()->SetTitle("p(False positive)");
            deepAKROC->GetHistogram()->SetMaximum(1.2);

            deepAKROC->Draw("AP");
            HTagROC->Draw("P SAME");

            PUtil::DrawHeader(canvas, "", "Work in progress");
            PUtil::DrawLegend(canvas, legend, 2);

            canvas->SaveAs(StrUtil::Replace("DeepAKvsHTag_@_@.pdf", "@", mass, channel).c_str());

            delete deepAKROC; delete HTagROC; delete legend; delete canvas;

            predDeepAK.erase(predDeepAK.begin() + bkgFile->Get<TTree>(channel.c_str())->GetEntries());
            predHTagAK.erase(predHTagAK.begin() + bkgFile->Get<TTree>(channel.c_str())->GetEntries());
            truth.erase(truth.begin() + bkgFile->Get<TTree>(channel.c_str())->GetEntries());
        }
    }
}
