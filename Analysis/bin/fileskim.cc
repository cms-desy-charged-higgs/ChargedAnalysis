#include <TFile.h>
#include <TTree.h>
#include <TROOT.h>

#include <ChargedAnalysis/Analysis/include/utils.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    std::string oldFile = std::string(argv[1]);
    std::string newFile = std::string(argv[2]);
    std::vector<std::string> skipObj = Utils::SplitString(std::string(argv[3]), " ");

    TFile* oldF = TFile::Open(oldFile.c_str(), "READ");
    TFile* newF = TFile::Open(newFile.c_str(), "RECREATE");

    TList* keys = oldF->GetListOfKeys();

    for(int i=0; i < keys->GetSize(); i++){
        if(Utils::FindInVec(skipObj, std::string(keys->At(i)->GetName())) != -1) continue;

        TObject* obj = oldF->Get(keys->At(i)->GetName());

        if(obj->InheritsFrom(TTree::Class())){
            TTree* oldTree = (TTree*)obj;
            TTree* newTree = oldTree->CloneTree();

            newTree->Write();
            
            delete obj;
            delete newTree;
        }

        else{
            obj->Write();
            delete obj;
        }
    }

    delete oldF;
    delete newF;
}
