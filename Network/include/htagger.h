#ifndef HTAGGER_H
#define HTAGGER_H

#include <torch/torch.h>

#include <TFile.h>
#include <TTree.h>
#include <Math/GenVector/LorentzVector.h>
#include <Math/GenVector/VectorUtil.h>
#include <Math/Vector3Dfwd.h>
#include <Math/Vector4Dfwd.h>

#include <ChargedAnalysis/Analysis/include/utils.h>

struct HTagger : torch::nn::Module{
    private:
        //Input layer
        torch::nn::LSTM lstmCharged{nullptr};
        torch::nn::LSTM lstmNeutral{nullptr};
        torch::nn::LSTM lstmSV{nullptr};

        //Conv1 layer
        torch::nn::Conv1d convLayer{nullptr};

        //Output layer
        torch::nn::Linear outLayer{nullptr};

        //Other stuff
        int nHidden;
        int nLSTM;
        float dropOut;

        std::ostringstream modelSummary;
        
    public:
        HTagger();
        HTagger(const int& nFeat, const int& nHidden, const int& nLSTM, const int& nConvFilter, const int& kernelSize, const float& dropOut);
        torch::Tensor forward(torch::Tensor inputCharged, torch::Tensor inputNeutral, torch::Tensor inputSV, const bool& isTraining);
        void Print();
        int GetNWeights();

        static std::vector<torch::Tensor> GatherInput(const std::string& fileName, const std::string& channel, const int& entryStart, const int& entryEnd, const int& FJindex);

};

#endif
