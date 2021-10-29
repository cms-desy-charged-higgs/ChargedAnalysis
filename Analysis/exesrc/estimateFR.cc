#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

#include <TFile.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <THStack.h>
#include <TLegend.h>

#include <RooRealVar.h>
#include <RooDataHist.h>
#include <RooDataSet.h>
#include <RooHistPdf.h>
#include <RooAddPdf.h>
#include <RooPlot.h>
#include <TMinuit.h>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/plotutil.h>

void Plot(const std::string outDir, const std::string& channel, const std::string& era, std::shared_ptr<TH1F> data, std::shared_ptr<TH1F> prompt, std::shared_ptr<TH1F> nonprompt, const RooRealVar& nPrompt, const RooRealVar& nNonPrompt){
    std::shared_ptr<TH1F> frameHist = RUtil::CloneSmart(prompt.get());
    frameHist->Reset();

    data->SetMarkerStyle(20);
    prompt->SetFillStyle(1001);
    prompt->SetFillColor(kBlue - 3);
    prompt->SetLineColor(kBlue - 3);
    nonprompt->SetFillStyle(1001);
    nonprompt->SetFillColor(kRed - 5);
    nonprompt->SetLineColor(kRed - 5);

    prompt->Scale(nPrompt.getValV()/prompt->Integral());

    std::shared_ptr<TH1F> nonpromptYield = RUtil::CloneSmart(nonprompt.get());
    for(int i = 1; i <= nonpromptYield->GetNbinsX(); ++i){
        nonpromptYield->SetBinContent(i, nNonPrompt.getValV());
        nonpromptYield->SetBinError(i, nNonPrompt.getError());
    }

    nonprompt->Scale(1./nonprompt->Integral());
    nonprompt->Multiply(nonpromptYield.get());

    std::shared_ptr<THStack> sum = std::make_shared<THStack>();
    sum->Add(prompt.get());
    sum->Add(nonprompt.get());

    std::shared_ptr<TH1F> unc = RUtil::CloneSmart(nonprompt.get());
    unc->Add(prompt.get());
    unc->SetFillStyle(3354);
    unc->SetFillColorAlpha(kBlack, 0.8);
    unc->SetMarkerColor(kBlack);

    std::shared_ptr<TCanvas> c = std::make_shared<TCanvas>("c", "c", 1000, 1000);
    std::shared_ptr<TPad> mainPad = std::make_shared<TPad>("mainPad", "mainPad", 0., 0. , 1., 1.);
    PUtil::SetPad(mainPad.get());
    mainPad->Draw();

    std::shared_ptr<TLegend> l = std::make_shared<TLegend>(0., 0., 1, 1);
    l->AddEntry(unc.get(), "Unc.", "F");
    l->AddEntry(prompt.get(), "prompt", "F");
    l->AddEntry(nonprompt.get(), "non-prompt", "F");
    l->AddEntry(data.get(), "data", "EP");

    float max = std::max(sum->GetMaximum(), data->GetMaximum());
    frameHist->SetMaximum(1.2*max);
    PUtil::SetHist(mainPad.get(), frameHist.get(), "Events");

    PUtil::DrawRatio(c.get(), mainPad.get(), data.get(), unc.get(), "Data/Pred");
    mainPad->cd();

    frameHist->Draw();
    sum->Draw("HIST SAME");
    unc->Draw("SAME E2");
    data->Draw("P SAME");

    PUtil::DrawLegend(mainPad.get(), l.get(), 4);
    PUtil::DrawHeader(mainPad.get(), PUtil::GetChannelTitle(channel), "Work in progress", PUtil::GetLumiTitle(era));

    std::filesystem::create_directories(outDir);
    c->SaveAs((outDir + "/postfit.pdf").c_str());
    c->SaveAs((outDir + "/postfit.png").c_str());
}

std::pair<float, float> Calc(const std::string& outDir, const std::string& webDir, const std::string& channel, const std::string& era, const std::string& binName, const std::string& process, const std::vector<std::string>& bkgProcesses, const std::string histName, const std::map<std::string, std::shared_ptr<TFile>>& inFilesLoose, const std::map<std::string, std::shared_ptr<TFile>>& inFilesTight, const std::map<std::string, std::shared_ptr<TFile>>& inFilesNoLep){
    TH1::AddDirectory(0);
    std::shared_ptr<TH1F> dataHistLoose, dataHistTight, dataHistNoLep, bkgHistLoose, bkgHistTight, bkgHistNoLep;

    //Data hists
    dataHistLoose = RUtil::CloneSmart(RUtil::Get<TH1F>(inFilesLoose.at(process).get(), histName));
    dataHistTight = RUtil::CloneSmart(RUtil::Get<TH1F>(inFilesTight.at(process).get(), histName));
    dataHistNoLep = RUtil::CloneSmart(RUtil::Get<TH1F>(inFilesTight.at(process).get(), histName));

    std::string xLabel = dataHistLoose->GetXaxis()->GetTitle();
    std::string yLabel = dataHistLoose->GetYaxis()->GetTitle();

    //Bkg hists
    for(const std::string p : bkgProcesses){
        if(bkgHistLoose == nullptr){
            bkgHistLoose = RUtil::CloneSmart(RUtil::Get<TH1F>(inFilesLoose.at(p).get(), histName));
            bkgHistTight = RUtil::CloneSmart(RUtil::Get<TH1F>(inFilesTight.at(p).get(), histName));
            bkgHistNoLep = RUtil::CloneSmart(RUtil::Get<TH1F>(inFilesNoLep.at(p).get(), histName));
        }

        else{
            bkgHistLoose->Add(RUtil::Get<TH1F>(inFilesLoose.at(p).get(), histName));
            bkgHistTight->Add(RUtil::Get<TH1F>(inFilesTight.at(p).get(), histName));
            bkgHistNoLep->Add(RUtil::Get<TH1F>(inFilesNoLep.at(p).get(), histName));
        }
    }

    //Silence as much as possible
    PUtil::SetStyle();
    RooMsgService::instance().setSilentMode(true);

    //Create templates for fakes from lepton failing tight selection
    std::shared_ptr<TH1F> fakeTemplate = RUtil::CloneSmart(dataHistNoLep.get());
    fakeTemplate->Add(bkgHistNoLep.get(), -1);

    //Create RooFit datasets from histograms
    RooRealVar x("x","x", dataHistLoose->GetBinLowEdge(1), dataHistLoose->GetBinLowEdge(dataHistLoose->GetNbinsX() + 1));
    RooDataHist dataLoose("dataLoose", "dataLoose", x, dataHistLoose.get());
    RooDataHist dataTight("dataTight", "dataTight", x, dataHistTight.get());
    RooDataHist promptLoose("promptLoose","promptLoose", x, bkgHistLoose.get());
    RooDataHist promptTight("promptLoose","promptLoose", x, bkgHistTight.get());
    RooDataHist fake("fake", "fake", x, fakeTemplate.get());

    //PDF from Roofit datasets
    RooHistPdf promptLoosePDF("promptLoosePDF", "promptLoosePDF", x, promptLoose, 0);
    RooHistPdf promptTightPDF("promptTightPDF", "promptTightPDF", x, promptTight, 0);
    RooHistPdf fakePDF("fakePDF", "fakePDF", x, fake, 0);

    //Define nBkg/nFake variable which is fitted
    RooRealVar nBkgLoose("nBkgLoose","nBkgLoose", 500, bkgHistLoose->Integral(), bkgHistLoose->Integral()),
               nBkgTight("nBkgTight","nBkgTight", 500, bkgHistTight->Integral(), bkgHistTight->Integral());
    RooRealVar nFakeLoose("nFakeLoose","nFakeLoose", 500, 0., dataHistLoose->Integral()), 
               nFakeTight("nFakeTight","nFakeTight", 500, 0., dataHistTight->Integral());

    //Define model as nBkg * bkgPDF + nFakes * fakePDF and fit
    RooAddPdf modelLoose("modelLoose","modelLoose", RooArgList(fakePDF, promptLoosePDF), RooArgList(nFakeLoose, nBkgLoose));
    RooAddPdf modelTight("modelTight","modelTight", RooArgList(fakePDF, promptTightPDF), RooArgList(nFakeTight, nBkgTight));

    modelLoose.fitTo(dataLoose);
    modelTight.fitTo(dataTight);

    //Postfit plots
    Plot(outDir + "/Loose/" + binName, channel, era, dataHistLoose, bkgHistLoose, fakeTemplate, nBkgLoose, nFakeLoose);
    Plot(webDir + "/Loose/" + binName, channel, era, dataHistLoose, bkgHistLoose, fakeTemplate, nBkgLoose, nFakeLoose);
    Plot(outDir + "/Tight/" + binName, channel, era, dataHistTight, bkgHistTight, fakeTemplate, nBkgTight, nFakeTight);
    Plot(webDir + "/Tight/" + binName, channel, era, dataHistTight, bkgHistTight, fakeTemplate, nBkgTight, nFakeTight);


    return {nFakeTight.getValV()/nFakeLoose.getValV(), nFakeTight.getValV()/nFakeLoose.getValV()*std::sqrt(std::pow(nFakeTight.getError()/nFakeTight.getValV(), 2) + std::pow(nFakeLoose.getError()/nFakeLoose.getValV(), 2)) };
}

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string outDir = parser.GetValue("out-dir");
    std::string webDir = parser.GetValue("web-dir");
    std::string era = parser.GetValue("era");
    std::string channel = parser.GetValue("channel");
    std::string histName = parser.GetValue("hist-name");
    std::string process = parser.GetValue("process");
    std::vector<std::string> binNames = parser.GetVector("bins");
    std::vector<std::string> bkgProcsesses = parser.GetVector("bkg-processes");

    std::vector<float> ptBins, etaBins;

    for(const std::string& bin : binNames){
        std::vector<float> binEdges = StrUtil::Split<float>(bin, "_");
        if(std::find(ptBins.begin(), ptBins.end(), binEdges.at(0)) == ptBins.end()) ptBins.push_back(binEdges.at(0));
        if(std::find(ptBins.begin(), ptBins.end(), binEdges.at(1)) == ptBins.end()) ptBins.push_back(binEdges.at(1));
        if(std::find(etaBins.begin(), etaBins.end(), binEdges.at(2)) == etaBins.end()) etaBins.push_back(binEdges.at(2));
        if(std::find(etaBins.begin(), etaBins.end(), binEdges.at(3)) == etaBins.end()) etaBins.push_back(binEdges.at(3));
    }

    std::shared_ptr<TFile> outFile = std::make_shared<TFile>((outDir + "/fakerate.root").c_str(), "RECREATE");
    std::shared_ptr<TH2F> outHist = std::make_shared<TH2F>("fakerate", "fakerate", ptBins.size() - 1, ptBins.data(), etaBins.size() - 1, etaBins.data());

    for(const std::string& bin : binNames){
        std::map<std::string, std::shared_ptr<TFile>> inFilesLoose, inFilesTight, inFilesNoLep;

        inFilesLoose[process] =  RUtil::Open(parser.GetValue(StrUtil::Merge("fake-loose-", process, "_", bin, "-file")));
        inFilesTight[process] =  RUtil::Open(parser.GetValue(StrUtil::Merge("fake-tight-", process, "_", bin, "-file")));
        inFilesNoLep[process] =  RUtil::Open(parser.GetValue(StrUtil::Merge("fake-no-lep-", process, "_", bin, "-file")));

        for(const std::string p : bkgProcsesses){
            inFilesLoose[p] =  RUtil::Open(parser.GetValue(StrUtil::Merge("fake-loose-", p, "_", bin, "-file")));
            inFilesTight[p] =  RUtil::Open(parser.GetValue(StrUtil::Merge("fake-tight-", p, "_", bin, "-file")));
            inFilesNoLep[p] =  RUtil::Open(parser.GetValue(StrUtil::Merge("fake-no-lep-", p, "_", bin, "-file")));
        }

        std::pair<float, float> fr = Calc(outDir, webDir, channel, era, bin, process, bkgProcsesses, histName, inFilesLoose, inFilesTight, inFilesNoLep);

        std::vector<float> binEdges = StrUtil::Split<float>(bin, "_");
        outHist->SetBinContent(outHist->FindBin(binEdges.at(0), binEdges.at(2)), fr.first);
        outHist->SetBinError(outHist->FindBin(binEdges.at(0), binEdges.at(2)), fr.second);
    }

    outFile->cd();
    outHist->GetXaxis()->SetTitle(StrUtil::Replace("p_{T}([P]^{loose}_{1}) [GeV]", "[P]", channel == "MuonIncl" ? "#mu" : "e").c_str());
    outHist->GetYaxis()->SetTitle(StrUtil::Replace("#eta([P]^{loose}_{1})", "[P]", channel == "MuonIncl" ? "#mu" : "e").c_str());
    outHist->Write();
}
