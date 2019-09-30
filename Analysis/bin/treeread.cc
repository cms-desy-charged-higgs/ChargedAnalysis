#include <ChargedAnalysis/Analysis/interface/treereader.h>

std::vector<std::string> SplitString(std::string splitString){
    std::istringstream iss(splitString);
    std::vector<std::string> results(std::istream_iterator<std::string>{iss},
                                 std::istream_iterator<std::string>());

    return results;
}

int main(int argc, char* argv[]){
    //Extract informations of command line
    std::string process = std::string(argv[1]);
    std::vector<std::string> xParameters = SplitString(std::string(argv[2]));
    std::vector<std::string> yParameters = SplitString(std::string(argv[3]));
    std::vector<std::string> cutStrings = SplitString(std::string(argv[4]));
    std::string outname = std::string(argv[5]);
    std::string channel = std::string(argv[6]);
    bool saveTree = std::string(argv[7]) == "True" ? true : false;
    bool saveCsv = std::string(argv[8]) == "True" ? true : false;
    std::vector<std::string> fileNames = SplitString(std::string(argv[9]));
    float eventFraction = std::stof(std::string(argv[10]));

    Py_Initialize();
    //Call and run TreeReader class
    TreeReader reader(process, xParameters, yParameters, cutStrings, outname, channel, saveTree, saveCsv);
    reader.Run(fileNames, eventFraction);
    reader.Merge();



    Py_Finalize();
}
