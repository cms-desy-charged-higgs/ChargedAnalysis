#include <ChargedAnalysis/Analysis/include/treeslimmer.h>

TreeSlimmer::TreeSlimmer(const std::string& inputFile, const std::string& inputChannel) :
    inputFile(inputFile),
    inputChannel(inputChannel){}

void TreeSlimmer::DoSlim(const std::string outputFile, const std::string outChannel, const std::vector<std::string>& cuts){
    //Get input tree
    TFile* inFile = TFile::Open(inputFile.c_str(), "READ");
    TTree* inTree = inFile->Get<TTree>(inputChannel.c_str());

    std::cout << "Read file: '" << inputFile << "'" << std::endl;
    std::cout << "Read tree '" << inputChannel << "'" << std::endl;

    //Prepare output tree and cutflow
    TFile* outFile = TFile::Open(outputFile.c_str(), "RECREATE");
    TTree* outTree = inTree->CloneTree(0, "fast");
    outTree->SetName(outChannel.c_str()); 
    outTree->SetTitle(outChannel.c_str()); 

    TH1F* cutflow = (TH1F*)inFile->Get<TH1F>(("cutflow_" + inputChannel).c_str())->Clone();
    cutflow->SetDirectory(outFile);
    cutflow->SetName(("cutflow_" + outChannel).c_str()); 
    cutflow->SetTitle(("cutflow_" + outChannel).c_str()); 

    //Prepare functions for cut
    TreeParser parser;
    std::vector<TreeFunction> cutFuncs;

    for(const std::string cut: cuts){
        TreeFunction func(inFile, inputChannel);
        
        parser.GetParticle(cut, func);
        parser.GetFunction(cut, func);
        parser.GetCut(cut, func);

        cutFuncs.push_back(func);
    }

    //Get lumi and xsec for weighting cut flow
    float xSec = 1., lumi = 1.;

    if(inFile->GetListOfKeys()->Contains("xSec")){
        xSec = inFile->Get<TH1F>("xSec")->GetBinContent(1);
    }

    if(inFile->GetListOfKeys()->Contains("Lumi")){
        lumi = inFile->Get<TH1F>("Lumi")->GetBinContent(1);
    }

    //Loop
    bool passed = true;

    for(int i = 0; i < inTree->GetEntries(); i++){
        if(i % 100000 == 0 and i != 0){
            std::cout << "Processed events: " << i << std::endl;
        }

        passed = true;

        TreeFunction::SetEntry(i);

        for(TreeFunction& cut: cutFuncs){
            passed = cut.GetPassed();

            if(passed) cutflow->Fill(cut.GetCutLabel().c_str(), xSec*lumi);
            else break;
        }

        if(passed){
            inTree->GetEntry(i);
            outTree->Fill();
        }
    }

    //Write everything
    outTree->Write();
    cutflow->Write();

    TList* keys = inFile->GetListOfKeys();

    for(int i=0; i < keys->GetSize(); i++){
        bool skipKey = false;

        TObject* obj = inFile->Get(keys->At(i)->GetName());

        if(obj->InheritsFrom(TTree::Class())) continue;
        if(Utils::Find<std::string>(obj->GetName(), "cutflow") == -1) obj->Write();
    }

    std::cout << "Saved tree: '" << outTree->GetName() << "' with " << outTree->GetEntries() << " entries" << std::endl;

    delete inFile; delete outFile;
}
