#ifndef DNNMODEL_H
#define DNNMODEL_H

#include <torch/torch.h>

#include <ChargedAnalysis/Utility/include/utils.h>

struct DNNModel : torch::nn::Module{
    private:
        //Input layer
        torch::nn::Linear inputLayer{nullptr};

        //Hidden layer
        std::vector<torch::nn::Linear> hiddenLayers;
       // torch::nn::Linear hiddenLayer{nullptr};
  
        //Output layer
        torch::nn::Linear outLayer{nullptr};

        //Other stuff
        int nInput;
        int nNodes;
        float dropOut;
        torch::Device device;

        std::ostringstream modelSummary;
        
    public:
        DNNModel(const int& nInput, const int& nNodes, const int& nHidden, const float& dropOut, torch::Device& device);
        torch::Tensor forward(torch::Tensor input);
        void Print();
        int GetNWeights();
};

#endif
