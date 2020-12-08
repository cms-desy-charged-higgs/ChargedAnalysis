/**
* @file dnndataset.cc
* @brief Source file for DNNDataset class, see dnndataset.h
*/

#include <ChargedAnalysis/Network/include/dnndataset.h>

DNNDataset::DNNDataset(std::shared_ptr<TFile>& inFile, const std::string& treeName, const std::vector<std::string>& parameters, const std::vector<std::string>& cutNames, const std::string& cleanJet, torch::Device& device, const int& classLabel) :
    device(device),
    classLabel(classLabel){

    TreeParser parser;

    std::vector<std::string> cleanInfo = StrUtil::Split(cleanJet, "/");

    for(const std::string& parameter: parameters){
        //Functor structure and arguments
        TreeFunction function(inFile, treeName);
        
        //Read in everything, orders matter
        parser.GetParticle(parameter, function);
        parser.GetFunction(parameter, function);

        if(cleanInfo.size() != 1){
            function.SetCleanJet<Axis::X>(cleanInfo[0], cleanInfo[1]);    
        }
        
        functions.push_back(function);
    }

    for(const std::string& cutName: cutNames){
        //Functor structure and arguments
        TreeFunction cut(inFile, treeName);
        
        //Read in everything, orders matter
        parser.GetParticle(cutName, cut);
        parser.GetFunction(cutName, cut);
        parser.GetCut(cutName, cut);

        if(cleanInfo.size() != 1){
            cut.SetCleanJet<Axis::X>(cleanInfo[0], cleanInfo[1]);    
        }
        
        cuts.push_back(cut);
    }

    for(int i = 0; i < inFile->Get<TTree>(treeName.c_str())->GetEntries(); i++){
        TreeFunction::SetEntry(i);
 
        bool passed=true;

        for(TreeFunction& cut : cuts){
            passed = passed && cut.GetPassed();

            if(!passed) break;
        }

        if(passed){
            trueIndex.push_back(i);
            nEntries++; 
        }
    }
}

torch::optional<size_t> DNNDataset::size() const {
    return nEntries;
}
        
DNNTensor DNNDataset::get(size_t index){
    int entry = trueIndex[index];

    TreeFunction::SetEntry(entry);
    std::vector<float> paramValues;

    for(TreeFunction& func : functions){
        paramValues.push_back(func.Get<Axis::X>());
    }
    
    return {torch::from_blob(paramValues.data(), {1, paramValues.size()}).clone().to(device), torch::tensor({classLabel}).to(device)};
}

DNNTensor DNNDataset::Merge(std::vector<DNNTensor>& tensors){
    std::vector<torch::Tensor> input, label; 

    for(DNNTensor& tensor: tensors){
        input.push_back(tensor.input);
        label.push_back(tensor.label);
    }

    return {torch::cat(input, 0), torch::cat(label, 0)};
}
