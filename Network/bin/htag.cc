#include <experimental/random>

#include <torch/torch.h>

#include <TH1F.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TLatex.h>

#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>
#include <ChargedAnalysis/Analysis/include/plotter.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/frame.h>


void Train(const torch::Tensor& charged, const torch::Tensor& neutral, const torch::Tensor& SV, const torch::Tensor& tagValue, const bool& isPhiUp, Frame* frame=NULL){
    std::string scorePath = std::string(std::getenv("CHDIR")) + "/DNN/ScorePlots";
    std::system((std::string("command rm -rf ") + scorePath).c_str());
    std::system((std::string("mkdir -p ") + scorePath).c_str());

    //Check if you are on CPU or GPU
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);

    //Convert data to torch tensor and shuffle them
    torch::Tensor randIndex = torch::randperm((int)tagValue.size(0), torch::TensorOptions().dtype(torch::kLong)).to(device);

    torch::Tensor target = tagValue.to(device).index_select(0, randIndex);
   
    torch::Tensor chargedTensor = charged.to(device).index_select(0, randIndex);
    torch::Tensor neutralTensor = neutral.to(device).index_select(0, randIndex);
    torch::Tensor SVTensor = SV.to(device).index_select(0, randIndex);

    //Hyperparameter for tuning
    int nHidden, nLSTM, nConvFilter, kernelSize, eventsInBatches;
    float dropOut, learningRate;

    //Output for tuning
    float valLoss, trainLoss, AUC;

    std::shared_ptr<HTagger> tagger;

    if(!frame){
        tagger = std::make_shared<HTagger>(7, 95, 1, 104, 10, 0.18);
    }

    else{
        while(true){
            nHidden = std::experimental::randint(20, 300);
            nLSTM = std::experimental::randint(1, 1);
            nConvFilter = std::experimental::randint(5, 200);
            kernelSize = std::experimental::randint(2, nHidden);
            eventsInBatches = std::experimental::randint(256, 4096);

            dropOut = std::experimental::randint(0, 50)/100.;
            learningRate = std::pow(10, std::experimental::randint(-3, -2));

            tagger = std::make_shared<HTagger>(7, nHidden, nLSTM, nConvFilter, kernelSize, dropOut);

            if(tagger->GetNWeights() < 300000) break;
        }
    }

    tagger->Print();
    tagger->to(device);

    //Optimizer
    float lr = !frame ? 0.001: learningRate;
    torch::optim::Adam optimizer(tagger->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));

    //Variables for number of batches and and batch size
    int nVali = 3000;
    int nTrain = tagValue.size(0) - nVali;

    int batchSize = !frame ? 1024 : eventsInBatches;
    int nEpochs = 1000;
    int nBatches = nTrain % batchSize == 0 ? nTrain/batchSize -1 : std::ceil(nTrain/batchSize);

    //Early stopping variables
    float minLoss = 1e6;
    int nPatience = 0;

    //Draw score during train loop
    TCanvas* canvas = new TCanvas("canvas", "canvas", 1000, 800);
    canvas->Draw();

    TH1F* higgsHist = new TH1F("Higgs", "Higgs", 30, 0, 1);
    higgsHist->SetLineColor(kBlue+1);
    higgsHist->SetFillStyle(3335);
    higgsHist->SetFillColor(kBlue);
    higgsHist->SetLineWidth(4);

    TH1F* topHist = new TH1F("Top", "Top", 30, 0, 1);
    topHist->SetLineColor(kRed+1);
    topHist->SetFillStyle(3353);
    topHist->SetFillColor(kRed);
    topHist->SetLineWidth(4);

    TGraph* ROC;
    TLatex* rocText;
    
    Plotter::SetPad(canvas);
    Plotter::SetStyle();
    Plotter::SetHist(topHist);

    //Print same other stats:
    std::cout << "Number of events: " << nTrain << " | Batch Size: " << batchSize << " | Learning rate: " << lr << std::endl;

    for(int i=0; i<nEpochs; i++){
        //Measure time
        Utils::RunTime timer;

        //Validation data
        torch::Tensor chargedVali = chargedTensor.narrow(0, nTrain, nVali);  
        torch::Tensor neutralVali = neutralTensor.narrow(0, nTrain, nVali); 
        torch::Tensor SVVali = SVTensor.narrow(0, nTrain, nVali); 
        torch::Tensor labelVali = target.narrow(0, nTrain, nVali);

        torch::Tensor predictionTrain;
        torch::Tensor lossTrain;

        torch::Tensor predictionVali;
        torch::Tensor lossVali;

        float averagedLoss = 0;

        if(nPatience >= 20){
            std::cout << "Training will be closed due to early stopping" << std::endl;
            break;
        }

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
            predictionTrain = tagger->forward(chargedTrain, neutralTrain, SVTrain, true);
            lossTrain = torch::binary_cross_entropy(predictionTrain, labelTrain);

            //Back propagation
            lossTrain.backward();
            optimizer.step();

            predictionVali = tagger->forward(chargedVali, neutralVali, SVVali, false);
            lossVali =  torch::binary_cross_entropy(predictionVali, labelVali);
            averagedLoss = (averagedLoss*j + lossVali.item<float>())/(j+1);

            //Progess bar
            std::string barString = "Epoch: " + std::to_string(i+1) + "/" + std::to_string(nEpochs) + 
                                    " | Batch: " + std::to_string(j) + "/" + std::to_string(nBatches) + 
                                    " | Loss: " + std::to_string(lossTrain.item<float>()).substr(0, 5) + "/" + std::to_string(averagedLoss).substr(0, 5) + " | Time: " + std::to_string(timer.Time()).substr(0, 2) + " s";

            Utils::ProgressBar(float(j)/nBatches*100, barString);
        }

        //Update histograms 
        for(unsigned int k=0; k < predictionVali.size(0); k++){
            if(labelVali[k].item<float>() == 0){
                topHist->Fill(predictionVali[k].item<float>());
            }

            else{
                higgsHist->Fill(predictionVali[k].item<float>());
            }
        }

        //Update canvas
        canvas->Clear();
        topHist->DrawNormalized("HIST");
        higgsHist->DrawNormalized("HIST SAME");
        Plotter::DrawHeader(false, "All channel", "Work in Progress");

        canvas->SaveAs((scorePath + "/score_" + std::to_string(i) + ".pdf").c_str());
        canvas->SaveAs((scorePath + "/score.pdf").c_str());

        canvas->Clear();
        ROC = Utils::GetROC(predictionVali, labelVali, 100);
        ROC->SetMarkerStyle(21);
        Plotter::SetHist(ROC->GetHistogram());
        ROC->Draw("AP");

        std::function<float(TGraph*)> integral = [](TGraph* graph){
            float AUC = 0;
            double x, y;

            for(int i=0; i < 100; i++){
                graph->GetPoint(i, x, y);
                AUC += y* 1/100.;
            }   

            return AUC;        
        };    
        
        rocText = new TLatex(0.5, 0.1, ("AUC: " + std::to_string(integral(ROC))).c_str());
        rocText->Draw("SAME");
        
        Plotter::DrawHeader(false, "All channel", "Work in Progress");

        canvas->SaveAs((scorePath + "/ROC_" + std::to_string(i) + ".pdf").c_str());
        canvas->SaveAs((scorePath + "/ROC.pdf").c_str());

        if(frame){
            AUC = integral(ROC);
            valLoss = lossVali.item<float>();
            trainLoss = lossTrain.item<float>();
        }
  
        //Check for early stopping
        if(minLoss > averagedLoss){
            minLoss = averagedLoss;
            nPatience=0;
        } 

        else nPatience++;
    }

    //Clean up
    delete canvas;
    delete higgsHist;
    delete topHist;
    delete ROC;

    //Save model
    if(!frame){
        std::string taggerName = isPhiUp ? "ForPhiDown.pt" : "ForPhiUp.pt";
    
        std::string outName = std::string(std::getenv("CHDIR")) + "/DNN/Model/htagger" + taggerName;
        tagger->to(torch::kCPU);
        torch::save(tagger, outName);
        std::cout << "Model was saved: " << outName << std::endl;
    }

    else frame->AddColumn({AUC, valLoss, trainLoss, nHidden, nLSTM, nConvFilter, kernelSize, dropOut, learningRate, eventsInBatches});
}

int main(int argc, char** argv){
    //Parser arguments
    Parser parser(argc, argv);
    bool optimize = parser.GetValue<bool>("optimize");
    int nEvents = parser.GetValue<int>("nEvents");

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

    //Extract data from TTrees with Treereader class
    std::vector<torch::Tensor> chargedTensors;
    std::vector<torch::Tensor> neutralTensors;
    std::vector<torch::Tensor> SVTensors;

    std::vector<torch::Tensor> phiUp;
    std::vector<torch::Tensor> phiDown;

    std::vector<int> nMax(3, 0);

    std::vector<torch::Tensor> tagValues;

    for(std::string& channel: channels){
        int nFJ = channel.find("2j1fj") != std::string::npos ? 1 : 2;
        int nSig = 0;

        for(const std::string& fileName: sigFiles){
            std::cout << "Readout data from file " << Utils::SplitString<std::string>(fileName, "/").back() << " and channel " << channel << std::endl;


            for(int n = 0; n < nFJ; n++){
                std::vector<torch::Tensor> input = HTagger::GatherInput(fileName, channel, 0, nEvents, n);
                chargedTensors.push_back(input[0]);
                neutralTensors.push_back(input[1]);
                SVTensors.push_back(input[2]);

                phiUp.push_back(input[3] + torch::full({input[3].size(0)}, nSig, torch::kLong));
                phiDown.push_back(input[4] + torch::full({input[4].size(0)}, nSig, torch::kLong));

                nMax[0] = input[0].size(1) > nMax[0] ? input[0].size(1) : nMax[0];
                nMax[1] = input[1].size(1) > nMax[1] ? input[1].size(1) : nMax[1];
                nMax[2] = input[2].size(1) > nMax[2] ? input[2].size(1) : nMax[2];

                tagValues.push_back(torch::from_blob(std::vector<float>(nEvents, 1).data(), {nEvents}).clone());

                nSig+= nEvents;
            }
        }

        int nTotal = nSig;

        for(const std::string& fileName: bkgFiles){
            std::cout << "Readout data from file " << Utils::SplitString<std::string>(fileName, "/").back() << " and channel " << channel << std::endl;

            for(int n = 0; n < nFJ; n++){
                std::vector<torch::Tensor> input = HTagger::GatherInput(fileName, channel, 0, nSig, n);
                chargedTensors.push_back(input[0]);
                neutralTensors.push_back(input[1]);
                SVTensors.push_back(input[2]);

                phiUp.push_back(input[3] + torch::full({input[3].size(0)}, nTotal, torch::kLong));
                phiDown.push_back(input[4] + torch::full({input[4].size(0)}, nTotal, torch::kLong));

                nMax[0] = input[0].size(1) > nMax[0] ? input[0].size(1) : nMax[0];
                nMax[1] = input[1].size(1) > nMax[1] ? input[1].size(1) : nMax[1];
                nMax[2] = input[2].size(1) > nMax[2] ? input[2].size(1) : nMax[2];

                tagValues.push_back(torch::from_blob(std::vector<float>(nSig, 0).data(), {nSig}).clone());

                nTotal+= nSig;
            }
        }
    }

    //Transform to tensors and do padding
    for(int i = 0; i < chargedTensors.size(); i++){
        if(chargedTensors[i].size(1) < nMax[0]) chargedTensors[i] = torch::constant_pad_nd(chargedTensors[i], {0,0, 0, nMax[0] - chargedTensors[i].size(1)}, 0);

        if(neutralTensors[i].size(1) < nMax[1]) neutralTensors[i] = torch::constant_pad_nd(neutralTensors[i], {0,0, 0, nMax[1] - neutralTensors[i].size(1)}, 0);

        if(SVTensors[i].size(1) < nMax[2]) SVTensors[i] = torch::constant_pad_nd(SVTensors[i], {0,0, 0, nMax[2] - SVTensors[i].size(1)}, 0);
    }

    torch::Tensor chargedTensor = torch::cat(chargedTensors, 0);
    torch::Tensor neutralTensor = torch::cat(neutralTensors, 0);
    torch::Tensor SVTensor = torch::cat(SVTensors, 0);
    torch::Tensor tagValue = torch::cat(tagValues, 0);

    torch::Tensor Up = torch::cat(phiUp, 0);
    torch::Tensor Down = torch::cat(phiDown, 0);

    //Do training
    if(!optimize){
        Train(chargedTensor.index({Up}), neutralTensor.index({Up}), SVTensor.index({Up}), tagValue.index({Up}), true); 
        Train(chargedTensor.index({Down}), neutralTensor.index({Down}), SVTensor.index({Down}), tagValue.index({Down}), false); 
    }

    //Do hyperspace optimisation
    else{
        Frame* frame = new Frame({"AUC", "valLoss", "trainLoss", "nHidden", "nLayer", "nConv", "kernelSize", "dropOut", "lr", "batchSize"});

        for(int i=0; i < 1000; i++){
            Train(chargedTensor.index({Up}), neutralTensor.index({Up}), SVTensor.index({Up}), tagValue.index({Up}), true, frame); 

            frame->Sort("AUC", false);
            frame->WriteCSV(std::string(std::getenv("CHDIR")) + "/DNN/HyperTuning/htagger.csv");
        }

        delete frame;
    }
} 
