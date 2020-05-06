#include <ChargedAnalysis/Analysis/include/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel):
    parameters(parameters),
    cutStrings(cutStrings),
    outname(outname),
    channel(channel){}

void TreeReader::PrepareLoop(TFile* outFile, TTree* inputTree){
    TreeParser parser;

    for(const std::string& parameter: parameters){
        //Functor structure and arguments
        TreeFunction function(inputTree->GetCurrentFile(), inputTree->GetName());
        
        //Read in everything, orders matter
        parser.GetParticle(parameter, function);
        parser.GetFunction(parameter, function);

        if(Utils::Find<std::string>(parameter, "h:") != -1){
            TH1F* hist = new TH1F();

            parser.GetBinning(parameter, hist);

            hist->SetName(function.GetName().c_str());       
            hist->SetTitle(function.GetName().c_str());
            hist->SetDirectory(outFile);

            hist->GetXaxis()->SetTitle(function.GetAxisLabel().c_str());              

            hists.push_back(hist);
            histFunctions.push_back(function);
        }

        if(Utils::Find<std::string>(parameter, "t:") != -1){
            if(outTree == NULL){
                outTree = new TTree(channel.c_str(), channel.c_str());
                outTree->SetDirectory(outFile);
            }

            branchNames.push_back(function.GetName());
            treeValues.push_back(1.);

            treeFunctions.push_back(function);
        }

        if(Utils::Find<std::string>(parameter, "csv:") != -1){
            CSVNames.push_back(function.GetName());

            CSVFunctions.push_back(function);
        }
    }

    //Declare branches of output tree if wished
    for(int i=0; i < branchNames.size(); i++){
        outTree->Branch(branchNames[i].c_str(), &treeValues[i]);
    }

    //Declare columns in CSV file if wished
    if(!CSVNames.empty()){
        frame = new Frame();
        frame->InitLabels(CSVNames);
    }

    for(const std::string& cut: cutStrings){
        //Functor structure and arguments
        TreeFunction function(inputTree->GetCurrentFile(), inputTree->GetName());

        parser.GetParticle(cut, function);
        parser.GetFunction(cut, function);
        parser.GetCut(cut, function);

        cutFunctions.push_back(function);

        std::cout << "Cut will be applied: '" << function.GetCutLabel() << "'" << std::endl;
    }
}

void TreeReader::EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd, const std::string& cleanJet){
    //Take time
    Utils::RunTime timer;

    //Get input tree
    TFile* inputFile = TFile::Open(fileName.c_str(), "READ");
    TTree* inputTree = inputFile->Get<TTree>(channel.c_str());

    TLeaf* nTrueInter = inputTree->GetLeaf("Misc_TrueInteraction");

    std::cout << "Read file: '" << fileName << "'" << std::endl;
    std::cout << "Read tree '" << channel << "' from " << entryStart << " to " << entryEnd << std::endl;

    gROOT->SetBatch(kTRUE);

    std::string name = outname;
    if(name.find("csv") != std::string::npos) name.replace(name.find("csv"), 3, "root");

    //Open output file and set up all histograms/tree and their function to call
    TFile* outFile = TFile::Open(name.c_str(), "RECREATE");
    PrepareLoop(outFile, inputTree);

    //Determine what to clean from jets
    std::vector<std::string> cleanInfo = Utils::SplitString<std::string>(cleanJet, "/");

    if(cleanInfo.size() != 1){
        for(int j=0; j < histFunctions.size(); j++){
            histFunctions[j].SetCleanJet(cleanInfo[0], cleanInfo[1]);
        }

        for(int j=0; j < cutFunctions.size(); j++){
            cutFunctions[j].SetCleanJet(cleanInfo[0], cleanInfo[1]);
        }

        for(int j=0; j < treeFunctions.size(); j++){
            treeFunctions[j].SetCleanJet(cleanInfo[0], cleanInfo[1]);
        }

        for(int j=0; j < CSVFunctions.size(); j++){
            CSVFunctions[j].SetCleanJet(cleanInfo[0], cleanInfo[1]);
        }
    }

    //Get number of generated events
    if(inputFile->GetListOfKeys()->Contains("nGen")){
        nGen = inputFile->Get<TH1F>("nGen")->Integral();
    }

    if(inputFile->GetListOfKeys()->Contains("xSec")){
        xSec = inputFile->Get<TH1F>("xSec")->GetBinContent(1);
    }

    if(inputFile->GetListOfKeys()->Contains("Lumi")){
        lumi = inputFile->Get<TH1F>("Lumi")->GetBinContent(1);
    }

    //Set all branch addresses needed for the event
    float weight = 1., value = -999.;
    bool passed = true;
    bool isData = true;

    //Calculate pile up weight histogram
    TH1F* pileUpWeight=NULL;

    if(inputFile->GetListOfKeys()->Contains("puMC")){
        isData = false;
        TH1F* puMC = (TH1F*)inputFile->Get("puMC");
        TH1F* puReal = (TH1F*)inputFile->Get("pileUp");

        pileUpWeight = (TH1F*)puReal->Clone();
        pileUpWeight->Scale(1./pileUpWeight->Integral());
        puMC->Scale(1./puMC->Integral());

        pileUpWeight->Divide(puMC);
        delete puMC; delete puReal;
    }

    //Cutflow
    TH1F* cutflow = inputFile->Get<TH1F>(("cutflow_" + channel).c_str());
    cutflow->SetName("cutflow"); cutflow->SetTitle("cutflow");
    if(inputTree->GetEntries() != 0) cutflow->Scale((1./nGen)*(entryEnd-entryStart)/inputTree->GetEntries());
    else cutflow->Scale(1./nGen);

    for (int i = entryStart; i < entryEnd; i++){
        if(i % 100000 == 0 and i != 0){
            std::cout << "Processed events: " << i-entryStart << " (" << (i-entryStart)/timer.Time() << " eve/s)" << std::endl;
        }

        //Set Entry
        TreeFunction::SetEntry(i);
        nTrueInter->GetBranch()->GetEntry(i);

        //Multiply all common weights
        weight = xSec*lumi;

        weight *= 1./nGen;
        if(pileUpWeight!=NULL) weight *= pileUpWeight->GetBinContent(pileUpWeight->FindBin(*(float*)nTrueInter->GetValuePointer()));

        //Check if event passed all cuts
        passed=true;

        for(int j=0; j < cutFunctions.size(); j++){
            passed = passed && cutFunctions[j].GetPassed();

            if(passed){
                weight *= cutFunctions[j].GetWeight();
                cutflow->Fill(cutFunctions[j].GetCutLabel().c_str(), weight);
            }

            else break;
        }

        if(!passed) continue;

        //Fill histogram
        for(int j=0; j < hists.size(); j++){
            hists[j]->Fill(histFunctions[j].Get(), weight);
        }

        //Fill trees
        for(int j=0; j < treeFunctions.size(); j++){
            treeValues[j] = treeFunctions[j].Get();
        }

        if(outTree != NULL) outTree->Fill();

        //Fill CSV file
        if(frame != NULL){
            std::vector<float> values;

            for(int j=0; j < CSVFunctions.size(); j++){
                values.push_back(CSVFunctions[j].Get());
            }

            frame->AddColumn(values);
        }
    }

    //Write all histograms and delete everything
    outFile->cd();
    for(TH1F* hist: hists){
        hist->Write(); 
        std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
        delete hist;
    }

    if(outTree != NULL){
        outTree->Write(); 
        std::cout << "Saved tree: '" << outTree->GetName() << "' with " << outTree->GetEntries() << " entries" << std::endl;
        std::cout << "Branches which are saved in tree:" << std::endl;

        for(std::string& branchName: branchNames){
            std::cout << branchName << std::endl;
        }

        delete outTree;
    }

    //Remove empty bins and write cutflow
    int nonEmpty = 0;

    for(int i=0; i < cutflow->GetNbinsX(); i++){
        if(std::string(cutflow->GetXaxis()->GetBinLabel(i)) != "") nonEmpty++;
    }

    cutflow->SetAxisRange(0, nonEmpty); 
    cutflow->Write();

    if(frame!=NULL){
        frame->WriteCSV(outname);
        delete frame;
    }

    //Delete everything
    delete cutflow; delete inputTree; delete inputFile; delete outFile;

    std::cout << "Closed output file: '" << outname << "'" << std::endl;
    std::cout << "Time passed for complete processing: " << timer.Time() << " s" << std::endl;
}
