#include <string>
#include <vector>

#include <ChargedAnalysis/Analysis/include/ntuplereader.h>
#include <ChargedAnalysis/Analysis/include/decoder.h>
#include <ChargedAnalysis/Utility/include/backtracer.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>
#include <ChargedAnalysis/Utility/include/csv.h>

void Loop(const std::string& fileName, const std::string& channel, const int& era, const std::vector<std::string> regions, const std::map<std::string, std::string>& outNames, const std::map<std::string, std::vector<std::string>>& cutStrings, const int& entryStart, const int& entryEnd){
    Decoder parser;

    std::shared_ptr<TFile> inFile = RUtil::Open(fileName);
    std::shared_ptr<TTree> inTree = RUtil::GetSmart<TTree>(inFile.get(), channel);

    NTupleReader reader(inTree, era);
    std::vector<NTupleFunction> cuts;
    int nCuts = 0; std::vector<int> cutIdxRange;

    std::vector<std::unique_ptr<CSV>> outFiles;
    for(const std::string& region : regions){
        outFiles.push_back(std::make_unique<CSV>(outNames.at(region), "w", std::vector<std::string>{"Index"}));
    
        std::cout << std::endl << "Cuts for region '" << region << "'" << std::endl;

        nCuts = 0;

        for(const std::string& cutString: cutStrings.at(region)){
            //Functor structure and arguments
            NTupleFunction cut = reader.BuildFunc();
                
            parser.GetParticle(cutString, cut);
            parser.GetFunction(cutString, cut);
            parser.GetCut(cutString, cut);
            cut.Compile();

            cuts.push_back(cut);
            ++nCuts;

            std::cout << "Cut will be applied: '" << cut.GetCutName() << "'" << std::endl;
        }

        cutIdxRange.push_back(nCuts + (cutIdxRange.size() == 0 ? 0 : cutIdxRange.back()));
    }

    bool passed = true;

    for(int entry = entryStart; entry < entryEnd; ++entry){
        reader.SetEntry(entry);

        for(int region = 0; region < regions.size(); ++region){
            passed = true;

            for(unsigned int j = region == 0 ? 0 : cutIdxRange[region-1]; j < cutIdxRange[region]; ++j){
                passed = passed and cuts[j].GetPassed();
                if(!passed) break;
            }

            if(passed) outFiles[region]->WriteRow(entry);
        }
    }
}

int main(int argc, char *argv[]){
    //Class for backtrace
    //Backtracer trace;

    //Parser arguments
    Parser parser(argc, argv);

    std::string fileName = parser.GetValue<std::string>("filename");
    std::string channel = parser.GetValue<std::string>("channel");
    std::string outFile = parser.GetValue<std::string>("out-file");

    int era = parser.GetValue<int>("era", 2017);
    int eventStart = parser.GetValue<int>("event-start");
    int eventEnd = parser.GetValue<int>("event-end");

    std::vector<std::string> regions = parser.GetVector("regions");

    std::map<std::string, std::string> outDir;
    std::map<std::string, std::vector<std::string>> cuts;

    for(const std::string& region : regions){
        outDir[region] = parser.GetValue(region + "-out-dir") + "/" + outFile;
        cuts[region] = parser.GetVector(region + "-cuts");
    }

    Loop(fileName, channel, era, regions, outDir, cuts, eventStart, eventEnd);
}
