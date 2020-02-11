#include <ChargedAnalysis/Analysis/include/treeappender.h>

TreeAppender::TreeAppender() {}

TreeAppender::TreeAppender(const std::string& oldFile, const std::string& oldTree, const std::string& newFile, const std::vector<std::string>& branchNames, const int& entryStart, const int& entryEnd) :
        oldFile(oldFile), 
        oldTree(oldTree),
        newFile(newFile),
        branchNames(branchNames),
        entryStart(entryStart), 
        entryEnd(entryEnd) {}

std::vector<float> TreeAppender::HScore(const int& FJindex){
    //Vector with final score (FatJetIndex/Score per event)
    std::vector<float> tagValues(entryEnd-entryStart, -999.);

    //Tagger
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    std::vector<std::shared_ptr<HTagger>> tagger(2, std::make_shared<HTagger>(7, 140, 1, 130, 57, 0.06));

    torch::load(tagger[0], std::string(std::getenv("CHDIR")) + "/DNN/Model/Even/htagger.pt");
    torch::load(tagger[1], std::string(std::getenv("CHDIR")) + "/DNN/Model/Odd/htagger.pt");

    tagger[0]->to(device);
    tagger[1]->to(device);

    //Get data set
    HTagDataset data = HTagDataset({oldFile}, {oldTree}, FJindex, device, true);
    std::vector<int> entries;

    for(int i = entryStart; i < entryEnd; i++){entries.push_back(i);}
    std::vector<int>::iterator entry = entries.begin();
    int batchSize = entryEnd - entryStart > 2500 ? 2500 : entryEnd - entryStart;

    //For right indexing
    std::vector<int> evenIndex, oddIndex;
    int counter = 0;
    
    while(entry != entries.end()){
        //Put signal + background in one vector and split by even or odd numbered event
        std::vector<HTensor> evenTensors;
        std::vector<HTensor> oddTensors;

        for(int i = 0; i < batchSize; i++){
            HTensor tensor = data.get(*entry);

            if(tensor.isEven.item<bool>() == true){
                evenTensors.push_back(tensor);
                evenIndex.push_back(counter);
            }

            else{
                oddTensors.push_back(tensor);
                oddIndex.push_back(counter);
            }

            counter++;
            ++entry;
            if(entry == entries.end()) break; 
        }

        //Prediction
        torch::NoGradGuard no_grad;
        HTensor even, odd; 
        torch::Tensor evenPredict, oddPredict;        

        if(evenTensors.size() != 0){    
            even = HTagDataset::PadAndMerge(evenTensors);
            evenPredict = tagger[1]->forward(even.charged, even.neutral, even.SV);
        }  

        if(oddTensors.size() != 0){    
            odd = HTagDataset::PadAndMerge(oddTensors);
            oddPredict = tagger[0]->forward(odd.charged, odd.neutral, odd.SV);
        }     

        //Put all predictions back in order again
        for(int j = 0; j < evenTensors.size(); j++){
            tagValues[evenIndex[j]] = evenPredict[j].item<float>();
        }

        for(int j = 0; j < oddTensors.size(); j++){
            tagValues[oddIndex[j]] = oddPredict[j].item<float>();
        }
    }

    return tagValues;
}

void TreeAppender::Append(){
    //Get old Tree
    TFile* oldF = TFile::Open(oldFile.c_str(), "READ");
    TTree* oldT = (TTree*)oldF->Get(oldTree.c_str());

    //Disables wished branches if they are already existing in old tree
    for(std::string& branchName: branchNames){
        if(oldT->GetListOfBranches()->Contains(branchName.c_str())){
            oldT->SetBranchStatus(branchName.c_str(), 0);
        }
    }

    //Clone Tree
    TFile* newF = TFile::Open(newFile.c_str(), "RECREATE");
    TTree* newT = oldT->CloneTree(0);
    newT->SetDirectory(newF);

    for(int i = entryStart; i < entryEnd; i++){
        oldT->GetEntry(i);
        newT->Fill();
    }

    if(false){
        return;
    }

    delete oldT;
    delete oldF;

    //Set new branches
    std::vector<float> branchValues(branchNames.size(), -999.);
    std::vector<std::vector<float>> values;
    std::vector<TBranch*> branches;

    for(unsigned int i=0; i < branchNames.size(); i++){
        if(branchNames[i] == "ML_HTagFJ1") values.push_back(HScore(0));
        if(branchNames[i] == "ML_HTagFJ2") values.push_back(HScore(1));

        TBranch* branch = newT->Branch(branchNames[i].c_str(), &branchValues[i]);
        branches.push_back(branch);
    }

    //Fill branches
    for(int i=0; i < newT->GetEntries(); i++){
        for(int j=0; j < branches.size(); j++){
            branchValues[j] = values[j][i];
            branches[j]->Fill();
        }
    }

    newF->cd();
    newT->Write();

    std::cout << "Sucessfully append branches " << branchNames << " in tree " << oldTree << " from range " << entryStart << " to " << entryEnd << " in file " << newFile << std::endl;    
    
    delete newT;
    delete newF;
}
