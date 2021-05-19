#include <ChargedAnalysis/Analysis/include/treeslimmer.h>

TreeSlimmer::TreeSlimmer(const std::string& inputFile, const std::string& inputChannel) :
    inputFile(inputFile),
    inputChannel(inputChannel){}

void TreeSlimmer::DoSlim(const std::string outputFile, const std::string outChannel, const std::vector<std::string>& cuts, const int& start, const int& end){
    //Get input tree
    std::shared_ptr<TFile> inFile = RUtil::Open(inputFile);
    std::shared_ptr<TTree> inTree = RUtil::GetSmart<TTree>(inFile.get(), inputChannel);

    std::cout << "Read file: '" << inputFile << "'" << std::endl;
    std::cout << "Read tree '" << inputChannel << "'" << std::endl;

    //Prepare output tree and cutflow
    std::shared_ptr<TFile> outFile(TFile::Open(outputFile.c_str(), "RECREATE"));
    std::shared_ptr<TTree> outTree(inTree->CloneTree(0, "fast"));
    outTree->SetName(outChannel.c_str()); 
    outTree->SetTitle(outChannel.c_str()); 

    std::shared_ptr<TH1F> cutflow = RUtil::GetSmart<TH1F>(inFile.get(), "cutflow_" + inputChannel);
    cutflow->SetDirectory(outFile.get());
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

    for(int i = start; i < end; i++){
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

    //Write everything
    outTree->Write();

    cutflow->Write();

    for(TObject* key : *(inFile->GetListOfKeys())){
        TObject* obj = inFile->Get(key->GetName());

        if(obj->InheritsFrom(TTree::Class())) continue;
        if(Utils::Find<std::string>(obj->GetName(), "cutflow") == -1) obj->Write();
    }

    std::cout << "Saved tree: '" << outTree->GetName() << "' with " << outTree->GetEntries() << " entries" << std::endl;
}
