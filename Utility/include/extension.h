#include <torch/torch.h>

#include <map>
#include <string>
#include <vector>

#include <TFile.h>
#include <Math/GenVector/LorentzVector.h>
#include <Math/GenVector/VectorUtil.h>
#include <Math/GenVector/PtEtaPhiM4D.h>
#include <Math/GenVector/PxPyPzE4D.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/frame.h>
#include <ChargedAnalysis/Analysis/include/treefunction.h>
#include <ChargedAnalysis/Analysis/include/treeparser.h>
#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Network/include/htagdataset.h>

namespace Extension {
    std::map<std::string, std::vector<float>> HScore(TFile* file, const std::string& channel);
    std::map<std::string, std::vector<float>> DNNScore(TFile* file, const std::string& channel);
    std::map<std::string, std::vector<float>> HReconstruction(TFile* file, const std::string& channel);
}
