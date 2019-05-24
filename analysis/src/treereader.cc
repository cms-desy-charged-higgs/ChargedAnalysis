#include <ChargedHiggs/analysis/interface/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(std::string &process, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::vector<std::string> &cutStrings, std::string &outname, std::string &channel, const bool& saveTree):
    process(process),
    xParameters(xParameters),
    yParameters(yParameters),
    cutStrings(cutStrings),
    outname(outname),
    channel(channel),
    saveTree(saveTree){

    //Start measure execution time
    start = std::chrono::steady_clock::now();
    outputFile = TFile::Open(outname.c_str(), "RECREATE");

    //Maps of all strings/enumeration
    strToOp = {{">", BIGGER}, {">=", EQBIGGER}, {"==", EQUAL}, {"<=", EQSMALLER}, {"<", SMALLER},  {"%", DIVISIBLE}, {"%!", NOTDIVISIBLE}};
    strToPart = {{"e", ELECTRON}, {"mu", MUON}, {"j", JET}, {"sj", SUBJET}, {"bj", BJET}, {"fj", FATJET}, {"bfj", BFATJET}, {"met", MET}, {"W", W}, {"Hc", HC}, {"h", h}};
    partLabel = {{ELECTRON, "e_{@}"}, {MUON, "#mu_{@}"}, {JET, "j_{@}"}, {SUBJET, "j^{sub}_{@}"}, {FATJET, "j_{q}^{AK8}"}, {BJET, "b-tagged j_{@}"}, {MET, "#slash{E}_{T}"}, {W, "W^{#pm}"}, {HC, "H^{#pm}"}, {h, "h_{@}"}};
    nPartLabel = {{ELECTRON, "electrons"}, {MUON, "muons"}, {JET, "jets"}, {FATJET, "fat jets"}, {BJET, "b-tagged jets"}, {BJET, "b-tagged fat jets"}, {SUBJET, "sub jets"}};

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
                    {NPART, "Number of @"},
                    {CONSTNUM, "Bin number"},
                    {NSIGPART, "Number of signal @"},
    };

    //Maps of all binning, functions and SF
    binning = {
                {MASS, {20., 100., 700.}},
                {PT, {30., 0., 200.}},
                {ETA, {30., -2.4, 2.4}},
                {PHI, {30., -TMath::Pi(), TMath::Pi()}},
                {DPHI, {30., 0., TMath::Pi()}},
                {DR, {30., 0., 6.}},
                {HT, {30., 0., 500.}},
                {NPART, {6., 0., 6.}},
                {BDTSCORE, {30., -0.5, 0.5}},
                {CONSTNUM, {3., 0., 2.}},
                {NSIGPART, {5., 0., 5.}},
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
    };

    eleID = {{1, {&Electron::isMedium, 0.2}}, {1, {&Electron::isTight, 0.1}}};
    eleSF = {{1, &Electron::mediumMvaSF}, {2, &Electron::mediumMvaSF}};

    muonID = {{1, {&Muon::isMedium, &Muon::isLooseIso}}, {2, {&Muon::isTight, &Muon::isTightIso}}};
    muonSF = {{1, {&Muon::mediumSF, &Muon::looseIsoMediumSF}}, {2, {&Muon::tightSF, &Muon::tightIsoTightSF}}};

    bJetID = {{0, &Jet::isLooseB}, {1, &Jet::isMediumB}, {2, &Jet::isTightB}};
    bJetSF = {{0, &Jet::loosebTagSF}, {1, &Jet::mediumbTagSF}, {2, &Jet::tightbTagSF}};
}

void TreeReader::ProgressBar(const int &progress){
    std::string progressBar = "["; 

    for(int i = 0; i < progress; i++){
        if(i%2) progressBar += "#";
    }

    for(int i = 0; i < 100 - progress; i++){
        if(i%2) progressBar += " ";
    }

    progressBar = progressBar + "] " + "Progress of process " + process + ": " + std::to_string(progress) + "%";
    std::cout << "\r" << progressBar << std::flush;

    if(progress == 100) std::cout << std::endl;

}

void TreeReader::ParallelisedLoop(const std::vector<TChain*> &chainWrapper, const int &entryStart, const int &entryEnd, const float &nGen)
{
    //Dont start immediately to avoid crashes of ROOT
    std::this_thread::sleep_for (std::chrono::seconds(1));

    //Define containers for histograms
    std::vector<Hist> histograms1D(merged1DHistograms.size(), Hist());
    std::vector<std::vector<Hist>> histograms2D;

    for(unsigned int i = 0; i < merged1DHistograms.size(); i++){
        histograms2D.push_back(std::vector<Hist>(merged2DHistograms[i].size(), Hist()));
    }

    //Lock thread unsafe operation
    mutex.lock();

    //vector with values
    std::vector<float> valuesX(merged1DHistograms.size(), 1.);

    //Tree that could be safed if wished
    TFile* file(NULL); TTree* tree(NULL);

    if(saveTree){
        file = TFile::Open(std::string(process + "_" + std::to_string(nHist) + ".root").c_str(), "RECREATE");
        tree = new TTree(process.c_str(), process.c_str());
    }

    //Create thread local histograms/branches
    for(unsigned int i = 0; i < merged1DHistograms.size(); i++){
        TH1F* hist1D = new TH1F(std::string("h" + std::to_string(nHist)).c_str(), std::string("h" + std::to_string(nHist)).c_str(), binning[merged1DHistograms[i].func][0], binning[merged1DHistograms[i].func][1], binning[merged1DHistograms[i].func][2]);
        nHist++;

        hist1D->Sumw2();
        histograms1D[i] = {hist1D, NULL, merged1DHistograms[i].parts, merged1DHistograms[i].indeces, merged1DHistograms[i].func, merged1DHistograms[i].funcValue};

        if(saveTree) tree->Branch(xParameters[i].c_str(), &valuesX[i]);

        for(unsigned int j = 0; j < merged2DHistograms[i].size(); j++){
            TH2F* hist2D = new TH2F(std::string("h" + std::to_string(nHist)).c_str(), std::string("h" + std::to_string(nHist)).c_str(), binning[merged1DHistograms[i].func][0], binning[merged1DHistograms[i].func][1], binning[merged1DHistograms[i].func][2], binning[merged2DHistograms[i][j].func][0], binning[merged2DHistograms[i][j].func][1], binning[merged2DHistograms[i][j].func][2]);
            nHist++;

            hist2D->Sumw2();
            histograms2D[i][j] = {NULL, hist2D, merged2DHistograms[i][j].parts, merged2DHistograms[i][j].indeces, merged2DHistograms[i][j].func, merged2DHistograms[i][j].funcValue};       
        }
    }

    //Local cutflow histogram
    TH1F* localCutFlow = (TH1F*)cutflow->Clone();
    localCutFlow->Reset();

    //Define TTreeReader 
    TTreeReader reader(chainWrapper[0]);

    TTreeReaderValue<std::vector<Electron>> electrons(reader, "electron");
    TTreeReaderValue<std::vector<Muon>> muons(reader, "muon");
    TTreeReaderValue<std::vector<Jet>> jets(reader, "jet");
    TTreeReaderValue<std::vector<Jet>> subjets(reader, "subjet");
    TTreeReaderValue<std::vector<FatJet>> fatjets(reader, "fatjet");
    TTreeReaderValue<TLorentzVector> MET(reader, "met");
    TTreeReaderValue<float> HT(reader, "HT");
    TTreeReaderValue<float> evtNum(reader, "eventNumber");

    TTreeReaderValue<float> lumi(reader, "lumi");
    TTreeReaderValue<float> xSec(reader, "xsec");
    TTreeReaderValue<float> puWeight(reader, "puWeight");
    TTreeReaderValue<float> genWeight(reader, "genWeight");

    //BDT intialization
    if(isBDT){
        std::map<std::string, std::string> chanPaths = {
                    {"e4j", "Electron4J"},
                    {"mu4j", "Muon4J"},
                    {"e2j1f", "Electron2J1F"},
                    {"mu2j1f", "Muon2J1F"},
                    {"e2f", "Electron2F"},
                    {"mu2f", "Muon2F"},
        };

        std::thread::id index = std::this_thread::get_id();

        std::string bdtPath = std::string(std::getenv("CMSSW_BASE")) + "/src/BDT/" + chanPaths[channel]; 

        std::vector<std::string> bdtVar = evenClassifier[index].SetEvaluation(bdtPath + "/Even/");
        oddClassifier[index].SetEvaluation(bdtPath + "/Odd/");

        bdtVar.pop_back();

        for(std::string param: bdtVar){
            bdtFunctions[index].push_back(ConvertStringToEnums(param));
        }
    }

    mutex.unlock();

    Event event;

    for (int i = entryStart; i < entryEnd; i++){
        //Load event and fill event class
        reader.SetEntry(i); 

        event = {*electrons, *muons, *jets, *subjets, *fatjets, *MET, 1., *HT, *evtNum}; 

        //Check if event passes cut
        bool passedCut = true;
 
        for(Hist &cut: cuts){
            passedCut*= Cut(event, cut);
        }   

        if(passedCut){
            valuesX.clear();

            //Fill additional weights
            std::vector<float> weightVec = {*genWeight < 2.f and *genWeight > 0.2 ? *genWeight: 1.f, *puWeight, *lumi, *xSec, 1.f/(nGen)};

            for(const float &weight: weightVec){
                event.weight *= weight;
            }

            for(unsigned int i = 0; i < histograms1D.size(); i++){
                valuesX.push_back((this->*funcDir[histograms1D[i].func])(event, histograms1D[i]));
                histograms1D[i].hist1D->Fill(valuesX[i], event.weight);

               for(unsigned int j = 0; j < histograms2D[i].size(); j++){
                    histograms2D[i][j].hist2D->Fill(valuesX[i], (this->*funcDir[histograms2D[i][j].func])(event, histograms2D[i][j]), event.weight);
                }

            }

            //Fill tree if wished
            if(saveTree) tree->Fill();
        }
    }

    //Lock again and merge local histograms into final histograms
    mutex.lock();

    for(unsigned int i = 0; i < merged1DHistograms.size(); i++){
        merged1DHistograms[i].hist1D->Add(histograms1D[i].hist1D);
        
        for(unsigned int j = 0; j < merged2DHistograms[i].size(); j++){
            merged2DHistograms[i][j].hist2D->Add(histograms2D[i][j].hist2D);
        }
    }

    //Write tree if wished
    if(saveTree){
        file->cd();
        tree->Write();
        file->Close();
    }

    //Progress bar
    progress += 100*(1.f/nCores);

    ProgressBar(progress);
    mutex.unlock();
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

    //Check if variable is BDT
    if(func == BDTSCORE) isBDT = true;

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
                indeces.push_back(-1);
                parts.push_back(strToPart[part]);
            }
        }

        if(isCutString){
            std::vector<std::string> cutVec(splittedString.end() - 2, splittedString.end());

            op = strToOp[cutVec[0]];
            cutValue = std::stof(cutVec[1]);

        }
    }

    return {NULL, NULL, parts, indeces, func, funcValue, {op, cutValue}};
}

void TreeReader::SetHistograms(){
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
        merged1DHistograms.push_back(confX);

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

        merged2DHistograms.push_back(temp2DHist);
    }

    //Cut configuration
    for(std::string &cut: cutStrings){
        this->cuts.push_back(ConvertStringToEnums(cut, true));
    }
}

void TreeReader::EventLoop(std::vector<std::string> &filenames, const float &frac){
    //Configure threads
    nCores = (int)std::thread::hardware_concurrency();   
    std::vector<int> threadsPerFile(filenames.size(), 0);   

    int entryStart;
    int entryEnd;
    float nGen = 1.;

    for(unsigned int i = 0; i < threadsPerFile.size(); i++){
        for(int j = 0; j < std::floor(nCores/threadsPerFile.size()); j++){
            threadsPerFile[i]++;
        }
    }

    for(int j = 0; j < nCores % (int)threadsPerFile.size(); j++){
        threadsPerFile[j]++;
    }

    std::vector<std::thread> threads;

    int assignedCores = 0;

    for(unsigned int i = 0; i < filenames.size(); i++){
        //Get nGen for each file
        TFile* file = TFile::Open(filenames[i].c_str());

        if(!file->GetListOfKeys()->Contains(channel.c_str())){
            std::cout << file->GetName() << " has no event tree. It will be skipped.." << std::endl;
            continue;
        }

        if(file->GetListOfKeys()->Contains("nGen")){
            TH1F* genHist = (TH1F*)file->Get("nGen");
            nGen = genHist->Integral();
        }

        cutflow = (TH1F*)file->Get(("cutflow_" + channel).c_str());

        for(int j = 0; j < threadsPerFile[i]; j++){
            //Make TChain for each file and wrap it with a vector, a TTree or bar TChain is not working
            std::vector<TChain*> v;
            TChain* chain = new TChain(channel.c_str());
            chain->Add(filenames[i].c_str());

            v.push_back(chain);

            //Define entry range for each thread
            if(j != threadsPerFile[i] - 1){
                entryStart = j*chain->GetEntries()*frac/threadsPerFile[i];
                entryEnd = (j+1)*chain->GetEntries()*frac/threadsPerFile[i];
            }

            else{
                entryStart = j*chain->GetEntries()*frac/threadsPerFile[i];
                entryEnd = chain->GetEntries()*frac;
            }

            threads.push_back(std::thread(&TreeReader::ParallelisedLoop, this, v, entryStart, entryEnd, nGen));

            //Set for each thread one core
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(assignedCores, &cpuset);

            pthread_setaffinity_np(threads[assignedCores].native_handle(), sizeof(cpu_set_t), &cpuset);

            assignedCores++;
        }
    }

    std::cout << assignedCores << std::endl;

    //Progress bar at 0 %
    ProgressBar(progress);

    //Let it run
    for(std::thread &thread: threads){
        thread.join();
    }

    //Progress bar at 100%
    ProgressBar(100);
}

//Write File

void TreeReader::Write(){
    if(saveTree){
        outputFile->Close();
        std::system(std::string("hadd -f " + outname + " " + process + "_*").c_str());
        std::system(std::string("command rm "  + process + "_*").c_str());
    }

    else{
        outputFile->cd();

        for(unsigned int i = 0; i < merged1DHistograms.size(); i++){
            merged1DHistograms[i].hist1D->Write();
            delete merged1DHistograms[i].hist1D;

            for(unsigned int j = 0; j < merged2DHistograms[i].size(); j++){
                merged2DHistograms[i][j].hist2D->Write();
                delete merged2DHistograms[i][j].hist2D;
            }
        }
        
        outputFile->Close(); 
    }

    //Remove files from gROOT
    TSeqCollection* fileList = gROOT->GetListOfFiles();
    
    for(int i = 0; i < fileList->GetSize(); i++){
        delete fileList->At(i);
    }

    end = std::chrono::steady_clock::now();
    std::cout << "Created histograms for process:" << process << " (" << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " s)" << std::endl;
}
