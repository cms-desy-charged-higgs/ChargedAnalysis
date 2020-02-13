#include <ChargedAnalysis/Network/include/htagger.h>

HTagger::HTagger(const int& nFeat, const int& nHidden, const int& nLSTM, const int& nConvFilter, const int& kernelSize, const float& dropOut, torch::Device& device):
    nLSTM(nLSTM),
    nHidden(nHidden),
    dropOut(dropOut),
    device(device)
    {

    //String for model summary
    modelSummary << "\n";
    modelSummary << "Summary of HTagger Model\n";
    modelSummary << "----------------------------------------\n";

    //Register LSTM Layers
    lstmCharged = register_module("LSTM charged input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true).dropout(dropOut)));
    lstmCharged->pretty_print(modelSummary); modelSummary << "\n";

    lstmNeutral = register_module("LSTM neutral input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true).dropout(dropOut)));
    lstmNeutral->pretty_print(modelSummary); modelSummary << "\n";

    lstmSV = register_module("LSTM SV input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true).dropout(dropOut)));
    lstmSV->pretty_print(modelSummary); modelSummary << "\n";

    //Convolution layer
    convLayer = register_module("Conv layer 1", torch::nn::Conv1d(3, nConvFilter, kernelSize));
    convLayer->pretty_print(modelSummary); modelSummary << "\n";


    //Output layer
    int outConv = nHidden + 2 - kernelSize -1;
    outLayer = register_module("Output layer", torch::nn::Linear(nConvFilter*outConv, 1));
    outLayer->pretty_print(modelSummary); modelSummary << "\n";

    modelSummary << "Number of trainable parameters: " << this->GetNWeights() << "\n";  

    //Send model to device
    this->to(device);
}

void HTagger::Print(){
    //Print model summary
    modelSummary << "----------------------------------------\n";
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
    //Pack into pytorch class so LSTM skipped padded values
    torch::Tensor chargedSeq = ((inputCharged.narrow(2, 0, 1) != 0).sum(1) - 1).squeeze(1);
    torch::Tensor neutralSeq = ((inputNeutral.narrow(2, 0, 1) != 0).sum(1) - 1).squeeze(1);
    torch::Tensor SVSeq = ((inputSV.narrow(2, 0, 1) != 0).sum(1) - 1).squeeze(1);

    torch::Tensor zero = torch::tensor({0}, torch::kLong).to(device);

    chargedSeq = torch::where(chargedSeq == -1, zero, chargedSeq);
    neutralSeq = torch::where(neutralSeq == -1, zero, neutralSeq);
    SVSeq = torch::where(SVSeq == -1, zero, SVSeq);

    torch::Tensor batchIndex = torch::arange(0, inputCharged.size(0), torch::kLong).to(device);

    //LSTM layer
    torch::nn::RNNOutput charged = lstmCharged->forward(inputCharged);
    torch::nn::RNNOutput neutral = lstmNeutral->forward(inputNeutral);
    torch::nn::RNNOutput SV = lstmSV->forward(inputSV);
    
    //Get last hidden state after layer N
    torch::Tensor chargedHidden = charged.output.index({batchIndex, chargedSeq}).unsqueeze(1);
    torch::Tensor neutralHidden = neutral.output.index({batchIndex,  neutralSeq}).unsqueeze(1);
    torch::Tensor SVHidden = SV.output.index({batchIndex, SVSeq}).unsqueeze(1);

    //Merge them together
    torch::Tensor z = torch::cat({chargedHidden, neutralHidden, SVHidden}, 1);

    //Convolution layer
    z = convLayer->forward(z);
    z = torch::relu(z).view({-1, z.size(1)*z.size(2)});

    //Output layer
    z = outLayer->forward(z);
    z = torch::sigmoid(z);

    return z;
}
