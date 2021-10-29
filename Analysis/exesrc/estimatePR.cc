#include <string>
#include <vector>
#include <map>
#include <memory>

#include <TFile.h>
#include <TH2F.h>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string outDir = parser.GetValue("out-dir");
    std::string histName = parser.GetValue("hist-name");
    std::string process = parser.GetValue("process");

    std::shared_ptr<TFile> inFilesLoose, inFilesTight;

    inFilesLoose =  RUtil::Open(parser.GetValue(StrUtil::Join("-", "prompt", "loose", process, "file")));
    inFilesTight =  RUtil::Open(parser.GetValue(StrUtil::Join("-", "prompt", "tight", process, "file")));

    TH1::AddDirectory(0);
    std::shared_ptr<TH2F> inHistsLoose, inHistsTight;

    inHistsLoose = RUtil::CloneSmart(RUtil::Get<TH2F>(inFilesLoose.get(), histName));
    inHistsTight = RUtil::CloneSmart(RUtil::Get<TH2F>(inFilesTight.get(), histName));

    std::string xLabel = inHistsLoose->GetXaxis()->GetTitle();
    std::string yLabel = inHistsLoose->GetYaxis()->GetTitle();

    std::shared_ptr<TFile> outFile = std::make_shared<TFile>((outDir + "/" + "promptrate.root").c_str(), "RECREATE");
    std::shared_ptr<TH2F> outHist = RUtil::CloneSmart(inHistsLoose.get());

    outHist->Divide(inHistsTight.get(), inHistsLoose.get());

    outHist->GetXaxis()->SetTitle(xLabel.c_str());
    outHist->GetYaxis()->SetTitle(yLabel.c_str());
    outHist->SetName("promptrate");
    outHist->SetTitle("promptrate");
    outHist->Write();
}
