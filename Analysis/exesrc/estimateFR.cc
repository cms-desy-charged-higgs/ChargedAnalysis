#include <string>
#include <vector>
#include <map>
#include <memory>

#include <TFile.h>
#include <TH2F.h>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/backtracer.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

void Calc(const std::string& outDir, const std::string& process, const std::vector<std::string>& processesToRemove, const std::string& mode, const std::map<std::string, std::shared_ptr<TFile>>& inFilesLoose, const std::map<std::string, std::shared_ptr<TFile>>& inFilesTight){
    TH1::AddDirectory(0);
    std::shared_ptr<TH2F> inHistsLoose, inHistsTight;

    std::string histName = RUtil::ListOfContent(inFilesLoose.at(process).get()).back();

    inHistsLoose = RUtil::GetSmart<TH2F>(inFilesLoose.at(process).get(), histName);
    inHistsTight = RUtil::GetSmart<TH2F>(inFilesTight.at(process).get(), histName);

    std::string xLabel = inHistsLoose->GetXaxis()->GetTitle();
    std::string yLabel = inHistsLoose->GetYaxis()->GetTitle();

    for(const std::string p : processesToRemove){
        inHistsLoose->Add(RUtil::Get<TH2F>(inFilesLoose.at(process).get(), histName), -1);
        inHistsTight->Add(RUtil::Get<TH2F>(inFilesTight.at(process).get(), histName), -1);
    }

    std::shared_ptr<TFile> outFile = std::make_shared<TFile>((outDir + "/" + mode + "rate.root").c_str(), "RECREATE");

    std::shared_ptr<TH2F> outHist = RUtil::CloneSmart(inHistsLoose.get());
    outHist->Divide(inHistsTight.get(), inHistsLoose.get());

    double errThr = mode == "prompt" ? 0.05 : 0.2;

    while(true){
        std::vector<double> currentX(outHist->GetNbinsX() + 1, 0.), currentY(outHist->GetNbinsY() + 1, 0.);
        unsigned int nX, nY;

        for(unsigned int i = 1; i <= outHist->GetNbinsX() + 1; ++i){
            currentX[i - 1] = outHist->GetXaxis()->GetBinLowEdge(i);
        }

        for(unsigned int i = 1; i <= outHist->GetNbinsY() + 1; ++i){
            currentY[i - 1] = outHist->GetYaxis()->GetBinLowEdge(i);
        }

        nX = currentX.size(); nY = currentY.size();

        for(unsigned int y = currentY.size() - 1; y > 1; --y){
            for(unsigned int x = currentX.size() - 1; x > 1; --x){
                double relErr = outHist->GetBinError(x, y)/outHist->GetBinContent(x, y);

                if(std::abs(relErr) > errThr or std::isnan(relErr) or outHist->GetBinContent(x, y) < 0){
                    currentY.erase(currentY.begin() + y - 1);
                    --y;

                    inHistsLoose = RUtil::Rebin2D(inHistsLoose, currentX, currentY);
                    inHistsTight = RUtil::Rebin2D(inHistsTight, currentX, currentY);

                    outHist = RUtil::CloneSmart(inHistsLoose.get());
                    outHist->Divide(inHistsTight.get(), inHistsLoose.get());

                    break;
                }
            }
        }

        for(unsigned int x = currentX.size() - 1; x > 1; --x){
            for(unsigned int y = currentY.size() - 1; y > 1; --y){
                double relErr = outHist->GetBinError(x, y)/outHist->GetBinContent(x, y);

                if(std::abs(relErr) > errThr or std::isnan(relErr) or outHist->GetBinContent(x, y) < 0){
                    currentX.erase(currentX.begin() + x - 1);
                    --x;

                    inHistsLoose = RUtil::Rebin2D(inHistsLoose, currentX, currentY);
                    inHistsTight = RUtil::Rebin2D(inHistsTight, currentX, currentY);

                    outHist = RUtil::CloneSmart(inHistsLoose.get());
                    outHist->Divide(inHistsTight.get(), inHistsLoose.get());

                    break;
                }
            }
        }
        
        if(nX == currentX.size() && nY == currentY.size()) break;
    }

    outHist->GetXaxis()->SetTitle(xLabel.c_str());
    outHist->GetYaxis()->SetTitle(yLabel.c_str());
    outHist->SetName((mode + "rate").c_str());
    outHist->SetTitle((mode + "rate").c_str());
    outHist->Write();
}

int main(int argc, char *argv[]){
    //Class for backtrace
    //Backtracer trace;

    //Parser arguments
    Parser parser(argc, argv);

    std::string outDir = parser.GetValue("out-dir");
    std::string mode = parser.GetValue("mode");
    std::string process = parser.GetValue("process");
    std::vector<std::string> processesToRemove = parser.GetVector("processes-to-remove");

    std::map<std::string, std::shared_ptr<TFile>> inFilesLoose, inFilesTight;

    inFilesLoose[process] =  RUtil::Open(parser.GetValue(StrUtil::Join("-", mode, "loose", process, "file")));
    inFilesTight[process] =  RUtil::Open(parser.GetValue(StrUtil::Join("-", mode, "tight", process, "file")));

    for(const std::string p : processesToRemove){
        inFilesLoose[p] =  RUtil::Open(parser.GetValue(StrUtil::Join("-", mode, "loose", p, "file")));
        inFilesTight[p] =  RUtil::Open(parser.GetValue(StrUtil::Join("-", mode, "tight", p, "file")));
    }

    Calc(outDir, process, processesToRemove, mode, inFilesLoose, inFilesTight);
}
