#include <filesystem>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/frame.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    Parser parser(argc, argv);

    std::vector<std::string> inFiles = parser.GetVector<std::string>("input");
    std::string outName = parser.GetValue<std::string>("output");

    Frame frame(inFiles);

    frame.WriteCSV(outName);
}
