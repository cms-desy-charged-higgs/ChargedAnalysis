#include <vector>
#include <string>

#include <TFile.h>
#include <TParameter.h>
#include <TH1F.h>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> fileNames = parser.GetVector<std::string>("input-files");

    float lumi = 1.;
    std::vector<float> xSecs, nGen;
    TParameter<float> nGenStiched("nGenStitched", 0.);
    std::vector<TParameter<float>> weights;

    for(const std::string& fileName : fileNames){
        std::shared_ptr<TFile> file = RUtil::Open(fileName);

        xSecs.push_back(RUtil::Get<TParameter<float>>(file.get(), "xSec")->GetVal());
        nGen.push_back(RUtil::Get<TParameter<float>>(file.get(), "nGen")->GetVal());
        nGenStiched.SetVal(nGenStiched.GetVal() + nGen.back());
    }

    std::vector<int> indeces = VUtil::SortedIndices(xSecs, [&](float i, float j){return i > j;});
    xSecs = VUtil::SortByIndex(xSecs, indeces);
    nGen = VUtil::SortByIndex(nGen, indeces);

    for(int i = 0; i < xSecs.size(); ++i){
        float w = i == 0 ? 1./(nGen[i]/xSecs[i]) : 1./(nGen[0]/xSecs[0] + nGen[i]/xSecs[i]);
        TParameter<float> weight(StrUtil::Merge("Weight_", i, "Jets").c_str(), w);
        weights.push_back(weight);
    }

    for(const std::string& fileName : fileNames){
        std::shared_ptr<TFile> file = RUtil::Open(fileName);

        std::string tmpName = fileName.substr(0, StrUtil::Find(fileName, "/").back()) + "/tmp.root";
        std::shared_ptr<TFile> tmpFile = std::make_shared<TFile>(tmpName.c_str(), "RECREATE");

        for(TObject* key : *(file->GetListOfKeys())){
            tmpFile->cd();

            TObject* obj = file->Get(key->GetName());

            if(StrUtil::Find(key->GetName(), "Weight_").empty() and StrUtil::Find(key->GetName(), "nGenStitched").empty()){
                if(obj->InheritsFrom(TTree::Class())){
                    TTree* tree = static_cast<TTree*>(obj);
                    TTree* newTree = tree->CloneTree(-1, "fast");

                    newTree->Write();
                }

                else obj->Write();
            }
        }

        nGenStiched.Write();

        for(TParameter<float>& w: weights) w.Write();

        file->Close();  
        tmpFile->Close();

        if(RUtil::Open(tmpName)) std::system(StrUtil::Merge("mv -v ", tmpName, " ", fileName).c_str());
    }
}
