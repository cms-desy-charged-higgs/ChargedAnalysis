#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/plotter1D.h>
#include <ChargedAnalysis/Analysis/include/plotter2D.h>

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string histDir =  parser.GetValue<std::string>("hist-dir");
    std::string channel = parser.GetValue<std::string>("channel");
    std::vector<std::string> processes = parser.GetVector<std::string>("processes");
    std::vector<std::string> outDirs = parser.GetVector<std::string>("out-dirs");

    //Call and run Plotter1D class
    Plotter1D plotter1D(histDir, channel, processes);
    plotter1D.ConfigureHists();
    plotter1D.Draw(outDirs);
}
