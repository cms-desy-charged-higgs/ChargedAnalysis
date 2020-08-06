#ifndef HTAGGER_H
#define HTAGGER_H

#include <torch/torch.h>

#include <ChargedAnalysis/Utility/include/utils.h>

struct HTagger : torch::nn::Module{
    private:
        //Input layer
        torch::nn::BatchNorm1d normCharged{nullptr};
        torch::nn::BatchNorm1d normNeutral{nullptr};
        torch::nn::BatchNorm1d normSV{nullptr};

        torch::nn::LSTM lstmCharged{nullptr};
        torch::nn::LSTM lstmNeutral{nullptr};
        torch::nn::LSTM lstmSV{nullptr};

        torch::nn::BatchNorm1d normHidden{nullptr};

        //Conv1 layer
        torch::nn::Conv1d convLayer{nullptr};
        torch::nn::ReLU reluLayer{nullptr};
        torch::nn::Dropout dropLayer{nullptr};
        torch::nn::MaxPool1d poolLayer{nullptr};

        //Output layer
        torch::nn::BatchNorm1d outNorm{nullptr};
        torch::nn::Linear outLayer{nullptr};
        torch::nn::Sigmoid sigLayer{nullptr};

        //Other stuff
        int nHidden;
        float dropOut;
        torch::Device device;

        std::ostringstream modelSummary;
        
    public:
        HTagger(const int& nFeat, const int& nHidden, const int& nConvFilter, const int& kernelSize, const float& dropOut, torch::Device& device);
        torch::Tensor forward(torch::Tensor inputCharged, torch::Tensor inputNeutral, torch::Tensor inputSV);
        void Print();
        int GetNWeights();
};

#endif
