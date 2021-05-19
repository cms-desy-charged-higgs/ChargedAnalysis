#include <memory>
#include <filesystem>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

#include <TFileMerger.h>
#include <TFile.h>
#include <TTree.h>

void Merge(const std::vector<std::string>& inFiles, const std::string& outFile, const std::vector<std::string>& excludeObjects){
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
    merger.SetFastMethod(true);
    merger.SetPrintLevel(99);

    std::vector<std::shared_ptr<TFile>> files;
    std::vector<TObject*> toIncludeLater;

    for(const std::string file: inFiles){
        files.push_back(RUtil::Open(file));
        merger.AddFile(files.back().get());

        //Get objects which should not be merged from first file
        if(files.size() == 1){
            for(const std::string& objName : excludeObjects){
                if(files.back()->Get(objName.c_str()) == nullptr){
                    std::cout << StrUtil::Join(" ", "Object with name", objName, "not existing! It will be skipped") << std::endl;
                    continue;
                }

                toIncludeLater.push_back(files.back()->Get(objName.c_str()));
            }
        }
    }

    merger.OutputFile(outFile.c_str(), "RECREATE");
    merger.Merge();

    std::unique_ptr<TFile> out(TFile::Open(outFile.c_str(), "UPDATE")); 

    for(TObject* obj : toIncludeLater){
        out->Delete(StrUtil::Merge(obj->GetName(), ";*").c_str());
        obj->Write();
        obj->Print();
    }

    out->Close();
}

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> inFiles = parser.GetVector<std::string>("input-files");
    std::string outFile = parser.GetValue<std::string>("out-file");
    std::vector<std::string> excludeObjects = parser.GetVector<std::string>("exclude-objects");
    bool deleteInput = parser.GetValue<bool>("delete-input");

    Merge(inFiles, outFile, excludeObjects);

    if(deleteInput) std::system(StrUtil::Merge("rm -rfv ", inFiles).c_str());
}
