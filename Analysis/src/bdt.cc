#include <ChargedAnalysis/Analysis/include/bdt.h>

BDT::BDT(const int &nTrees, const float &minNodeSize, const float &learningRate, const int &nCuts, const int &treeDepth, const int &dropOut, const std::string &sepType){
    //Write string which configures the hyperparameter space
    std::map<std::string, std::string> config = {
                                                    {"NTrees", std::to_string(nTrees)},
                                                    {"MinNodeSize", std::to_string(minNodeSize)},
                                                    {"AdaBoostBeta", std::to_string(learningRate)},
                                                    {"nCuts", std::to_string(nCuts)},
                                                    {"MaxDepth", std::to_string(treeDepth)},
                                                    {"UseNvars", std::to_string(dropOut)},
                                                    {"SeparationType", sepType},
    };

    trainString = "!H:BoostType=AdaBoost:UseBaggedBoost:BaggedSampleFraction=0.5:UseRandomisedTrees=True";  

    for(std::pair<std::string, std::string> conf: config){
        trainString += ":" + conf.first + "=" + conf.second;
    }
}

BDT::~BDT(){
    delete reader;
}

float BDT::Train(std::vector<std::string> &xParameters, std::string &treeDir, std::string &resultDir, std::vector<std::string> &signals, std::vector<std::string> &backgrounds, std::string &evType){

    //Open output file
    TFile* outputFile = TFile::Open((resultDir + "/BDT.root").c_str(), "RECREATE");

    //Relevant TMVA instances
    TMVA::Tools::Instance();
    TMVA::Factory* factory = new TMVA::Factory("BDT", outputFile);

    std::ostringstream thisString;
    thisString << (void*)this;
    std::string tempDir = "BDT_" + thisString.str();
    TMVA::DataLoader* loader = new TMVA::DataLoader(tempDir);

    (TMVA::gConfig().GetVariablePlotting()).fMaxNumOfAllowedVariablesForScatterPlots = 0.;

    //Add trainings variables
    for(std::string &xParameter: xParameters){
        loader->AddVariable(xParameter);
    }

    loader->AddVariable("mass");

    //Add bkg/sig training and test trees
    std::string train = evType == "Even" ? "Odd": "Even";
    int nSig = 0., nBkg = 0.;


    for(std::string &signal: signals){
        std::string massPoint = std::string(signal.begin()+4, signal.begin()+7);

        TFile* sigFile = TFile::Open((treeDir + "/" + train + "/" + massPoint + "/" + signal + ".root").c_str());

        TTree* sigTree = (TTree*)sigFile->Get(signal.c_str());

        sigTree->GetBranch(("const_" + massPoint).c_str())->SetTitle("mass");
        sigTree->GetBranch(("const_" + massPoint).c_str())->SetName("mass");

        loader->AddSignalTree(sigTree);

        nSig+= ((TTree*)sigFile->Get(signal.c_str()))->GetEntries();
   
        for(std::string &background: backgrounds){
            TFile* bkgFile = TFile::Open((treeDir + "/" + train + "/" + massPoint + "/" + background + ".root").c_str());

            TTree* bkgTree = (TTree*)bkgFile->Get(background.c_str());
            bkgTree->GetBranch(("const_" + massPoint).c_str())->SetTitle("mass");
            bkgTree->GetBranch(("const_" + massPoint).c_str())->SetName("mass");

            loader->AddBackgroundTree(bkgTree);

            nBkg+= ((TTree*)bkgFile->Get(background.c_str()))->GetEntries();
        }
    }

    //Configure BDT training
    loader->PrepareTrainingAndTestTree("", 0.8*nSig, 0.8*nBkg, 0.2*nSig, 0.2*nBkg);
    factory->BookMethod(loader, TMVA::Types::kBDT, "BDT", trainString);
    
    //Do the training and testing
    factory->TrainAllMethods();
    factory->TestAllMethods();
    factory->EvaluateAllMethods();

    float ROCCurve = factory->GetROCIntegral(loader, "BDT");

    outputFile->Close();
    delete factory;
    delete loader;

    std::system(std::string("rsync -a --delete-after " + tempDir + "/weights " + resultDir).c_str());
    std::system(std::string("command rm -r " + tempDir).c_str());

   
    return ROCCurve;
}

std::vector<std::string> BDT::SetEvaluation(const std::string &bdtPath){
    std::vector<std::string> paramNames;

    //Get input variables from TestTree branch names
    TFile* bdtFile = TFile::Open((bdtPath + "/BDT.root").c_str());

    std::string dir = bdtFile->GetListOfKeys()->At(0)->GetName();

    TTree* tree = (TTree*)bdtFile->Get((dir + "/TestTree").c_str());
    TObjArray* branches = tree->GetListOfBranches();

    for(int i = 2; i < branches->GetEntries() - 2; i++){
        std::string varName = ((TBranch*)branches->At(i))->GetName();

        //Fill parameter names
        paramNames.push_back(varName);
    }

   //Initialize reader;
    reader = new TMVA::Reader(paramNames, "Silent");
    reader->BookMVA("BDT", bdtPath + "/weights/BDT_BDT.weights.xml");

    delete bdtFile;

    return paramNames;
}

float BDT::Evaluate(const std::vector<float> &paramValues){
    return reader->EvaluateMVA(paramValues, "BDT");
}
