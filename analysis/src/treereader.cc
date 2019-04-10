#include <ChargedHiggs/analysis/interface/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(std::string &process, std::vector<std::string> &xParameters,
std::vector<std::string> &yParameters, std::vector<std::string> &cutstrings, const bool& saveTree):
    process(process),
    xParameters(xParameters),
    yParameters(yParameters),
    cutstrings(cutstrings),
    saveTree(saveTree){

    start = std::chrono::steady_clock::now();

    SetHistMap();   
    SetCutMap();
    SetTriggerMap();
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

    //Define Outtree if wished
    TTree* tree = new TTree((std::to_string(entryEnd)).c_str(), (std::to_string(entryEnd)).c_str());
    std::vector<float> branchValues;
    std::vector<TBranch*> branches; 

    //Define containers for histograms
    std::vector<TH1F*> histograms1D;
    std::vector<std::vector<TH2F*>> histograms2D;
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

        std::vector<TH2F*> hists2D;

        if(saveTree){
            branchValues.push_back(0.);
            branches.push_back(tree->Branch(xParameter.c_str(), &branchValues[branchValues.size()-1]));
        }

        for(const std::string &yParameter: yParameters){
            TH2F* hist2D = new TH2F((xParameter + "_VS_" + yParameter + std::to_string(entryEnd)).c_str(), (xParameter + "_VS_" + yParameter + std::to_string(entryEnd)).c_str(), histValues[xParameter].nBins, histValues[xParameter].Min, histValues[xParameter].Max, histValues[yParameter].nBins, histValues[yParameter].Min, histValues[yParameter].Max);

            hist2D->Sumw2();
            hists2D.push_back(hist2D);

            if(yFunctions.size() <= yParameter){
                yFunctions.push_back(histValues[yParameter].parameterValue);
            }
        }

        histograms2D.push_back(hists2D);
    }

    //Define TTreeReader 
    TTreeReader reader(chainWrapper[0]);

    //Vector of values for trigger
    std::vector<std::pair<Trigger, TTreeReaderValue<int>>> triggers;

    for(std::string &xParameter: xParameters){
        if(trigNames.find(xParameter) != trigNames.end()){
            TTreeReaderValue<int> triggerValue(reader, trigNames[xParameter].second.c_str());

            triggers.push_back({trigNames[xParameter].first, triggerValue});
        }
    }

    for(std::string &yParameter: yParameters){
        if(trigNames.find(yParameter) != trigNames.end()){   
            TTreeReaderValue<int> triggerValue(reader, trigNames[yParameter].second.c_str());
 
            triggers.push_back({trigNames[yParameter].first, triggerValue});
        }
    }

    TTreeReaderValue<std::vector<Electron>> electrons(reader, "electron");
    TTreeReaderValue<std::vector<Muon>> muons(reader, "muon");
    TTreeReaderValue<std::vector<Jet>> jets(reader, "jet");
    TTreeReaderValue<std::vector<Jet>> fatjets(reader, "fatjet");
    TTreeReaderValue<TLorentzVector> MET(reader, "met");

    TTreeReaderValue<float> HT(reader, "HT");
    TTreeReaderValue<float> lumi(reader, "lumi");
    TTreeReaderValue<float> xSec(reader, "xsec");
    TTreeReaderValue<float> puWeight(reader, "puWeight");
    TTreeReaderValue<float> genWeight(reader, "genWeight");

    mutex.unlock();

    //Fill vector with wished cut operation
    std::vector<cut> cuts; 
        
    for(const std::string &cutstring: cutstrings){
        cuts.push_back(cutValues[cutstring]);
    }

    Event event;
    std::map<Trigger, int> triggerDecisions = {};

    for (int i = entryStart; i < entryEnd; i++){
        //Load event and fill event class
        reader.SetEntry(i); 

        //Fill Map with trigger decision if wished
        for(std::pair<Trigger, TTreeReaderValue<int>> &decision: triggers){
            triggerDecisions[decision.first] = *(decision.second);
        }
    
        event = {*electrons, *muons, *jets, *fatjets, *MET, 1., *HT, triggerDecisions}; 

        //Check if event passes cut
        bool passedCut = true;
 
        for(const cut &cutValue: cuts){
            passedCut *= (this->*cutValue)(event);
        }
       
        //Fill histograms
        if(passedCut){
            //Fill additional weights
            std::vector<float> weightVec = {*genWeight < 2.f and *genWeight > 0.2 ? *genWeight: 1.f, *puWeight, *lumi, *xSec,  1.f/(nGen)};

            for(const float &weight: weightVec){
                event.weight *= weight;
            }

            for(unsigned i=0; i < xParameters.size(); i++){
                if(saveTree){
                    branchValues[i] = (this->*xFunctions[i])(event);
                    branches[i]->Fill();
                }

                histograms1D[i]->Fill((this->*xFunctions[i])(event), event.weight);

                for(unsigned j=0; j < yParameters.size(); j++){
                    histograms2D[i][j]->Fill((this->*xFunctions[i])(event), (this->*yFunctions[j])(event), event.weight);                       
                }
            }
        }
    }

    //Lock again and merge local histograms into final histograms
    mutex.lock();

    for(unsigned int i = 0; i < xParameters.size(); i++){
        merged1DHistograms[i]->Add(histograms1D[i]);

        for(unsigned int j = 0; j < yParameters.size(); j++){
            merged2DHistograms[i][j]->Add(histograms2D[i][j]);
        }
    }

    //listTree->Add(tree);

    progress += 100*(1.f/nCores);

    ProgressBar(progress);
    mutex.unlock();
}

void TreeReader::EventLoop(std::vector<std::string> &filenames, std::string &channel){
    //Define final histogram for each parameter
    for(const std::string &xParameter: xParameters){
        TH1F* hist = new TH1F(xParameter.c_str(), xParameter.c_str(), histValues[xParameter].nBins, histValues[xParameter].Min, histValues[xParameter].Max);
        hist->Sumw2();

        hist->GetXaxis()->SetTitle(histValues[xParameter].Label.c_str());

        merged1DHistograms.push_back(hist);

        std::vector<TH2F*> merged2DHists;

        for(const std::string &yParameter: yParameters){
            TH2F* hist = new TH2F((xParameter + "_VS_" + yParameter).c_str(), (xParameter + "_VS_" + yParameter).c_str(), histValues[xParameter].nBins, histValues[xParameter].Min, histValues[xParameter].Max, histValues[yParameter].nBins, histValues[yParameter].Min, histValues[yParameter].Max);

            hist->Sumw2();

            hist->GetXaxis()->SetTitle(histValues[xParameter].Label.c_str());
            hist->GetYaxis()->SetTitle(histValues[yParameter].Label.c_str());

            merged2DHists.push_back(hist);
        }
        
        merged2DHistograms.push_back(merged2DHists);
    }

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

    if(saveTree){
        outTree = TTree::MergeTrees(listTree);
        outTree->SetName(channel.c_str());
    }

    //Progress bar at 100%
    ProgressBar(100);
}

//Write File

void TreeReader::Write(std::string &outname){
    TFile* outputFile = TFile::Open(outname.c_str(), "RECREATE");

    for(TH1F* hist: merged1DHistograms){
        hist->Write();
        delete hist;
    }

    for(std::vector<TH2F*> hists: merged2DHistograms){
        for(TH2F* hist: hists){
            hist->Write();
            delete hist;
        }
    }

    if(saveTree){
        outTree->Write();
    }
    
    end = std::chrono::steady_clock::now();
    std::cout << "Created histograms for process:" << process << " (" << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " s)" << std::endl;
        

    outputFile->Close(); 
}
