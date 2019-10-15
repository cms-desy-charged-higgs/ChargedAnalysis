#include <ChargedAnalysis/Network/include/htagger.h>

HTagger::HTagger(){}

HTagger::HTagger(const int& nFeat, const int& nHidden, const int& nLSTM, const int& nConvFilter, const int& kernelSize){
    lstmCharged = register_module("LSTM charged input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true)));
    lstmNeutral = register_module("LSTM neutral input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true)));
    lstmSV = register_module("LSTM SV input", torch::nn::LSTM(torch::nn::LSTMOptions(nFeat, nHidden).layers(nLSTM).batch_first(true)));

    convLayer = register_module("Conv layer 1", torch::nn::Conv1d(3, nConvFilter, kernelSize));
    int kappa = nHidden + 2 - kernelSize -1;
    outLayer = register_module("Output layer", torch::nn::Linear(nConvFilter*kappa, 1));
}

torch::Tensor HTagger::forward(torch::Tensor inputCharged, torch::Tensor inputNeutral, torch::Tensor inputSV){

    //LSTM layer
    torch::nn::RNNOutput charged = lstmCharged->forward(inputCharged);
    torch::nn::RNNOutput neutral = lstmCharged->forward(inputNeutral);
    torch::nn::RNNOutput SV = lstmCharged->forward(inputSV);

    //Get last hidden state after layer N
    torch::Tensor chargedHidden = charged.state.narrow(0, 1, 1).squeeze(0).narrow(0, 1, 1);
    torch::Tensor neutralHidden = neutral.state.narrow(0, 1, 1).squeeze(0).narrow(0, 1, 1);
    torch::Tensor SVHidden = SV.state.narrow(0, 1, 1).squeeze(0).narrow(0, 1, 1);

    //Merge them together
    torch::Tensor z = torch::cat({chargedHidden, neutralHidden, SVHidden}, 0).permute({1, 0, 2});
    
    z = convLayer->forward(z);
    z = torch::relu(z).view({-1, z.size(1)*z.size(2)});
    z = outLayer->forward(z);
    z = torch::sigmoid(z);
        
    return z;
}
