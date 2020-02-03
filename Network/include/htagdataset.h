#ifndef HTAGDATASET_H
#define HTAGDATASET_H

#include <torch/torch.h>
#include <string>
#include <vector>
#include <bitset>

#include <TFile.h>
#include <TTree.h>
#include <TChain.h>

#include <ChargedAnalysis/Utility/include/utils.h>

struct HTensor{
    torch::Tensor charged;
    torch::Tensor neutral;
    torch::Tensor SV;
    torch::Tensor label;
    torch::Tensor isEven; 

    static HTensor Add(HTensor& h1, HTensor& h2){
        return {torch::cat({h1.charged, h2.charged}, 0), torch::cat({h1.neutral, h2.neutral}, 0), torch::cat({h1.SV, h2.SV}, 0), torch::cat({h1.label, h2.label}, 0), torch::cat({h1.isEven, h2.isEven}, 0)};
    }

    static HTensor Merge(std::vector<HTensor> tensors){
        std::vector<torch::Tensor> charged, neutral, SV, label, isEven; 

        for(HTensor t: tensors){
            charged.push_back(t.charged);
            neutral.push_back(t.neutral);
            SV.push_back(t.SV);
            label.push_back(t.label);
            isEven.push_back(t.isEven);
        }

        return {torch::cat(charged, 0), torch::cat(neutral, 0), torch::cat(SV, 0), torch::cat(label, 0), torch::cat(isEven, 0)};
    }
};

class HTagDataset : public torch::data::datasets::Dataset<HTagDataset, HTensor>{
    private:
        TChain* chain;

        int fatIndex;
        torch::Device device;
        bool isSignal;

        std::vector<std::vector<float>*> particleVec;
        std::vector<std::vector<float>*> secVtxVec;
        float eventNumber; 

    public:
        HTagDataset(const std::vector<std::string>& files, const std::vector<std::string>& channels, const int& fatIndex, torch::Device& device, const bool& isSignal);
        ~HTagDataset();

        torch::optional<size_t> size() const;
        HTensor get(size_t index);
        static HTensor PadAndMerge(std::vector<HTensor>& tensors);
        void clear();
};

#endif
