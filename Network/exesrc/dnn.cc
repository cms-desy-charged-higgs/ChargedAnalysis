#include <torch/torch.h>

#include <random>
#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <experimental/random>

#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/dnndataset.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Utility/include/frame.h>

typedef torch::disable_if_t<false, std::unique_ptr<torch::data::StatelessDataLoader<DNNDataset, torch::data::samplers::SequentialSampler>, std::default_delete<torch::data::StatelessDataLoader<DNNDataset, torch::data::samplers::SequentialSampler>>>> DataLoader;

float Train(std::shared_ptr<DNNModel> model, std::vector<DNNDataset>& sigSets, std::vector<DNNDataset>& bkgSets,  std::vector<int>& masses, torch::Device& device, const int& batchSize, const float& lr, const bool& optimize, const std::string& outPath, const std::vector<std::string>&  bkgClasses){
    //Calculate weights to equalize pure number of events for signal and background
    std::vector<int> nBkg(bkgClasses.size(), 0);
    int nSig = 0, nBkgTotal = 0;
    
    for(DNNDataset& set : sigSets) nSig += set.size().value();
    
    for(int i = 0; i < bkgClasses.size(); ++i){
        for(DNNDataset& set : bkgSets){        
            if(set.GetClass() != i) continue;
        
            nBkg[i] += set.size().value();
            nBkgTotal += set.size().value();
        }
    }

    float wSig = (nBkgTotal+nSig)/float(nSig);
    std::vector<float> wBkg(bkgClasses.size(), 1.);
    
    for(int i = 0; i < bkgClasses.size(); ++i){
        wBkg[i] = (nBkgTotal+nSig)/float(nBkg[i]);
    }

    //Calculate number of batches
    int nBatches = nSig+nBkgTotal % batchSize == 0 ? (nSig+nBkgTotal)/batchSize -1 : std::ceil((nSig+nBkgTotal)/batchSize);

    //Get dataloader
    std::vector<DataLoader> signal;
    std::vector<DataLoader> bkg;
    DataLoader background;

    for(DNNDataset& set : sigSets){
        float ratio = set.size().value()/float(nSig);
        signal.push_back(std::move(torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(set, ratio * 1./wSig*batchSize)));
    }

    for(int i = 0; i < bkgClasses.size(); ++i){
        for(DNNDataset& set : bkgSets){
            if(set.GetClass() != i) continue;
        
            float ratio = set.size().value()/float(nBkg[i]);
            bkg.push_back(std::move(torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(set, ratio * 1./wBkg[i]*batchSize)));
        }
    }

    torch::Tensor weight =  torch::from_blob(VUtil::Append(wBkg, wSig).data(), {bkgClasses.size() + 1}).clone().to(device);
    std::cout << weight << std::endl;
    
    //Optimizer
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));
    torch::nn::NLLLoss loss(torch::nn::NLLLossOptions().weight(weight));

    //Initial best loss for later early stopping
    float bestLoss = 1e7;
    int patience = optimize ? 2 : 10;
    int notBetter = 0;

    for(int i=0; i < 10000; i++){
        //Measure time
        Utils::RunTime timer;

        int finishedBatches = 0;   
        float meanTrainLoss=0., meanTestLoss=0.;

        //Iterator returning batched data
        std::vector<torch::data::Iterator<std::vector<DNNTensor>>> sigIter;
        std::vector<torch::data::Iterator<std::vector<DNNTensor>>> bkgIter;

        for(DataLoader& sig : signal){
            sigIter.push_back(sig->begin());
        }

        for(DataLoader& b : bkg){
            bkgIter.push_back(b->begin());
        }

        //Declaration of input train/test tensors
        DNNTensor train, test;
        std::vector<int> trainMass, testMass;
        
        if(optimize and timer.Time() > 60*5) break;

        while(nBatches - finishedBatches  != 0){
            //Do fewer n of batches for hyperopt
            if(optimize and finishedBatches == 50) break;

            //Set gradients to zero
            optimizer.zero_grad();

            //Put signal + background in batch vector
            std::vector<DNNTensor> batch;
            std::vector<int> batchMass;
            
            for(int k = 0; k < sigSets.size(); k++){
                for(DNNTensor& tensor : *sigIter[k]){
                    batchMass.push_back(masses.at(k));
                    batch.push_back(tensor);
                }
            }

            for(int k = 0; k < bkgSets.size(); k++){
                for(DNNTensor& tensor : *bkgIter[k]){
                    int index = std::experimental::randint(0, (int)masses.size() -1);
                    batchMass.push_back(masses.at(index));
                    batch.push_back(tensor);
                }
            }
            
            //Shuffle batch
            int seed = int(std::time(0));
            std::srand(seed);
            std::random_shuffle (batch.begin(), batch.end());
            std::random_shuffle (batchMass.begin(), batchMass.end());

            //Set batch for testing and go to next iteration
            if(finishedBatches == 0){
                test = DNNDataset::Merge(batch);
                testMass = batchMass;

                finishedBatches++;
                continue;
            }

            //Set batch for training
            train = DNNDataset::Merge(batch);
            trainMass = batchMass;
           
            //Prediction    
            model->train();
            torch::Tensor predictionTrain = model->forward(train.input, torch::from_blob(trainMass.data(), {trainMass.size(), 1}, torch::kInt).clone().to(device));

            model->eval();
            torch::Tensor predictionTest = model->forward(test.input, torch::from_blob(testMass.data(), {testMass.size(), 1}, torch::kInt).clone().to(device));

            //Calculate loss and mean of loss of the batch
            torch::Tensor lossTrain = loss(predictionTrain, train.label);
            torch::Tensor lossTest = loss(predictionTest, test.label);

            meanTrainLoss = (meanTrainLoss*finishedBatches + lossTrain.item<float>())/(finishedBatches+1);
            meanTestLoss = (meanTestLoss*finishedBatches + lossTest.item<float>())/(finishedBatches+1);

            //Draw signal/background score during training of monitoring
            if(finishedBatches % 10 == 0){
                torch::Tensor predLab = std::get<1>(torch::max(predictionTest, 1));
                std::vector<int> predLabel(predLab.data_ptr<long>(), predLab.data_ptr<long>() + predLab.numel());
                
                train.label = train.label.contiguous();
                std::vector<int> trueLabel(train.label.data_ptr<long>(), train.label.data_ptr<long>() + train.label.numel());
                
                PUtil::DrawConfusion(trueLabel, predLabel, VUtil::Append(bkgClasses, "H^{#pm} + h"), outPath);
            }

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
            for(int k = 0; k < bkgIter.size(); k++) ++bkgIter[k];
            for(int k = 0; k < sigIter.size(); k++) ++sigIter[k];
            
            if(optimize and timer.Time() > 60*5) break;
        }

        //Early stopping        
        if(meanTestLoss > bestLoss) notBetter++;
        else{
            bestLoss = meanTestLoss;
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
    std::string channel = parser.GetValue<std::string>("channel");
    std::string cleanJet = parser.GetValue<std::string>("clean-jets");
    std::vector<std::string> sigFiles = parser.GetVector<std::string>("sig-files");
    std::vector<std::string> bkgClasses = parser.GetVector<std::string>("bkg-classes");
    std::vector<std::string> parameters = parser.GetVector<std::string>("parameters");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts");
    std::vector<int> masses = parser.GetVector<int>("masses");

    std::string optParam = parser.GetValue<std::string>("opt-param", "");
    std::unique_ptr<Frame> hyperParam;

    if(optParam != "") hyperParam = std::make_unique<Frame>(optParam);

    int batchSize = parser.GetValue<int>("batch-size", optParam != "" ? hyperParam->Get("batch-size", 0) : 2000);
    int nNodes = parser.GetValue<int>("n-nodes", optParam != ""  ? hyperParam->Get("n-nodes", 0) : 40);
    int nLayers = parser.GetValue<int>("n-layers", optParam != "" ? hyperParam->Get("n-layers", 0) : 3);
    float dropOut = parser.GetValue<float>("drop-out", optParam != "" ? hyperParam->Get("drop-out", 0) : 0.1);
    float lr = parser.GetValue<float>("lr", optParam != "" ? hyperParam->Get("lr", 0) : 1e-4);
    if(lr < 0) lr = std::pow(10, lr);

    bool optimize = parser.GetValue<bool>("optimize");

    //Check if you are on CPU or GPU
    torch::Device device(torch::kCPU);

    //Restrict number of threads to one
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    //Pytorch dataset class
    std::vector<std::shared_ptr<TFile>> files;
    std::vector<DNNDataset> sigSets;
    std::vector<DNNDataset> bkgSets;

    //Collect input data
    for(std::string fileName: sigFiles){
        std::shared_ptr<TFile> file(TFile::Open(fileName.c_str(), "READ")); 
        files.push_back(file);

        DNNDataset sigSet(file, channel, parameters, cuts, cleanJet, device, bkgClasses.size());
        sigSets.push_back(std::move(sigSet));
    }

    for(int i = 0; i < bkgClasses.size(); ++i){
        std::vector<std::string> bkgFiles = parser.GetVector<std::string>(StrUtil::Replace("@-files", "@", bkgClasses.at(i)));
          
        for(std::string fileName: bkgFiles){
            std::shared_ptr<TFile> file(TFile::Open(fileName.c_str(), "READ")); 
            files.push_back(file);

            DNNDataset bkgSet(file, channel, parameters, cuts, cleanJet, device, i);
            bkgSets.push_back(std::move(bkgSet));
        }
    }

    //Create model
    std::shared_ptr<DNNModel> model = std::make_shared<DNNModel>(parameters.size(), nNodes, nLayers, dropOut, masses.size() > 1 ? true : false, bkgClasses.size() + 1, device);
    
    if(std::filesystem::exists(outPath + "/model.pt")) torch::load(model, outPath + "/model.pt");

    model->Print();

    //Do training
    float bestLoss = Train(model, sigSets, bkgSets, masses, device, batchSize, lr, optimize, outPath, bkgClasses);
    if(optimize) std::cout << "\n" << bestLoss << std::endl;
}
