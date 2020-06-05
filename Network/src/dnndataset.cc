#include <ChargedAnalysis/Network/include/dnndataset.h>

DNNDataset::DNNDataset(const std::vector<std::string>& files, torch::Device& device, const bool& isSignal) :
    device(device),
    isSignal(isSignal){

    for(const std::string& file: files){
        std::ifstream myfile(file);

        myfile.unsetf(std::ios_base::skipws);

        nLines += std::count(
            std::istreambuf_iterator<char>(myfile),
            std::istreambuf_iterator<char>(), 
            '\n'); 

        myfile.close();

        std::string header;
        std::ifstream* myFile = new std::ifstream(file, std::ios::in);
        std::getline(*myFile, header);

        this->files.push_back(myFile);
    }

    fileIndex = 0;
}

torch::optional<size_t> DNNDataset::size() const {
    return nLines;
}
        
DNNTensor DNNDataset::get(size_t index){
    std::string line;

    if(!std::getline(*files[fileIndex], line)){
        files[fileIndex]->clear();
        files[fileIndex]->seekg(0, std::ios::beg);
        fileIndex;

        if(fileIndex == files.size()-1) fileIndex = 0;

        std::getline(*files[fileIndex], line);
        std::getline(*files[fileIndex], line);
    }

    std::vector<float> paramValues = Utils::SplitString<float>(line, "\t");

    return {torch::from_blob(paramValues.data(), {1, paramValues.size()}).clone().to(device), torch::tensor({float(isSignal)}).to(device)};
}

DNNTensor DNNDataset::Merge(std::vector<DNNTensor>& tensors){
    std::vector<torch::Tensor> input, label; 

    for(DNNTensor& tensor: tensors){
        input.push_back(tensor.input);
        label.push_back(tensor.label);
    }

    return {torch::cat(input, 0), torch::cat(label, 0)};
}
