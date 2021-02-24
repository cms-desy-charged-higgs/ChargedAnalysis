/**
* @file dnndataset.cc
* @brief Source file for DNNDataset class, see dnndataset.h
*/

#include <ChargedAnalysis/Network/include/dnndataset.h>

DNNDataset::DNNDataset(std::shared_ptr<TTree>& tree, const std::vector<std::string>& parameters, const std::vector<std::string>& cutNames, const int& era, const bool& isEven, torch::Device& device, const int& classLabel) :
    device(device),
    classLabel(classLabel){

    Decoder parser;

    for(const std::string& parameter: parameters){
        //Functor structure and arguments
        NTupleReader function(tree, era);
        
        //Read in everything, orders matter
        parser.GetParticle(parameter, function);
        parser.GetFunction(parameter, function);
        function.Compile();

        functions.push_back(function);
    }

    for(const std::string& cutName: cutNames){
        //Functor structure and arguments
        NTupleReader cut(tree, era);
        
        //Read in everything, orders matter
        parser.GetParticle(cutName, cut);
        parser.GetFunction(cutName, cut);
        parser.GetCut(cutName, cut);
        cut.Compile();
        
        cuts.push_back(cut);
    }

    TLeaf* evNr = RUtil::Get<TLeaf>(tree.get(), "Misc_eventNumber");

    for(int i = 0; i < tree->GetEntries(); ++i){
        NTupleReader::SetEntry(i);

        if(int(1/RUtil::GetEntry<float>(evNr, i)*10e10) % 2 == isEven) continue; 
 
        bool passed=true;

        for(NTupleReader& cut : cuts){
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

    NTupleReader::SetEntry(entry);
    std::vector<float> paramValues;

    for(NTupleReader& func : functions){
        paramValues.push_back(func.Get());
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
