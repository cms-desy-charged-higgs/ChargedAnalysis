#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Analysis/include/plotterLimit.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<int> masses = parser.GetVector<int>("masses");
    std::string limitDir = parser.GetValue<std::string>("limit-dir");
    std::vector<std::string> outDirs = parser.GetVector<std::string>("out-dirs");
  
    //Call and run Plotter1D class
    PlotterLimit plotter(limitDir, masses);
    plotter.ConfigureHists();
    plotter.Draw(outDirs);
}
