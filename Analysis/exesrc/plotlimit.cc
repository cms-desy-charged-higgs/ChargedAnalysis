#include <string>
#include <vector>

#include <ChargedAnalysis/Analysis/include/plotterLimit.h>
#include <ChargedAnalysis/Utility/include/parser.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<int> chargedMasses = parser.GetVector<int>("charged-masses");
    std::vector<int> neutralMasses = parser.GetVector<int>("neutral-masses");
    std::string era = parser.GetValue<std::string>("era");
    std::vector<std::string> channels = parser.GetVector<std::string>("channels");
    std::string limitDir = parser.GetValue<std::string>("limit-dir");
    std::vector<std::string> outDirs = parser.GetVector<std::string>("out-dirs");
    std::vector<float> xSecs = parser.GetVector<float>("x-secs");
  
    //Call and run Plotter1D class
    PlotterLimit plotter(limitDir, chargedMasses, neutralMasses, channels, era, xSecs);
    plotter.ConfigureHists();
    plotter.Draw(outDirs);
}
