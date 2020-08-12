#include <memory>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/utils.h>

#include <TFile.h>

void Merge(const std::vector<std::string> inFiles, const std::string outFile, const bool& optimize){
    //Remove outfile if it is already there
    std::system(StrUtil::Merge("rm -rfv ", outFile).c_str());

    //Call hadd
    std::system(StrUtil::Merge("hadd -f ", optimize ? " -O " : "", outFile, " ", inFiles).c_str());

    //Objects which not be hadded and transferred from first input file to output file
    std::vector<std::string> notMerge = {"Lumi", "xSec", "pileUp", "pileUpUp", "pileUpDown"};

    std::unique_ptr<TFile> in(TFile::Open(inFiles[0].c_str(), "READ"));
    std::unique_ptr<TFile> out(TFile::Open(outFile.c_str(), "UPDATE"));

    for(const std::string& objName : notMerge){
        if(out->Get(objName.c_str()) == nullptr) continue;

        out->Delete((objName + ";*").c_str());
        in->Get(objName.c_str())->Write();
    }

    if(!optimize){
        std::system(StrUtil::Merge("rm -rfv", inFiles).c_str());
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

    if(dCache != "") Utils::CopyToCache(outFile, dCache);
}
