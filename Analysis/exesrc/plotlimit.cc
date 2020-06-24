#include <string>
#include <vector>

#include <ChargedAnalysis/Analysis/include/plotterLimit.h>
#include <ChargedAnalysis/Utility/include/parser.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<int> masses = parser.GetVector<int>("masses");
    std::vector<std::string> channels = parser.GetVector<std::string>("channels");
    std::string limitDir = parser.GetValue<std::string>("limit-dir");
    std::vector<std::string> outDirs = parser.GetVector<std::string>("out-dirs");
  
    //Call and run Plotter1D class
    PlotterLimit plotter(limitDir, masses, channels);
    plotter.ConfigureHists();
    plotter.Draw(outDirs);
}
