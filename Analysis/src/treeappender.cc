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

void TreeAppender::Append(const std::string& outName, Parser& parser){
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

    for(const std::string& function: appendFunctions){
        std::map<std::string, std::vector<float>> funcValues;

        if(function == "DNN"){
            std::string dnnDir = parser.GetValue("DNN-base-dir");
            dnnDir = StrUtil::Replace(dnnDir, "{C}", treeName);
            dnnDir = StrUtil::Replace(dnnDir, "{E}", era);

            std::string hyperParam = parser.GetValue("DNN-hyper-param-file");
            hyperParam = StrUtil::Replace(hyperParam, "{C}", treeName);
            hyperParam = StrUtil::Replace(hyperParam, "{E}", era);

            funcValues = Extension::DNNScore(oldT, dnnDir, hyperParam, era);
        }

        else if(function == "HReco"){
            funcValues = Extension::HReconstruction(oldT, era);
        }

        for(const std::pair<std::string, std::vector<float>>& f : funcValues){
            oldT->SetBranchStatus(f.first.c_str(), 0);

            branchNames.push_back(f.first);
            values[f.first] = f.second;  
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
