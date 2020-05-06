#include <ChargedAnalysis/Analysis/include/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel):
    parameters(parameters),
    cutStrings(cutStrings),
    outname(outname),
    channel(channel){

    particles = {
        {"e", ELECTRON},
        {"mu", MUON},
        {"j", JET},
        {"sj", SUBJET},
        {"fj", FATJET},
        {"bj", BJET},
        {"bsj", BSUBJET},
        {"met", MET},
    };

    workingPoints = {
        {"l", LOOSE},
        {"m", MEDIUM},
        {"t", TIGHT},
    };

    comparisons = {
        {"bigger", BIGGER},
        {"smaller", SMALLER},
        {"equal", EQUAL},
        {"divisible", DIVISIBLE},
        {"notdivisible", NOTDIVISIBLE},
    };
}

void TreeReader::GetFunction(const std::string& parameter, TreeFunction& func){
    if(Utils::Find<std::string>(parameter, "f:") == -1) throw std::runtime_error("No function key 'f' in '" + parameter + "'");

    std::string funcName; float value = -999.;

    std::string funcLine = parameter.substr(parameter.find("f:")+2, parameter.substr(parameter.find("f:")).find("/")-2);
    
    for(std::string& funcParam: Utils::SplitString<std::string>(funcLine, ",")){
        std::vector<std::string> fInfo = Utils::SplitString<std::string>(funcParam, "=");

        if(fInfo[0] == "n") funcName = fInfo[1];
        else if(fInfo[0] == "v") value = std::stoi(fInfo[1]);
        else throw std::runtime_error("Invalid key '" + fInfo[0] + "' in parameter '" +  funcLine + "'");
    }

    func.SetFunction(funcName, value); 
}

void TreeReader::GetParticle(const std::string& parameter, TreeFunction& func){
    if(Utils::Find<std::string>(parameter, "p:") == -1) return;

    std::string partLine = parameter.substr(parameter.find("p:")+2, parameter.substr(parameter.find("p:")).find("/")-2);

    for(std::string& particle: Utils::SplitString<std::string>(partLine, "~")){
        Particle part = VACUUM; WP wp = NONE; int idx = 0;

        for(const std::string& partParam: Utils::SplitString<std::string>(particle, ",")){
            std::vector<std::string> pInfo = Utils::SplitString<std::string>(partParam, "=");

            if(pInfo[0] == "n") part = particles[pInfo[1]];
            else if (pInfo[0] == "wp") wp = workingPoints[pInfo[1]];
            else if (pInfo[0] == "i") idx = std::atoi(pInfo[1].c_str());
            else throw std::runtime_error("Invalid key '" + pInfo[0] + "' in parameter '" +  partLine + "'");
        }

        func.SetP1(part, idx, wp);
    }
}

void TreeReader::GetCut(const std::string& parameter, TreeFunction& func){
    if(Utils::Find<std::string>(parameter, "c:") == -1) throw std::runtime_error("No cut key 'c' in '" + parameter + "'");

    Comparison comp; float compValue = -999.;

    std::string cutLine = parameter.substr(parameter.find("c:")+2, parameter.substr(parameter.find("c:")).find("/")-2);

    for(std::string& cutParam: Utils::SplitString<std::string>(cutLine, ",")){
        std::vector<std::string> cInfo = Utils::SplitString<std::string>(cutParam, "=");

        if(cInfo[0] == "n") comp = comparisons[cInfo[1]];
        else if(cInfo[0] == "v") compValue = std::stof(cInfo[1]);
        else throw std::runtime_error("Invalid key '" + cInfo[0] + "' in parameter '" +  cutLine + "'");
    }

    func.SetCut(comp, compValue);
}

void TreeReader::GetBinning(const std::string& parameter, TH1* hist){
    if(Utils::Find<std::string>(parameter, "h:") == -1) throw std::runtime_error("No hist key 'h' in '" + parameter + "'");

    std::string histLine = parameter.substr(parameter.find("h:")+2, parameter.substr(parameter.find("h:")).find("/")-2);

    int bins = 30; float xlow = 0; float xhigh = 1; 

    for(std::string& histParam: Utils::SplitString<std::string>(histLine, ",")){
        std::vector<std::string> hInfo = Utils::SplitString<std::string>(histParam, "=");

        if(hInfo[0] == "nxb") bins = std::stof(hInfo[1]);
        else if(hInfo[0] == "xl") xlow = std::stof(hInfo[1]);
        else if(hInfo[0] == "xh") xhigh = std::stof(hInfo[1]);
        else throw std::runtime_error("Invalid key '" + hInfo[0] + " in parameter '" +  histLine + "'");
    }

    hist->SetBins(bins, xlow, xhigh);
}

void TreeReader::PrepareLoop(TFile* outFile, TTree* inputTree){
    for(const std::string& parameter: parameters){
        //Functor structure and arguments
        TreeFunction function(inputTree->GetCurrentFile(), inputTree->GetName());
        
        //Read in everything, orders matter
        GetParticle(parameter, function);
        GetFunction(parameter, function);

        if(Utils::Find<std::string>(parameter, "h:") != -1){
            TH1F* hist = new TH1F();

            TreeReader::GetBinning(parameter, hist);

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

        GetParticle(cut, function);
        GetFunction(cut, function);
        GetCut(cut, function);

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
            histFunctions[j].SetCleanJet(particles[cleanInfo[0]], workingPoints[cleanInfo[1]]);
        }

        for(int j=0; j < cutFunctions.size(); j++){
            cutFunctions[j].SetCleanJet(particles[cleanInfo[0]], workingPoints[cleanInfo[1]]);
        }

        for(int j=0; j < treeFunctions.size(); j++){
            treeFunctions[j].SetCleanJet(particles[cleanInfo[0]], workingPoints[cleanInfo[1]]);
        }

        for(int j=0; j < CSVFunctions.size(); j++){
            CSVFunctions[j].SetCleanJet(particles[cleanInfo[0]], workingPoints[cleanInfo[1]]);
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
