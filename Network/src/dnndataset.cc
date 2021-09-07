/**
* @file dnndataset.cc
* @brief Source file for DNNDataset class, see dnndataset.h
*/

#include <ChargedAnalysis/Network/include/dnndataset.h>

DNNDataSet::DNNDataSet(const std::string& fileName, const std::string& channel, const std::string& entryListName, const std::vector<std::string>& parameters, const int& era, torch::Device& device, const int& classLabel) :
    fileName(fileName),
    channel(channel),
    era(era),
    parameters(parameters),
    device(device),
    classLabel(classLabel){

    CSV idx(entryListName, "r", ",");

    entryList = std::vector<std::size_t>(idx.GetNRows());

    for(int row = 0; row < idx.GetNRows(); ++row){
        entryList[row] = idx.Get<int>(row, 0);
    }
}

void DNNDataSet::Init(){
    //Open file/get tree (call this function in local thread)
    file = RUtil::Open(fileName);
    tree = RUtil::GetSmart<TTree>(file.get(), channel);

    Decoder parser;
    functions = std::vector<NTupleReader>(parameters.size());
    cache = std::make_shared<NCache>();

    for(std::size_t idx = 0; idx < parameters.size(); ++idx){
        //Functor structure and arguments
        NTupleReader function(tree, era, cache);
        
        //Read in everything, orders matter
        parser.GetParticle(parameters.at(idx), function);
        parser.GetFunction(parameters.at(idx), function);
        function.Compile();

        functions[idx] = function;
    }
}

std::size_t DNNDataSet::Size(){
    return entryList.size();
}

DNNTensor DNNDataSet::Get(const std::size_t& entry){
    cache->clear();
    std::vector<float> paramValues(parameters.size());

    for(std::size_t idx = 0; idx < functions.size(); ++idx){
        paramValues.at(idx) = functions.at(idx).Get(entryList.at(entry));
    }

    return {torch::from_blob(paramValues.data(), {1, paramValues.size()}).clone().to(device), torch::tensor({classLabel}).to(device)};
}

std::vector<DNNTensor> DNNDataSet::GetBatch(const std::size_t& entryStart, const std::size_t& entryEnd){
    std::vector<DNNTensor> batch(entryEnd - entryStart);

    for(std::size_t entry = entryStart, idx = 0; entry < entryEnd; ++entry, ++idx){
        batch.at(idx) = Get(entry);
    }

    return batch;
}

DNNTensor DNNDataSet::Merge(const std::vector<DNNTensor>& tensors){
    std::vector<torch::Tensor> input(tensors.size()), label(tensors.size()); 

    for(std::size_t idx = 0; idx < tensors.size(); ++idx){
        input[idx] = tensors[idx].input;
        label[idx] = tensors[idx].label;
    }

    return {torch::cat(input, 0), torch::cat(label, 0)};
}
