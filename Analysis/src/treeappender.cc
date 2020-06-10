#include <ChargedAnalysis/Analysis/include/treeappender.h>

TreeAppender::TreeAppender() {}

TreeAppender::TreeAppender(const std::string& fileName, const std::string& treeName, const std::vector<std::string>& appendFunctions, const std::string& dCacheDir) :
        fileName(fileName), 
        treeName(treeName),
        appendFunctions(appendFunctions),
        dCacheDir(dCacheDir){}

void TreeAppender::Append(){
    //Get old Tree
    TFile* oldF = TFile::Open(fileName.c_str(), "READ");
    TTree* oldT = (TTree*)oldF->Get(treeName.c_str());

    std::cout << "Read file: '" << fileName << "'" << std::endl;
    std::cout << "Read tree '" << treeName << "'" << std::endl;

    //Clone Tree
    std::string tmpFile = "/tmp/tmp_" + std::to_string(getpid()) + ".root";

    TFile* newF = TFile::Open(tmpFile.c_str(), "RECREATE");
    TTree* newT = oldT->CloneTree(-1, "fast");;

    //Set new branches
    std::map<std::string, TBranch*> branches;
    std::vector<std::string> branchNames;
    std::map<std::string, float> branchValues;
    std::map<std::string, std::vector<float>> values;

    std::map<std::string, std::map<std::string, std::vector<float>>(*)(TFile*, const std::string&)> expandFunctions = {
        {"HTagger", &Extension::HScore}, 
        {"DNN", &Extension::DNNScore}, 
        {"HReco", &Extension::HReconstruction}, 
    };

    for(const std::string& function: appendFunctions){
        for(const std::pair<std::string, std::vector<float>>& funcValues : expandFunctions.at(function)(oldF, treeName)){
            oldT->SetBranchStatus(funcValues.first.c_str(), 0);

            branchValues[funcValues.first] = -999.;
            branches[funcValues.first] = newT->Branch(funcValues.first.c_str(), &branchValues[funcValues.first]);
            branchNames.push_back(funcValues.first);

            values[funcValues.first] = funcValues.second;  
        }
    }

    //Fill branches
    for(int i=0; i < oldT->GetEntries(); i++){
        if(i % 100000 == 0 and i != 0){
            std::cout << "Processed events: " << i << std::endl;
        }

        for(std::string& branchName: branchNames){
            branchValues[branchName] = values[branchName][i];       
            branches[branchName]->Fill();
        }
    }

    newF->cd();
    newT->Write();

    std::cout << "Sucessfully append branches " << branchNames << " in tree " << treeName << std::endl;

    TList* keys = oldF->GetListOfKeys();

    for(int i=0; i < keys->GetSize(); i++){
        bool skipKey = false;

        TObject* obj = oldF->Get(keys->At(i)->GetName());
        if(obj->InheritsFrom(TTree::Class())) continue;

        obj->Write();
    }
    

    delete oldT;
    delete oldF;
    delete newT;
    delete newF;

    std::system(("mv -vf " + tmpFile + " " + fileName).c_str());

    if(dCacheDir != "") Utils::CopyToCache(fileName, dCacheDir);
}
