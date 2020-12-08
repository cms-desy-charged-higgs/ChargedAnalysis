/**
* @file treeappender.cc
* @brief Source file for TreeAppender class, see treeappender.h
*/

#include <ChargedAnalysis/Analysis/include/treeappender.h>

TreeAppender::TreeAppender() {}

TreeAppender::TreeAppender(const std::string& fileName, const std::string& treeName, const int& era, const std::vector<std::string>& appendFunctions) :
        fileName(fileName), 
        treeName(treeName),
        era(era),
        appendFunctions(appendFunctions){}

void TreeAppender::Append(const std::string& outName){
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    //Get old Tree
    std::shared_ptr<TFile> oldF = RUtil::Open(fileName);
    std::shared_ptr<TTree> oldT = RUtil::GetSmart<TTree>(oldF.get(), treeName);

    std::cout << "Read file: '" << fileName << "'" << std::endl;
    std::cout << "Read tree '" << treeName << "'" << std::endl;

    //Get values for new branches
    std::map<std::string, TBranch*> branches;
    std::vector<std::string> branchNames;
    std::map<std::string, float> branchValues;
    std::map<std::string, std::vector<float>> values;

    std::map<std::string, std::map<std::string, std::vector<float>>(*)(std::shared_ptr<TFile>&, const std::string&, const int&)> expandFunctions = {
        {"HTagger", &Extension::HScore}, 
        {"DNN", &Extension::DNNScore}, 
        {"HReco", &Extension::HReconstruction}, 
    };

    for(const std::string& function: appendFunctions){
        for(const std::pair<std::string, std::vector<float>>& funcValues : expandFunctions.at(function)(oldF, treeName, era)){
            oldT->SetBranchStatus(funcValues.first.c_str(), 0);

            branchNames.push_back(funcValues.first);
            values[funcValues.first] = funcValues.second;  
        }
    }

    //Clone Tree
    std::shared_ptr<TFile> newF(TFile::Open(outName.c_str(), "RECREATE"));
    std::shared_ptr<TTree> newT(oldT->CloneTree(-1, "fast"));

    for(const std::string& name : branchNames){
        branchValues[name] = -999.;
        branches[name] = newT->Branch(name.c_str(), &branchValues[name]);
    }

    //Fill branches
    for(int i=0; i < oldT->GetEntries(); i++){
        for(std::string& branchName: branchNames){
            branchValues[branchName] = values[branchName][i];       
            branches[branchName]->Fill();
        }
    }

    newF->cd();
    newT->Write();

    std::cout << "Sucessfully append branches " << branchNames << " in tree " << treeName << std::endl;

    TList* keys(oldF->GetListOfKeys());

    for(int i=0; i < keys->GetSize(); i++){
        bool skipKey = false;

        TObject* obj(oldF->Get(keys->At(i)->GetName()));
        if(obj->InheritsFrom(TTree::Class())) continue;

        obj->Write();
    }
}
