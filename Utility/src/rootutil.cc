#include <ChargedAnalysis/Utility/include/rootutil.h>

std::shared_ptr<TFile> RUtil::Open(const std::string& fileName, const std::experimental::source_location& location){
    std::shared_ptr<TFile> file(TFile::Open(fileName.c_str(), "READ"));
    
    //Check if file is not null pointer
    if(file == nullptr){
        throw std::runtime_error(StrUtil::PrettyError(location, "ROOT file not existing: ", fileName));
    }

    //Check if file is not zombie
    if(file->IsZombie()){
        throw std::runtime_error(StrUtil::PrettyError(location, "ROOT file is Zombie: ", fileName));
    }

    return file;
}

bool RUtil::BranchExists(TTree* tree, const std::string& branchName, const std::experimental::source_location& location){
    if(tree->GetListOfBranches()->FindObject(branchName.c_str())) return true;
    return false;
}

std::vector<std::string> RUtil::ListOfContent(TDirectory* f, const std::experimental::source_location& location){
    std::vector<std::string> content;

    for(TObject* key : *(f->GetListOfKeys())){
        std::string objName = key->GetName();

        if(f->Get(objName.c_str())->InheritsFrom(TDirectory::Class())){
            std::vector<std::string> subContent = ListOfContent(RUtil::Get<TDirectory>(f, objName));
            content = VUtil::Merge(content, subContent);
        }

        content.push_back(objName);
    }

    return content;
}

int RUtil::GetLen(TLeaf* leaf, const int& entry){
    if(leaf->GetBranch()->GetReadEntry() != entry) leaf->GetBranch()->GetEntry(entry);
    return leaf->GetLen();
}
