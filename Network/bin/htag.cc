#include <torch/torch.h>

#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>

torch::Tensor Merge(std::vector<std::vector<float>>& jetParam, const int& pos){
    int nMax = 0;

    std::vector<torch::Tensor> jetVec;
    
    for(unsigned int i=0; i < jetParam.size(); i++){
        if((i-pos)%3==0){
            int size =  Utils::Ratio(jetParam[i].size(), 7);
            nMax = nMax < size ? size: nMax;
        }
    }

    for(unsigned int i=0+pos; i < jetParam.size(); i++){
        if((i-pos)%3==0){
            int size =  Utils::Ratio(jetParam[i].size(), 7);
            torch::Tensor t = torch::from_blob(jetParam[i].data(), {1, size, 7});
            if (size != nMax) t = torch::constant_pad_nd(t, {0,0, 0, nMax-size}, -999);
            jetVec.push_back(t);
        }
    }

    torch::Tensor tensor = torch::cat(jetVec, 0);

    return tensor;
}


int main(){
    std::map<std::string, int> fileNames = {
                            {std::string(std::getenv("CHDIR")) + "/Skim/HPlusAndH_ToWHH_ToL4B_200_100/merged/HPlusAndH_ToWHH_ToL4B_200_100.root", 1},

                            {std::string(std::getenv("CHDIR")) + "/Skim/TTJets_SingleLeptFromT_TuneCP5_13TeV-madgraphMLM-pythia8/merged/TTJets_SingleLeptFromT_TuneCP5_13TeV-madgraphMLM-pythia8.root", 0}                    
    };

    std::vector<std::vector<float>> jetParam;
    std::vector<float> tagValue;
    int nEvents = 500;

    //Create treereader instance
    for(std::pair<std::string, int> fileName: fileNames){
        TreeReader reader("", {"*_fj1"}, {}, {}, "", "e2f", "Tensor");
        reader.EventLoop(fileName.first, 0, nEvents, &jetParam);

        for(unsigned int i=0; i < nEvents; i++){
            tagValue.push_back(fileName.second);
        }
    }

    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);

    torch::Tensor randIndex = torch::randperm((int)tagValue.size(), torch::TensorOptions().dtype(torch::kLong)).to(device);

    torch::Tensor target = torch::from_blob(tagValue.data(), {(int)tagValue.size(), 1}).to(device).index_select(0, randIndex);
    torch::Tensor chargedTensor = Merge(jetParam, 0).to(device).index_select(0, randIndex);
    torch::Tensor neutralTensor = Merge(jetParam, 1).to(device).index_select(0, randIndex);
    torch::Tensor SVTensor = Merge(jetParam, 2).to(device).index_select(0, randIndex);

    HTagger tagger(7, 20, 3, 10, 5);
    tagger.to(device);

    torch::optim::Adam optimizer(tagger.parameters(), /*lr=*/0.001);

    int batchSize = 128;
    int nEpochs = 20;
    int nBatches = std::ceil(tagValue.size()/batchSize);

    for(int i=0; i<nEpochs; i++){
        for(int j=0; j < nBatches; j++){
            std::cout << "Train batch " << j+1 << " of " << nBatches << std::endl;

            torch::Tensor chargedInput = chargedTensor.narrow(0, batchSize*j, batchSize);  
            torch::Tensor neutralInput = neutralTensor.narrow(0, batchSize*j, batchSize); 
            torch::Tensor SVInput = SVTensor.narrow(0, batchSize*j, batchSize); 
            torch::Tensor labels = target.narrow(0, batchSize*j, batchSize);

            optimizer.zero_grad();
            torch::Tensor prediction = tagger.forward(chargedInput, neutralInput, SVInput);
            torch::Tensor loss = torch::binary_cross_entropy(prediction, labels).to(device);

            loss.backward();
            optimizer.step();

            if(j+1==nBatches) std::cout << loss << std::endl;
        }
    }
} 
