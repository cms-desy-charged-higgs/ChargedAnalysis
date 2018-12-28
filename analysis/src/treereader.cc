#include <ChargedHiggs/analysis/interface/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(std::string &process):
    process(process),
    histValues({}),
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
}

//Add Histograms

void TreeReader::AddHistogram(std::vector<std::string> &parameters){

    for(std::string parameter: parameters){
        TH1F* hist = new TH1F((parameter + "_" + process).c_str(), parameter.c_str(), histValues[parameter].nBins, histValues[parameter].xMin, histValues[parameter].xMax);

        histograms[hist] = histValues[parameter].histFunc;
    } 
}

float TreeReader::GetWeight(Event event, std::vector<float> weights){
    float totalWeight = 1.;

    for(float weight: weights){
        totalWeight *= weight;
    }

    for(Lepton electron: event.leptons){
        totalWeight *= electron.recoSF*electron.mediumMvaSF;
    }

    //totalWeight *= event.jets[0].bTagSF*event.jets[1].bTagSF;

    return totalWeight;
}

bool TreeReader::Cut(Event event){
    bool cut = true;

    cut *= (event.leptons.size() == 1)*(event.leptons[0].isMedium)*(event.leptons[0].isolation < 0.1)*(event.leptons[0].fourVec.Pt() > 100.);

    //*(event.jets.size() == 4)*(event.jets[0].isMediumB)*(event.jets[1].isMediumB);

    return cut;
}

void TreeReader::EventLoop(std::vector<std::string> &filenames){
    Processes proc = procDic[process];    

    TChain* chain = new TChain("Events");   
    
    for(std::string filename: filenames){
        chain->Add(filename.c_str());
    }
        
    std::vector<Lepton>* electrons = NULL;
    std::vector<Jet>* jets = NULL;
    TLorentzVector* MET = NULL;

    chain->SetBranchAddress("electron", &electrons);
    chain->SetBranchAddress("jet", &jets);
    chain->SetBranchAddress("MET", &MET);

    float lumi, nGen, xSec, puWeight, genWeight;

    if(proc != DATA){
 
        chain->SetBranchAddress("lumi", &lumi);
        chain->SetBranchAddress("ngen", &nGen);
        chain->SetBranchAddress("xsec", &xSec);
        chain->SetBranchAddress("puWeight", &puWeight);
        chain->SetBranchAddress("genWeight", &genWeight);
    }

    for (int i = 0, N = chain->GetEntries(); i < N; ++i) {
        chain->GetEntry(i);
        Event event = {*electrons, *jets, *MET};

        if(Cut(event)){
            for(std::pair<TH1F*, std::function<float (Event)>> hist: histograms){
                if(proc != DATA){
                    hist.first->Fill(hist.second(event), GetWeight(event, {lumi, 1.f/nGen, xSec, puWeight, genWeight}));
                }
                
                else{
                    hist.first->Fill(hist.second(event));
                }

            }
        }
    }
}

//Write File

void TreeReader::Write(std::string &outname){

    TFile* outfile = TFile::Open(outname.c_str(), "RECREATE");

    for(std::pair<TH1F*, std::function<float (Event)>> hist: histograms){
        hist.first->Write();
    }

    outfile->Close();
}
