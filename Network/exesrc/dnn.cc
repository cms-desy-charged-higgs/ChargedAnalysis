#include <torch/torch.h>

#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <experimental/random>

#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/dnndataset.h>
#include <ChargedAnalysis/Network/include/dataloader.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stopwatch.h>
#include <ChargedAnalysis/Utility/include/profiler.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/plotutil.h>
#include <ChargedAnalysis/Utility/include/csv.h>
#include <ChargedAnalysis/Utility/include/backtracer.h>

torch::Tensor CCE(torch::Tensor& pred, torch::Tensor& oneHotLabels){    
    return -torch::mean(torch::sum(torch::log(pred)*oneHotLabels, {1}));
}

float PlotPerformance(const torch::Tensor& prediction, const torch::Tensor& oneHotLabel, const std::vector<std::string>& bkgClasses, const std::vector<std::string>& outPaths, const bool& isVali){
    //From one hot encoded to vector with long labels
    torch::Tensor trueLabel = torch::argmax(oneHotLabel, {1}).to(torch::kCPU);
    std::vector<long> trueL(trueLabel.data_ptr<long>(), trueLabel.data_ptr<long>() + trueLabel.numel());

    torch::Tensor predLabel = torch::argmax(prediction, {1}).to(torch::kCPU);
    std::vector<long> predL(predLabel.data_ptr<long>(), predLabel.data_ptr<long>() + predLabel.numel());

    //Draw confusion
    std::vector<std::string> allClasses = VUtil::Append(bkgClasses, "HPlus");
    float accuracy = PUtil::DrawConfusion(trueL, predL, allClasses, outPaths, isVali);      

    torch::Tensor predAtCPU = prediction.to(torch::kCPU);

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
    
        for(int n = 0; n < predAtCPU.size(0); ++n){
            if(trueL.at(n) != l) continue;

            for(int o = 0; o < allClasses.size(); ++o){
                hists[o]->Fill(predAtCPU[n][o].item<float>());
            }
        }

        for(const std::shared_ptr<TH1F>& h : hists) h->Draw("SAME HIST");
        PUtil::DrawLegend(c.get(), leg.get(), allClasses.size());
                    
        for(const std::string outPath : outPaths){
            c->SaveAs((outPath + "/score_" + allClasses[l] + (isVali ? "_vali" : "_train") + ".pdf").c_str());
            c->SaveAs((outPath + "/score_" + allClasses[l] + (isVali ? "_vali" : "_train") + ".png").c_str());
        }
    }

    return accuracy;
}

std::pair<float, float> Validate(std::shared_ptr<DNNModel>& model, DataLoader& loader, torch::Device& device, const std::vector<std::string>& bkgClasses, const std::vector<std::string>& outPaths){
    //Go to eval mode and get loss
    torch::NoGradGuard no_grad;
    model->eval();

    //Declaration of input train/test tensors
    double lossV = 0.;
    torch::Tensor prediction, oneHotLabel;

    for(std::size_t i = 0; i < loader.GetNValiBatches(); ++i){
        DNNTensor batch = loader.GetBatch(true);

        torch::Tensor pred = model->forward(batch.input, batch.mHPlus, batch.mH);
        torch::Tensor loss = CCE(pred, batch.label);

        lossV = (lossV*i + loss.item<double>())/(i+1);

        if(prediction.size(0) == 0){
            prediction = pred;
            oneHotLabel = batch.label;
        }

        else{
            prediction = torch::cat({prediction, pred}, 0);
            oneHotLabel = torch::cat({oneHotLabel, batch.label}, 0);
        }
    }

    float accuracy = PlotPerformance(prediction, oneHotLabel, bkgClasses, outPaths, true);

    std::cout << StrUtil::Merge<5>("Test accuracy: ", accuracy, "% | Test loss : ", lossV) << std::endl;

    return {lossV, accuracy};
}

std::pair<float, float> Train(std::shared_ptr<DNNModel>& model, DataLoader& loader, torch::Device& device, const float& lr, const bool& optimize, const std::vector<std::string>& outPaths, const std::vector<std::string>&  bkgClasses){
    //Initial best loss for later early stopping
    float bestLoss = 1e7;
    int patience = optimize ? 2 : 100;
    int notBetter = 0;

    float trainAcc = 0., valAcc = 0., trainLoss = 0., valLoss = 0.;

    //Optimizer
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions().lr(lr).weight_decay(lr/10.));

    StopWatch totalTimer; 
    totalTimer.Start();

    for(int i=0; i < 10000; ++i){
        //Measure time
        StopWatch timer; 
        timer.Start();

        //Initialze dataloader
        loader.InitEpoch();

        DNNTensor batch;
        torch::Tensor pred;

        for(std::size_t j = 0; j < loader.GetNTrainBatches(); ++j){
            //Set gradients to zero
            model->zero_grad();

            //Load batch
            batch = loader.GetBatch(false);

            //Prediction
            model->train();
            pred = model->forward(batch.input, batch.mHPlus, batch.mH);

            //Loss
            torch::Tensor lossTrain = CCE(pred, batch.label);

            //Back propagation
            lossTrain.backward();
            optimizer.step();
            trainLoss = lossTrain.item<float>();

            //Progess bar
            std::string barString = StrUtil::Merge<5>("Processed: ", 100*float(j+1)/loader.GetNTrainBatches(), " %",
                                                       " | Epoch: ", i+1,
                                                       " | Batch: ", j+1, "/", loader.GetNTrainBatches(), 
                                                       " | Loss: ", trainLoss,
                                                       " | Overtrain: ", notBetter, "/", patience, 
                                                       " | Time: ", timer.GetTime(), " s"
                                        );

            std::cout << "\r";
            std::cout << barString;

            //Redo last batch after model update for perm plotting
            if(j + 1 == loader.GetNTrainBatches()){
                pred = model->forward(batch.input, batch.mHPlus, batch.mH);
                trainLoss = CCE(pred, batch.label).item<float>();
            }
        }

        //Plot train performance
        std::cout << std::endl;

        trainAcc = PlotPerformance(pred, batch.label, bkgClasses, outPaths, false);
        std::cout << StrUtil::Merge<5>("Train accuracy: ", trainAcc, "% | Train loss : ", trainLoss) << std::endl;

        //Validation
        std::tie(valLoss, valAcc) = Validate(model, loader, device, bkgClasses, outPaths);
        if(!optimize) PUtil::DrawLoss(outPaths, trainLoss, valLoss, trainAcc, valAcc);
 
        //Early stopping      
        if(valLoss > bestLoss) notBetter++;

        else{
            bestLoss = valLoss;
            notBetter = 0;

            //Save model
            if(!optimize){
                model->to(torch::kCPU);
                torch::save(model, outPaths.at(0) + "/model.pt");
                std::cout << "Model was saved: " + outPaths.at(0) + "/model.pt" << std::endl;
                model->to(device);
            }
        }

        if(notBetter == patience) break;
        if(optimize and totalTimer.GetTime() > 60*10) break;
    }

    return {valAcc, bestLoss};
}

int main(int argc, char** argv){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> outPaths = parser.GetVector<std::string>("out-path"); 
    std::string channel = parser.GetValue<std::string>("channel");
    int era = parser.GetValue<int>("era");
    std::vector<std::string> sigFiles = parser.GetVector<std::string>("sig-files");
    std::vector<std::string> sigEntryList = parser.GetVector<std::string>("sig-entry-list");
    std::vector<std::string> bkgClasses = parser.GetVector<std::string>("bkg-classes");
    std::vector<std::string> parameters = parser.GetVector<std::string>("parameters");

    int batchSize, nNodes, nLayers;
    float dropOut, lr;

    std::string optParam = parser.GetValue<std::string>("opt-param", "");
    bool optimize = parser.GetValue<bool>("optimize");

    //Check if you are on CPU or GPU
    torch::Device device = torch::kCPU;

    if(torch::cuda::is_available()){
        device = torch::kCUDA;
        std::cout << "GPU used for training!" << std::endl;
    }

    else{
        device = torch::kCPU;
        std::cout << "CPU used for training!" << std::endl;
    }

    //Create model
    std::shared_ptr<DNNModel> model;

    if(optimize){
        while(true){
            batchSize = std::experimental::randint(200, 4000);
            nNodes = std::experimental::randint(30, 500);
            nLayers = std::experimental::randint(1, 10);
            dropOut = 1./std::experimental::randint(2, 20);
            lr = std::pow(10, -std::experimental::randint(2, 5));

            model = std::make_shared<DNNModel>(parameters.size(), nNodes, nLayers, dropOut, true, bkgClasses.size() + 1, device);

            if(model->GetNWeights() < 400000) break;
        }
    }

    else if(!optParam.empty()){
        CSV hyperParam(optParam, "r", "\t");

        batchSize = hyperParam.Get<int>(0, "batch-size");
        nNodes = hyperParam.Get<int>(0, "n-nodes");
        nLayers = hyperParam.Get<int>(0, "n-layers");
        dropOut = hyperParam.Get<float>(0, "drop-out");
        lr = hyperParam.Get<float>(0, "lr");

        model = std::make_shared<DNNModel>(parameters.size(), nNodes, nLayers, dropOut, true, bkgClasses.size() + 1, device);
    }

    else throw std::runtime_error("No hyperparameter are given!");

    if(std::filesystem::exists(outPaths.at(0) + "/model.pt") and !optimize) torch::load(model, outPaths.at(0) + "/model.pt");

    model->to(device);
    model->Print();

    //Save model parameter
    CSV modelParam(outPaths.at(0) + "/model.csv", "w", {"n-nodes", "n-layers", "drop-out"}, "\t");
    modelParam.WriteRow(nNodes, nLayers, dropOut);
    modelParam.Close();

    //Restrict number of threads to one
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    std::vector<std::size_t> chargedMass(sigFiles.size()), neutralMass(sigFiles.size());

    //Collect input data
    for(std::size_t idx = 0; idx < sigFiles.size(); ++idx){
        chargedMass[idx] = std::atoi(StrUtil::Split(StrUtil::Split(sigFiles.at(idx), "/").back(), "_").at(3).c_str());
        neutralMass[idx] = std::atoi(StrUtil::Split(StrUtil::Split(sigFiles.at(idx), "/").back(), "_").at(4).c_str());
    }

    DNNDataSet sigSet(sigFiles, channel, sigEntryList, parameters, era, device, bkgClasses.size(), bkgClasses.size() + 1, chargedMass, neutralMass);
    std::vector<DNNDataSet> bkgSets;

    for(int i = 0; i < bkgClasses.size(); ++i){
        std::vector<std::string> bkgFiles = parser.GetVector<std::string>(StrUtil::Replace("@-files", "@", bkgClasses.at(i)));
        std::vector<std::string> bkgEntryList = parser.GetVector<std::string>(StrUtil::Replace("@-entry-list", "@", bkgClasses.at(i)));
          
        DNNDataSet bkgSet(bkgFiles, channel, bkgEntryList, parameters, era, device, i, bkgClasses.size() + 1, chargedMass, neutralMass);
        bkgSets.push_back(std::move(bkgSet));
    }

    //Dataloader
    DataLoader loader(sigSet, bkgSets, batchSize, 0.1, optimize);

    //Do training
    float accuracy, loss; 
    std::tie(accuracy, loss) = Train(model, loader, device, lr, optimize, outPaths, bkgClasses);

    if(optimize){ 
        CSV hyperParam(outPaths.at(0) + "/hyperparam.csv", "w", {"batch-size", "n-nodes", "n-layers", "drop-out", "lr", "loss", "acc"}, "\t");
        hyperParam.WriteRow(batchSize, nNodes, nLayers, dropOut, lr, loss, accuracy);
    }
}
