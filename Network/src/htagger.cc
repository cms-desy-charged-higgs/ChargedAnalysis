#include <ChargedAnalysis/Network/include/htagger.h>

HTagger::HTagger(const int& nFeat, const int& nHidden, const int& nConvFilter, const int& kernelSize, const float& dropOut, torch::Device& device):
    nHidden(nHidden),
    dropOut(dropOut),
    device(device)
    {

    //String for model summary
    modelSummary << "\n";
    modelSummary << "Summary of HTagger Model\n";
    modelSummary << "----------------------------------------\n";

    //Register LSTM Layers
    lstmCharged = register_module("LSTM charged input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).batch_first(true)));
    lstmCharged->pretty_print(modelSummary); modelSummary << "\n";

    lstmNeutral = register_module("LSTM neutral input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).batch_first(true)));
    lstmNeutral->pretty_print(modelSummary); modelSummary << "\n";

    lstmSV = register_module("LSTM SV input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).batch_first(true)));
    lstmSV->pretty_print(modelSummary); modelSummary << "\n";

    convLayer = register_module("Conv layer 1", torch::nn::Conv1d(3, nConvFilter, kernelSize));
    convLayer->pretty_print(modelSummary); modelSummary << "\n";

    //Activation function
    reluLayer = register_module("ReLU layer", torch::nn::ReLU());
    reluLayer->pretty_print(modelSummary); modelSummary << "\n";

    dropLayer = register_module("Dropout layer", torch::nn::Dropout(dropOut));
    dropLayer->pretty_print(modelSummary); modelSummary << "\n";

    poolLayer = register_module("Pool layer", torch::nn::MaxPool1d(torch::nn::MaxPool1dOptions(kernelSize)));
    poolLayer->pretty_print(modelSummary); modelSummary << "\n";

    //Output layer
    int outConv = (nHidden - kernelSize)/kernelSize;

    //int outConv = nHidden + 2 - kernelSize -1;
    outLayer = register_module("Output layer", torch::nn::Linear(nConvFilter*outConv, 1));
    outLayer->pretty_print(modelSummary); modelSummary << "\n";

    //Outlayer activation
    sigLayer = register_module("Sigmoid layer", torch::nn::Sigmoid());
    sigLayer->pretty_print(modelSummary); modelSummary << "\n";

    modelSummary << "Number of trainable parameters: " << this->GetNWeights() << "\n";
    modelSummary << "----------------------------------------\n";

    //Send model to device
    this->to(device);
}

void HTagger::Print(){
    //Print model summary
    std::cout << modelSummary.str() << std::endl;
}

int HTagger::GetNWeights(){
    //Number of trainable parameters
    int nWeights = 0;

    for(at::Tensor t: this->parameters()){
        if(t.requires_grad()) nWeights += t.numel();
    }

    return nWeights;
}

torch::Tensor HTagger::forward(torch::Tensor inputCharged, torch::Tensor inputNeutral, torch::Tensor inputSV){
    //Get non-padded length for each batch
    torch::Tensor charTrueLen = (inputCharged.index({torch::indexing::Slice(), torch::indexing::Slice(), 0}) != 0).sum(1).squeeze().to(torch::kCPU);
    torch::Tensor neutralTrueLen = (inputNeutral.index({torch::indexing::Slice(), torch::indexing::Slice(), 0}) != 0).sum(1).squeeze().to(torch::kCPU);
    torch::Tensor SVTrueLen = (inputSV.index({torch::indexing::Slice(), torch::indexing::Slice(), 0}) != 0).sum(1).squeeze().to(torch::kCPU);

    charTrueLen = torch::where(charTrueLen > 0, charTrueLen, torch::ones({1}).to(torch::kLong));
    neutralTrueLen = torch::where(neutralTrueLen > 0, neutralTrueLen, torch::ones({1}).to(torch::kLong));
    SVTrueLen = torch::where(SVTrueLen > 0, SVTrueLen, torch::ones({1}).to(torch::kLong));

    //Pack sequences
    torch::nn::utils::rnn::PackedSequence chargedPack = torch::nn::utils::rnn::pack_padded_sequence(inputCharged, charTrueLen, true, false);
    torch::nn::utils::rnn::PackedSequence neutralPack = torch::nn::utils::rnn::pack_padded_sequence(inputNeutral, neutralTrueLen, true, false);
    torch::nn::utils::rnn::PackedSequence SVPack = torch::nn::utils::rnn::pack_padded_sequence(inputSV, SVTrueLen, true, false);

    //LSTM layer
    torch::Tensor chargedHidden = std::get<0>(std::get<1>(lstmCharged->forward_with_packed_input(chargedPack))).transpose(1, 0);
    torch::Tensor neutralHidden = std::get<0>(std::get<1>(lstmNeutral->forward_with_packed_input(neutralPack))).transpose(1, 0);
    torch::Tensor SVHidden = std::get<0>(std::get<1>(lstmSV->forward_with_packed_input(SVPack))).transpose(1, 0);
    
    torch::Tensor z = torch::cat({chargedHidden, neutralHidden, SVHidden}, 1);

    //Convolution layer
    z = convLayer->forward(z);
    z = reluLayer->forward(z);
    z = dropLayer->forward(z);
    z = poolLayer->forward(z);
    z = torch::flatten(z, 1, 2);
    
    //Output layer
    z = outLayer->forward(z);
    z = sigLayer->forward(z).squeeze();

    return z;
}
