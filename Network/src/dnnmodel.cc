#include <ChargedAnalysis/Network/include/dnnmodel.h>

DNNModel::DNNModel(const int& nInput, const int& nNodes, const int& nHidden, const float& dropOut, const bool& isParametrized, torch::Device& device):
    nInput(nInput),
    nNodes(nNodes),
    dropOut(dropOut),
    device(device)
    {

    //String for model summary
    modelSummary << "\n";
    modelSummary << "Summary of DNN Model\n";
    modelSummary << "----------------------------------------\n";

    inputLayer = register_module("Input layer", torch::nn::Linear(nInput+isParametrized, nNodes));
    inputLayer->pretty_print(modelSummary); modelSummary << "\n";

    reluInLayer = register_module("Input ReLU layer", torch::nn::ReLU());
    reluInLayer->pretty_print(modelSummary); modelSummary << "\n";

    for(int i = 0; i < nHidden; i++){
        torch::nn::Linear hiddenLayer = register_module("Hidden layer " + std::to_string(i+1), torch::nn::Linear(nNodes, nNodes));
        hiddenLayer->pretty_print(modelSummary); modelSummary << "\n";
        hiddenLayers.push_back(hiddenLayer);

        torch::nn::Dropout dropLayer = register_module("Dropout layer " + std::to_string(i+1), torch::nn::Dropout(dropOut));
        dropLayer->pretty_print(modelSummary); modelSummary << "\n";
        dropLayers.push_back(dropLayer);

        torch::nn::ReLU reluLayer = register_module("ReLU layer " + std::to_string(i+1), torch::nn::ReLU());
        reluLayer->pretty_print(modelSummary); modelSummary << "\n";
        activationLayers.push_back(reluLayer);
    }

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

torch::Tensor DNNModel::forward(torch::Tensor input){
    //Normalize input, but not mass row
    for(int i = 0; i < nInput; i++){
        input.index_put_({torch::indexing::Slice(), torch::indexing::Ellipsis, i}, (input.index({torch::indexing::Slice(), torch::indexing::Ellipsis, i}) - input.mean(0, true).squeeze()[i])/input.std(0, false, true).squeeze()[i]);
    }

    input.index_put_({torch::indexing::Slice(), torch::indexing::Ellipsis, nInput}, input.index({torch::indexing::Slice(), torch::indexing::Ellipsis, nInput})/1000);

    //Pack into pytorch class so LSTM skipped padded values
    torch::Tensor z = inputLayer->forward(input);
    z = reluInLayer->forward(z);

    //Merge them together
    for(int i = 0; i < hiddenLayers.size(); i++){
        z = hiddenLayers[i]->forward(z);
        z = dropLayers[i]->forward(z);
        z = activationLayers[i]->forward(z);
    }

    //Output layer
    z = outLayer->forward(z);
    z = sigLayer->forward(z).squeeze();

    return z;
}
