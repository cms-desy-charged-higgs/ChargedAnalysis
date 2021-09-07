#include <memory>
#include <filesystem>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

#include <TFileMerger.h>
#include <TFile.h>
#include <TTree.h>

void Merge(const std::vector<std::string>& inFiles, const std::string& outFile, const bool& optimize, const std::vector<std::string>& excludeObjects){
    //Set Max Tree Size to 1 TB
    TTree::SetMaxTreeSize(1000000000000);

    //Remove outfile if it is already there
    std::system(StrUtil::Merge("rm -rfv ", outFile).c_str());

    //Create output directory if needed
    if(!StrUtil::Find(outFile, "/").empty()){
        std::filesystem::create_directories(outFile.substr(0, StrUtil::Find(outFile, "/").back()));
    }
    
    //Copy only with one file
    if(inFiles.size() == 1){
        std::system(StrUtil::Merge("cp -fv ", inFiles[0], " ", outFile).c_str());
        
        return;
    }
    
    //Set file merger
    TFileMerger merger(false);
    merger.SetFastMethod(!optimize);
    merger.SetPrintLevel(99);

    for(const std::string file: inFiles) merger.AddFile(file.c_str());

    merger.OutputFile(outFile.c_str(), "RECREATE");
    merger.Merge();

    std::unique_ptr<TFile> in(TFile::Open(inFiles[0].c_str(), "READ")); 
    std::unique_ptr<TFile> out(TFile::Open(outFile.c_str(), "UPDATE")); 

    for(const std::string& objName : excludeObjects){
        TObject* obj = in->Get(objName.c_str());

        if(obj == nullptr){
            std::cout << StrUtil::Join(" ", "Object with name", objName, "not existing! It will be skipped") << std::endl;
            continue;
        }

        out->Delete(StrUtil::Merge(obj->GetName(), ";*").c_str());
        obj->Write();
    }

    out->Close();
}

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> inFiles = parser.GetVector<std::string>("input-files");
    std::string outFile = parser.GetValue<std::string>("out-file");
    std::vector<std::string> excludeObjects = parser.GetVector<std::string>("exclude-objects");
    bool optimize = parser.GetValue<bool>("optimize");
    bool deleteInput = parser.GetValue<bool>("delete-input");

    Merge(inFiles, outFile, optimize, excludeObjects);

    if(deleteInput) std::system(StrUtil::Merge("rm -rfv ", inFiles).c_str());
}
