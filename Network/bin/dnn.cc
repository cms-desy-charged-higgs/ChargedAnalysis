#include <torch/torch.h>

#include <random>

#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/dnndataset.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/frame.h>

typedef torch::disable_if_t<false, std::unique_ptr<torch::data::StatelessDataLoader<DNNDataset, torch::data::samplers::SequentialSampler>, std::default_delete<torch::data::StatelessDataLoader<DNNDataset, torch::data::samplers::SequentialSampler>>>> DataLoader;

void Train(std::shared_ptr<DNNModel> model, std::vector<DNNDataset>& sigSets, std::vector<DNNDataset>& bkgSets, torch::Device& device, const int& batchSize, const std::string& outPath){
    //Create directories
    std::system((std::string("mkdir -p ") + outPath).c_str());

    std::vector<DataLoader> signal;
    std::vector<DataLoader> background;
    int nBatches = 0;
    std::vector<float> wSig; std::vector<float> wBkg;
    
    for(int i = 0; i < sigSets.size(); i++){
        int nSig = sigSets[i].size().value();
        int nBkg = bkgSets[i].size().value();

        nBatches += nSig+nBkg % batchSize == 0 ? (nSig+nBkg)/batchSize : (nSig+nBkg)/batchSize + 1;

        wSig.push_back((nSig+nBkg)/float(nSig));
        wBkg.push_back((nSig+nBkg)/float(nBkg));

        //Create dataloader for sig and bkg
        signal.push_back(torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(sigSets[i], 1./wSig.back()*batchSize));
        background.push_back(torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(bkgSets[i], 1./wBkg.back()*batchSize));
    }

    //Optimizer
    float forTest = 0.05;

    float lr = 0.0001;
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));

    //Initial best loss for later early stopping
    float bestLoss = 1e7;

    for(int i=0; i < 10000; i++){
        //Measure time
        Utils::RunTime timer;

        int finishedBatches = 0;

        //Iterator returning batched data
        std::vector<torch::data::Iterator<std::vector<DNNTensor>>> sigIter;
        std::vector<torch::data::Iterator<std::vector<DNNTensor>>> bkgIter;
    
        for(int k=0; k < background.size(); k++){
            sigIter.push_back(signal[k]->begin());
            bkgIter.push_back(background[k]->begin());
        }

        float meanTrainLoss=0., meanTestLoss=0.;

        std::default_random_engine generator;
        std::uniform_int_distribution<int> distribution(0, (int)signal.size()-1);

        while(nBatches - finishedBatches != 0){
            int index = distribution(generator);

            if(bkgIter[index] == background[index]->end()){
                continue;
            }
    
            //Set gradients to zero
            model->zero_grad();
            optimizer.zero_grad();

            //Put signal + background in one vector and split by even or odd numbered event
            std::vector<DNNTensor> batch;

            for(DNNTensor& tensor: *sigIter[index]){
                batch.push_back(tensor);
            }

            for(DNNTensor& tensor: *bkgIter[index]){
                batch.push_back(tensor);
            }

            //Shuffle batch
            std::random_shuffle (batch.begin(), batch.end());

            //Shuffle batch
            int batchSize = batch.size();

            //Split to train and test data
            std::vector<DNNTensor> trainData = {batch.begin(), batch.end()-forTest*batchSize};
            std::vector<DNNTensor> testData = {batch.end()+1-forTest*batchSize, batch.end()};

            //Do padding
            DNNTensor train = DNNDataset::Merge(trainData);
            DNNTensor test = DNNDataset::Merge(testData);

            //Weight
            std::vector<float> weightsTrain;
            std::vector<float> weightsTest;
            
            for(int i = 0; i < train.label.size(0); i++){
                weightsTrain.push_back(train.label[i].item<float>() == 1 ? wSig[index] : wBkg[index]);
            }

            for(int i = 0; i < test.label.size(0); i++){
                weightsTest.push_back(test.label[i].item<float>() == 1 ? wSig[index] : wBkg[index]);
            }

            torch::Tensor weightTrain = torch::from_blob(weightsTrain.data(), {weightsTrain.size()}).clone().to(device);
            torch::Tensor weightTest = torch::from_blob(weightsTest.data(), {weightsTest.size()}).clone().to(device);

            //Prediction
            torch::Tensor predictionTrain = model->forward(train.input);
            torch::Tensor lossTrain = torch::binary_cross_entropy(predictionTrain, train.label, weightTrain);

            torch::Tensor predictionTest = model->forward(test.input);
            torch::Tensor lossTest = torch::binary_cross_entropy(predictionTest, test.label, weightTest);

            if(finishedBatches % 10 == 0) Utils::DrawScore(predictionTrain, train.label, outPath);

            meanTrainLoss = (meanTrainLoss*finishedBatches + lossTrain.item<float>())/(finishedBatches+1);
            meanTestLoss = (meanTestLoss*finishedBatches + lossTest.item<float>())/(finishedBatches+1);

            //Back propagation
            lossTrain.backward();
            optimizer.step();

            //Progess bar
            std::string barString = "Epoch: " + std::to_string(i+1) + 
                                    " | Batch: " + std::to_string(finishedBatches+1) + "/" + std::to_string(nBatches) + 
                                    " | Batch size: " + std::to_string(train.input.size(0)) +
                                    " | Mean Loss: " + std::to_string(meanTrainLoss).substr(0, 5) +
                                    "/" + std::to_string(meanTestLoss).substr(0, 5) 
                                    + " | Time: " + std::to_string(timer.Time()).substr(0, 4) + " s";

            finishedBatches++;

            Utils::ProgressBar(float(finishedBatches)/nBatches*100, barString);

            //Increment iterators
            ++sigIter[index];
            ++bkgIter[index];
        }

        //Early stopping
        if(meanTrainLoss > bestLoss) break;
        else bestLoss = meanTrainLoss;

        //Save model
        model->to(torch::kCPU);
        torch::save(model, outPath + "/model.pt");
        std::cout << "Model was saved: " + outPath + "/model.pt" << std::endl;
        model->to(device);
    }
}

int main(int argc, char** argv){
    //Parser arguments
    Parser parser(argc, argv);
    std::string outPath = parser.GetValue<std::string>("out-path"); 
    int batchSize = 2*parser.GetValue<int>("batch-size");
    std::vector<std::string> sigFiles = parser.GetVector<std::string>("sig-files");
    std::vector<std::string> bkgFiles = parser.GetVector<std::string>("bkg-files");

    //Check if you are on CPU or GPU
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);

    //Pytorch dataset class
    std::vector<DNNDataset> sigSets;
    std::vector<DNNDataset> bkgSets;

    for(std::string file: sigFiles){
        std::cout << file.substr(Utils::Find<std::string>(file, "HPlus")+5, 3) << std::endl;
        int mass = std::stoi(file.substr(Utils::Find<std::string>(file, "HPlus")+5, 3));

        DNNDataset sigSet({file}, device, true);
        sigSet.SetMass(mass);
        sigSets.push_back(sigSet);

        DNNDataset bkgSet(bkgFiles, device, false);
        bkgSet.SetMass(mass);
        bkgSets.push_back(bkgSet);
    }

    Frame* frame = new Frame(sigFiles[0]);
    int nParameters = frame->GetNLabels();
    delete frame;

    std::shared_ptr<DNNModel> model = std::make_shared<DNNModel>(nParameters, 2*nParameters, 2, 0.1, device);
    model->Print();

    Train(model, sigSets, bkgSets, device, int(batchSize), outPath);
}
