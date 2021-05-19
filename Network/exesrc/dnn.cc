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
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Utility/include/frame.h>

float Validate(std::shared_ptr<DNNModel>& model, torch::nn::CrossEntropyLoss& loss, DNNTensor& test, std::vector<int>& testMass, const std::vector<std::string>& bkgClasses, const std::string& outPath){
    //Go to eval mode and get loss
    torch::NoGradGuard no_grad;
    model->eval();
    torch::Tensor predictionTest = model->forward(test.input, torch::from_blob(testMass.data(), {testMass.size(), 1}, torch::kInt).clone());
    torch::Tensor lossTest = loss->forward(predictionTest, test.label);

    //Get test pred as score and pred labels
    torch::Tensor pred = torch::nn::functional::softmax(predictionTest, torch::nn::functional::SoftmaxFuncOptions(1));
    torch::Tensor predLab = std::get<1>(torch::max(pred, 1));
    std::vector<long> predLabel(predLab.data_ptr<long>(), predLab.data_ptr<long>() + predLab.numel());
                
    //True label
    test.label = test.label.contiguous();
    std::vector<long> trueLabel(test.label.data_ptr<long>(), test.label.data_ptr<long>() + test.label.numel());
              
    //Draw confusion  
    std::vector<std::string> allClasses = VUtil::Append(bkgClasses, "HPlus");
    float accuracy = PUtil::DrawConfusion(trueLabel, predLabel, allClasses, outPath);         

    //Draw all score of all classes
    for(int l = 0; l < allClasses.size(); ++l){
        std::shared_ptr<TCanvas> c = std::make_shared<TCanvas>("c", "c", 1000, 1000);
        std::shared_ptr<TLegend> leg = std::make_shared<TLegend>(0., 0., 1, 1);
        PUtil::SetStyle();
        PUtil::SetPad(c.get());

        std::vector<std::shared_ptr<TH1F>> hists;   

        for(const std::string& cls : allClasses){
            hists.push_back(std::make_shared<TH1F>(cls.c_str(), cls.c_str(), 20, 0, 1));
            hists.back()->SetLineColor(20 + 5*hists.size());
            hists.back()->SetLineWidth(4);
            hists.back()->GetXaxis()->SetTitle(("Probabilities for " + allClasses[l]).c_str());
            leg->AddEntry(hists.back().get(), cls.c_str(), "L");
        }

        PUtil::SetHist(c.get(), hists.back().get());
    
        for(int n = 0; n < trueLabel.size(); ++n){
            if(trueLabel[n] != l) continue;

            for(int o = 0; o < allClasses.size(); ++o){
                hists[o]->Fill(pred[n][o].item<float>());
            }
        }

        for(const std::shared_ptr<TH1F>& h : hists) h->Draw("SAME HIST");
        PUtil::DrawLegend(c.get(), leg.get(), allClasses.size());
                    
        c->SaveAs((outPath + "/score_" + allClasses[l] + ".pdf").c_str());
    }

    std::cout << StrUtil::Merge("Test accuracy: ", accuracy, "% | Test loss : ", lossTest.item<float>()) << std::endl;

    return lossTest.item<float>();
}

float Train(std::shared_ptr<DNNModel>& model, std::vector<DNNDataset>& sigSets, std::vector<DNNDataset>& bkgSets,  std::vector<int>& masses, torch::Device& device, const int& batchSize, const float& lr, const bool& optimize, const std::string& outPath, const std::vector<std::string>&  bkgClasses){
    //Calculate weights to equalize pure number of events for signal and background
    std::vector<int> nBkg(bkgClasses.size(), 0);
    int nSig = 0, nBkgTotal = 0;
    
    for(DNNDataset& set : sigSets) nSig += set.size().value();
    
    for(DNNDataset& set : bkgSets){        
        nBkg[set.GetClass()] += set.size().value();
        nBkgTotal += set.size().value();
    }

    float wSig = (nBkgTotal+nSig)/float(nSig);
    std::vector<float> wBkg(bkgClasses.size(), 1.);
    
    for(int i = 0; i < bkgClasses.size(); ++i){
        wBkg[i] = (nBkgTotal+nSig)/float(nBkg[i]);
    }

    //Calculate number of batches
    int nBatches = nSig+nBkgTotal % batchSize == 0 ? (nSig+nBkgTotal)/batchSize - 1 : std::ceil((nSig+nBkgTotal)/batchSize);

    //Get dataloader
    std::vector<std::vector<std::pair<int, int>>> signal;
    std::vector<std::vector<std::pair<int, int>>> bkg;

    for(DNNDataset& set : sigSets){
        int bSize  = float(set.size().value())*batchSize/(nBkgTotal+nSig);
        int skip = bSize > 1 ? 0 : std::ceil(1./bSize);
    
        std::vector<std::pair<int, int>> batches(nBatches, std::pair<int, int>{});

        for(std::size_t i = 0; i < nBatches; ++i){
            if(skip == 0){
                batches.at(i) = {i*bSize, (i + 1)*bSize};
            }

            else if(i % skip == 0){
                batches.at(i/skip) = {i/skip*bSize, (i/skip + 1)*bSize};
            }
        }

        signal.push_back(batches);
    }

    for(DNNDataset& set : bkgSets){
        float bSize  = float(set.size().value())*batchSize/(nBkgTotal+nSig);
        int skip = bSize > 1 ? 0 : std::ceil(1./bSize);
        
        std::vector<std::pair<int, int>> batches(nBatches, std::pair<int, int>{});

        for(std::size_t i = 0; i < nBatches; ++i){
            if(skip == 0){
                batches.at(i) = {i*bSize, (i + 1)*bSize};
            }

            else if(i % skip == 0){
                batches.at(i/skip) = {i/skip*bSize, (i/skip + 1)*bSize};
            }
        }

        bkg.push_back(batches);
    }

    torch::Tensor weight =  torch::from_blob(VUtil::Append(wBkg, wSig).data(), {bkgClasses.size() + 1}).clone().to(device);
    
    //Optimizer
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));
    torch::nn::CrossEntropyLoss loss(torch::nn::CrossEntropyLossOptions().weight(weight));

    //Initial best loss for later early stopping
    float bestLoss = 1e7;
    int patience = optimize ? 2 : 40;
    int notBetter = 0;

    int forVali = 10./100 * nBatches;

    for(int i=0; i < 10000; ++i){
        std::vector<float> trainLoss;

        //Measure time
        Utils::RunTime timer;

        float meanTrainLoss=0., meanTestLoss=0.;

        //Declaration of input train/test tensors
        DNNTensor train, test;
        std::vector<int> trainMass, testMass;
        
        std::vector<int> rndBatchIdx = VUtil::Range(0, nBatches - 1 - forVali, nBatches - forVali);
        int seed = int(std::time(0));
        std::srand(seed);
        std::random_shuffle(rndBatchIdx.begin(), rndBatchIdx.end());

        if(optimize and timer.Time() > 60*15) break;

        for(std::size_t j = 0; j < nBatches; ++j){
            //Do fewer n of batches for hyperopt
            if(optimize and j == 20) j = nBatches - forVali;

            //Set gradients to zero
            model->zero_grad();

            //Put signal + background in batch vector
            std::vector<DNNTensor> batch;
            std::vector<int> batchMass;
            
            int idx = j < nBatches - forVali ? rndBatchIdx.at(j) : j;

            for(int k = 0; k < sigSets.size(); ++k){
                for(int l = signal.at(k).at(idx).first; l < signal.at(k).at(idx).second; ++l){
                    batchMass.push_back(masses.at(k));
                    batch.push_back(sigSets.at(k).get(l));
                }
            }

            for(int k = 0; k < bkgSets.size(); ++k){
                for(int l = bkg.at(k).at(idx).first; l < bkg.at(k).at(idx).second; ++l){
                    int index = std::experimental::randint(0, (int)masses.size() -1);
                    batchMass.push_back(masses.at(index));

                    batch.push_back(bkgSets.at(k).get(l));
                }
            }

            //Set batch for testing and go to next iteration
            if(j >= nBatches - forVali){
                if(j == nBatches - forVali){
                    test = DNNDataset::Merge(batch);
                    testMass = batchMass;
                }

                else{
                    test = DNNDataset::Merge({test, DNNDataset::Merge(batch)});
                    testMass = VUtil::Merge(testMass, batchMass);
                }
            }

            //Set batch for training
            train = DNNDataset::Merge(batch);
            trainMass = batchMass;
           
            //Prediction    
            model->train();
            torch::Tensor predictionTrain = model->forward(train.input, torch::from_blob(trainMass.data(), {trainMass.size(), 1}, torch::kInt).clone().to(device));

            //Loss
            torch::Tensor lossTrain = loss->forward(predictionTrain, train.label);
        
            trainLoss.push_back(lossTrain.item<float>());
            meanTrainLoss = std::accumulate(trainLoss.begin(), trainLoss.end(), 0.)/trainLoss.size();

            //Back propagation
            lossTrain.backward();
            optimizer.step();

            //Progess bar
            std::string barString = StrUtil::Merge<5>("Epoch: ", i+1,
                                                   " | Batch: ", j, "/", nBatches - 1 - forVali, 
                                                   " | Loss: ", meanTrainLoss,
                                                   " | Overtrain: ", notBetter, "/", patience, 
                                                   " | Time: ", timer.Time(), " s"
                                    );

            if(j < nBatches - forVali){
                Utils::ProgressBar(float(j+1)/(nBatches - forVali)*100, barString);
            }

            if(j == nBatches - 1){
                meanTestLoss = Validate(model,loss, test, testMass, bkgClasses, outPath);
            }
        }

        //Early stopping      
        if (meanTestLoss > bestLoss){
            notBetter++;
            if(notBetter % 10 == 0 and !optimize) torch::load(model, outPath + "/model.pt");
        }

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
    int era = parser.GetValue<int>("era");
    bool isEven = parser.GetValue<bool>("is-even");
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
    std::vector<std::shared_ptr<TTree>> trees;
    std::vector<DNNDataset> sigSets;
    std::vector<DNNDataset> bkgSets;

    //Create model
    std::shared_ptr<DNNModel> model = std::make_shared<DNNModel>(parameters.size(), nNodes, nLayers, dropOut, masses.size() == 1 ? false : true, bkgClasses.size() + 1, device);
    
    if(std::filesystem::exists(outPath + "/model.pt") and !optimize) torch::load(model, outPath + "/model.pt");

    model->Print();

    //Collect input data
    for(std::string fileName: sigFiles){
        files.push_back(RUtil::Open(fileName)); 
        trees.push_back(RUtil::GetSmart<TTree>(files.back().get(), channel));

        DNNDataset sigSet(trees.back(), parameters, cuts, era, optimize ? -1 :isEven, device, bkgClasses.size());
        sigSets.push_back(std::move(sigSet));
    }

    for(int i = 0; i < bkgClasses.size(); ++i){
        std::vector<std::string> bkgFiles = parser.GetVector<std::string>(StrUtil::Replace("@-files", "@", bkgClasses.at(i)));
          
        for(std::string fileName: bkgFiles){
            files.push_back(RUtil::Open(fileName)); 
            trees.push_back(RUtil::GetSmart<TTree>(files.back().get(), channel));

            DNNDataset bkgSet(trees.back(), parameters, cuts, era, optimize ? -1 :isEven, device, i);
            bkgSets.push_back(std::move(bkgSet));
        }
    }

    //Do training
    float bestLoss = Train(model, sigSets, bkgSets, masses, device, batchSize, lr, optimize, outPath, bkgClasses);
    if(optimize) std::cout << "\n" << "Best loss: " << bestLoss << std::endl;
}
