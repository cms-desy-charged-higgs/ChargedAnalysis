#include <string>
#include <map>

#include <TFile.h>
#include <TTree.h>

#include <TMVA/Factory.h>
#include <TMVA/DataLoader.h>
#include <TMVA/Tools.h>

class BDT{
    private:
        std::string trainString;

    public:
        BDT(const int &nTrees = 500, const float &minNodeSize = 2.5, const float &learningRate = 0.5, const int &nCuts = 20, const int &treeDepth = 3, const std::string &sepType = "GiniIndex");

        float Train(std::vector<std::string> &xParameters, std::string &treeDir, std::string &resultDir, std::vector<std::string> &signals, std::vector<std::string> &backgrounds, std::string &evType);
        void Evaluate();
};
