#include <torch/torch.h>

#include <random>
#include <map>
#include <vector>
#include <string>
#include <experimental/random>

#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/dnndataset.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/frame.h>

typedef torch::disable_if_t<false, std::unique_ptr<torch::data::StatelessDataLoader<DNNDataset, torch::data::samplers::SequentialSampler>, std::default_delete<torch::data::StatelessDataLoader<DNNDataset, torch::data::samplers::SequentialSampler>>>> DataLoader;

float Train(std::shared_ptr<DNNModel> model, std::vector<DNNDataset>& sigSets, DNNDataset& bkgSet,  std::vector<int>& masses, torch::Device& device, const int& batchSize, const float& lr, const bool& optimize, const std::string& outPath){
    //Create directories
    std::system((std::string("mkdir -p ") + outPath).c_str());

    //Calculate weights to equalize pure number of events for signal and background
    int nBkg = bkgSet.size().value();
    int nSig = 0;
    for(DNNDataset& set : sigSets) nSig += set.size().value();

    float wSig = (nSig+nBkg)/float(nSig);
    float wBkg = (nSig+nBkg)/float(nBkg);

    //Calculate number of batches
    int nBatches = nSig+nBkg % batchSize == 0 ? (nSig+nBkg)/batchSize -1 : std::ceil((nSig+nBkg)/batchSize);

    //Get dataloader
    std::vector<DataLoader> signal;
    DataLoader background;

    for(DNNDataset& set : sigSets){
        float ratio = set.size().value()/float(nSig);
        signal.push_back(std::move(torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(set, ratio * 1./wSig*batchSize)));
    }

    background = torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(bkgSet, (1./wBkg*batchSize));

    //Recalculate class weights after equalizing all signal mass hypotheses
    int nSigWeighted = sigSets.size()*nSig;
    wBkg = (nSigWeighted + nBkg)/float(nBkg);

    std::vector<float> wSigWeighted;
    for(DNNDataset& set : sigSets) wSigWeighted.push_back((nSigWeighted + nBkg)/float(nSigWeighted) * nSig/float(set.size().value()));

    //Optimizer
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));

    //Initial best loss for later early stopping
    float bestLoss = 1e7;
    int patience = optimize ? 2 : 20;
    int notBetter = 0;

    for(int i=0; i < 10000; i++){
        //Measure time
        Utils::RunTime timer;

        int finishedBatches = 0;   
        float meanTrainLoss=0., meanTestLoss=0.;

        //Iterator returning batched data
        std::vector<torch::data::Iterator<std::vector<DNNTensor>>> sigIter;
        torch::data::Iterator<std::vector<DNNTensor>> bkgIter = background->begin();

        for(DataLoader& sig : signal){
            sigIter.push_back(sig->begin());
        }

        //Declaration of input train/test tensors
        DNNTensor train, test;
        std::vector<float> trainWeight, testWeight;
        std::vector<int> trainMass, testMass;

        while(nBatches - finishedBatches  != 0){
            //Do fewer n of batches for hyperopt
            if(optimize and finishedBatches == 50) break;

            //Set gradients to zero
            optimizer.zero_grad();

            //Put signal + background in batch vector
            std::vector<DNNTensor> batch;
            std::vector<int> batchMass;
            std::vector<float> weights;

            for(int k = 0; k < sigSets.size(); k++){
                for(DNNTensor& tensor : *sigIter[k]){
                    batchMass.push_back(masses.at(k));
                    batch.push_back(tensor);
                    weights.push_back(wSigWeighted[k]);
                }
            }

            for(DNNTensor& tensor : *bkgIter){
                int index = std::experimental::randint(0, (int)masses.size() -1);
                batchMass.push_back(masses.at(index));
                batch.push_back(tensor);
                weights.push_back(wBkg);
            }

            //Shuffle batch
            int seed = int(std::time(0));

            std::srand(seed);
            std::random_shuffle (batch.begin(), batch.end());

            std::srand(seed);
            std::random_shuffle (batchMass.begin(), batchMass.end());

            std::srand(seed);
            std::random_shuffle (weights.begin(), weights.end());

            //Set batch for testing and go to next iteration
            if(finishedBatches == 0){
                test = DNNDataset::Merge(batch);
                testMass = batchMass;
                testWeight = weights;
    
                finishedBatches++;
                continue;
            }

            //Set batch for training
            train = DNNDataset::Merge(batch);
            trainMass = batchMass;
            trainWeight = weights;
   
            //Prediction    
            model->train();
            torch::Tensor predictionTrain = model->forward(train.input, torch::from_blob(trainMass.data(), {trainMass.size(), 1}, torch::kInt).clone().to(device));

            model->eval();
            torch::Tensor predictionTest = model->forward(test.input, torch::from_blob(testMass.data(), {testMass.size(), 1}, torch::kInt).clone().to(device));

            //Calculate loss and mean of loss of the batch
            torch::Tensor weightTrain = torch::from_blob(trainWeight.data(), {trainWeight.size()}).clone().to(device);
            torch::Tensor lossTrain = torch::nn::functional::binary_cross_entropy(predictionTrain, train.label, torch::nn::BCELossOptions().reduction(torch::kMean).weight(weightTrain));

            torch::Tensor weightTest = torch::from_blob(testWeight.data(), {testWeight.size()}).clone().to(device);
            torch::Tensor lossTest = torch::nn::functional::binary_cross_entropy(predictionTest, test.label, torch::nn::BCELossOptions().reduction(torch::kMean).weight(weightTest));

            meanTrainLoss = (meanTrainLoss*finishedBatches + lossTrain.item<float>())/(finishedBatches+1);
            meanTestLoss = (meanTestLoss*finishedBatches + lossTest.item<float>())/(finishedBatches+1);

            //Draw signal/background score during training of monitoring
            if(finishedBatches % 10 == 0) Utils::DrawScore(predictionTest, test.label, outPath);

            //Back propagation
            lossTrain.backward();
            optimizer.step();

            //Progess bar
            std::string barString = StrUtil::Merge<4>("Epoch: ", i+1,
                                                   " | Batch: ", finishedBatches, "/", nBatches - 1, 
                                                   " | Mean Loss: ", meanTrainLoss, "/", meanTestLoss,
                                                   " | Overtrain count: ", notBetter, "/", patience, 
                                                   " | Time: ", timer.Time(), " s"
                                    );

            finishedBatches++;

            Utils::ProgressBar(float(finishedBatches)/nBatches*100, barString);

            //Increment iterators
            ++bkgIter;
            for(int k = 0; k < sigIter.size(); k++) ++sigIter[k];
        }

        //Early stopping        
        if(meanTrainLoss > bestLoss or std::abs(meanTrainLoss-meanTestLoss)/meanTrainLoss > 0.1) notBetter++;
        else{
            bestLoss = meanTrainLoss;
            notBetter = 0;

            //Save model
            if(!optimize){
                model->to(torch::kCPU);
                torch::save(model, outPath + "/model.pt");
                std::cout << "Model was saved: " + outPath + "/model.pt" << std::endl;
                model->to(device);
            }
        }

        if(notBetter == patience) break;
    }

    return bestLoss;
}

int main(int argc, char** argv){
    //Parser arguments
    Parser parser(argc, argv);

    std::string outPath = parser.GetValue<std::string>("out-path"); 
    std::vector<std::string> sigFiles = parser.GetVector<std::string>("sig-files");
    std::vector<std::string> bkgFiles = parser.GetVector<std::string>("bkg-files");

    std::string optParam = parser.GetValue<std::string>("opt-param", "");
    std::unique_ptr<Frame> hyperParam;

    if(optParam != "") hyperParam = std::make_unique<Frame>(optParam);

    int batchSize = parser.GetValue<int>("batch-size", optParam != "" ? hyperParam->Get("batchSize", 0) : 2000);
    int nNodes = parser.GetValue<int>("n-nodes", optParam != ""  ? hyperParam->Get("nNodes", 0) : 40);
    int nLayers = parser.GetValue<int>("n-layers", optParam != "" ? hyperParam->Get("nLayers", 0) : 3);
    float dropOut = parser.GetValue<float>("drop-out", optParam != "" ? hyperParam->Get("dropOut", 0) : 0.1);
    float lr = parser.GetValue<float>("lr", optParam != "" ? hyperParam->Get("lr", 0) : 1e-4);

    bool optimize = parser.GetValue<bool>("optimize");

    //Check if you are on CPU or GPU
    torch::Device device(torch::kCPU);

    //Restrict number of threads to one
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    //Pytorch dataset class
    std::vector<DNNDataset> sigSets;
    std::vector<DNNDataset> bkgSets;
    std::vector<int> masses;

    //Collect input data in csv format and create dataset classes
    for(std::string file: sigFiles){
        int mass = std::stoi(file.substr(Utils::Find<std::string>(file, "HPlus")+5, 3));

        DNNDataset sigSet({file}, device, true);
        sigSets.push_back(std::move(sigSet));
        masses.push_back(mass);
    }

    DNNDataset bkgSet(bkgFiles, device, false);

    //Get number of parameters
    std::unique_ptr<Frame> frame = std::make_unique<Frame>(sigFiles[0]);
    int nParameters = frame->GetNLabels();

    //Create model
    std::shared_ptr<DNNModel> model = std::make_shared<DNNModel>(nParameters, nNodes, nLayers, dropOut, masses.size() > 1 ? true : false, device);
    model->Print();

    //Do training
    float bestLoss = Train(model, sigSets, bkgSet, masses, device, batchSize, lr, optimize, outPath);
    if(optimize) std::cout << "Best loss value: " << bestLoss << std::endl;
}
