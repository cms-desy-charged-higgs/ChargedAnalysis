/**
* @file extension.h
* @brief Header file for Extension name space
*/

#ifndef EXTENSION_H
#define EXTENSION_H

#include <torch/torch.h>

#include <map>
#include <string>
#include <vector>
#include <random>
#include <memory>

#include <TFile.h>
#include <Math/GenVector/LorentzVector.h>
#include <Math/GenVector/VectorUtil.h>
#include <Math/GenVector/PtEtaPhiM4D.h>
#include <Math/GenVector/PxPyPzE4D.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/frame.h>
#include <ChargedAnalysis/Analysis/include/treefunction.h>
#include <ChargedAnalysis/Analysis/include/treeparser.h>
#include <ChargedAnalysis/Network/include/dnnmodel.h>
#include <ChargedAnalysis/Network/include/htagger.h>
#include <ChargedAnalysis/Network/include/htagdataset.h>

/**
* @brief Library with functions which are used by the TreeAppender class to calculate quantities of interest
*/

namespace Extension {
    std::map<std::string, std::vector<float>> HScore(std::shared_ptr<TFile>& file, const std::string& channel, const int& era);
    std::map<std::string, std::vector<float>> DNNScore(std::shared_ptr<TFile>& file, const std::string& channel, const int& era);
    std::map<std::string, std::vector<float>> HReconstruction(std::shared_ptr<TFile>& file, const std::string& channel, const int& era);
}

#endif
