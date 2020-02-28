#include <ChargedAnalysis/Analysis/include/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel):
    parameters(parameters),
    cutStrings(cutStrings),
    outname(outname),
    channel(channel){
}

void TreeReader::PrepareLoop(TFile* outFile){
    for(const std::string& parameter: Utils::Merge<std::string>(parameters, cutStrings)){
        //Functor structure and arguments
        Function function;
        FuncArgs arg;
        TH1F* hist; TTree* tree;

        bool isHist=false, isCut=false;

        //Histogram
        std::string histName = "", xLabel = "";
        std::vector<std::string> partLabels;

        for(std::string& parameterInfo: Utils::SplitString<std::string>(parameter, "/")){
            std::vector<std::string> info = Utils::SplitString<std::string>(parameterInfo, ":");

            //Which function for property to call e.g. pt -> TreeFunction::Pt
            if(info[0] == "f"){
                for(std::string funcInfo: Utils::SplitString<std::string>(Utils::FindInBracket(info[1]), ",")){
                     std::vector<std::string> fInfo = Utils::SplitString<std::string>(funcInfo, "=");
                       
                     //Default values
                     function.func = TreeFunction::funcMap["constant"].first; int value=-999;

                     if(fInfo[0] == "n"){
                        function.func = TreeFunction::funcMap.at(fInfo[1]).first; 
                        function.funcName = fInfo[1];
                        xLabel = TreeFunction::funcMap.at(fInfo[1]).second;
                        histName = fInfo[1];
                     }

                    if(fInfo[0] == "v"){
                        value = std::atoi(fInfo[1].c_str());
                        histName += "_" + fInfo[0];
                    }

                    //Fill values
                    arg.value = value;
                }
            }

            if(info[0] == "p"){
                std::vector<std::string> particles = Utils::SplitString<std::string>(info[1], "~");

                for(std::string& particle: particles){
                    //Default values
                    int index = 0; WP wp = NONE; std::string partLabel = ""; 

                    for(std::string partInfo: Utils::SplitString<std::string>(Utils::FindInBracket(particle), ",")){
                        std::vector<std::string> pInfo = Utils::SplitString<std::string>(partInfo, "=");        

                        if(pInfo[0] == "n"){
                            arg.parts.push_back(TreeFunction::partMap.at(pInfo[1]).first); 
                            function.partName = pInfo[1];
                            histName += "_" + pInfo[1];
                            partLabel = TreeFunction::partMap.at(pInfo[1]).second;
                        }

                        if(pInfo[0] == "i"){
                            index = std::atoi(pInfo[1].c_str()) - 1; 
                            function.index = pInfo[1];
                            histName += "_" + pInfo[1];
                        }

                        if(pInfo[0] == "wp"){
                            wp = TreeFunction::workingPointMap.at(pInfo[1]); 
                            function.wp = pInfo[1];
                            histName += "_" + pInfo[1];
                        }
                    }

                    //Fill values
                    arg.index.push_back(index);
                    arg.wp.push_back(wp);

                    partLabels.push_back(Utils::Format<int>("@", partLabel, index+1, arg.parts[index] == MET ? true : false));
                }
            }

            //Binning, e.g 30,0,10 -> 30 bins, xmin 0, xmax 30
            if(info[0] == "b"){
                std::vector<float> bins = Utils::SplitString<float>(Utils::FindInBracket(info[1]), ",");

                hist = new TH1F("", "", bins[0], bins[1], bins[2]);
                isHist=true;
            }

            if(info[0] == "c"){
                std::vector<std::string> cuts = Utils::SplitString<std::string>(info[1], ",");
                isCut=true;

                for(std::string& cut: cuts){
                    for(std::string cutInfo: Utils::SplitString<std::string>(Utils::FindInBracket(cut), ",")){
                        std::vector<std::string> cInfo = Utils::SplitString<std::string>(cutInfo, "=");
            
                        if(cInfo[0] == "n"){
                            arg.comp = TreeFunction::comparisonMap.at(cInfo[1]);
                        }

                        if(cInfo[0] == "v"){
                            arg.compValue = std::stof(cInfo[1]);
                        }
                    }
                }
            }
        }

        if(isHist){
            hist->SetName(histName.c_str());       
            hist->SetTitle(histName.c_str());  
            hist->SetDirectory(outFile);

            for(std::string& partLabel: partLabels){
                xLabel = Utils::Format<std::string>("@", xLabel, partLabel);
            }

            hist->GetXaxis()->SetTitle(xLabel.c_str());              

            hists.push_back(hist);
            histArgs.push_back(arg);
            histFunctions.push_back(function);
        }

        if(isCut){
            cutArgs.push_back(arg);
            cutFunctions.push_back(function);
        }
    }
}

void TreeReader::EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd, const std::string& cleanJet){
    //Get input tree
    TFile* inputFile = TFile::Open(fileName.c_str(), "READ");
    TTree* inputTree = (TTree*)inputFile->Get(channel.c_str());

    TH1::AddDirectory(kFALSE);
    gROOT->SetBatch(kTRUE);

    //Open output file and set up all histograms/tree and their function to call
    TFile* outFile = TFile::Open(outname.c_str(), "RECREATE");
    PrepareLoop(outFile);

    //Objects to hold values 
    std::map<Particle, std::vector<float>*> Px, Py, Pz, E, Isolation, looseSF, mediumSF, tightSF, triggerSF, recoSF, loosebTagSF, mediumbTagSF, tightbTagSF, FatJetIdx, isFromh, oneSubJettiness, twoSubJettiness, threeSubJettiness, looseIsoLooseSF, looseIsoMediumSF, looseIsoTightSF, tightIsoMediumSF, tightIsoTightSF;
    std::map<Particle, std::vector<bool>*> isLoose, isMedium, isTight, isLooseIso, isMediumIso, isTightIso, isLooseB, isMediumB, isTightB;

    //Name of TBranch mapped to objects holding the read out branch values
    std::map<std::string, std::map<Particle, std::vector<float>*>&> floatProperties = {
        {"Px", Px},
        {"Py", Py},
        {"Pz", Pz},
        {"E", E},
        {"FatJetIdx", FatJetIdx},
        {"looseSF", looseSF},
        {"mediumSF", mediumSF},
        {"tightSF", tightSF},
        {"triggerSF", triggerSF},
        {"recoSF", recoSF},
        {"loosebTagSF", loosebTagSF},
        {"mediumbTagSF", mediumbTagSF},
        {"tightbTagSF", tightbTagSF},
        {"looseIsoLooseSF", looseIsoLooseSF},
        {"looseIsoMediumSF", looseIsoMediumSF},
        {"looseIsoTightSF", looseIsoTightSF},
        {"tightIsoMediumSF", tightIsoMediumSF},
        {"tightIsoTightSF", tightIsoTightSF},
    };

    std::map<std::string, std::map<Particle, std::vector<bool>*>&> boolProperties = {
        {"isLoose", isLoose},
        {"isMedium", isMedium},
        {"isTight", isTight},
        {"isLooseB", isLooseB},
        {"isMediumB", isMediumB},
        {"isTightB", isTightB},
        {"isLooseIso", isLooseIso},
        {"isMediumIso", isMediumIso},
        {"isTightIso", isTightIso},
    };

    std::map<std::string, Particle> particles = {
        {"Electron", ELECTRON},
        {"Muon", MUON},
        {"Jet", JET},
        {"FatJet", FATJET},
    };

    //Set all branch address for all particles/quantities
    for(std::pair<const std::string, Particle>& part: particles){
        for(std::pair<const std::string, std::map<Particle, std::vector<float>*>&> property: floatProperties){
            std::string branchName = Utils::Join("_", {part.first, property.first});

            if(inputTree->GetListOfBranches()->Contains(branchName.c_str())){
                property.second[part.second] = NULL;
                inputTree->SetBranchAddress(branchName.c_str(), &property.second[part.second]);
            }
        }

        for(std::pair<const std::string, std::map<Particle, std::vector<bool>*>&> property: boolProperties){
            std::string branchName = Utils::Join("_", {part.first, property.first});

            if(inputTree->GetListOfBranches()->Contains(branchName.c_str())){
                property.second[part.second] = NULL;
                inputTree->SetBranchAddress(branchName.c_str(), &property.second[part.second]);
            }
        }
    }

    //Vector of  weights
    std::vector<std::string> weightNames = {"lumi", "xsec", "prefireWeight"};
    std::vector<float> weights(weightNames.size(), 0);

    for(unsigned int idx=0; idx < weightNames.size(); idx++){;
        inputTree->SetBranchAddress(("Weight_" + weightNames[idx]).c_str(), &weights[idx]);
    }

    //Set all other branch values needed
    float MET_Px, MET_Py, HT, eventNumber, nTrue, nGen=1.;

    inputTree->SetBranchAddress("MET_Px", &MET_Px); 
    inputTree->SetBranchAddress("MET_Py", &MET_Py);
    inputTree->SetBranchAddress("HT", &HT);
    inputTree->SetBranchAddress("Misc_eventNumber", &eventNumber);
    inputTree->SetBranchAddress("Misc_TrueInteraction", &nTrue);
    
    //Get number of generated events
    if(inputFile->GetListOfKeys()->Contains("nGen")){
        nGen = ((TH1F*)inputFile->Get("nGen"))->Integral();
    }

    //Calculate pile up weight histogram
    TH1F* pileUpWeight=NULL;
    bool isData=true;

    if(inputFile->GetListOfKeys()->Contains("puMC")){
        TH1F* puMC=NULL; TH1F* puReal=NULL;
        isData=false;

        puMC = (TH1F*)inputFile->Get("puMC");
        puReal = (TH1F*)inputFile->Get("pileUp");

        pileUpWeight = (TH1F*)puReal->Clone();
        pileUpWeight->Scale(1./pileUpWeight->Integral());
        puMC->Scale(1./puMC->Integral());

        pileUpWeight->Divide(puMC);
        delete puMC; delete puReal;
    }

    //Determine what to clean from jets
    std::vector<std::string> cleanInfo = Utils::SplitString<std::string>(cleanJet, "/");
    Particle partToClean; WP wpToClean = NONE;

    if(cleanJet!=""){
        partToClean=TreeFunction::partMap.at(cleanInfo[0]).first;
        wpToClean=TreeFunction::workingPointMap.at(cleanInfo[1]);
    }

    Event event;
    
    for (int i = entryStart; i < entryEnd; i++){
        //Load event
        inputTree->GetEntry(i);

        //Clear event
        event.particles.clear();
        event.SF.clear();
        event.weight = 1.;

        //Get all particles and put them into event container by particle type/working point
        for(const Particle part: {ELECTRON, MUON, JET, FATJET}){
            for(int j=0; j < Px[part]->size(); j++){
                ROOT::Math::PxPyPzEVector LV(Px[part]->at(j), Py[part]->at(j), Pz[part]->at(j), E[part]->at(j));
                float partWeight = 1.;

                if(part==ELECTRON){
                    if(isTight[part]->at(j) && isTightIso[part]->at(j)){
                        event.particles[part][TIGHT].push_back(LV);
                        event.SF[part][TIGHT].push_back(isData ? 1 : partWeight*tightSF[part]->at(j));
                    }

                    if(isMedium[part]->at(j) && isMediumIso[part]->at(j)){
                        event.particles[part][MEDIUM].push_back(LV);
                        event.SF[part][MEDIUM].push_back(isData ? 1 : partWeight*mediumSF[part]->at(j));
                    }

                    if(isLoose[part]->at(j) && isLooseIso[part]->at(j)){
                        event.particles[part][LOOSE].push_back(LV);
                        event.SF[part][LOOSE].push_back(isData ? 1 : partWeight*looseSF[part]->at(j));
                    }
                }

                if(part==MUON){
                    if(isTight[part]->at(j) && isLooseIso[part]->at(j)){
                        event.particles[part][TIGHT].push_back(LV);
                        event.SF[part][TIGHT].push_back(isData ? 1 : tightSF[part]->at(j)*looseIsoTightSF[part]->at(j)*triggerSF[part]->at(j));
                    }

                    if(isMedium[part]->at(j) && isLooseIso[part]->at(j)){
                        event.particles[part][MEDIUM].push_back(LV);
                        event.SF[part][MEDIUM].push_back(isData ? 1 : looseSF[part]->at(j)*looseIsoMediumSF[part]->at(j)*triggerSF[part]->at(j));
                    }

                    if(isLoose[part]->at(j) && isLooseIso[part]->at(j)){
                        event.particles[part][LOOSE].push_back(LV);
                        event.SF[part][LOOSE].push_back(isData ? 1 : looseSF[part]->at(j)*looseIsoLooseSF[part]->at(j)*triggerSF[part]->at(j));
                    }
                   
                    event.particles[part][NONE].push_back(LV);
                }

                if(part==JET){
                    bool isCleaned=true;

                    if(wpToClean != NONE){
                        for(ROOT::Math::PxPyPzEVector& p: event.particles[partToClean][wpToClean]){
                            if(ROOT::Math::VectorUtil::DeltaR(p, LV) < 0.4){
                                isCleaned=false;
                                break;
                            }
                        }
                    }

                    if(!isCleaned) continue;

                    bool isSubJet=false;
                    if(FatJetIdx[part]->size() > 0){
                        if(isSubJet) isSubJet=true; 
                    }

                    if(isTightB[part]->at(j)){
                        if(isSubJet){
                            event.particles[BSUBJET][TIGHT].push_back(LV);
                            event.SF[BSUBJET][TIGHT].push_back(isData ? 1 : tightbTagSF[part]->at(j));
                        }

                        else{
                            event.particles[BJET][TIGHT].push_back(LV);
                            event.SF[BJET][TIGHT].push_back(isData ? 1 : tightbTagSF[part]->at(j));
                        }
                    }

                    if(isMediumB[part]->at(j)){
                        if(isSubJet){
                            event.particles[BSUBJET][MEDIUM].push_back(LV);
                            event.SF[BSUBJET][MEDIUM].push_back(isData ? 1 : mediumbTagSF[part]->at(j));
                        }

                        else{
                            event.particles[BJET][MEDIUM].push_back(LV);
                            event.SF[BJET][MEDIUM].push_back(isData ? 1 : mediumbTagSF[part]->at(j));
                        }
                    }

                    if(isLooseB[part]->at(j)){
                        if(isSubJet){
                            event.particles[BSUBJET][LOOSE].push_back(LV);
                            event.SF[BSUBJET][LOOSE].push_back(isData ? 1 : loosebTagSF[part]->at(j));
                        }

                        else{
                            event.particles[BJET][LOOSE].push_back(LV);
                            event.SF[BJET][LOOSE].push_back(isData ? 1 : loosebTagSF[part]->at(j));
                        }
                    }

                    if(isSubJet) event.particles[SUBJET][NONE].push_back(LV);
                    else event.particles[JET][NONE].push_back(LV);
                }

                if(part==FATJET){
                    event.particles[FATJET][NONE].push_back(LV);
                }
            }
        }

        //MET
        event.particles[MET][NONE].push_back(ROOT::Math::PxPyPzEVector(MET_Px, MET_Py, 0, 0));
        //HT
        event.HT = HT;

        //Check if event passed all cuts
        bool passed=true;

        for(int j=0; j < cutFunctions.size(); j++){
            passed = passed && cutFunctions[j](event, cutArgs[j], true);
        }

        if(!passed) continue;

        //Multiply all common weights
        for(float& w: weights){
            event.weight *= w;
        }

        event.weight*= 1./nGen;
        if(pileUpWeight!=NULL) event.weight *= pileUpWeight->GetBinContent(pileUpWeight->FindBin(nTrue));

        //Fill histogram
        for(int j=0; j < hists.size(); j++){
            float xValue = histFunctions[j](event, histArgs[j]);
            hists[j]->Fill(xValue, event.weight);
        }
    }

    //Write all histograms and delete everything
    outFile->cd();
    for(TH1F* hist: hists){hist->Write(); delete hist;}

    delete inputTree; delete inputFile; delete outFile;
}
