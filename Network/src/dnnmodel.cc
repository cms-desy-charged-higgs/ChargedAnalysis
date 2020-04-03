#include <ChargedAnalysis/Network/include/dnnmodel.h>

DNNModel::DNNModel(const int& nInput, const int& nNodes, const int& nHidden, const float& dropOut, torch::Device& device):
    nInput(nInput),
    nNodes(nNodes),
    dropOut(dropOut),
    device(device)
    {

    //String for model summary
    modelSummary << "\n";
    modelSummary << "Summary of DNN Model\n";
    modelSummary << "----------------------------------------\n";

    //Input layer
    inputLayer = register_module("Input layer", torch::nn::Linear(nInput+1, nNodes));
    inputLayer->pretty_print(modelSummary); modelSummary << "\n";

    for(int i = 0; i < nHidden; i++){
        torch::nn::Linear hiddenLayer = register_module("Hidden layer " + std::to_string(i+1), torch::nn::Linear(nNodes, nNodes));
        hiddenLayer->pretty_print(modelSummary); modelSummary << "\n";
        hiddenLayers.push_back(hiddenLayer);
    }

    //Output layer
    outLayer = register_module("Output layer", torch::nn::Linear(nNodes, 1));
    outLayer->pretty_print(modelSummary); modelSummary << "\n";

    modelSummary << "Number of trainable parameters: " << this->GetNWeights() << "\n";  

    //Send model to device
    this->to(device);
}

void DNNModel::Print(){
    //Print model summary
    modelSummary << "----------------------------------------\n";
    std::cout << modelSummary.str() << std::endl;
}

int DNNModel::GetNWeights(){
    //Number of trainable parameters
    int nWeights = 0;

    for(at::Tensor t: this->parameters()){
        if(t.requires_grad()) nWeights += t.numel();
    }

    return nWeights;
}

torch::Tensor DNNModel::forward(torch::Tensor input){
    //Pack into pytorch class so LSTM skipped padded values
    torch::Tensor z = inputLayer->forward(input);
    z = torch::relu(z);

    //Merge them together
    for(torch::nn::Linear& hiddenLayer: hiddenLayers){
        z = hiddenLayer->forward(z);
        z = torch::relu(z);
    }

    //Output layer
    z = outLayer->forward(z);
    z = torch::sigmoid(z);

    return z;
}
