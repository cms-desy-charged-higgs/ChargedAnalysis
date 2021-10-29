#include <string>
#include <vector>

#include <ChargedAnalysis/Analysis/include/plotterLimit.h>
#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::vector<std::string> masses = parser.GetVector<std::string>("masses");
    std::vector<int> neutralMasses, chargedMasses;

    for(const std::string& mass : masses){
        std::vector<int> m = StrUtil::Split<int>(mass, "-");

        chargedMasses.push_back(m.at(0));   
        neutralMasses.push_back(m.at(1));
    }

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
