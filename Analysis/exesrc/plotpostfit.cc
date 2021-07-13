#include <string>
#include <vector>

#include <ChargedAnalysis/Analysis/include/plotterPostfit.h>
#include <ChargedAnalysis/Utility/include/parser.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string inFile = parser.GetValue<std::string>("in-file");
    std::string sigProcess = parser.GetValue<std::string>("sig-process");
    std::vector<std::string> bkgProcesses = parser.GetVector<std::string>("bkg-processes");
    std::vector<std::string> outDirs = parser.GetVector<std::string>("out-dirs");
  
    //Call and run Plotter1D class
    PlotterPostfit plotterPre(inFile, bkgProcesses, sigProcess, false);
    plotterPre.ConfigureHists();
    plotterPre.Draw(outDirs);

    PlotterPostfit plotterPost(inFile, bkgProcesses, sigProcess, true);
    plotterPost.ConfigureHists();
    plotterPost.Draw(outDirs);
}
