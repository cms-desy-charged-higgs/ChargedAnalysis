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
    strToPart = {{"e", ELECTRON}, {"mu", MUON}, {"j", JET}, {"bj", BJET}, {"fj", FATJET}, {"bfj", BFATJET}, {"met", MET}, {"W", W}, {"Hc", HC}, {"h", h}};
    partLabel = {{ELECTRON, "e_{}"}, {MUON, "#mu_{}"}, {JET, "j_{}"}, {FATJET, "j_{}^{AK8}"}, {BJET, "b-tagged j_{}"}, {MET, "#slash{E}_{T}"}, {W, "W^{#pm}"}, {HC, "H^{#pm}"}, {h, "h_{}"}};

    strToFunc = {
                    {"m", MASS}, 
                    {"phi", PHI}, 
                    {"eta", ETA}, 
                    {"pt", PT},     
                    {"dphi", DPHI},
                    {"dR", DR},
                    {"lN", LOOSENPART},
                    {"mN", MEDIUMNPART},
                    {"tN", TIGHTNPART},
                    {"HT", HT},
                    {"evNr", EVENTNUMBER},
                    {"bdt", BDTSCORE},

    };
    
    funcLabel = {
                    {MASS, "m() [GeV]"}, 
                    {PHI, "#phi() [rad]"}, 
                    {ETA, "#eta() [rad]"}, 
                    {PT, "p_{T}() [GeV]"}, 
                    {DPHI, "#Delta#phi(,) [rad]"}, 
                    {DR, "#DeltaR(,) [rad]"},
                    {HT, "H_{T} [GeV]"}, 
                    {BDTSCORE, "BDT score"},
    };

    //Maps of all binning, functions and SF
    binning = {
                {MASS, {30., 20., 300.}},
                {PT, {30., 0., 200.}},
                {ETA, {30., -2.4, 2.4}},
                {PHI, {30., 0., 2.*TMath::Pi()}},
                {DPHI, {30., 0., TMath::Pi()}},
                {DR, {30., 0., 6.}},
                {HT, {30., 0., 500.}},
                {LOOSENPART, {6., 0., 6.}},
                {MEDIUMNPART, {6., 0., 6.}},
                {TIGHTNPART, {6., 0., 6.}},
                {BDTSCORE, {30., -0.5, 0.5}},
    };

    funcDir = {
                {MASS, &TreeReader::Mass},
                {PT, &TreeReader::Pt},
                {PHI, &TreeReader::Phi},
                {ETA, &TreeReader::Eta},
                {DPHI, &TreeReader::DeltaPhi},
                {DR, &TreeReader::DeltaR},
                {HT, &TreeReader::HadronicEnergy},
                {LOOSENPART, &TreeReader::NParticle},
                {MEDIUMNPART, &TreeReader::NParticle},
                {TIGHTNPART, &TreeReader::NParticle},
                {EVENTNUMBER, &TreeReader::EventNumber},
                {BDTSCORE, &TreeReader::BDTScore},
    };

    eleID = {
                {MEDIUMNPART, {&Electron::isMedium, 0.2}}, 
                {TIGHTNPART, {&Electron::isTight, 0.1}}
    };

    eleSF = {
                {MEDIUMNPART, &Electron::mediumMvaSF}, 
                {TIGHTNPART, &Electron::mediumMvaSF}
    };

    muonID = {
                {MEDIUMNPART, {&Muon::isMedium, &Muon::isLooseIso}}, 
                {TIGHTNPART, {&Muon::isTight, &Muon::isTightIso}}
    };

    muonSF = {
                {MEDIUMNPART, {&Muon::mediumSF, &Muon::looseIsoMediumSF}}, 
                {TIGHTNPART, {&Muon::tightSF, &Muon::tightIsoTightSF}}
    };

    bJetID = {
                {LOOSENPART, &Jet::isLooseB}, 
                {MEDIUMNPART, &Jet::isMediumB}, 
                {TIGHTNPART, &Jet::isTightB}
    };

    bJetSF = {
                {LOOSENPART, &Jet::loosebTagSF}, 
                {MEDIUMNPART, &Jet::mediumbTagSF}, 
                {TIGHTNPART, &Jet::tightbTagSF}
    };
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
    std::vector<Hist> histograms1D;

    //Lock thread unsafe operation
    mutex.lock();

    //vector with values
    std::vector<float> values(merged1DHistograms.size(), 1.);

    //Tree that could be safed if wished
    TFile* file(NULL); TTree* tree(NULL);

    if(saveTree){
        file = TFile::Open(std::string(process + "_" + std::to_string(nHist) + ".root").c_str(), "RECREATE");
        tree = new TTree(process.c_str(), process.c_str());
    }

    //Create thread local histograms/branches
    for(unsigned int index = 0; index < merged1DHistograms.size(); index++){
        TH1F* hist1D = new TH1F(std::string("h" + std::to_string(nHist)).c_str(), std::string("h" + std::to_string(nHist)).c_str(), binning[merged1DHistograms[index].func][0], binning[merged1DHistograms[index].func][1], binning[merged1DHistograms[index].func][2]);
        nHist++;

        hist1D->Sumw2();
        histograms1D.push_back({hist1D, merged1DHistograms[index].parts, merged1DHistograms[index].indeces, merged1DHistograms[index].func});

        if(saveTree) tree->Branch(xParameters[index].c_str(), &values[index]);
    }

    //Define TTreeReader 
    TTreeReader reader(chainWrapper[0]);

    TTreeReaderValue<std::vector<Electron>> electrons(reader, "electron");
    TTreeReaderValue<std::vector<Muon>> muons(reader, "muon");
    TTreeReaderValue<std::vector<Jet>> jets(reader, "jet");
    TTreeReaderValue<std::vector<Jet>> fatjets(reader, "fatjet");
    TTreeReaderValue<TLorentzVector> MET(reader, "met");
    TTreeReaderValue<float> HT(reader, "HT");
    TTreeReaderValue<float> evtNum(reader, "eventNumber");

    TTreeReaderValue<float> lumi(reader, "lumi");
    TTreeReaderValue<float> xSec(reader, "xsec");
    TTreeReaderValue<float> puWeight(reader, "puWeight");
    TTreeReaderValue<float> genWeight(reader, "genWeight");

    if(std::find(xParameters.begin(), xParameters.end(), "bdt") != xParameters.end() or std::find(cutStrings.begin(), cutStrings.end(), "bdt") != cutStrings.end()){
        //BDT intialization
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

        for(std::string param: bdtVar){
            bdtFunctions[index].push_back(ConvertStringToEnums(param));
        }
    }

    mutex.unlock();

    Event event;

    for (int i = entryStart; i < entryEnd; i++){
        //Load event and fill event class
        reader.SetEntry(i); 

        event = {*electrons, *muons, *jets, *fatjets, *MET, 1., *HT, *evtNum}; 

        //Check if event passes cut
        bool passedCut = true;
 
        for(Hist &cut: cuts){
            passedCut *= Cut(event, cut);
        }   

        if(passedCut){
            values.clear();

            //Fill additional weights
            std::vector<float> weightVec = {*genWeight < 2.f and *genWeight > 0.2 ? *genWeight: 1.f, *puWeight, *lumi, *xSec, 1.f/(nGen)};

            for(const float &weight: weightVec){
                event.weight *= weight;
            }

            for(unsigned int index = 0; index < histograms1D.size(); index++){
                values.push_back((this->*funcDir[histograms1D[index].func])(event, histograms1D[index]));
                histograms1D[index].hist->Fill(values[index], event.weight);
            }

            //Fill tree if wished
            if(saveTree) tree->Fill();
        }
    }

    //Lock again and merge local histograms into final histograms
    mutex.lock();

    for(unsigned int i = 0; i < merged1DHistograms.size(); i++){
        merged1DHistograms[i].hist->Add(histograms1D[i].hist);
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
    Operator op; float cutValue;
    std::vector<Particle> parts;
    std::vector<int> indeces;

    //If no particle is involved, like HT_>_100
    if(splittedString.size() == 3 and isCutString){
        op = strToOp[splittedString[1]];
        cutValue = std::stof(splittedString[2]);
    }
    
    else{
        for(std::string part: std::vector<std::string>(splittedString.begin()+1, splittedString.end() - isCutString)){ 
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
            op = strToOp[std::vector<std::string>(splittedString.begin() + parts.size(), splittedString.end())[0]];
            cutValue = std::stof(std::vector<std::string>(splittedString.begin() + parts.size(), splittedString.end())[1]);
        }
    }

    return {NULL, parts, indeces, func, {op, cutValue}};
}

void TreeReader::SetHistograms(){
    //Define final histogram for each parameter
    for(std::string xParameter: xParameters){
        //Split input string into information for particle and function to call value
        Hist conf = ConvertStringToEnums(xParameter);
  
        //Create final histogram
        TH1F* hist = new TH1F(xParameter.c_str(), xParameter.c_str(), binning[conf.func][0], binning[conf.func][1], binning[conf.func][2]);
        hist->Sumw2();

        std::string fLabel = funcLabel[conf.func];

        //Set axis label
        if(conf.parts.size() == 1){
            std::string pLabel = partLabel[conf.parts[0]];
            if(conf.indeces[0] != -1) pLabel.insert(pLabel.find("{") + 1, std::to_string(conf.indeces[0]));

            hist->GetXaxis()->SetTitle(fLabel.insert(fLabel.find("(") + 1, pLabel).c_str());
        }

        else if(conf.parts.size() == 2){
            std::string pLabel1 = partLabel[conf.parts[0]];
            if(conf.indeces[0] != -1) pLabel1.insert(pLabel1.find("{") + 1, std::to_string(conf.indeces[0]));
            std::string pLabel2 = partLabel[conf.parts[1]];
            if(conf.indeces[1] != -1) pLabel2.insert(pLabel2.find("{") + 1, std::to_string(conf.indeces[1]));

            hist->GetXaxis()->SetTitle(fLabel.insert(fLabel.find("(") + 1, pLabel1).insert(fLabel.find(",") + 1, pLabel2).c_str());
        }

        conf.hist = hist;

        //Add hist to collection
        merged1DHistograms.push_back(conf);
    }

    //Cut configuration
    for(std::string cut: cutStrings){
        this->cuts.push_back(ConvertStringToEnums(cut, true));
    }
}

void TreeReader::EventLoop(std::vector<std::string> &filenames){
    //Configure threads
    nCores = (int)std::thread::hardware_concurrency();   
    int threadsPerFile = nCores/filenames.size();   

    std::vector<std::thread> threads;
    int entryStart;
    int entryEnd;

    float nGen = 1.;
    int assignedCore = 0;

    for(const std::string &filename: filenames){
        //Get nGen for each file
        TFile* file = TFile::Open(filename.c_str());
        if(!file->GetListOfKeys()->Contains(channel.c_str())){
            std::cout << file->GetName() << " has no event tree. It will be skipped.." << std::endl;
            continue;
        }

        if(file->GetListOfKeys()->Contains("nGen")){
            TH1F* genHist =  (TH1F*)file->Get("nGen");
            nGen = genHist->Integral();
        }

        for(int i = 0; i < threadsPerFile; i++){
            //Make TChain for each file and wrap it with a vector, a TTree or bar TChain is not working
            std::vector<TChain*> v;
            TChain* chain = new TChain(channel.c_str());
            chain->Add(filename.c_str());

            v.push_back(chain);

            //Define entry range for each thread
            if(i != threadsPerFile -1){
                entryStart = i*chain->GetEntries()/threadsPerFile;
                entryEnd = (i+1)*chain->GetEntries()/threadsPerFile;
            }

            else{
                entryStart = i*chain->GetEntries()/threadsPerFile;
                entryEnd = chain->GetEntries();
            }

            threads.push_back(std::thread(&TreeReader::ParallelisedLoop, this, v, entryStart, entryEnd, nGen));

            //Set for each thread one core
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(assignedCore, &cpuset);

            pthread_setaffinity_np(threads[assignedCore].native_handle(), sizeof(cpu_set_t), &cpuset);

            assignedCore++;
        }
    }

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

        for(Hist hist: merged1DHistograms){
            hist.hist->Write();
            delete hist.hist;
        }
        
        outputFile->Close(); 
    }

    end = std::chrono::steady_clock::now();
    std::cout << "Created histograms for process:" << process << " (" << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " s)" << std::endl;
}
