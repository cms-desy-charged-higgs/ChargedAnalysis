#include <experimental/random>
#include <algorithm>
#include <vector>
#include <string>

#include <torch/torch.h>

#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Network/include/htagdataset.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/utils.h>

void Train(std::shared_ptr<HTagger> tagger, HTagDataset& sigSet, HTagDataset& bkgSet, torch::Device& device, int batchSize, bool trainEven){
    //Create directories
    std::string scorePath = std::string(std::getenv("CHDIR")) + "/DNN/Model/" + (trainEven ? "Even/" : "Odd/");
    std::system((std::string("mkdir -p ") + scorePath).c_str());
    
    //Set batch size
    int nSig = sigSet.size().value(); int nBkg = bkgSet.size().value();
    int nBatches = nSig+nBkg % batchSize == 0 ? (nSig+nBkg)/batchSize -1 : std::ceil((nSig+nBkg)/batchSize);
    float forTest = 0.05;

    float wSig = (nSig+nBkg)/float(nSig);
    float wBkg = (nSig+nBkg)/float(nBkg);

    //Optimizer
    float lr = 0.001;
    torch::optim::Adam optimizer(tagger->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));

    //Create dataloader for sig and bkg
    auto sigLoader = torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(sigSet, 1./wSig*batchSize);
    auto bkgLoader = torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(bkgSet, 1./wBkg*batchSize);

    //Initial best loss for later early stopping
    float bestLoss = 1e7;

    for(int i=0; i < 10000; i++){
        //Measure time
        Utils::RunTime timer;

        //Iterator returning batched data
        torch::data::Iterator<std::vector<HTensor>> signal = sigLoader->begin();
        torch::data::Iterator<std::vector<HTensor>> background = bkgLoader->begin();
    
        float meanTrainLoss, meanTestLoss;

        for(int j=0; j <= nBatches; j++){
            //Set gradients to zero
            tagger->zero_grad();
            optimizer.zero_grad();

            //Put signal + background in one vector and split by even or odd numbered event
            std::vector<HTensor> batch;

            for(HTensor& tensor: *signal){
                if(tensor.isEven.item<bool>() == trainEven) batch.push_back(tensor);
            }

            for(HTensor& tensor: *background){
                if(tensor.isEven.item<bool>() == trainEven) batch.push_back(tensor);
            }

            //Shuffle batch
            std::random_shuffle (batch.begin(), batch.end());

            //Split to train and test data
            std::vector<HTensor> trainData = {batch.begin(), batch.end()-forTest*batchSize};
            std::vector<HTensor> testData = {batch.end()+1-forTest*batchSize, batch.end()};

            //Do padding
            HTensor train = HTagDataset::PadAndMerge(trainData);
            HTensor test = HTagDataset::PadAndMerge(testData);

            //Weight
            std::vector<float> weightsTrain;
            std::vector<float> weightsTest;
            
            for(int i = 0; i < train.label.size(0); i++){
                weightsTrain.push_back(train.label[i].item<float>() == 1 ? wSig : wBkg);
            }

            for(int i = 0; i < test.label.size(0); i++){
                weightsTest.push_back(test.label[i].item<float>() == 1 ? wSig : wBkg);
            }

            torch::Tensor weightTrain = torch::from_blob(weightsTrain.data(), {weightsTrain.size()}).clone().to(device);
            torch::Tensor weightTest = torch::from_blob(weightsTest.data(), {weightsTest.size()}).clone().to(device);

            //Prediction
            torch::Tensor predictionTrain = tagger->forward(train.charged, train.neutral, train.SV);
            torch::Tensor lossTrain = torch::binary_cross_entropy(predictionTrain, train.label, weightTrain);

            torch::Tensor predictionTest = tagger->forward(test.charged, test.neutral, test.SV);
            torch::Tensor lossTest = torch::binary_cross_entropy(predictionTest, test.label, weightTest);

            if(j % 20 == 0) Utils::DrawScore(predictionTrain, train.label, scorePath);

            meanTrainLoss = (meanTrainLoss*j + lossTrain.item<float>())/(j+1);
            meanTestLoss = (meanTestLoss*j + lossTest.item<float>())/(j+1);

            //Back propagation
            lossTrain.backward();
            optimizer.step();

            //Increment iterators
            ++signal;
            ++background;

            //Progess bar
            std::string barString = "Epoch: " + std::to_string(i+1) + 
                                    " | Batch: " + std::to_string(j) + "/" + std::to_string(nBatches) + 
                                    " | Batch size: " + std::to_string(train.charged.size(0)) +
                                    " | Mean Loss: " + std::to_string(meanTrainLoss).substr(0, 5) +
                                    "/" + std::to_string(meanTestLoss).substr(0, 5) 
                                    + " | Time: " + std::to_string(timer.Time()).substr(0, 4) + " s";

            Utils::ProgressBar(float(j)/nBatches*100, barString);
        }

        //Early stopping
        if(meanTrainLoss > bestLoss) break;
        else bestLoss = meanTrainLoss;

        //Save model
        tagger->to(torch::kCPU);
        torch::save(tagger, scorePath + "/htagger.pt");
        std::cout << "Model was saved: " + scorePath + "/htagger.pt" << std::endl;
        tagger->to(device);
    }
}

int main(int argc, char** argv){
    //Parser arguments
    Parser parser(argc, argv);
    bool optimize = parser.GetValue<bool>("optimize");  
    bool trainEven = parser.GetValue<bool>("even"); 
    int batchSize = 2*parser.GetValue<int>("batch-size");

    //File names and channels for training
    std::vector<std::string> sigFiles = {
                            std::string(std::getenv("CHDIR")) + "/Skim/HPlusAndH_ToWHH_ToL4B_200_100/merged/HPlusAndH_ToWHH_ToL4B_200_100.root",
                            std::string(std::getenv("CHDIR")) + "/Skim/HPlusAndH_ToWHH_ToL4B_300_100/merged/HPlusAndH_ToWHH_ToL4B_300_100.root",
                            std::string(std::getenv("CHDIR")) + "/Skim/HPlusAndH_ToWHH_ToL4B_400_100/merged/HPlusAndH_ToWHH_ToL4B_400_100.root",
                            std::string(std::getenv("CHDIR")) + "/Skim/HPlusAndH_ToWHH_ToL4B_500_100/merged/HPlusAndH_ToWHH_ToL4B_500_100.root",
                            std::string(std::getenv("CHDIR")) + "/Skim/HPlusAndH_ToWHH_ToL4B_600_100/merged/HPlusAndH_ToWHH_ToL4B_600_100.root",
    };

    std::vector<std::string> bkgFiles = {
                std::string(std::getenv("CHDIR")) + "/Skim/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8/merged/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8.root",                   
    };

    
    std::vector<std::string> channels = {"mu2j1fj", "e2j1fj", "mu2fj", "e2fj"};

    //Check if you are on CPU or GPU
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);

    //Pytorch dataset class
    HTagDataset sigSet = HTagDataset(sigFiles, channels, 0, device, true);
    HTagDataset bkgSet = HTagDataset(bkgFiles, channels, 0, device, false);

    //Model for Htaggers
    std::shared_ptr<HTagger> tagger = std::make_shared<HTagger>(7, 140, 1, 130, 57, 0.06);
    tagger->to(device);
    tagger->Print();

    Train(tagger, sigSet, bkgSet, device, batchSize, trainEven);

    return 0;
}
