#include <experimental/random>
#include <algorithm>
#include <vector>
#include <string>

#include <torch/torch.h>

#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Network/include/htagdataset.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stopwatch.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

typedef torch::disable_if_t<false, std::unique_ptr<torch::data::StatelessDataLoader<HTagDataset, torch::data::samplers::SequentialSampler>, std::default_delete<torch::data::StatelessDataLoader<HTagDataset, torch::data::samplers::SequentialSampler>>>> DataLoader;

float Train(std::string& outDir, std::shared_ptr<HTagger>& tagger, std::vector<HTagDataset>& sigSets, std::vector<HTagDataset>& bkgSets, torch::Device& device, int& batchSize, float& lr, bool& optimize){    
    //Set batch size
    int nSig = std::accumulate(sigSets.begin(), sigSets.end(), 0, [&](int i, HTagDataset set){return i+set.size().value();}); 
    int nBkg = std::accumulate(bkgSets.begin(), bkgSets.end(), 0, [&](int i, HTagDataset set){return i+set.size().value();}); 
    int nBatches = nSig+nBkg % batchSize == 0 ? (nSig+nBkg)/batchSize -1 : std::ceil((nSig+nBkg)/batchSize);

    float wSig = (nSig+nBkg)/float(nSig);
    float wBkg = (nSig+nBkg)/float(nBkg);

    //Optimizer
    torch::optim::Adam optimizer(tagger->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));

    //Create dataloader for sig and bkg
    std::vector<DataLoader> signal, background; 

    for(HTagDataset set : sigSets){
        float ratio = set.size().value()/float(nSig);
        int nEff = ratio * 1./wSig*batchSize;

        signal.push_back(std::move(torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(set, nEff)));
    }

    for(HTagDataset set : bkgSets){
        float ratio = set.size().value()/float(nBkg);
        background.push_back(std::move(torch::data::make_data_loader<torch::data::samplers::SequentialSampler>(set, ratio * 1./wBkg*batchSize)));
    }

    //Initial best loss for later early stopping
    float bestLoss = 1e7;
    //Measure time
    StopWatch timer; 
    timer.Start();

    for(int i=0; i < 10000; i++){
        //Iterator returning batched data
        std::vector<torch::data::Iterator<std::vector<HTensor>>> sigIter;
        std::vector<torch::data::Iterator<std::vector<HTensor>>> bkgIter;

        for(DataLoader& sig : signal){
            sigIter.push_back(sig->begin());
        }

        for(DataLoader& bkg : background){
            bkgIter.push_back(bkg->begin());
        }
    
        int finishedBatches = 0;   
        float meanTrainLoss=0., meanTestLoss=0.;
        HTensor train, test;
        std::vector<float> trainWeight, testWeight;

        if(optimize and timer.GetTime() > 60*10) break;

        while(nBatches - finishedBatches  != 0){
            //Do fewer n of batches for hyperopt
            if(optimize and finishedBatches == 5) break;

            //Set gradients to zero
            tagger->zero_grad();
            optimizer.zero_grad();

            //Put signal + background in one vector and split by even or odd numbered event
            std::vector<HTensor> batch;
            std::vector<float> weights;

            for(int k = 0; k < sigSets.size(); k++){
                for(HTensor& tensor : *sigIter[k]){
                    batch.push_back(tensor);
                    weights.push_back(wSig);     
                }
            }
    
            for(int k = 0; k < bkgSets.size(); k++){
                for(HTensor& tensor : *bkgIter[k]){
                    batch.push_back(tensor);
                    weights.push_back(wBkg);     
                }
            }

            //Shuffle batch
            int seed = int(std::time(0));

            std::srand(seed);
            std::random_shuffle (batch.begin(), batch.end());

            std::srand(seed);
            std::random_shuffle (weights.begin(), weights.end());

            //Set batch for testing and go to next iteration
            if(finishedBatches == 0){
                test = HTagDataset::PadAndMerge(batch);
                testWeight = weights;
    
                finishedBatches++;
                continue;
            }

            //Set batch for training
            train = HTagDataset::PadAndMerge(batch);
            trainWeight = weights;

            //Prediction
            torch::Tensor weightTrain = torch::from_blob(trainWeight.data(), {trainWeight.size()}).clone().to(device);
            torch::Tensor predictionTrain = tagger->forward(train.charged, train.neutral, train.SV);
            torch::Tensor lossTrain = torch::binary_cross_entropy(predictionTrain, train.label, weightTrain);

            torch::Tensor weightTest = torch::from_blob(testWeight.data(), {testWeight.size()}).clone().to(device);
            torch::Tensor predictionTest = tagger->forward(test.charged, test.neutral, test.SV);
            torch::Tensor lossTest = torch::binary_cross_entropy(predictionTest, test.label, weightTest);

           // if(finishedBatches % 20 == 0) Utils::DrawScore(predictionTrain, train.label, outDir);

            meanTrainLoss = (meanTrainLoss*finishedBatches + lossTrain.item<float>())/(finishedBatches+1);
            meanTestLoss = (meanTestLoss*finishedBatches + lossTest.item<float>())/(finishedBatches+1);

            //Back propagation
            lossTrain.backward();
            optimizer.step();

            //Increment iterators
            for(int k = 0; k < bkgIter.size(); k++) ++bkgIter[k];
            for(int k = 0; k < sigIter.size(); k++) ++sigIter[k];

            //Progess bar
            std::string barString = StrUtil::Merge<4>("Epoch: ", i+1,
                                                   " | Batch: ", finishedBatches, "/", nBatches - 1, 
                                                   " | Mean Loss: ", meanTrainLoss, "/", meanTestLoss, 
                                                   " | Time: ", timer.GetTime(), " s"
                                    );

            finishedBatches++;
        }

        //Early stopping
        if(meanTestLoss > bestLoss) break;
        else bestLoss = meanTestLoss;

        //Save model
        if(!optimize){
            tagger->to(torch::kCPU);
            torch::save(tagger, outDir + "/htagger.pt");
            std::cout << "Model was saved: " + outDir + "/htagger.pt" << std::endl;
            tagger->to(device);
        }
    }

    return bestLoss;
}

int main(int argc, char** argv){
    //Parser arguments
    Parser parser(argc, argv);
    
    std::string outPath = parser.GetValue<std::string>("out-dir");
    std::vector<std::string> bkgFiles = parser.GetVector<std::string>("bkg-files");
    std::vector<std::string> sigFiles = parser.GetVector<std::string>("sig-files");
    std::vector<std::string> channels = parser.GetVector<std::string>("channels");
    std::vector<std::string> cuts = parser.GetVector<std::string>("cuts");

    bool optimize = parser.GetValue<bool>("optimize");
    std::string optParam = parser.GetValue<std::string>("opt-param", "");
   /* std::unique_ptr<Frame> hyperParam;

    if(optParam != "") hyperParam = std::make_unique<Frame>(optParam);

    int batchSize = parser.GetValue<int>("batch-size", optParam != "" ? hyperParam->Get("batchSize", 0) : 2000);
    int nHidden = parser.GetValue<int>("n-hidden", optParam != ""  ? hyperParam->Get("nHidden", 0) : 140);
    int nConvFilter = parser.GetValue<int>("n-convfilter", optParam != "" ? hyperParam->Get("nConvFilter", 0) : 130);
    int kernelSize = parser.GetValue<int>("n-kernelsize", optParam != "" ? hyperParam->Get("kernelSize", 0) : 57);
    float dropOut = parser.GetValue<float>("drop-out", optParam != "" ? hyperParam->Get("dropOut", 0) : 0.1);
    float lr = parser.GetValue<float>("lr", optParam != "" ? hyperParam->Get("lr", 0) : 1e-4); 

    //Check if you are on CPU or GPU
    torch::Device device(optimize ? torch::kCPU : torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    //Model for Htaggers
    std::shared_ptr<HTagger> tagger = std::make_shared<HTagger>(7, nHidden, nConvFilter, kernelSize, dropOut, device);
    tagger->Print();

    if(tagger->GetNWeights() > 400000) throw std::runtime_error("Number of weights too big");

    //Pytorch dataset class
    std::vector<HTagDataset> sigSets, bkgSets;
    std::vector<std::shared_ptr<TFile>> files;
    std::vector<std::shared_ptr<TTree>> trees;

    for(const std::string& chan: channels){
        for(const std::string& file: sigFiles){
            if(StrUtil::Find(file, chan).empty()) continue;

            std::string cleanJet = !StrUtil::Find(chan, "Muon").empty() ? "mu/m" : "e/m";

            files.push_back(std::make_shared<TFile>(file.c_str(), "READ"));
            trees.push_back(std::shared_ptr<TTree>(files.back()->Get<TTree>(chan.c_str())));

            sigSets.push_back(HTagDataset(files.back(), trees.back(), cuts, cleanJet, 0, device, true, 25));

            if(!StrUtil::Find(chan, "2FJ").empty()){
                sigSets.push_back(HTagDataset(files.back(), trees.back(), cuts, cleanJet, 1, device, true, 25));
            }
        }

        for(const std::string& file: bkgFiles){
            if(StrUtil::Find(file, chan).empty()) continue;

            std::string cleanJet = !StrUtil::Find(chan, "Muon").empty() ? "mu/m" : "e/m";

            files.push_back(std::make_shared<TFile>(file.c_str(), "READ"));
            trees.push_back(std::shared_ptr<TTree>(files.back()->Get<TTree>(chan.c_str())));

            bkgSets.push_back(HTagDataset(files.back(), trees.back(), cuts, cleanJet, 0, device, false, 6));

            if(!StrUtil::Find(chan, "2FJ").empty()){
                bkgSets.push_back(HTagDataset(files.back(), trees.back(), cuts, cleanJet, 1, device, false, 6));
            }
        }
    }

    float bestLoss = Train(outPath, tagger, sigSets, bkgSets, device, batchSize, lr, optimize);
    if(optimize) std::cout << "Best loss value: " << bestLoss << std::endl; */
}
