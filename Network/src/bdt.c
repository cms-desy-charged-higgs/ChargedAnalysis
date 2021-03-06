#include <ChargedAnalysis/Network/include/bdt.h>

BDT::BDT(const int &nTrees, const float &minNodeSize, const float &learningRate, const int &nCuts, const int &treeDepth, const int &dropOut, const std::string &sepType){
    //Write string which configures the hyperparameter space
    std::map<std::string, std::string> config = {
                                                    {"NTrees", std::to_string(nTrees)},
                                                    {"MinNodeSize", std::to_string(minNodeSize)},
                                                    {"AdaBoostBeta", std::to_string(learningRate)},
                                                    {"Shrinkage", std::to_string(learningRate)},
                                                    {"nCuts", std::to_string(nCuts)},
                                                    {"MaxDepth", std::to_string(treeDepth)},
                                                    {"UseNvars", std::to_string(dropOut)},
                                                    {"SeparationType", sepType},
    };

    trainString = "!H:BoostType=Grad:UseBaggedBoost:BaggedSampleFraction=0.5:UseRandomisedTrees=True";  

    for(std::pair<std::string, std::string> conf: config){
        trainString += ":" + conf.first + "=" + conf.second;
    }
}

BDT::~BDT(){
    //delete reader;
    if(reader){
        delete reader;
    }
}

float BDT::Train(std::vector<std::string> &parameters, std::string &treeDir, std::string &resultDir, std::string &signal, std::vector<std::string> &backgrounds, std::vector<std::string>& masses, const bool& optimize){
    //Name of tmp dir    
    std::ostringstream thisString;
    thisString << (void*)this;
    
    std::system(("cd " + std::string(std::getenv("CHDIR"))).c_str());
    std::string tempDir = thisString.str();
    std::system(("mkdir -p " + tempDir).c_str());

    //Open output file
    TFile* outputFile = TFile::Open((tempDir + "/BDT.root").c_str(), "RECREATE");

    //Relevant TMVA instances
    TMVA::Tools::Instance();
    TMVA::Factory* factory = new TMVA::Factory("BDT", outputFile, optimize ? "Silent" : "");
    TMVA::DataLoader* loader = new TMVA::DataLoader(tempDir);

    (TMVA::gConfig().GetVariablePlotting()).fMaxNumOfAllowedVariablesForScatterPlots = 0.;

    //Save used parameters
    std::ofstream parameterFile;
    parameterFile.open(resultDir + "/parameters.txt");

    for(std::string &parameter: parameters){
        if(Utils::Find<std::string>(parameter, "const") == -1.) parameterFile << parameter << std::endl;
    }

    parameterFile.close();

    //Add bkg/sig training and test trees
    int nSignal=0; int nBkg=0;
    float train = 0.99;
    float test = 0.01;

    std::vector<TFile*> files;
    std::vector<TTree*> trees;

    for(std::string &mass: masses){
        std::string sigName = signal;
        sigName.replace(sigName.find("{"), 2, mass);

        TFile* sigFile = TFile::Open((treeDir + "/" + sigName + "/" + sigName + ".root").c_str());
        TTree* sigTree = (TTree*)sigFile->Get(sigFile->GetListOfKeys()->At(0)->GetName());

        files.push_back(sigFile);
        trees.push_back(sigTree);

        //Add trainings variables
        if(loader->GetDataSetInfo().GetListOfVariables().empty()){
            for(int i=0; i < sigTree->GetListOfBranches()->GetEntries(); i++){
                std::string variable = sigTree->GetListOfBranches()->At(i)->GetName();
            
                if(Utils::Find<std::string>(variable, "const") != -1.) loader->AddVariable("mass");
                else loader->AddVariable(variable);
            }
        }

        sigTree->GetBranch(("const" + mass).c_str())->SetTitle("mass");
        sigTree->GetBranch(("const" + mass).c_str())->SetName("mass");

        nSignal+=sigTree->GetEntries();
        loader->AddSignalTree(sigTree);
   
        for(std::string &background: backgrounds){
            TFile* bkgFile = TFile::Open((treeDir + "/" + background + "/" + background + ".root").c_str());

            TTree* bkgTree = (TTree*)bkgFile->Get(bkgFile->GetListOfKeys()->At(0)->GetName());
            bkgTree->GetBranch(("const" + mass).c_str())->SetTitle("mass");
            bkgTree->GetBranch(("const" + mass).c_str())->SetName("mass");

            gROOT->cd();
         
            loader->AddBackgroundTree(bkgTree);
            nBkg+=bkgTree->GetEntries();

            files.push_back(bkgFile);
            trees.push_back(bkgTree);
        }
    }

    //Change to lower amount of events for hyperopt
    if(optimize){nSignal = 50000; nBkg = 50000;}

    nBkg = nBkg > 2*nSignal ? nSignal : 2*nBkg;

    //Configure BDT training
    loader->PrepareTrainingAndTestTree("", train*nSignal, train*nBkg, test*nSignal, test*nBkg);
    factory->BookMethod(loader, TMVA::Types::kBDT, "BDT", trainString);
    
    //Do the training and testing
    factory->TrainAllMethods();
    factory->TestAllMethods();
    factory->EvaluateAllMethods();

    float ROCCurve = factory->GetROCIntegral(loader, "BDT");

    outputFile->Close();
    delete factory;
    delete loader;

    for(TTree* tree: trees){delete tree;}
    for(TFile* file: files){delete file;}

    if(!optimize) std::system(std::string("rsync -a --delete-after " + tempDir + "/* " + resultDir).c_str());
    std::system(std::string("command rm -r " + tempDir).c_str());

    return ROCCurve;
}

void BDT::SetEvaluation(const std::string &bdtPath){
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
}

float BDT::Evaluate(const std::vector<float> &paramValues){
    return reader->EvaluateMVA(paramValues, "BDT");
}
