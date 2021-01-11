#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/plotter1D.h>
#include <ChargedAnalysis/Analysis/include/plotter2D.h>

int main(int argc, char *argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string era = parser.GetValue<std::string>("era", "2017");
    std::string channel = parser.GetValue<std::string>("channel");
    std::vector<std::string> bkgProcesses = parser.GetVector<std::string>("bkg-processes", {});
    std::vector<std::string> bkgFiles = parser.GetVector<std::string>("bkg-files", {});
    std::vector<std::string> sigProcesses = parser.GetVector<std::string>("sig-processes", {});
    std::vector<std::string> sigFiles = parser.GetVector<std::string>("sig-files", {});
    std::string data = parser.GetValue<std::string>("data", "");
    std::string dataFile = parser.GetValue<std::string>("data-file", "");
    std::vector<std::string> outDirs = parser.GetVector<std::string>("out-dirs");

    //Call and run plotter class
    Plotter1D plotter1D(channel, era, bkgProcesses, bkgFiles, sigProcesses, sigFiles, data, dataFile);
    plotter1D.ConfigureHists();
    plotter1D.Draw(outDirs);

  //  Plotter2D plotter2D(histDir, channel, processes, era);
  //  plotter2D.ConfigureHists();
  //  plotter2D.Draw(outDirs);
}
