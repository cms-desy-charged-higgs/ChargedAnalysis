#include <ChargedHiggs/analysis/interface/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(std::string &process, std::vector<std::string> &xParameters,
std::vector<std::string> &yParameters, std::vector<std::string> &cutstrings):
    process(process),
    xParameters(xParameters),
    yParameters(yParameters),
    cutstrings(cutstrings),
    procDic({
            {"DY+j", DY},
            {"W+j", WJ},
            {"SingleE", DATA},
            {"SingleMu", DATA},
            {"VV+VVV", VV},
            {"QCD", QCD},
            {"TT+X", TT},
            {"T+X", T},
            {"L4B_150_75", SIGNAL},
    }){

    SetHistMap();   
    SetCutMap();
}

void TreeReader::ProgressBar(int progress){
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

    //Check out process you have
    Processes proc = procDic[process];

    std::vector<TH1F*> histograms1D;
    std::vector<TH2F*> histograms2D;
    std::vector<parameterFunc> xFunctions;
    std::vector<parameterFunc> yFunctions;

    //Lock thread unsafe operation
    mutex.lock();

    //Create thread local histograms
    for(const std::string &xParameter: xParameters){
        TH1F* hist1D = new TH1F((xParameter + std::to_string(entryEnd)).c_str(), (xParameter + std::to_string(entryEnd)).c_str(), histValues[xParameter].nBins, histValues[xParameter].Min, histValues[xParameter].Max);

        hist1D->Sumw2();
        histograms1D.push_back(hist1D);

        xFunctions.push_back(histValues[xParameter].parameterValue);

        for(const std::string &yParameter: yParameters){
            TH2F* hist2D = new TH2F((xParameter + "_VS_" + yParameter + std::to_string(entryEnd)).c_str(), (xParameter + "_VS_" + yParameter + std::to_string(entryEnd)).c_str(), histValues[xParameter].nBins, histValues[xParameter].Min, histValues[xParameter].Max, histValues[yParameter].nBins, histValues[yParameter].Min, histValues[yParameter].Max);

            hist2D->Sumw2();
            histograms2D.push_back(hist2D);

            yFunctions.push_back(histValues[yParameter].parameterValue);
        }
    }

    //Define TTreeReader 
    TTreeReader reader(chainWrapper[0]);

    //Check if electron or muon branche exist, if not give Event class empty vector
    bool hasElectronCol = false;
    std::vector<Electron> emptyEle = {};

    bool hasMuonCol = false;
    std::vector<Muon> emptyMuon = {};

    TTreeReaderValue<std::vector<Electron>> electrons;
    TTreeReaderValue<std::vector<Muon>> muons;
    
    if(chainWrapper[0]->GetBranchStatus("electron")){
        electrons = TTreeReaderValue<std::vector<Electron>>(reader, "electron");
        hasElectronCol = true;
    }

    if(chainWrapper[0]->GetBranchStatus("muon")){
        muons = TTreeReaderValue<std::vector<Muon>>(reader, "muon");
        hasMuonCol = true;
    }

    TTreeReaderValue<std::vector<Jet>> jets(reader, "jet");
    TTreeReaderValue<Quantities> quantities(reader, "quantities");
    TTreeReaderValue<TLorentzVector> MET(reader, "MET");

    TTreeReaderValue<float> lumi;
    TTreeReaderValue<float> xSec;
    TTreeReaderValue<float> puWeight;
    TTreeReaderValue<float> genWeight;

    if(proc != DATA){
        lumi = TTreeReaderValue<float>(reader, "lumi");
        xSec = TTreeReaderValue<float>(reader, "xsec");
        puWeight = TTreeReaderValue<float>(reader, "puWeight");
        genWeight = TTreeReaderValue<float>(reader, "genWeight");
    } 
    mutex.unlock();

    //Fill vector with wished cut operation
    std::vector<cut> cuts; 
        
    for(const std::string &cutstring: cutstrings){
        cuts.push_back(cutValues[cutstring]);
    }
   
    for (int i = entryStart; i < entryEnd; i++){
        //Load event and fill event class
        reader.SetEntry(i);  
        Event event = {hasElectronCol ? *electrons : emptyEle, hasMuonCol ? *muons: emptyMuon, *jets, *quantities, *MET, 1.}; 

        //Check if event passes cut
        bool passedCut = true;
 
        for(const cut &cutValue: cuts){
            passedCut *= (this->*cutValue)(event);
        }
       
        //Fill histograms
        if(passedCut){
            if(proc != DATA){
            //Fill additional weights
                std::vector<float> weightVec = {*genWeight < 2 and *genWeight > 0.2 ? *genWeight: (float)1., *puWeight, (*lumi)*(float)1e3, *xSec, 1.f/nGen};

                for(const float &weight: weightVec){
                    event.weight *= weight;
                }
            }

            for(unsigned i=0; i < histograms1D.size(); i++){
                if(proc != DATA){
                    histograms1D[i]->Fill((this->*xFunctions[i])(event), event.weight);
                }
                    
                else{
                    histograms1D[i]->Fill((this->*xFunctions[i])(event));
                }

                for(unsigned j=0; j < histograms2D.size(); j++){
                    if(proc != DATA){
                        histograms2D[j]->Fill((this->*xFunctions[i])(event), (this->*yFunctions[j])(event), event.weight);
                    }
                        
                    else{
                        histograms2D[j]->Fill((this->*xFunctions[i])(event), (this->*yFunctions[j])(event));
                    }
                }
            }
        }
    }

    //Lock again and merge local histograms into final histograms
    mutex.lock();

    for(unsigned int i = 0; i < merged1DHistograms.size(); i++){
        merged1DHistograms[i]->Add(histograms1D[i]);
    }

    for(unsigned int j = 0; j < merged2DHistograms.size(); j++){
        merged2DHistograms[j]->Add(histograms2D[j]);
    }

    progress += 100*(1.f/nCores);

    ProgressBar(progress);
    mutex.unlock();
}

void TreeReader::EventLoop(std::vector<std::string> &filenames){

    //Define final histogram for each parameter
    for(const std::string &xParameter: xParameters){
        TH1F* hist = new TH1F(xParameter.c_str(), xParameter.c_str(), histValues[xParameter].nBins, histValues[xParameter].Min, histValues[xParameter].Max);
        hist->Sumw2();

        hist->GetXaxis()->SetTitle(histValues[xParameter].Label.c_str());

        merged1DHistograms.push_back(hist);

        for(const std::string &yParameter: yParameters){
            TH2F* hist = new TH2F((xParameter + "_VS_" + yParameter).c_str(), (xParameter + "_VS_" + yParameter).c_str(), histValues[xParameter].nBins, histValues[xParameter].Min, histValues[xParameter].Max, histValues[yParameter].nBins, histValues[yParameter].Min, histValues[yParameter].Max);

            hist->Sumw2();

            hist->GetXaxis()->SetTitle(histValues[xParameter].Label.c_str());
            hist->GetYaxis()->SetTitle(histValues[yParameter].Label.c_str());

            merged2DHistograms.push_back(hist);
        }

    }

    //Configure threads
    nCores = (int)std::thread::hardware_concurrency();   
    int threadsPerFile = nCores/filenames.size();   

    std::vector<std::thread> threads;
    int entryStart;
    int entryEnd;

    int assignedCore = 0;

    for(const std::string &filename: filenames){
        //Get nGen for each file
        TFile* file = TFile::Open(filename.c_str());
        if(!file->GetListOfKeys()->Contains("Events")){
            std::cout << file->GetName() << " has no event tree. It will be skipped.." << std::endl;
            continue;
        }

        float nGen = 1.;

        if(procDic[process] != DATA){
            TH1F* hist = (TH1F*)file->Get("nGenWeighted");  
            nGen = hist->Integral();
            file->Close();
        }

        for(int i = 0; i < threadsPerFile; i++){
            //Make TChain for each file and wrap it with a vector, a TTree or bar TChain is not working
            std::vector<TChain*> v;
            TChain* chain = new TChain("Events");
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

void TreeReader::Write(std::string &outname){
    TFile* outputFile = TFile::Open(outname.c_str(), "UPDATE");

    for(TH1F* hist: merged1DHistograms){
        hist->Write();
        delete hist;
    }

    for(TH2F* hist: merged2DHistograms){
        hist->Write();
        delete hist;
    }
    
    outputFile->Close(); 
}
