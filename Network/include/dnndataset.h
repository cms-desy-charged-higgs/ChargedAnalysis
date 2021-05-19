#ifndef DNNDATASET_H
#define DNNDATASET_H

#include <torch/torch.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <ChargedAnalysis/Analysis/include/ntuplereader.h>
#include <ChargedAnalysis/Analysis/include/decoder.h>

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

class DNNDataset : public torch::data::datasets::Dataset<DNNDataset, DNNTensor>{
    private:
        int nEntries = 0;
        std::vector<int> trueIndex;

        std::vector<NTupleReader> functions;
        std::vector<NTupleReader> cuts;

        torch::Device device;
        int classLabel;

    public:
        /**
        * @brief Constructor for DNNDataset
        * @param files Vector with CSV file names
        * @param device Pytorch class for usage of CPU/GPU
        * @param isSignal Boolean to check if files are signal files
        */
        DNNDataset(std::shared_ptr<TTree>& tree, const std::vector<std::string>& parameters, const std::vector<std::string>& cuts, const int& era, const int& isEven, torch::Device& device, const int& classLabel);

        /**
        * @brief Function to get number of events in the dataset
        */
        torch::optional<size_t> size() const;

        /**
        * @brief Getter function to get event of the data set
        * @param index Index of event in the data set
        * @return Returns corresponding DNNDataset
        */
        DNNTensor get(size_t index);
        
        int GetClass(){return classLabel;}

        /**
        * @brief Static function to merge several DNNTensor instances
        * @param tensors Vector with DNNTensors
        * @return Returns Merged DNNTensor
        */
        static DNNTensor Merge(const std::vector<DNNTensor>& tensors);
};

#endif
