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
    std::string channel = parser.GetValue<std::string>("channel");
    std::vector<std::string> limitFiles = parser.GetVector<std::string>("limit-files");
    std::vector<std::string> outDirs = parser.GetVector<std::string>("out-dirs");
    std::vector<float> xSecs = parser.GetVector<float>("x-secs");
  
    //Call and run Plotter1D class
    PlotterLimit plotter(limitFiles, chargedMasses, neutralMasses, channel, era, xSecs);
    plotter.ConfigureHists();
    plotter.Draw(outDirs);
}
