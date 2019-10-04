#include <ChargedAnalysis/Analysis/include/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(std::string &process, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::vector<std::string> &cutStrings, std::string &outname, std::string &channel, const bool& saveTree, const bool& saveCsv):
    process(process),
    xParameters(xParameters),
    yParameters(yParameters),
    cutStrings(cutStrings),
    outname(outname),
    channel(channel),
    saveTree(saveTree),
    saveCsv(saveCsv){

    //Maps of all strings/enumeration
    strToOp = {{">", BIGGER}, {">=", EQBIGGER}, {"==", EQUAL}, {"<=", EQSMALLER}, {"<", SMALLER},  {"%", DIVISIBLE}, {"%!", NOTDIVISIBLE}};
    strToPart = {{"e", ELECTRON}, {"mu", MUON}, {"j", JET}, {"sj", SUBJET}, {"bsj", BSUBJET}, {"bj", BJET}, {"fj", FATJET}, {"bfj", BFATJET}, {"h1j", H1JET}, {"h2j", H2JET}, {"met", MET}, {"W", W}, {"Hc", HC}, {"genHc", GENHC}, {"h", h}, {"genh", GENH}};
    partLabel = {{ELECTRON, "e_{@}"}, {MUON, "#mu_{@}"}, {JET, "j_{@}"}, {SUBJET, "j^{sub}_{@}"}, {FATJET, "j_{@}^{AK8}"}, {BJET, "b-tagged j_{@}"}, {H1JET, "j^{h_{1}}_{@}"}, {H2JET, "j^{h_{2}}_{@}"}, {MET, "#vec{p}^{miss}_{T}"}, {W, "W^{#pm}"}, {HC, "H^{#pm}"}, {GENHC, "H^{#pm}_{gen}"}, {h, "h_{@}"}, {GENH, "h_{@}^{gen}"}};
    nPartLabel = {{ELECTRON, "electrons"}, {MUON, "muons"}, {JET, "jets"}, {FATJET, "fat jets"}, {BJET, "b-tagged jets"}, {BFATJET, "b-tagged fat jets"}, {SUBJET, "sub jets"}, {BSUBJET, "b-tagged sub jets"}};

    strToFunc = {
                    {"m", MASS}, 
                    {"phi", PHI}, 
                    {"eta", ETA}, 
                    {"pt", PT},     
                    {"dphi", DPHI},
                    {"dR", DR},
                    {"N", NPART},
                    {"HT", HT},
                    {"evNr", EVENTNUMBER},
                    {"bdt", BDTSCORE},
                    {"const", CONSTNUM},
                    {"Nsig", NSIGPART},
                    {"tau", SUBTINESS},
                    {"htag", HTAGGER},
    };
    
    funcLabel = {
                    {MASS, "m(@) [GeV]"}, 
                    {PHI, "#phi(@) [rad]"}, 
                    {ETA, "#eta(@) [rad]"}, 
                    {PT, "p_{T}(@) [GeV]"}, 
                    {DPHI, "#Delta#phi(@, @) [rad]"}, 
                    {DR, "#DeltaR(@, @) [rad]"},
                    {HT, "H_{T} [GeV]"}, 
                    {BDTSCORE, "BDT score"},
                    {NPART, "N_{@}"},
                    {CONSTNUM, "Bin number"},
                    {NSIGPART, "N^{gen matched}_{@}"},
                    {SUBTINESS, "#tau(@)"},
                    {HTAGGER, "Higgs score (@)"},
    };

    //Maps of all binning, functions and SF
    binning = {
                {MASS, {20., 100., 600.}},
                {PT, {30., 0., 200.}},
                {ETA, {30., -2.4, 2.4}},
                {PHI, {30., -TMath::Pi(), TMath::Pi()}},
                {DPHI, {30., 0., TMath::Pi()}},
                {DR, {30., 0., 6.}},
                {HT, {30., 0., 500.}},
                {NPART, {6., 0., 6.}},
                {BDTSCORE, {30., -0.5, 0.3}},
                {CONSTNUM, {3., 0., 2.}},
                {NSIGPART, {5., 0., 5.}},
                {SUBTINESS, {30., 0., 0.4}},
                {HTAGGER, {30., 0., 1.}},
    };

    funcDir = {
                {MASS, &TreeReader::Mass},
                {PT, &TreeReader::Pt},
                {PHI, &TreeReader::Phi},
                {ETA, &TreeReader::Eta},
                {DPHI, &TreeReader::DeltaPhi},
                {DR, &TreeReader::DeltaR},
                {HT, &TreeReader::HadronicEnergy},
                {NPART, &TreeReader::NParticle},
                {EVENTNUMBER, &TreeReader::EventNumber},
                {BDTSCORE, &TreeReader::BDTScore},
                {CONSTNUM, &TreeReader::ConstantNumber},
                {NSIGPART, &TreeReader::NSigParticle},
                {SUBTINESS, &TreeReader::Subtiness},
                {HTAGGER, &TreeReader::HTagger},
    };

    ID = [&](int WP, RecoParticle part){
        if(WP == 0) return part.isLoose && part.looseIso;
        else if(WP == 1) return part.isMedium && part.mediumIso;
        else if(WP == 2) return part.isTight && part.tightIso;

        return false;
    };

    SF = [&](int WP, RecoParticle part){
        return part.IDSF[(int)WP]*part.otherSF;
    };

    //Cut configuration
    for(std::string &cut: cutStrings){
        cuts.push_back(ConvertStringToEnums(cut, true));
    }

    //Check if cutflow should be written out
    std::vector<std::string>::iterator it = std::find(this->xParameters.begin(), this->xParameters.end(), "cutflow");

    if(it != this->xParameters.end()){
        writeCutFlow = true;
        this->xParameters.erase(it);
    }   
}

TreeReader::Hist TreeReader::ConvertStringToEnums(std::string &input, const bool &isCutString){
    //Function which handles splitting of string input
    std::vector<std::string> splittedString;
    std::string string;
    std::istringstream splittedStream(input);
    while (std::getline(splittedStream, string,  '_')){
        splittedString.push_back(string);
    }

    //Translate strings into enumeration
    Function func = strToFunc[splittedString[0]];
    float funcValue = -999.;
    Operator op; float cutValue;
    std::vector<Particle> parts;
    std::vector<int> indeces;

    //Check if variable is special
    if(func == BDTSCORE) isBDT = true;
    if(func == HTAGGER) isHTag = true;

    //If functions has special value
    int partStart = 1;
    int partEnd = isCutString ? 2 : 0;

    if(splittedString.size() > 1){
        try{
            funcValue = std::stof(splittedString[1]);
            partStart++;
        }

        catch(...){}
    }

    //If no particle is involved, like HT_>_100
    if(splittedString.size() == 3 and isCutString){
        op = strToOp[splittedString[1]];
        cutValue = std::stof(splittedString[2]);
    }
    
    else{
        for(std::string part: std::vector<std::string>(splittedString.begin()+partStart, splittedString.end() - partEnd)){ 
            try{
                indeces.push_back(std::stoi(std::string(part.end()-1 ,part.end())));
                parts.push_back(strToPart[std::string(part.begin(),part.end()-1)]);
            }

            catch(...){
                indeces.push_back(1);
                parts.push_back(strToPart[part]);
            }
        }

        if(isCutString){
            std::vector<std::string> cutVec(splittedString.end() - 2, splittedString.end());

            std::string fLabel = funcLabel[func];

            for(unsigned int k = 0; k < parts.size(); k++){
                std::string pLabel = (func != NPART and func != NSIGPART) ? partLabel[parts[k]] : nPartLabel[parts[k]];

                if(pLabel.find("@") != std::string::npos){
                    pLabel.replace(pLabel.find("@"), 1, std::to_string(indeces[k]));
                }

                if(fLabel.find("@") != std::string::npos){
                    fLabel.replace(fLabel.find("@"), 1, pLabel);
                }
            }

            std::string cutName = fLabel + " " + cutVec[0] + " " + cutVec[1];
            cutNames.push_back(cutName);

            op = strToOp[cutVec[0]];
            cutValue = std::stof(cutVec[1]);
        }
    }

    return {NULL, NULL, parts, indeces, func, funcValue, {op, cutValue}};
}

std::tuple<std::vector<TreeReader::Hist>, std::vector<std::vector<TreeReader::Hist>>> TreeReader::SetHistograms(TFile* outputFile){
    //Histograms
    std::vector<Hist> histograms1D;
    std::vector<std::vector<Hist>> histograms2D;

    //Save pairs of XY parameters to avoid redundant plots
    std::vector<std::string> parameterPairs;

    //Define final histogram for each parameter
    for(unsigned int i = 0; i < xParameters.size(); i++){
        //Vector for 2D hists
        std::vector<Hist> temp2DHist;

        //Split input string into information for particle and function to call value
        Hist confX = ConvertStringToEnums(xParameters[i]);

        //Create final histogram
        TH1F* hist1D = new TH1F(xParameters[i].c_str(), xParameters[i].c_str(), binning[confX.func][0], binning[confX.func][1], binning[confX.func][2]);
        hist1D->Sumw2();
        hist1D->SetDirectory(outputFile);

        std::string fLabelX = funcLabel[confX.func];
        
        for(unsigned int k = 0; k < confX.parts.size(); k++){
            std::string pLabel = (confX.func != NPART and confX.func != NSIGPART) ? partLabel[confX.parts[k]] : nPartLabel[confX.parts[k]];

            if(pLabel.find("@") != std::string::npos){
                pLabel.replace(pLabel.find("@"), 1, std::to_string(confX.indeces[k]));
            }

            if(fLabelX.find("@") != std::string::npos){
                fLabelX.replace(fLabelX.find("@"), 1, pLabel);
            }
        }

        hist1D->GetXaxis()->SetTitle(fLabelX.c_str());
        confX.hist1D = hist1D;

        //Add hist to collection
        histograms1D.push_back(confX);

        for(unsigned int j = 0; j < yParameters.size(); j++){
            //Split input string into information for particle and function to call value
            Hist confY = ConvertStringToEnums(yParameters[j]);

            bool isNotRedundant = true;

            for(std::string pair: parameterPairs){
                if(pair.find(xParameters[i]) == std::string::npos and pair.find(yParameters[j]) == std::string::npos) isNotRedundant = false;
            }

            if(isNotRedundant and xParameters[i] != yParameters[j]){
                parameterPairs.push_back(xParameters[i] + yParameters[j]);
            
                TH2F* hist2D = new TH2F((xParameters[i] + "_VS_" + yParameters[j]).c_str() , (xParameters[i] + "_VS_" + yParameters[j]).c_str(), binning[confX.func][0], binning[confX.func][1], binning[confX.func][2], binning[confY.func][0], binning[confY.func][1], binning[confY.func][2]);
                hist2D->Sumw2();
                hist2D->SetDirectory(outputFile);

                std::string fLabelY = funcLabel[confY.func];

                for(unsigned int k = 0; k < confY.parts.size(); k++){
                    std::string pLabel = (confY.func != NPART and confY.func != NSIGPART) ? partLabel[confY.parts[k]] : nPartLabel[confY.parts[k]];
                    if(pLabel.find("@") != std::string::npos){
                        pLabel.replace(pLabel.find("@"), 1, std::to_string(confY.indeces[k]));
                    }

                    if(fLabelY.find("@") != std::string::npos){
                        fLabelY.replace(fLabelY.find("@"), 1, pLabel);
                    }
                }

                hist2D->GetXaxis()->SetTitle(fLabelX.c_str());
                hist2D->GetYaxis()->SetTitle(fLabelY.c_str());
                confY.hist2D = hist2D;
                temp2DHist.push_back(confY);
            }
        }

        histograms2D.push_back(temp2DHist);
    }

    return {histograms1D, histograms2D};
}

std::vector<std::vector<std::pair<int, int>>> TreeReader::EntryRanges(std::vector<std::string> &filenames, int &nJobs, std::string &channel, const float &frac){
    std::vector<int> jobsPerFile(filenames.size(), 0); 

    //Calculate number of jobs per file
    for(unsigned int i = 0; i < jobsPerFile.size(); i++){
        for(int j = 0; j < std::floor(nJobs/jobsPerFile.size()); j++){
             jobsPerFile[i]++;
        }
    }

    std::vector<std::vector<std::pair<int, int>>> entryRange(filenames.size(), std::vector<std::pair<int, int>>());

    for(unsigned int i = 0; i < filenames.size(); i++){
        //Get nGen for each file
        TFile* file = TFile::Open(filenames[i].c_str());
        TTree* tree = NULL;

        if(!file->GetListOfKeys()->Contains(channel.c_str())){
            std::cout << file->GetName() << " has no event tree. It will be skipped.." << std::endl;
        }

        else tree = (TTree*)file->Get(channel.c_str());

        for(int j = 0; j < jobsPerFile[i]; j++){
            if(j != jobsPerFile[i] - 1 and tree != NULL){
                entryRange[i].push_back({j*tree->GetEntries()*frac/jobsPerFile[i], (j+1)*tree->GetEntries()*frac/jobsPerFile[i]});
            }

            else if(tree != NULL){
                entryRange[i].push_back({j*tree->GetEntries()*frac/jobsPerFile[i], tree->GetEntries()*frac});
            }

            else{
                entryRange[i].push_back({-1., -1.});
            }
        }
    }

    return entryRange;
}

void TreeReader::EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd){
    TH1::AddDirectory(kFALSE);
    gROOT->SetBatch(kTRUE);
    gPrintViaErrorHandler = kTRUE;  
    gErrorIgnoreLevel = kWarning;

    //ROOT files
    TFile* inputFile = TFile::Open(fileName.c_str(), "READ");
    TFile* outputFile = TFile::Open(std::string(process + "_" + std::to_string(getpid()) + ".root").c_str(), "RECREATE");

    //Define containers for histograms
    std::vector<Hist> histograms1D;
    std::vector<std::vector<Hist>> histograms2D;
    TTree* outputTree = new TTree(process.c_str(), process.c_str());
    outputTree->SetDirectory(outputFile);

    //Set up histograms
    std::tie(histograms1D, histograms2D) = SetHistograms(outputFile);

    //vector with values
    std::vector<float> valuesX(histograms1D.size(), 1.);

    //Create thread local histograms/branches
    for(unsigned int i = 0; i < histograms1D.size(); i++){
        if(saveTree) outputTree->Branch(xParameters[i].c_str(), &valuesX[i]);
    }

    //Define TTreeReader 
    TTree* inputTree = (TTree*)inputFile->Get(channel.c_str());

    //Vector of particle
    std::map<std::string, std::vector<std::string>> floatVariables = {
        {"Muon_", {"E", "Px", "Py", "Pz", "Charge", "looseIsoMediumSF", "tightIsoMediumSF", "looseIsoTightSF", "tightIsoTightSF", "mediumSF", "tightSF", "triggerSF"}},
        {"Electron_", {"E", "Px", "Py", "Pz", "Isolation", "Charge", "mediumSF", "tightSF", "recoSF"}},
        {"Jet_", {"E", "Px", "Py", "Pz", "loosebTagSF", "mediumbTagSF", "tightbTagSF", "FatJetIdx", "isFromh"}},
        {"FatJet_", {"E", "Px", "Py", "Pz", "oneSubJettiness", "twoSubJettiness", "threeSubJettiness", "loosebTagSF", "mediumbTagSF", "isFromh"}},
    };

    std::map<std::string, std::vector<std::string>> boolVariables = {
        {"Muon_", {"isMediumIso", "isTightIso", "isMedium", "isTight", "isTriggerMatched", "isFromHc"}},
        {"Electron_", {"isMedium", "isTight", "isTriggerMatched", "isFromHc"}},
        {"Jet_", {"isLooseB", "isMediumB", "isTightB"}},
        {"FatJet_", {"isMediumB", "isTightB"}},
    };
        
    std::vector<std::vector<float>*> muonVecFloat(floatVariables["Muon_"].size(), NULL);
    std::vector<std::vector<float>*> eleVecFloat(floatVariables["Electron_"].size(), NULL);
    std::vector<std::vector<float>*> jetVecFloat(floatVariables["Jet_"].size(), NULL);
    std::vector<std::vector<float>*> fatjetVecFloat(floatVariables["FatJet_"].size(), NULL);

    std::vector<std::vector<bool>*> muonVecBool(boolVariables["Muon_"].size(), NULL);
    std::vector<std::vector<bool>*> eleVecBool(boolVariables["Electron_"].size(), NULL);
    std::vector<std::vector<bool>*> jetVecBool(boolVariables["Jet_"].size(), NULL);
    std::vector<std::vector<bool>*> fatjetVecBool(boolVariables["FatJet_"].size(), NULL);

    for(const std::string& partName: {"Muon_", "Electron_", "Jet_", "FatJet_"}){
        for(unsigned int idx=0; idx < floatVariables[partName].size(); idx++){
            if(partName == "Muon_"){
                inputTree->SetBranchAddress((partName + floatVariables[partName][idx]).c_str(), &muonVecFloat[idx]);
            }

            if(partName == "Electron_"){
                inputTree->SetBranchAddress((partName + floatVariables[partName][idx]).c_str(), &eleVecFloat[idx]);
            }

            if(partName == "Jet_"){
                inputTree->SetBranchAddress((partName + floatVariables[partName][idx]).c_str(), &jetVecFloat[idx]);
            }

            if(partName == "FatJet_"){
                inputTree->SetBranchAddress((partName + floatVariables[partName][idx]).c_str(), &fatjetVecFloat[idx]);
            }
        }

        for(unsigned int idx=0; idx < boolVariables[partName].size(); idx++){
            if(partName == "Muon_"){
                inputTree->SetBranchAddress((partName + boolVariables[partName][idx]).c_str(), &muonVecBool[idx]);
            }

            if(partName == "Electron_"){
                inputTree->SetBranchAddress((partName + boolVariables[partName][idx]).c_str(), &eleVecBool[idx]);
            }

            if(partName == "Jet_"){
                inputTree->SetBranchAddress((partName + boolVariables[partName][idx]).c_str(), &jetVecBool[idx]);
            }

            if(partName == "FatJet_"){
                inputTree->SetBranchAddress((partName + boolVariables[partName][idx]).c_str(), &fatjetVecBool[idx]);
            }
        }
    }

    //Other things to read out  
    float MET_Px; float MET_Py; float HT; float eventNumber;

    inputTree->SetBranchAddress("MET_Px", &MET_Px); 
    inputTree->SetBranchAddress("MET_Py", &MET_Py);
    inputTree->SetBranchAddress("HT", &HT);
    inputTree->SetBranchAddress("Misc_eventNumber", &eventNumber);
    
    //Vector of  weights
    std::vector<std::string> weightNames = {"lumi", "xsec"};
    std::vector<float> weights(weightNames.size(), 0);

    for(unsigned int idx=0; idx < weightNames.size(); idx++){;
        inputTree->SetBranchAddress(("Weight_" + weightNames[idx]).c_str(), &weights[idx]);
    }  

    //Other stuff 
    float nTrue=0.;
    inputTree->SetBranchAddress("Misc_TrueInteraction", &nTrue);

    //For DNN Higgs Tagger
    std::vector<std::string> particleVariables = {"E", "Px", "Py", "Pz", "Vx", "Vy", "Vz", "Charge", "FatJetIdx"};
    std::vector<std::vector<float>*> particleVec(particleVariables.size(), NULL);
    std::vector<std::vector<float>*> secVtxVec(particleVariables.size(), NULL);

    if(isHTag){
        for(unsigned int idx=0; idx < particleVariables.size(); idx++){
            inputTree->SetBranchAddress(("JetParticle_" + particleVariables[idx]).c_str(), &particleVec[idx]);
            inputTree->SetBranchAddress(("SecondaryVertex_" + particleVariables[idx]).c_str(), &secVtxVec[idx]);
        }
    
        classifier.SetModel("jetmodel", "JetModel", std::string(std::getenv("CHDIR")) + "/DNN/Model/jetmodel_094.h5");
    }

    //Number of generated events
    float nGen = 1.;

    if(inputFile->GetListOfKeys()->Contains("nGen")){
        TH1F* genHist = (TH1F*)inputFile->Get("nGen");
        nGen = genHist->Integral();
        delete genHist;
    }


    //Calculate pile up weight histogram
    TH1F* puMC=NULL; TH1F* pileUpWeight=NULL;

    if(inputFile->GetListOfKeys()->Contains("puMC")){
        puMC = (TH1F*)inputFile->Get("puMC");
        pileUpWeight = (TH1F*)puMC->Clone(); pileUpWeight->Clear();

        pileUpWeight->SetName("pileUpWeight");
        inputTree->Draw("Misc_TrueInteraction>>pileUpWeight", "Misc_TrueInteraction > 1 && Misc_TrueInteraction < 200");
        pileUpWeight->Divide(puMC);
        delete puMC;
    }

    //Cutflow
    TH1F* cutflow = (TH1F*)inputFile->Get(("cutflow_" + channel).c_str())->Clone();
    cutflow->SetName("cutflow"); cutflow->SetTitle("cutflow");
    cutflow->Scale((1./nGen)*(entryEnd-entryStart)/inputTree->GetEntries());

    //BDT intialization
    if(isBDT){
        std::map<std::string, std::string> chanPaths = {
                    {"e4j", "Ele4J"},
                    {"mu4j", "Muon4J"},
                    {"e2j1f", "Ele2J1F"},
                    {"mu2j1f", "Muon2J1F"},
                    {"e2f", "Ele2F"},
                    {"mu2f", "Muon2F"},
        };

        std::string bdtPath = std::string(std::getenv("CHDIR")) + "/BDT/" + chanPaths[channel]; 

        std::vector<std::string> bdtVar = evenClassifier.SetEvaluation(bdtPath + "/Even/");
        oddClassifier.SetEvaluation(bdtPath + "/Odd/");

        bdtVar.pop_back();

        for(std::string param: bdtVar){
            bdtFunctions.push_back(ConvertStringToEnums(param));
        }
    }

    Event event;

    for (int i = entryStart; i < entryEnd; i++){
        //Load event and fill event class
        inputTree->GetEntry(i); 

        //Clear events 
        event.Clear(); 

        //Fill vectors with numbers
        for(unsigned int idx=0; idx < muonVecFloat[0]->size(); idx++){
            RecoParticle muon;

            muon.LV = ROOT::Math::PxPyPzEVector(muonVecFloat[1]->at(idx), muonVecFloat[2]->at(idx), muonVecFloat[3]->at(idx), muonVecFloat[0]->at(idx));
            if(muonVecFloat[6]->size() != 0){
                muon.IDSF = {1.,  muonVecFloat[6]->at(idx)*muonVecFloat[9]->at(idx), muonVecFloat[8]->at(idx)*muonVecFloat[10]->at(idx)};
                muon.otherSF = muonVecFloat[11]->at(idx);
            }

            muon.mediumIso = muonVecBool[0]->at(idx);
            muon.tightIso = muonVecBool[1]->at(idx);
            muon.isMedium = muonVecBool[2]->at(idx);
            muon.isTight = muonVecBool[3]->at(idx);
            muon.isTriggerMatched = muonVecBool[4]->at(idx);

            if(muonVecBool[5]->size()!=0){
                muon.isFromSignal = muonVecBool[5]->at(idx);
            }

            event.particles[MUON].push_back(muon);
        }

        for(unsigned int idx=0; idx < eleVecFloat[0]->size(); idx++){
            RecoParticle electron;

            electron.LV = ROOT::Math::PxPyPzEVector(eleVecFloat[1]->at(idx), eleVecFloat[2]->at(idx), eleVecFloat[3]->at(idx), eleVecFloat[0]->at(idx));
            if(eleVecFloat[6]->size() != 0){
                electron.IDSF = {1., eleVecFloat[6]->at(idx), eleVecFloat[7]->at(idx)};
                electron.otherSF = eleVecFloat[8]->at(idx);
            }

            electron.mediumIso = eleVecFloat[4]->at(idx) < 0.2;
            electron.tightIso = eleVecFloat[4]->at(idx) < 0.1;
            electron.isMedium = eleVecBool[0]->at(idx);
            electron.isTight = eleVecBool[1]->at(idx);
            electron.isTriggerMatched = eleVecBool[2]->at(idx);

            if(eleVecBool[3]->size()!=0){
                electron.isFromSignal = eleVecBool[3]->at(idx);
            }

            event.particles[ELECTRON].push_back(electron);
        }

        for(unsigned int idx=0; idx < jetVecFloat[0]->size(); idx++){
            RecoParticle jet;

            jet.LV = ROOT::Math::PxPyPzEVector(jetVecFloat[1]->at(idx), jetVecFloat[2]->at(idx), jetVecFloat[3]->at(idx), jetVecFloat[0]->at(idx));
       
            jet.isLoose = jetVecBool[0]->at(idx);
            jet.isMedium = jetVecBool[1]->at(idx);
            jet.isTight = jetVecBool[2]->at(idx);

            if(jetVecFloat[8]->size() != 0){
                jet.IDSF = {jetVecFloat[4]->at(idx), jetVecFloat[5]->at(idx), jetVecFloat[6]->at(idx)};
                jet.isFromSignal = jetVecFloat[8]->at(idx);
            }

            if(jetVecFloat[7]->size() == 0) event.particles[JET].push_back(jet);
            else if(jetVecFloat[7]->at(idx) == -1) event.particles[JET].push_back(jet);
            else event.particles[SUBJET].push_back(jet);
        }

        for(unsigned int idx=0; idx < fatjetVecFloat[0]->size(); idx++){
            RecoParticle fatJet;

            fatJet.LV = ROOT::Math::PxPyPzEVector(fatjetVecFloat[1]->at(idx), fatjetVecFloat[2]->at(idx), fatjetVecFloat[3]->at(idx), fatjetVecFloat[0]->at(idx));

            event.particles[FATJET].push_back(fatJet);
        }

        if(isHTag){
            for(unsigned int idx=0; idx < particleVec[0]->size(); idx++){
                RecoParticle jetParticle;
                jetParticle.LV = ROOT::Math::PxPyPzEVector(particleVec[1]->at(idx), particleVec[2]->at(idx), particleVec[3]->at(idx), particleVec[0]->at(idx));
                jetParticle.Vtx = ROOT::Math::XYZVector(particleVec[4]->at(idx), particleVec[5]->at(idx), particleVec[6]->at(idx));
                jetParticle.charge = particleVec[7]->at(idx);
                jetParticle.FatJetIdx = particleVec[8]->at(idx);
   
                event.particles[JPART].push_back(jetParticle);
            }

            for(unsigned int idx=0; idx < secVtxVec[0]->size(); idx++){
                RecoParticle secVtx;
                secVtx.LV = ROOT::Math::PxPyPzEVector(secVtxVec[1]->at(idx), secVtxVec[2]->at(idx), secVtxVec[3]->at(idx), secVtxVec[0]->at(idx));
                secVtx.Vtx = ROOT::Math::XYZVector(secVtxVec[4]->at(idx), secVtxVec[5]->at(idx), secVtxVec[6]->at(idx));
                secVtx.charge = secVtxVec[7]->at(idx);
                secVtx.FatJetIdx = secVtxVec[8]->at(idx);
   
                event.particles[SV].push_back(secVtx);
            }
        }

        RecoParticle met;
        met.LV = ROOT::Math::PxPyPzEVector(MET_Px, MET_Py, 0, 0);
        event.particles[MET].push_back(met);

        event.eventNumber = eventNumber;
        event.HT = HT;

        //Fill additional weights
        for(float& weight: weights){
            event.weight *= weight;
        }

        event.weight *= 1./nGen;

        if(pileUpWeight!=NULL){
            //event.weight *= nTrue/pileUpWeight->GetBinContent(pileUpWeight->FindBin(nTrue));
        }

        WBoson(event);
        Higgs(event);

        //Check if event passes cut
        bool passedCut = true;
 
        for(unsigned int k = 0; k < cuts.size(); k++){
            passedCut = Cut(event, cuts[k]);

            //Fill Cutflow if event passes selection
            if(passedCut){
                cutflow->Fill(cutNames[k].c_str(), event.weight);
            }

            else{break;}
        }

        if(passedCut){
            valuesX.clear();
            std::string csvString; 

            for(unsigned int l = 0; l < histograms1D.size(); l++){
                valuesX.push_back((this->*funcDir[histograms1D[l].func])(event, histograms1D[l]));
                histograms1D[l].hist1D->Fill(valuesX[l], event.weight);

                //Write information in string for csv
                if(saveCsv){
                    csvString+=std::to_string(valuesX[l]) + ",";
                }

               for(unsigned int m = 0; m < histograms2D[l].size(); m++){
                    histograms2D[l][m].hist2D->Fill(valuesX[l], (this->*funcDir[histograms2D[l][m].func])(event, histograms2D[l][m]), event.weight);
                }
            }

            //Fill tree if wished
            if(saveTree) outputTree->Fill();

            //Fill csv vector if wished
            if(saveCsv){
                csvString.replace(csvString.end()-1, csvString.end(), "\n");
                csvData.push_back(csvString);
            };
        }
    }

    //Write histograms
    outputFile->cd();
        
    if(writeCutFlow) cutflow->Write();
    if(saveTree) outputTree->Write();
    
    else{
        for(unsigned int l = 0; l < histograms1D.size(); l++){
            histograms1D[l].hist1D->Write();
            delete histograms1D[l].hist1D;

            for(unsigned int m = 0; m < histograms2D[l].size(); m++){
                histograms2D[l][m].hist2D->Write();
                delete histograms2D[l][m].hist2D;
            }
        }
    }

    delete inputTree;
    delete outputTree;
    delete outputFile;
    delete inputFile;
    delete cutflow;
    delete pileUpWeight;
}

void TreeReader::Merge(){
    std::system(std::string("hadd -f -v 0 " + outname + " " + process + "_*").c_str());
    std::system(std::string("command rm " + process + "_*").c_str());

    TSeqCollection* fileList = gROOT->GetListOfFiles();

    for(int i=0; i < fileList->GetSize(); i++){
        delete fileList->At(i);
    }
}
