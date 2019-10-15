#ifndef HTAGGER_H
#define HTAGGER_H

#include <torch/torch.h>

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

    public:
        HTagger();
        HTagger(const int& nFeat, const int& nHidden, const int& nLSTM, const int& nConvFilter, const int& kernelSize);
        torch::Tensor forward(torch::Tensor inputCharged, torch::Tensor inputNeutral, torch::Tensor inputSV);
};

#endif
