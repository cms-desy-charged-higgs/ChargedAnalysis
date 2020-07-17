#ifndef DNNDATASET_H
#define DNNDATASET_H

#include <torch/torch.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <ChargedAnalysis/Utility/include/utils.h>

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
        std::vector<std::ifstream*> files;
        int fileIndex;
        int nLines = 0;

        torch::Device device;
        bool isSignal;

        float eventNumber; 

    public:
        /**
        * @brief Constructor for DNNDataset
        * @param files Vector with CSV file names
        * @param device Pytorch class for usage of CPU/GPU
        * @param isSignal Boolean to check if files are signal files
        */
        DNNDataset(const std::vector<std::string>& files, torch::Device& device, const bool& isSignal);

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

        /**
        * @brief Static function to merge several DNNTensor instances
        * @param tensors Vector with DNNTensors
        * @return Returns Merged DNNTensor
        */
        static DNNTensor Merge(std::vector<DNNTensor>& tensors);
};

#endif
