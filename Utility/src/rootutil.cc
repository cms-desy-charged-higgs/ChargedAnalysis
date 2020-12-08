#include <ChargedAnalysis/Utility/include/rootutil.h>

std::shared_ptr<TFile> RUtil::Open(const std::string& fileName, const std::experimental::source_location& location){
    std::shared_ptr<TFile> file(TFile::Open(fileName.c_str(), "READ"));
    
    //Check if file is not null pointer
    if(file == nullptr){
        throw std::runtime_error(StrUtil::Merge("In file '", location.file_name(), "' in fuction '", location.function_name(), "' in line ", location.line(), "ROOT file not existing: ", fileName));
    }

    //Check if file is not zombie
    if(file->IsZombie()){
        throw std::runtime_error(StrUtil::Merge("In file '", location.file_name(), "' in fuction '", location.function_name(), "' in line ", location.line(), "ROOT file is Zombie: ", fileName));
    }

    return file;
}
