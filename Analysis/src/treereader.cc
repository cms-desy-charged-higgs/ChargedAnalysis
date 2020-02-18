#include <ChargedAnalysis/Analysis/include/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(const std::string &process, const std::vector<std::string> &xParameters, const std::vector<std::string> &yParameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel, const std::string& saveMode):
    process(process),
    xParameters(xParameters),
    yParameters(yParameters),
    cutStrings(cutStrings),
    outname(outname),
    channel(channel){
}

void TreeReader::PrepareLoop(TFile* outFile){
    for(const std::string& xParameter: Utils::Merge<std::string>(xParameters, cutStrings)){
        //Functor structure and arguments
        Function function;
        FuncArgs arg;
        TH1F* hist; TTree* tree;

        bool isHist=false, isCut=false;

        //Histogram
        std::string histName = "", xLabel = "";
        std::vector<std::string> partLabels;

        for(std::string& parameterInfo: Utils::SplitString<std::string>(xParameter, "/")){
            std::vector<std::string> info = Utils::SplitString<std::string>(parameterInfo, ":");

            //Which function for property to call e.g. pt -> TreeFunction::Pt
            if(info[0] == "f"){
                for(std::string funcInfo: Utils::SplitString<std::string>(Utils::FindInBracket(info[1]), ",")){
                     std::vector<std::string> fInfo = Utils::SplitString<std::string>(funcInfo, "=");
                       
                     //Default values
                     function.func = TreeFunction::funcMap["constant"].first; int value=-999;

                     if(fInfo[0] == "n"){
                        function.func = TreeFunction::funcMap[fInfo[1]].first; 
                        function.funcName = fInfo[1];
                        xLabel = TreeFunction::funcMap[fInfo[1]].second;
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
                            arg.parts.push_back(TreeFunction::partMap[pInfo[1]].first); 
                            function.partName = pInfo[1];
                            histName += "_" + pInfo[1];
                            partLabel = TreeFunction::partMap[pInfo[1]].second;
                        }

                        if(pInfo[0] == "i"){
                            index = std::atoi(pInfo[1].c_str()) - 1; 
                            function.index = pInfo[1];
                            histName += "_" + pInfo[1];
                        }

                        if(pInfo[0] == "wp"){
                            wp = TreeFunction::workingPointMap[pInfo[1]]; 
                            function.wp = pInfo[1];
                            histName += "_" + pInfo[1];
                        }
                    }

                    //Fill values
                    arg.index.push_back(index);
                    arg.wp.push_back(wp);

                    partLabels.push_back(Utils::Format<int>("@", partLabel, index+1));
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
                            arg.comp = TreeFunction::comparisonMap[cInfo[1]];
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

void TreeReader::EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd){
    TFile* inputFile = TFile::Open(fileName.c_str(), "READ");
    TTree* inputTree = (TTree*)inputFile->Get(channel.c_str());

    TH1::AddDirectory(kFALSE);
    gROOT->SetBatch(kTRUE);

    TFile* outFile = TFile::Open(outname.c_str(), "RECREATE");
    PrepareLoop(outFile);

    std::map<Particle, std::vector<float>*> Px, Py, Pz, E, Isolation, looseSF, mediumSF, tightSF, triggerSF, recoSF, loosebTagSF, mediumbTagSF, tightbTagSF, FatJetIdx, isFromh, oneSubJettiness, twoSubJettiness, threeSubJettiness;
    std::map<Particle, std::vector<bool>*> isLoose, isMedium, isTight, isLooseIso, isTightIso, isLooseB, isMediumB, isTightB;

    float MET_Px, MET_Py, HT, eventNumber, nTrue, nGen;

    std::map<std::string, std::map<Particle, std::vector<float>*>&> floatProperties = {
        {"Px", Px},
        {"Py", Py},
        {"Pz", Pz},
        {"E", E},
        {"Isolation", Isolation},
        {"FatJetIdx", FatJetIdx},
    };

    std::map<std::string, std::map<Particle, std::vector<bool>*>&> boolProperties = {
        {"isLoose", isLoose},
        {"isMedium", isMedium},
        {"isTight", isTight},
        {"isLooseB", isLooseB},
        {"isMediumB", isMediumB},
        {"isTightB", isTightB},
        {"isLooseIso", isLooseIso},
        {"isTightIso", isTightIso},
    };

    std::map<std::string, Particle> particles = {
        {"Electron", ELECTRON},
        {"Muon", MUON},
        {"Jet", JET},
        {"FatJet", FATJET},
    };

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

    Event event;

    for (int i = entryStart; i < entryEnd; i++){
        //Load event and fill event class
        inputTree->GetEntry(i);

        event.particles.clear();

        for(const Particle part: {ELECTRON, MUON, JET, FATJET}){
            for(int j=0; j < Px[part]->size(); j++){
                ROOT::Math::PxPyPzEVector LV(Px[part]->at(j), Py[part]->at(j), Pz[part]->at(j), E[part]->at(j));

                if(part==ELECTRON){
                    if(isMedium[part]->at(j) && Isolation[part]->at(j) < 0.2) event.particles[part][MEDIUM].push_back(LV);
                    else if(isTight[part]->at(j) & Isolation[part]->at(j) < 0.1) event.particles[part][TIGHT].push_back(LV);
                    else event.particles[part][NONE].push_back(LV);
                }

                if(part==MUON){
                    if(isLoose[part]->at(j) && isLooseIso[part]->at(j)) event.particles[part][LOOSE].push_back(LV);
                    else if(isTight[part]->at(j) && isTightIso[part]->at(j)) event.particles[part][TIGHT].push_back(LV);
                    else event.particles[part][NONE].push_back(LV);
                }

                if(part==JET){
                    bool isSubJet=false;
                    if(FatJetIdx[part]->size() > 0){
                        if(isSubJet) isSubJet=true; 
                    }

                    if(isLooseB[part]->at(j)){
                        if(isSubJet) event.particles[BJET][LOOSE].push_back(LV);
                        else event.particles[BSUBJET][LOOSE].push_back(LV);
                    }

                    else if(isMediumB[part]->at(j)){
                        if(isSubJet) event.particles[BJET][MEDIUM].push_back(LV);
                        else event.particles[BSUBJET][MEDIUM].push_back(LV);
                    }

                    else if(isTightB[part]->at(j)){
                        if(isSubJet) event.particles[BJET][TIGHT].push_back(LV);
                        else event.particles[BSUBJET][TIGHT].push_back(LV);
                    }

                    else{
                        if(isSubJet) event.particles[JET][NONE].push_back(LV);
                        else event.particles[SUBJET][NONE].push_back(LV);
                    }
                }

                if(part==FATJET){
                    if(isLooseB[part]->at(j)) event.particles[BFATJET][LOOSE].push_back(LV);
                    else if(isMediumB[part]->at(j)) event.particles[BFATJET][MEDIUM].push_back(LV);
                    else event.particles[FATJET][NONE].push_back(LV);
                }

                else{
                    event.particles[part][NONE].push_back(LV);
                }
            }
        }

        //Check if event passed all cuts
        bool passed=true;

        for(int j=0; j < cutFunctions.size(); j++){
            passed = passed && cutFunctions[j](event, cutArgs[j], true);
        }

        if(!passed) continue;

        //Fill histogram
        for(int j=0; j < hists.size(); j++){
            float xValue = histFunctions[j](event, histArgs[j]);
            hists[j]->Fill(xValue);
        }
    }

    std::cout << "Loop finished" << std::endl;
    outFile->cd();
    for(TH1F* hist: hists){hist->Write(); delete hist;}

    delete inputTree; delete inputFile; delete outFile;
}
