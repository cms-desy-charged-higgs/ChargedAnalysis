#include <string>
#include <vector>
#include <memory>

#include <ChargedAnalysis/Utility/include/backtracer.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/plotutil.h>

#include <TFile.h>
#include <TH2F.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string inputFile = parser.GetValue("input-file");
    std::string mode = parser.GetValue("mode");
    std::string era = parser.GetValue("era");
    std::string channel = parser.GetValue("channel");
    std::vector<std::string> outDir = parser.GetVector("out-dir");

    for(const std::string& dir : outDir) std::system(StrUtil::Merge("mkdir -p ", dir).c_str());

    std::shared_ptr<TFile> file = RUtil::Open(inputFile);
    std::shared_ptr<TH2F> rate = RUtil::GetSmart<TH2F>(file.get(), mode + "rate");

    std::shared_ptr<TCanvas> c = std::make_shared<TCanvas>("c", "c", 1000, 1000);

    PUtil::SetStyle();
    PUtil::SetPad(c.get());
    PUtil::SetHist(c.get(), rate.get());
    c->SetLogx(true);
    c->SetRightMargin(0.15);

    gStyle->SetPaintTextFormat(".2f");
    rate->Draw("TEXT45 COLZ");
    PUtil::DrawHeader(c.get(), PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

    for(const std::string& dir : outDir){
        c->SaveAs((dir + "/" + mode + "rate.pdf").c_str());
        c->SaveAs((dir + "/" + mode + "rate.png").c_str());
    }

    c->Clear();

    std::shared_ptr<TLegend> l = std::make_shared<TLegend>(0., 0., 1, 1);

    for(int yBin = 1; yBin <= rate->GetNbinsY(); ++yBin){
        TH1D* slice = rate->ProjectionX(std::to_string(yBin).c_str(), yBin, yBin);

        if(yBin == 1){
            PUtil::SetHist(c.get(), slice);
            slice->GetYaxis()->SetTitle(mode.substr(0, 1).c_str());
            slice->SetMinimum(0);
        }

        l->AddEntry(slice, StrUtil::Merge(rate->GetYaxis()->GetBinLowEdge(yBin), " < ", rate->GetYaxis()->GetTitle(), " < ", rate->GetYaxis()->GetBinLowEdge(yBin)+ rate->GetYaxis()->GetBinWidth(yBin)).c_str(), "EP");

        slice->SetMarkerStyle(21);
        slice->SetMarkerColor(kBlack -5 + 5*yBin);
        slice->SetLineColor(kBlack -5 + 5*yBin);

        if(yBin == 1) slice->Draw("EP");
        else slice->Draw("SAME EP");
    }

    PUtil::DrawLegend(c.get(), l.get(), 2);
    PUtil::DrawHeader(c.get(), PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

    for(const std::string& dir : outDir){
        c->SaveAs((dir + "/" + mode + "rate_1D.pdf").c_str());
        c->SaveAs((dir + "/" + mode + "rate_1D.png").c_str());
    }
}
