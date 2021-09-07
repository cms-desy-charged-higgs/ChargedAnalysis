#include <torch/torch.h>

#include <map>
#include <vector>
#include <string>
#include <filesystem>

#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/dnndataset.h>
#include <ChargedAnalysis/Network/include/dataloader.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stopwatch.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Utility/include/csv.h>
#include <ChargedAnalysis/Utility/include/backtracer.h>

std::pair<float, float> Validate(std::shared_ptr<DNNModel>& model, torch::nn::CrossEntropyLoss& loss, DNNTensor& test, std::vector<int>& testChargedMass, std::vector<int>& testNeutralMass, const std::vector<std::string>& bkgClasses, const std::string& outPath){
    //Go to eval mode and get loss
    torch::NoGradGuard no_grad;
    model->eval();

    torch::Tensor predictionTest = model->forward(test.input, torch::from_blob(testChargedMass.data(), {testChargedMass.size(), 1}, torch::kInt).clone(), torch::from_blob(testNeutralMass.data(), {testNeutralMass.size(), 1}, torch::kInt).clone());
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

    return {lossTest.item<float>(), accuracy};
}

float Train(std::shared_ptr<DNNModel>& model, std::vector<DNNDataSet>& sigSets, std::vector<DNNDataSet>& bkgSets, torch::Device& device, const int& batchSize, const float& lr, const bool& optimize, const std::string& outPath, const std::vector<std::string>&  bkgClasses){
    //Initial best loss for later early stopping
    float bestLoss = 1e7;
    int patience = optimize ? 2 : 40;
    int notBetter = 0;

    float vali = 0.1;
    std::vector<float> epochs, meanTrainLoss, meanTestLoss, accuracy;

    //Dataloader
    DataLoader loader(sigSets, bkgSets, bkgClasses.size() + 1, batchSize, 2);
    torch::Tensor weight = loader.GetClassWeights().to(device);

    //Optimizer
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));
    torch::nn::CrossEntropyLoss loss(torch::nn::CrossEntropyLossOptions().weight(weight));

    for(int i=0; i < 10000; ++i){
        epochs.push_back(i);
        float meanLoss;

        //Measure time
        StopWatch timer; 
        timer.Start();

        //Initialze dataloader
        loader.InitEpoch(vali);
        
        //Declaration of input train/test tensors
        DNNTensor train, test;
        std::vector<int> trainChargedMass, testChargedMass, trainNeutralMass, testNeutralMass;

        if(optimize and timer.SetTimeMark() > 60*15) break;

        for(std::size_t j = 0; j < loader.GetNBatches(); ++j){
            //Set gradients to zero
            model->zero_grad();

            //Load batch
            std::vector<DNNTensor> batch;
            std::vector<int> batchChargedMass, batchNeutralMass;
           
            std::tie(batch, batchChargedMass, batchNeutralMass) = loader.GetBatch();

            //Set batch for testing and go to next iteration
            if(j >= loader.GetNTrainBatches()){
                if(testChargedMass.size() == 0){
                    test = DNNDataSet::Merge(batch);
                    testChargedMass = batchChargedMass;
                    testNeutralMass = batchNeutralMass;
                }

                else{
                    test = DNNDataSet::Merge({test, DNNDataSet::Merge(batch)});
                    testChargedMass = VUtil::Merge(testChargedMass, batchChargedMass);
                    testNeutralMass = VUtil::Merge(testNeutralMass, batchNeutralMass);
                }
            }

            //Set batch for training
            train = DNNDataSet::Merge(batch);
            trainChargedMass = batchChargedMass;
            trainNeutralMass = batchNeutralMass;

            //Prediction    
            model->train();
            torch::Tensor predictionTrain = model->forward(train.input, torch::from_blob(trainChargedMass.data(), {trainChargedMass.size(), 1}, torch::kInt).clone().to(device), torch::from_blob(trainNeutralMass.data(), {trainNeutralMass.size(), 1}, torch::kInt).clone().to(device));

            //Loss
            torch::Tensor lossTrain = loss->forward(predictionTrain, train.label);
            meanLoss = (meanLoss*j + lossTrain.item<float>())/(j + 1);

            //Back propagation
            lossTrain.backward();
            optimizer.step();

            //Progess bar
            std::string barString = StrUtil::Merge<5>("Processed: ", 100*float(j+1)/loader.GetNTrainBatches(), " %",
                                                   " | Epoch: ", i+1,
                                                   " | Batch: ", j, "/", loader.GetNTrainBatches() - 1, 
                                                   " | Loss: ", meanLoss,
                                                   " | Overtrain: ", notBetter, "/", patience, 
                                                   " | Time: ", timer.GetTime(), " s"
                                    );

            if(j < loader.GetNTrainBatches()){
                std::cout << "" << "\r";
                std::cout << barString;
            }

            if(j == loader.GetNBatches() - 1){
                std::cout << std::endl;
                float mLoss, acc;
                std::tie(mLoss, acc) = Validate(model,loss, test, testChargedMass, testNeutralMass, bkgClasses, outPath);

                meanTestLoss.push_back(mLoss);
                accuracy.push_back(acc);
                meanTrainLoss.push_back(meanLoss);
            }
        }

        PUtil::DrawLoss(outPath, epochs, meanTrainLoss, meanTestLoss, accuracy);

        //Early stopping      
        if (meanTestLoss.back() > bestLoss){
            notBetter++;
            if(notBetter % 10 == 0 and !optimize) torch::load(model, outPath + "/model.pt");
        }

        else{
            bestLoss = meanTestLoss.back();
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
    std::vector<std::string> sigFiles = parser.GetVector<std::string>("sig-files");
    std::vector<std::string> sigEntryList = parser.GetVector<std::string>("sig-entry-list");
    std::vector<std::string> bkgClasses = parser.GetVector<std::string>("bkg-classes");
    std::vector<std::string> parameters = parser.GetVector<std::string>("parameters");

    std::string optParam = parser.GetValue<std::string>("opt-param", "");
    std::unique_ptr<CSV> hyperParam;

    if(optParam != "") hyperParam = std::make_unique<CSV>(optParam, "r", "\t");

    int batchSize = parser.GetValue<int>("batch-size", optParam != "" ? hyperParam->Get<int>(0, "batch-size") : 2000);
    int nNodes = parser.GetValue<int>("n-nodes", optParam != ""  ? hyperParam->Get<int>(0, "n-nodes") : 40);
    int nLayers = parser.GetValue<int>("n-layers", optParam != "" ? hyperParam->Get<int>(0, "n-layers") : 3);
    float dropOut = parser.GetValue<float>("drop-out", optParam != "" ? hyperParam->Get<float>(0, "drop-out") : 0.1);
    float lr = parser.GetValue<float>("lr", optParam != "" ? hyperParam->Get<float>(0, "lr") : 1e-4);
    if(lr < 0) lr = std::pow(10, lr);

    bool optimize = parser.GetValue<bool>("optimize");

    //Check if you are on CPU or GPU
    torch::Device device(torch::kCPU);

    //Restrict number of threads to one
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    //
    std::vector<DNNDataSet> sigSets;
    std::vector<DNNDataSet> bkgSets;

    //Create model
    std::shared_ptr<DNNModel> model = std::make_shared<DNNModel>(parameters.size(), nNodes, nLayers, dropOut, true, bkgClasses.size() + 1, device);
    
    if(std::filesystem::exists(outPath + "/model.pt") and !optimize) torch::load(model, outPath + "/model.pt");

    model->Print();

    //Collect input data
    for(std::size_t idx = 0; idx < sigFiles.size(); ++idx){
        int chargedMass = std::atoi(StrUtil::Split(StrUtil::Split(sigFiles.at(idx), "/").back(), "_").at(3).c_str());
        int neutralMass = std::atoi(StrUtil::Split(StrUtil::Split(sigFiles.at(idx), "/").back(), "_").at(4).c_str());

        DNNDataSet sigSet(sigFiles.at(idx), channel, sigEntryList.at(idx), parameters, era, device, bkgClasses.size());
        sigSet.chargedMass = chargedMass;
        sigSet.neutralMass = neutralMass;
        sigSets.push_back(std::move(sigSet));
    }

    for(int i = 0; i < bkgClasses.size(); ++i){
        std::vector<std::string> bkgFiles = parser.GetVector<std::string>(StrUtil::Replace("@-files", "@", bkgClasses.at(i)));
        std::vector<std::string> bkgEntryList = parser.GetVector<std::string>(StrUtil::Replace("@-files", "@", bkgClasses.at(i)));
          
        for(std::size_t idx = 0; idx < bkgFiles.size(); ++idx){
            DNNDataSet bkgSet(bkgFiles.at(idx), channel, bkgEntryList.at(idx), parameters, era, device, i);
            bkgSets.push_back(std::move(bkgSet));
        }
    }

    //Do training
    float bestLoss = Train(model, sigSets, bkgSets, device, batchSize, lr, optimize, outPath, bkgClasses);
    if(optimize) std::cout << "\n" << "Best loss: " << bestLoss << std::endl;
}
