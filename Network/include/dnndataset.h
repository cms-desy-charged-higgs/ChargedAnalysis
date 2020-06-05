#ifndef DNNDATASET_H
#define DNNDATASET_H

#include <torch/torch.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <ChargedAnalysis/Utility/include/utils.h>

struct DNNTensor{
    torch::Tensor input;
    torch::Tensor label;
};

class DNNDataset : public torch::data::datasets::Dataset<DNNDataset, DNNTensor>{
    private:
        std::vector<std::ifstream*> files;
        int fileIndex;
        int nLines = 0;

        torch::Device device;
        bool isSignal;

        float eventNumber; 

    public:
        DNNDataset(const std::vector<std::string>& files, torch::Device& device, const bool& isSignal);
        ~DNNDataset(){}

        torch::optional<size_t> size() const;
        DNNTensor get(size_t index);
        static DNNTensor Merge(std::vector<DNNTensor>& tensors);
        void clear();
};

#endif
