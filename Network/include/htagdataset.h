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
};

class HTagDataset : public torch::data::datasets::Dataset<HTagDataset, HTensor>{
    private:
        TChain* chain;

        int fatIndex;
        torch::Device device;
        bool isSignal;

        std::vector<std::string> particleVariables;
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
