#include <experimental/random>

#include <torch/torch.h>

#include <TH1F.h>
#include <TCanvas.h>

#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>
#include <ChargedAnalysis/Analysis/include/plotter.h>
#include <ChargedAnalysis/Analysis/include/utils.h>
#include <ChargedAnalysis/Analysis/include/frame.h>


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

    //Instance of htagger class
    std::vector<float> hyperParam = {
           0., //Place holder for Val loss
           0., //Place holder for train loss
           (float)std::experimental::randint(20, 150),  //Number of hidden features
           (float)std::experimental::randint(1, 3),  //Number of LSTM layer
           (float)std::experimental::randint(5, 50), //Number of Conv filter
           (float)std::experimental::randint(2, 20),  //Kernel size of Conv filter
            std::experimental::randint(0, 50)/100., //Dropout
            std::pow(10, std::experimental::randint(-4, -2)), //Learning rate
           (float)std::experimental::randint(32, 2048), //BatchSize
    };

    std::shared_ptr<HTagger> tagger;

    if(!frame){
        tagger = std::make_shared<HTagger>(7, 45, 2, 16, 10, 0.09);
    }

    else{
        tagger = std::make_shared<HTagger>(7, hyperParam[2], hyperParam[3], hyperParam[4], hyperParam[5], hyperParam[6]);
    }

    tagger->Print();
    tagger->to(device);

    //Optimizer
    float lr = !frame ? 0.001: hyperParam[7];
    torch::optim::Adam optimizer(tagger->parameters(), torch::optim::AdamOptions(lr).weight_decay(lr/10.));

    //Variables for number of batches and and batch size
    int nVali = 3000;
    int nTrain = tagValue.size(0) - nVali;

    int batchSize = !frame ? 1024 : hyperParam[8];
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

        if(frame){
            hyperParam[0] = lossVali.item<float>();
            hyperParam[1] = lossTrain.item<float>();
        }

        //Update canvas
        canvas->Clear();
        topHist->DrawNormalized("HIST");
        higgsHist->DrawNormalized("HIST SAME");
        Plotter::DrawHeader(false, "All channel", "Work in Progress");

        canvas->SaveAs((scorePath + "/score_" + std::to_string(i) + ".pdf").c_str());
        canvas->SaveAs((scorePath + "/score.pdf").c_str());
 
        //Check for early stopping
        if(minLoss >= averagedLoss){
            minLoss = averagedLoss;
            nPatience=0;
        } 

        else nPatience++;

        if(nPatience == 10){
            std::cout << "Training will be closed due to early stopping" << std::endl;
            break;
        }
    }

    //Clean up
    delete canvas;
    delete higgsHist;
    delete topHist;

    //Save model
    if(!frame){
        std::string taggerName = isPhiUp ? "ForPhiDown.pt" : "ForPhiUp.pt";
    
        std::string outName = std::string(std::getenv("CHDIR")) + "/DNN/Model/htagger" + taggerName;
        tagger->to(torch::kCPU);
        torch::save(tagger, outName);
        std::cout << "Model was saved: " << outName << std::endl;
    }

    else frame->AddColumn(hyperParam);
}

int main(int argc, char** argv){
    //Parser arguments
    bool optimize = std::atoi(argv[1]);

    //File names and channels for training
    std::map<std::string, int> fileNames = {
                            {std::string(std::getenv("CHDIR")) + "/Skim/HPlusAndH_ToWHH_ToL4B_200_100/merged/HPlusAndH_ToWHH_ToL4B_200_100.root", 1},

                            {std::string(std::getenv("CHDIR")) + "/Skim/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8/merged/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8.root", 0}                    
    };
    
    std::vector<std::string> channels = {"mu2j1f", "e2j1f", "mu2f", "e2f"};

    //Extract data from TTrees with Treereader class
    std::vector<torch::Tensor> chargedTensors;
    std::vector<torch::Tensor> neutralTensors;
    std::vector<torch::Tensor> SVTensors;

    std::vector<torch::Tensor> phiUp;
    std::vector<torch::Tensor> phiDown;

    std::vector<int> nMax(3, 0);
    int nTotal = 0;

    std::vector<torch::Tensor> tagValues;

    for(std::string& channel: channels){
        int nEvents = !optimize ? 0 : 10000;
        int nFJ = channel.find("2j1f") != std::string::npos ? 1 : 2;

        for(const std::pair<std::string, int>& fileName: fileNames){
            std::cout << "Readout data from file " << Utils::SplitString(fileName.first, "/").back() << " and channel " << channel << std::endl;

            //Get event count of signal and use this also in background
            if(fileName.second and !optimize){
                TFile* file = TFile::Open(fileName.first.c_str());
                TTree* tree = (TTree*)file->Get(channel.c_str());
                nEvents = tree->GetEntries();

                delete tree;
                delete file;
            }

            else if(!fileName.second and !optimize) nEvents *= 2;

            for(int n = 0; n < nFJ; n++){
                std::vector<torch::Tensor> input = HTagger::GatherInput(fileName.first, channel, 0, nEvents, n);
                chargedTensors.push_back(input[0]);
                neutralTensors.push_back(input[1]);
                SVTensors.push_back(input[2]);

                phiUp.push_back(input[3] + torch::full({input[3].size(0)}, nTotal, torch::kLong));
                phiDown.push_back(input[4] + torch::full({input[4].size(0)}, nTotal, torch::kLong));

                nMax[0] = input[0].size(1) > nMax[0] ? input[0].size(1) : nMax[0];
                nMax[1] = input[1].size(1) > nMax[1] ? input[1].size(1) : nMax[1];
                nMax[2] = input[2].size(1) > nMax[2] ? input[2].size(1) : nMax[2];

                tagValues.push_back(torch::from_blob(std::vector<float>(nEvents, fileName.second).data(), {nEvents}).clone());

                nTotal+= nEvents;
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
        Frame* frame = new Frame({"valLoss", "trainLoss", "nHidden", "nLayer", "nConv", "kernelSize", "dropOut", "lr", "batchSize"});

        for(int i=0; i < 1000; i++){
            Train(chargedTensor.index({Up}), neutralTensor.index({Up}), SVTensor.index({Up}), tagValue.index({Up}), true, frame); 

            frame->Sort("valLoss", true);
            frame->ToCsv(std::string(std::getenv("CHDIR")) + "/DNN/HyperTuning/htagger.csv");
        }

        delete frame;
    }
} 
