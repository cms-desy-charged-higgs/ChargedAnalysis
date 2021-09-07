#ifndef DNNDATASET_H
#define DNNDATASET_H

#include <torch/torch.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <TTree.h>
#include <TFile.h>

#include <ChargedAnalysis/Analysis/include/ntuplereader.h>
#include <ChargedAnalysis/Analysis/include/decoder.h>
#include <ChargedAnalysis/Utility/include/csv.h>

/**
* @brief Structure with pytorch Tensors of event kinematics for mass-parametrized DNN
*/

struct DNNTensor{
    torch::Tensor input;    //!< Tensor with kinematic input
    torch::Tensor label;    //!< Label to check if Signal or not (isSignal = 1, isBackground = 0)
   
};

/**
* @brief Custom pytorch dataset class for the input for the Higgs tagger
*/

class DNNDataSet{
    private:
        std::string fileName, channel;
        int era, classLabel;
        std::vector<std::string> parameters;
        torch::Device device;

        std::shared_ptr<NCache> cache;
        std::vector<NTupleReader> functions;

        std::vector<std::size_t> entryList;
        std::shared_ptr<TFile> file;
        std::shared_ptr<TTree> tree;

    public:
        DNNDataSet(const std::string& fileName, const std::string& channel, const std::string& entryListName, const std::vector<std::string>& parameters, const int& era, torch::Device& device, const int& classLabel);

        int chargedMass, neutralMass;
        int GetClass(){return classLabel;}

        void Init();
        DNNTensor Get(const std::size_t& entry);
        std::vector<DNNTensor> GetBatch(const std::size_t& entryStart, const std::size_t& entryEnd);
        std::size_t Size();
        static DNNTensor Merge(const std::vector<DNNTensor>& tensors);
};

#endif
