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

            if(passed) cutflow->Fill(cut.GetCutLabel().c_str(), 1);
            else break;
        }

        if(passed){
            inTree->GetEntry(i);
            outTree->Fill();
        }
    }

    cutflow->Write();
    outTree->Write();

    std::cout << "Saved tree: '" << outTree->GetName() << "' with " << outTree->GetEntries() << " entries" << std::endl;

    delete inFile; delete outFile;
}
