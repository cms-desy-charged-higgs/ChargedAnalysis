#include <ChargedHiggs/analysis/interface/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(std::string &process, std::vector<std::string> &parameters, std::vector<std::string> &cutstrings):
    process(process),
    parameters(parameters),
    cutstrings(cutstrings),
    procDic({
            {"DY+j", DY},
            {"W+j", WJ},
            {"data", DATA},
            {"VV+VVV", VV},
            {"QCD", QCD},
            {"TT+X", TT},
            {"T+X", T}
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

float TreeReader::GetWeight(Event event, std::vector<float> weights){
    float totalWeight = 1.;

    //Weights given to this functions
    for(float weight: weights){
        totalWeight *= weight;
    }

    //Weights related to electrons
    for(Lepton electron: event.leptons){
        if(electron.isMedium){
            totalWeight *= electron.recoSF*electron.mediumMvaSF;
        }
    }

    //Weights related to jets
    for(Jet jet: event.jets){
        if(jet.isMediumB){
            //totalWeight *= jet.bTagSF;
        }
    }

    return totalWeight;
}

void TreeReader::ParallelisedLoop(const std::vector<TChain*> &chainWrapper, const int &entryStart, const int &entryEnd, const float nGen)
{ 
    //Dont start immediately to avoid crashes of ROOT
    std::this_thread::sleep_for (std::chrono::seconds(1));

    //Check out process you have
    Processes proc = procDic[process];

    std::vector<TH1F*> histograms;
    std::vector<std::function<float (Event)>> parameterFunctions;

    //Lock thread unsafe operation
    mutex.lock();

    //Create thread local histograms
    for(std::string parameter: parameters){
        TH1F* hist = new TH1F((parameter + std::to_string(entryEnd)).c_str(), (parameter + std::to_string(entryEnd)).c_str(), histValues[parameter].nBins, histValues[parameter].xMin, histValues[parameter].xMax);
        histograms.push_back(hist);

        parameterFunctions.push_back(histValues[parameter].histFunc);
    }

    //Define TTreeReader 
    TTreeReader reader(chainWrapper[0]);

    TTreeReaderValue<std::vector<Lepton>> electrons(reader, "electron");
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
    std::vector<std::function<bool (Event)>> cuts; 
        
    for(std::string cutstring: cutstrings){
        cuts.push_back(cutValues[cutstring]);
    }
   
    for (int i = entryStart; i < entryEnd; i++){
        //Load event and fill event class
        reader.SetEntry(i);  
        Event event = {*electrons, *jets, *quantities, *MET}; 

        //Check if event passes cut
        bool cut = true;
 
        for(std::function<bool (Event)> cutValue: cuts){
            cut *= cutValue(event);
        }
        
        //Fill histograms
        if(cut){
            for(unsigned i=0; i < histograms.size(); i++){
                if(proc != DATA){
                    float weight = GetWeight(event, {*genWeight, *puWeight, (*lumi)*(float)1e3, *xSec, 1.f/nGen});
                    histograms[i]->Fill(parameterFunctions[i](event), weight);
                }
                    
                else{
                    histograms[i]->Fill(parameterFunctions[i](event));
                }

            }
        }
    }

    //Lock again and merge local histograms into final histograms
    mutex.lock();

    for(unsigned int i = 0; i < mergedHistograms.size(); i++){
        mergedHistograms[i]->Add(histograms[i]);
    }

    progress += 100*(1.f/nCores);

    ProgressBar(progress);
    mutex.unlock();
}

void TreeReader::EventLoop(std::vector<std::string> &filenames){

    //Progress bar at 0 %
    ProgressBar(progress);

    //Define final histogram for each parameter
    for(std::string parameter: parameters){
        mergedHistograms.push_back(new TH1F(parameter.c_str(), parameter.c_str(), histValues[parameter].nBins, histValues[parameter].xMin, histValues[parameter].xMax));
    }

    //Configure threads
    nCores = (int)std::thread::hardware_concurrency();   
    int threadsPerFile = nCores/filenames.size();   

    std::vector<std::thread> threads;
    int entryStart;
    int entryEnd;

    int assignedCore = 0;

    for(std::string filename: filenames){
        //Get nGen for each file
        TFile* file = TFile::Open(filename.c_str());
        TH1F* hist = (TH1F*)file->Get("nGen");  
        float nGen = hist->Integral();
        file->Close();

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

    //Let it run
    for(std::thread &thread: threads){
        thread.join();
    }

    //Progress bar at 100%
    ProgressBar(100);
}

//Write File

void TreeReader::Write(std::string &outname){
    TFile* outputFile = TFile::Open(outname.c_str(), "RECREATE");

    for(TH1F* hist: mergedHistograms){
        hist->Write();
        delete hist;
    }
    
    outputFile->Close(); 
}
