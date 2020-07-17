/**
* @file htagdataset.h
* @brief Header file for HTagDataset class
*/

#ifndef HTAGDATASET_H
#define HTAGDATASET_H

#include <torch/torch.h>
#include <string>
#include <vector>
#include <memory>
#include <bitset>

#include <TFile.h>
#include <TTree.h>
#include <TLeaf.h>
#include <TChain.h>

#include <ChargedAnalysis/Utility/include/utils.h>

/**
* @brief Structure with pytorch Tensors of charged/neutral particle for the Higgs tagger
*/

struct HTensor{
    torch::Tensor charged;  //!< Tensor with charged PF candidates of the fat jet
    torch::Tensor neutral;  //!< Tensor with neutral PF candidates of the fat jet
    torch::Tensor SV;   //!< Tensor with secondary verteces of the fat jet
    torch::Tensor label;    //!< Label to check if Higgs or not (isHiggs = 1, isTop = 0)
    torch::Tensor isEven;   //!< Bool to check if event has even event number
};

/**
* @brief Custom pytorch dataset class for the input for the Higgs tagger
*/

class HTagDataset : public torch::data::datasets::Dataset<HTagDataset, HTensor>{
    private:
        std::shared_ptr<TChain> chain;

        int fatIndex;
        torch::Device device;
        bool isSignal;
        int matchedPart;

        std::vector<TLeaf*> jetPart;
        std::vector<TLeaf*> vtx;

        TLeaf* jetCharge;
        TLeaf* jetIdx;  
        TLeaf* vtxIdx;

        int nEntries = 0;
        std::vector<int> trueIndex;
    
        float eventNumber; 

    public:
        /**
        * @brief Constructor for HTagDataset
        * @param files Vector with ROOT file names
        * @param end fatIndex Index which indicates which fat jet of the event should be used
        * @param device Pytorch class for usage of CPU/GPU
        * @param isSignal Boolean to check if files are signal files
        */
        HTagDataset(const std::vector<std::string>& files, const int& fatIndex, torch::Device& device, const bool& isSignal, const int& matchedPart = -1);

        /**
        * @brief Function to get number of fat jets in the dataset
        */
        torch::optional<size_t> size() const;

        /**
        * @brief Getter function to get fat jet of the data set
        * @param index Index of fat jet in the data set
        * @return Returns HTensor of corresponding fat jet
        */
        HTensor get(size_t index);

        /**
        * @brief Static function to merge several HTensor instances while inserting zeros in HTensor member if needed
        * @param tensors Vector with HTensor instances containing the fat jet information
        * @return Returns Merged and padded HTensor
        */
        static HTensor PadAndMerge(std::vector<HTensor>& tensors);
};

#endif
