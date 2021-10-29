/**
* @file dnndataset.cc
* @brief Source file for DNNDataset class, see dnndataset.h
*/

#include <ChargedAnalysis/Network/include/dnndataset.h>
#include <ChargedAnalysis/Utility/include/stopwatch.h>

DNNDataSet::DNNDataSet(const std::vector<std::string>& fileNames, const std::string& channel, const std::vector<std::string>& entryListName, const std::vector<std::string>& parameters, const int& era, torch::Device& device, const int& classLabel, const int& nClass, const std::vector<std::size_t>& chargedMasses, const std::vector<std::size_t>& neutralMasses) :
    fileNames(fileNames),
    channel(channel),
    era(era),
    parameters(parameters),
    device(device),
    classLabel(classLabel), 
    nClass(nClass),
    chargedMasses(chargedMasses),
    neutralMasses(neutralMasses){

    std::vector<std::shared_ptr<CSV>> idx(fileNames.size(), 0);
    std::vector<std::size_t> p(fileNames.size(), 0);

    for(std::size_t i = 0; i < fileNames.size(); ++i){
        idx[i] = std::make_shared<CSV>(entryListName[i], "r", "\t");
    }

    while(true){
        int nFin = 0;

        for(std::size_t i = 0; i < fileNames.size(); ++i){
            if(p[i] >= idx[i]->GetNRows()){
                ++nFin;
                continue;
            }
    
            entryList.push_back({i, idx[i]->Get<int>(p[i], 0)});
            ++p[i];
        }

        if(nFin == fileNames.size()) break;
    }
}

void DNNDataSet::Init(){
    Decoder parser;

    for(std::size_t i = 0; i < fileNames.size(); ++i){
        //Open file/get tree (call this function in local thread)
        files.push_back(RUtil::Open(fileNames.at(i)));
        trees.push_back(RUtil::GetSmart<TTree>(files[i].get(), channel));

        readers.push_back(std::make_shared<NTupleReader>(trees.back(), era));
          
        for(std::size_t p = 0; p < parameters.size(); ++p){
            //Functor structure and arguments
            NTupleFunction function = readers[i]->BuildFunc();
            
            //Read in everything, orders matter
            parser.GetParticle(parameters.at(p), function);
            parser.GetFunction(parameters.at(p), function);
            function.Compile();

            functions.push_back(function);
        }
    }
}

std::size_t DNNDataSet::Size() const{
    return entryList.size();
}

torch::Tensor DNNDataSet::OneHotEncoder(const int& classLabel, const int& nClasses, torch::Device& device){
    torch::Tensor hotLabels = torch::zeros({1, nClasses}).to(device);
    hotLabels.index_put_({0, classLabel}, 1);

    return hotLabels;
}

DNNTensor DNNDataSet::Merge(const std::vector<DNNTensor>& tensors){
    std::vector<torch::Tensor> input(tensors.size()), label(tensors.size()), mHPlus(tensors.size()), mH(tensors.size());
    
    for(std::size_t i = 0; i < input.size(); ++i){
        input[i] = tensors[i].input;
        label[i] = tensors[i].label;
        mHPlus[i] = tensors[i].mHPlus;
        mH[i] = tensors[i].mH;
    }

    return {torch::cat(input, 0), torch::cat(label, 0), torch::cat(mHPlus, 0), torch::cat(mH, 0)};
}

DNNTensor DNNDataSet::Get(const std::size_t& entry){
    std::size_t fIdx, entr;
    std::tie(fIdx, entr) = entryList.at(entry);

    readers.at(fIdx)->SetEntry(entr);
    std::vector<float> paramValues(parameters.size());

    for(std::size_t idx = 0; idx < parameters.size(); ++idx){
        paramValues.at(idx) = functions.at(fIdx*parameters.size() + idx).Get();
        if(std::isnan(paramValues.at(idx))) paramValues.at(idx) = 0;
    }

    return {torch::from_blob(paramValues.data(), {1, paramValues.size()}).clone().to(device), 
            OneHotEncoder(classLabel, nClass, device),
            torch::full({1, 1}, (float)chargedMasses.at(classLabel == nClass - 1 ? fIdx : std::experimental::randint(0, (int)chargedMasses.size() -1))).to(device),
            torch::full({1, 1}, (float)neutralMasses.at(classLabel == nClass - 1 ? fIdx : std::experimental::randint(0, (int)neutralMasses.size() -1))).to(device),
    };
}
