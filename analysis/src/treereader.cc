#include <ChargedHiggs/analysis/interface/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(std::string &process):
    process(process),
    histValues({}){
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

    return totalWeight;
}

//Internal Loop, which fills the Histograms

void TreeReader::InnerLoop(std::string filename){
    std::lock_guard<std::mutex> lockGuard(mutex);
    
    TFile* file = TFile::Open(filename.c_str(), "READ");
    TTreeReader reader("Events", file);

    TTreeReaderValue<std::vector<Lepton>> electronReader(reader, "electron");
    TTreeReaderValue<std::vector<Jet>> jetReader(reader, "jet");
    TTreeReaderValue<TLorentzVector> METReader(reader, "MET");

    TTreeReaderValue<float> lumiReader(reader, "lumi");
    TTreeReaderValue<float> nGenReader(reader, "ngen");
    TTreeReaderValue<float> xSecReader(reader, "xsec");
    TTreeReaderValue<float> genReader(reader, "genWeight");
    TTreeReaderValue<float> PUReader(reader, "puWeight");

    std::cout << filename << std::endl;

    while(reader.Next()){
            
        Event event = {*electronReader, *jetReader, *METReader};
        std::vector<float> weights = {*lumiReader,1.f/ *nGenReader, *xSecReader, *genReader, *PUReader};

                
        for(std::pair<TH1F*, std::function<float (Event)>> hist: histograms){
            hist.first->Fill(hist.second(event), GetWeight(event, weights)); 
        }        
    }

    file->Close(); 
}

//Loops over all filenames and parallize reading of all trees

void TreeReader::EventLoop(std::vector<std::string> &filenames){
    for(std::string filename: filenames){
        threads.push_back(std::thread(&TreeReader::InnerLoop, this, filename));
    }

    for(std::thread &thread: threads){
        thread.join();
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
