#include <ChargedAnalysis/Network/include/dnnmodel.h>

DNNModel::DNNModel(const int& nInput, const int& nNodes, const int& nHidden, const float& dropOut, const bool& isParametrized, torch::Device& device):
    nInput(nInput),
    nNodes(nNodes),
    dropOut(dropOut),
    device(device),
    isParametrized(isParametrized)
    {

    //String for model summary
    modelSummary << "\n";
    modelSummary << "Summary of DNN Model\n";
    modelSummary << "----------------------------------------\n";

    //Register all layer
    inNormLayer = register_module("Input norm layer", torch::nn::BatchNorm1d(nInput));
    inNormLayer->pretty_print(modelSummary); modelSummary << "\n";

    inputLayer = register_module("Input layer", torch::nn::Linear(nInput+isParametrized, nNodes));
    inputLayer->pretty_print(modelSummary); modelSummary << "\n";

    reluInLayer = register_module("Input ReLU layer", torch::nn::ReLU());
    reluInLayer->pretty_print(modelSummary); modelSummary << "\n";

    for(int i = 0; i < nHidden; i++){
        torch::nn::BatchNorm1d normLayer = register_module("Norm layer " + std::to_string(i+1), torch::nn::BatchNorm1d(nNodes));
        normLayer->pretty_print(modelSummary); modelSummary << "\n";
        normLayers.push_back(std::move(normLayer));

        torch::nn::Linear hiddenLayer = register_module("Hidden layer " + std::to_string(i+1), torch::nn::Linear(nNodes, nNodes));
        hiddenLayer->pretty_print(modelSummary); modelSummary << "\n";
        hiddenLayers.push_back(std::move(hiddenLayer));

        torch::nn::ReLU reluLayer = register_module("ReLU layer " + std::to_string(i+1), torch::nn::ReLU());
        reluLayer->pretty_print(modelSummary); modelSummary << "\n";
        activationLayers.push_back(std::move(reluLayer));

        torch::nn::Dropout dropLayer = register_module("Dropout layer " + std::to_string(i+1), torch::nn::Dropout(dropOut));
        dropLayer->pretty_print(modelSummary); modelSummary << "\n";
        dropLayers.push_back(dropLayer);
    }

    outNormLayer = register_module("Output norm layer", torch::nn::BatchNorm1d(nNodes));
    outNormLayer->pretty_print(modelSummary); modelSummary << "\n";

    outLayer = register_module("Output layer", torch::nn::Linear(nNodes, 1));
    outLayer->pretty_print(modelSummary); modelSummary << "\n";

    sigLayer = register_module("Sigmoid layer", torch::nn::Sigmoid());
    sigLayer->pretty_print(modelSummary); modelSummary << "\n";

    modelSummary << "Number of trainable parameters: " << this->GetNWeights() << "\n";
    modelSummary << "----------------------------------------\n";

    //Send model to device
    this->to(device);
}

void DNNModel::Print(){
    //Print model summary
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

torch::Tensor DNNModel::forward(torch::Tensor input, torch::Tensor masses){
    //Normalize input without mass and merge with mass if mass parametrized
    torch::Tensor z = inNormLayer->forward(input);
    if(isParametrized) z = torch::cat({z, masses.to(torch::kFloat)/1000.}, 1);

    //Input layer
    z = inputLayer->forward(z);
    z = reluInLayer->forward(z);

    //Hidden layer
    for(int i = 0; i < hiddenLayers.size(); i++){
        z = normLayers[i]->forward(z);
        z = hiddenLayers[i]->forward(z);
        z = activationLayers[i]->forward(z);
        z = dropLayers[i]->forward(z);
    }

    //Output layer
    z = outNormLayer->forward(z);
    z = outLayer->forward(z);
    z = sigLayer->forward(z).squeeze();

    return z;
}
