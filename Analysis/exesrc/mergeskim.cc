#include <memory>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/utils.h>

#include <TFileMerger.h>
#include <TFile.h>
#include <TTree.h>

void Merge(const std::vector<std::string> inFiles, const std::string outFile, const bool& optimize){
    TTree::SetMaxTreeSize(1000000000000);

    //Remove outfile if it is already there
    std::system(StrUtil::Merge("rm -rfv ", outFile).c_str());

    //Call hadd
    TFileMerger merger(false);
    merger.SetFastMethod(!optimize);
    merger.SetPrintLevel(99);

    for(const std::string file: inFiles){
        merger.AddFile(file.c_str());
    }

    merger.OutputFile(outFile.c_str());
    merger.Merge();

    //Objects which not be hadded and transferred from first input file to output file
    std::vector<std::string> notMerge = {"Lumi", "xSec", "pileUp", "pileUpUp", "pileUpDown"};

    std::unique_ptr<TFile> in(TFile::Open(inFiles[0].c_str(), "READ"));
    std::unique_ptr<TFile> out(TFile::Open(outFile.c_str(), "UPDATE"));

    for(const std::string& objName : notMerge){
        if(out->Get(objName.c_str()) == nullptr) continue;

        out->Delete((objName + ";*").c_str());
        in->Get(objName.c_str())->Write();
    }
}

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> inFiles = parser.GetVector<std::string>("input-files");
    std::string outFile = parser.GetValue<std::string>("out-file");
    std::string dCache = parser.GetValue<std::string>("dCache", "");
    bool optimize = parser.GetValue<bool>("optimize");
    
    Merge(inFiles, outFile, optimize);

    if(!dCache.empty()){
        Utils::CopyToCache(outFile, dCache);
        std::system(StrUtil::Merge("rm -rfv ", inFiles).c_str());
    }
}
