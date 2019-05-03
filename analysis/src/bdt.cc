#include <ChargedHiggs/analysis/interface/bdt.h>

BDT::BDT(const int &nTrees, const float &minNodeSize, const float &learningRate, const int &nCuts, const int &treeDepth, const std::string &sepType){
    std::map<std::string, std::string> config = {
                                                    {"NTrees", std::to_string(nTrees)},
                                                    {"MinNodeSize", std::to_string(minNodeSize)},
                                                    {"AdaBoostBeta", std::to_string(learningRate)},
                                                    {"nCuts", std::to_string(nCuts)},
                                                    {"MaxDepth", std::to_string(treeDepth)},
                                                    {"SeparationType", sepType},
    };

    trainString = "!H:BoostType=AdaBoost:UseBaggedBoost:BaggedSampleFraction=0.5:UseRandomisedTrees=True";

    for(std::pair<std::string, std::string> conf: config){
        trainString += ":" + conf.first + "=" + conf.second;
    }
}

float BDT::Train(std::vector<std::string> &xParameters, std::string &treeDir, std::string &resultDir, std::vector<std::string> &signals, std::vector<std::string> &backgrounds, std::string &evType){
    TFile* outputFile = TFile::Open((resultDir + "/" + evType + "/BDT.root").c_str(), "RECREATE");

    TMVA::Tools::Instance();
    TMVA::Factory* factory = new TMVA::Factory("BDT", outputFile);

    TMVA::DataLoader* loader = new TMVA::DataLoader("TempBDTdir");

    for(std::string &xParameter: xParameters){
        loader->AddVariable(xParameter);
    }

    std::string train = evType;
    std::string test = evType == "Even" ? "Odd": "Even";

    for(std::string &signal: signals){
        TFile* sigTestFile = TFile::Open((treeDir + "/" + test + "/" + signal + ".root").c_str());
        TFile* sigTrainFile = TFile::Open((treeDir + "/" + train + "/" + signal + ".root").c_str());
        loader->AddSignalTree((TTree*)sigTestFile->Get(signal.c_str()), 1., TMVA::Types::kTesting);
        loader->AddSignalTree((TTree*)sigTrainFile->Get(signal.c_str()), 1., TMVA::Types::kTraining);
    }

    for(std::string &background: backgrounds){
        TFile* bkgTestFile = TFile::Open((treeDir + "/" + test + "/" + background + ".root").c_str());
        TFile* bkgTrainFile = TFile::Open((treeDir + "/" + train + "/" + background + ".root").c_str());
        loader->AddBackgroundTree((TTree*)bkgTestFile->Get(background.c_str()), 1., TMVA::Types::kTesting);
        loader->AddBackgroundTree((TTree*)bkgTrainFile->Get(background.c_str()), 1., TMVA::Types::kTraining);
    }

    loader->PrepareTrainingAndTestTree("", 5000, 5000, 5000, 5000);

    factory->BookMethod(loader, TMVA::Types::kBDT, "BDT", trainString);
    
    factory->TrainAllMethods();
    factory->TestAllMethods();
    factory->EvaluateAllMethods();

    std::system(std::string("rsync -a --delete-after TempBDTdir/weights " + resultDir + "/" + evType).c_str());
    std::system(std::string("command rm -r TempBDTdir").c_str());

    return factory->GetROCIntegral(loader, "BDT");
}

void BDT::Evaluate(){}
