#ifndef HTAGGER_H
#define HTAGGER_H

#include <torch/torch.h>

#include <ChargedAnalysis/Utility/include/utils.h>

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
        torch::Device device;

        std::ostringstream modelSummary;
        
    public:
        HTagger(const int& nFeat, const int& nHidden, const int& nLSTM, const int& nConvFilter, const int& kernelSize, const float& dropOut, torch::Device& device);
        torch::Tensor forward(torch::Tensor inputCharged, torch::Tensor inputNeutral, torch::Tensor inputSV);
        void Print();
        int GetNWeights();
};

#endif
