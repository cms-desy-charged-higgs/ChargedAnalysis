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
    std::string era = parser.GetValue<std::string>("era", "2017");

    //Call and run plotter class
    Plotter1D plotter1D(histDir, channel, processes, era);
    plotter1D.ConfigureHists();
    plotter1D.Draw(outDirs);

    Plotter2D plotter2D(histDir, channel, processes, era);
    plotter2D.ConfigureHists();
    plotter2D.Draw(outDirs);
}
