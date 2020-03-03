#include <TFile.h>
#include <TTree.h>
#include <TROOT.h>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/utils.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string oldFile = parser.GetValue<std::string>("old-file");
    std::string newFile = parser.GetValue<std::string>("new-file");
    std::vector<std::string> skipObj = parser.GetVector<std::string>("skip-objs");

    TFile* oldF = TFile::Open(oldFile.c_str(), "READ");
    TFile* newF = TFile::Open(newFile.c_str(), "RECREATE");

    TList* keys = oldF->GetListOfKeys();

    for(int i=0; i < keys->GetSize(); i++){
        bool skipKey = false;

        for(std::string& skip: skipObj){
            if(Utils::Find<std::string>(skip, std::string(keys->At(i)->GetName())) != -1){
                skipKey = true;
                break;
            }
        }

        if(skipKey) continue;

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
