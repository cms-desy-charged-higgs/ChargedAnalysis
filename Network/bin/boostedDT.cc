#include <experimental/random>
#include <string>
#include <vector>
#include <sstream>
#include <thread>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/frame.h>
#include <ChargedAnalysis/Network/include/bdt.h>

int main(int argc, char* argv[]){
    //Define hyperparameter variables
    int nTrees, minNodeSize, nCuts, treeDepth; 
    float lr, dropOut;
    std::string sepType;

    //Extract informations of command line
    Parser parser(argc, argv);

    bool optimize = parser.GetValue<bool>("optimize");
    std::vector<std::string> xParameters = parser.GetVector<std::string>("x-parameters");
    std::string treeDir = parser.GetValue<std::string>("tree-dir");
    std::string resultDir = parser.GetValue<std::string>("result-dir");
    std::string signal = parser.GetValue<std::string>("signal");
    std::vector<std::string> backgrounds = parser.GetVector<std::string>("backgrounds");
    std::vector<std::string> masses = parser.GetVector<std::string>("masses");
    sepType = parser.GetValue<std::string>("sep-type");

    //Check if this execute should do hyperparameter optimization or not
    if(optimize){
        nTrees = std::experimental::randint(200, 4000);
        minNodeSize = std::experimental::randint(2, 20);
        lr = std::experimental::randint(10, 100)/100.;
        nCuts = std::experimental::randint(5, 100);
        treeDepth = std::experimental::randint(2, 20);
        dropOut = std::experimental::randint(2, (int)xParameters.size());
    }

    else{
        nTrees = parser.GetValue<int>("n-trees");
        minNodeSize = parser.GetValue<int>("min-node-size");
        lr = parser.GetValue<float>("lr");
        nCuts = parser.GetValue<int>("n-cuts");
        treeDepth = parser.GetValue<int>("tree-depth");
        dropOut = parser.GetValue<float>("drop-out");
    }

    //Train BDT
    BDT bdt(nTrees, minNodeSize, lr, nCuts, treeDepth, dropOut, sepType);
    float ROC = bdt.Train(xParameters, treeDir, resultDir, signal, backgrounds, masses, optimize);

    if(optimize){
        std::string pathName = std::string(std::getenv("CHDIR")) + "/BDT/HyperTuning/";
        std::system(("mkdir -p " + pathName).c_str());
     
        std::ostringstream pid;
        pid << std::this_thread::get_id();

        Frame frame({"ROC", "n-trees", "min-node-size", "lr", "n-cuts", "tree-depth", "drop-out"});
        frame.AddColumn({ROC, nTrees, minNodeSize, lr, nCuts, treeDepth, dropOut});
        frame.WriteCSV(std::string(std::getenv("CHDIR")) + "/BDT/HyperTuning/" + pid.str() + ".csv");
    }
}
