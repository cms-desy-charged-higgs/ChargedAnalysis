#ifndef DNNDATASET_H
#define DNNDATASET_H

#include <torch/torch.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <experimental/random>

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
    torch::Tensor mHPlus;
    torch::Tensor mH;
};

/**
* @brief Custom pytorch dataset class for the input for the Higgs tagger
*/

class DNNDataSet{
    private:
        std::string channel;
        std::vector<std::string> fileNames;
        int era, classLabel, nClass;
        std::vector<std::string> parameters;
        torch::Device device;

        std::vector<std::shared_ptr<NTupleReader>> readers;
        std::vector<NTupleFunction> functions;

        std::vector<std::size_t> chargedMasses, neutralMasses;
        std::vector<std::pair<std::size_t, std::size_t>> entryList;
        std::vector<std::shared_ptr<TFile>> files;
        std::vector<std::shared_ptr<TTree>> trees;

        torch::Tensor OneHotEncoder(const int& classLabel, const int& nClasses, torch::Device& device);

    public:
        DNNDataSet(const std::vector<std::string>& fileNames, const std::string& channel, const std::vector<std::string>& entryListName, const std::vector<std::string>& parameters, const int& era, torch::Device& device, const int& classLabel, const int& nClass, const std::vector<std::size_t>& chargedMasses, const std::vector<std::size_t>& neutralMasses);

        void Init();
        std::size_t Size() const;

        int GetClass(){return classLabel;}
        DNNTensor Get(const std::size_t& entry);

        static DNNTensor Merge(const std::vector<DNNTensor>& tensors);
};

#endif
