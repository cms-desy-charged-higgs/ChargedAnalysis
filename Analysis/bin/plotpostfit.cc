#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/plotterPostfit.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    int mass = parser.GetValue<int>("mass");
    std::string limitDir = parser.GetValue<std::string>("limit-dir");
    std::vector<std::string> channels = parser.GetVector<std::string>("channels");
    std::vector<std::string> outDirs = parser.GetVector<std::string>("out-dirs");
  
    //Call and run Plotter1D class
    PlotterPostfit plotter(limitDir, mass, channels);
    plotter.ConfigureHists();
    plotter.Draw(outDirs);
}
