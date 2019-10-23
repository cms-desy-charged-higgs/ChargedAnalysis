#include <torch/torch.h>

#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>

torch::Tensor Merge(std::vector<std::vector<float>>& jetParam, const int& pos, const int& padValue){
    int nMax = 0;

    std::vector<torch::Tensor> jetVec;
    
    for(unsigned int i=0+pos; i < jetParam.size(); i++){
        if((i-pos)%3==0){
            int size =  Utils::Ratio(jetParam[i].size(), 7);
            nMax = nMax < size ? size: nMax;
        }
    }

    for(unsigned int i=0+pos; i < jetParam.size(); i++){
        if((i-pos)%3==0){
            int size =  Utils::Ratio(jetParam[i].size(), 7);
            torch::Tensor t = torch::from_blob(jetParam[i].data(), {1, size, 7});
            if (size != nMax) t = torch::constant_pad_nd(t, {0,0, 0, nMax-size}, padValue);
            jetVec.push_back(t);
        }
    }

    torch::Tensor tensor = torch::cat(jetVec, 0);

    return tensor;
}

torch::Tensor BCE(torch::Tensor pred, torch::Tensor trueV){
    torch::Tensor loss = torch::where(trueV==1, -torch::log(pred), -torch::log(1-pred));

    return loss.sum()/pred.size(0);
}

int main(){
    //File names and channels for training
    std::map<std::string, int> fileNames = {
                            {std::string(std::getenv("CHDIR")) + "/Skim/HPlusAndH_ToWHH_ToL4B_200_100/merged/HPlusAndH_ToWHH_ToL4B_200_100.root", 1},

                            {std::string(std::getenv("CHDIR")) + "/Skim/TTJets_SingleLeptFromT_TuneCP5_13TeV-madgraphMLM-pythia8/merged/TTJets_SingleLeptFromT_TuneCP5_13TeV-madgraphMLM-pythia8.root", 0}                    
    };
    
    std::vector<std::string> channels = {"mu2j1f", "e2j1f", "mu2f", "e2f"};


    //Extract data from TTrees with Treereader class
    std::vector<std::vector<float>> jetParam;
    std::vector<float> tagValue;
    std::vector<std::string> process;
    int nEvents = 24000;

    for(const std::pair<std::string, int>& fileName: fileNames){
        for(std::string& channel: channels){
            std::cout << "Readout data from file " << Utils::SplitString(fileName.first, "/").back() << " and channel " << channel << std::endl;

            TreeReader reader("", {"*_fj1"}, {}, {}, "", channel, "Tensor");
            reader.EventLoop(fileName.first, 0, nEvents, &jetParam);

            for(unsigned int i=0; i < nEvents; i++){
                tagValue.push_back(fileName.second);
            }
        }
    }

    //Check if you are on CPU or GPU
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);

    //Convert data to torch tensor and shuffle them
    torch::Tensor randIndex = torch::randperm((int)tagValue.size(), torch::TensorOptions().dtype(torch::kLong)).to(device);

    torch::Tensor target = torch::from_blob(tagValue.data(), {(int)tagValue.size(), 1}).to(device).index_select(0, randIndex);

    torch::Tensor chargedTensor = Merge(jetParam, 0, 0).to(device).index_select(0, randIndex);
    torch::Tensor neutralTensor = Merge(jetParam, 1, 0).to(device).index_select(0, randIndex);
    torch::Tensor SVTensor = Merge(jetParam, 2, 0).to(device).index_select(0, randIndex);

    //Instance of htagger class
    std::shared_ptr<HTagger> tagger = std::make_shared<HTagger>(7, 50, 3, 10, 2, 0.2);
    tagger->Print();
    tagger->to(device);

    //Variables for number of batches and and batch size
    int nVali = 1000;
    int nTrain = tagValue.size() - nVali;

    int batchSize = 2048;
    int nEpochs = 1000;
    int nBatches = std::ceil(nTrain/batchSize);

    torch::optim::Adam optimizer(tagger->parameters(), torch::optim::AdamOptions(1e-3).weight_decay(1e-4));

    //Early stopping variables
    float minLoss = 1e6;
    int nPatience = 0;

    for(int i=0; i<nEpochs; i++){
        int epochProcess = 0;

        //Validation data
        torch::Tensor chargedVali = chargedTensor.narrow(0, nTrain, nVali);  
        torch::Tensor neutralVali = neutralTensor.narrow(0, nTrain, nVali); 
        torch::Tensor SVVali = SVTensor.narrow(0, nTrain, nVali); 
        torch::Tensor labelVali = target.narrow(0, nTrain, nVali);

        float lossValidation = 1;

        for(int j=0; j <= nBatches; j++){
            //Set gradients to zero
            tagger->zero_grad();
            optimizer.zero_grad();

            //Training data
            torch::Tensor chargedTrain = chargedTensor.narrow(0, batchSize*j, j!=nBatches  ? batchSize : nTrain - j*batchSize);  
            torch::Tensor neutralTrain = neutralTensor.narrow(0, batchSize*j, j!=nBatches  ? batchSize : nTrain - j*batchSize);  
            torch::Tensor SVTrain = SVTensor.narrow(0, batchSize*j, j!=nBatches  ? batchSize : nTrain - j*batchSize);  
            torch::Tensor labelTrain = target.narrow(0, batchSize*j, j!=nBatches  ? batchSize : nTrain - j*batchSize);

            //Get prediction of training and test sample
            torch::Tensor predictionTrain = tagger->forward(chargedTrain, neutralTrain, SVTrain, true);
            torch::Tensor lossTrain = BCE(predictionTrain.squeeze(1), labelTrain.squeeze(1));

            //Back propagation
            lossTrain.backward();
            optimizer.step();

            torch::Tensor predictionVali = tagger->forward(chargedVali, neutralVali, SVVali, false);
            torch::Tensor lossVali = BCE(predictionVali.squeeze(1), labelVali.squeeze(1));

            lossValidation = lossVali.item<float>();

            //Progess bar
            std::string barString = "Epoch: " + std::to_string(i+1) + "/" + std::to_string(nEpochs) + 
                                    " | Batch: " + std::to_string(j) + "/" + std::to_string(nBatches) + 
                                    " | Loss: " + std::to_string(lossTrain.item<float>()).substr(0, 5) + "/" + std::to_string(lossVali.item<float>()).substr(0, 5) + " | Score Extrema: " + std::to_string(predictionTrain.min().item<float>()).substr(0, 5) + "/" + std::to_string(predictionTrain.max().item<float>()).substr(0, 5);

            Utils::ProgressBar(float(j)/nBatches*100, barString);
        }
        
        //Check for early stopping
        if(minLoss > lossValidation){
            minLoss = lossValidation;
            nPatience=0;
        } 

        else nPatience++;

        if(nPatience == 5){
            std::cout << "Training will be closed due to early stopping" << std::endl;
            break;
        }
    }

    //Save model
    std::string outName = std::string(std::getenv("CHDIR")) + "/DNN/Model/htagger.pt";
    
    tagger->to(torch::kCPU);
    torch::save(tagger, outName);
    std::cout << "Model was saved: " << outName << std::endl;
} 
