#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/csv.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    Parser parser(argc, argv);

    std::vector<std::string> inFiles = parser.GetVector<std::string>("input-files");
    std::string outFile = parser.GetValue<std::string>("out-file");

    CSV::Merge(outFile, inFiles);
}
