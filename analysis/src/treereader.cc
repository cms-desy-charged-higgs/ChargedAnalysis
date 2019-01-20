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
    SetCutMap();
}

//Add Histograms

void TreeReader::AddHistogram(std::vector<std::string> &parameters){

    for(std::string parameter: parameters){
        TH1F* hist = new TH1F(parameter.c_str(), parameter.c_str(), histValues[parameter].nBins, histValues[parameter].xMin, histValues[parameter].xMax);

        histograms[hist] = histValues[parameter].histFunc;
    } 
}

float TreeReader::GetWeight(Event event, std::vector<float> weights){
    float totalWeight = 1.;

    for(float weight: weights){
        totalWeight *= weight;
    }

    for(Lepton electron: event.leptons){
        if(electron.isMedium){
            totalWeight *= electron.recoSF*electron.mediumMvaSF;
        }
    }

    for(Jet jet: event.jets){
        if(jet.isMediumB){
            //totalWeight *= jet.bTagSF;
        }
    }

    return totalWeight;
}

void TreeReader::EventLoop(std::vector<std::string> &filenames, std::vector<std::string> &cutstrings){
    Processes proc = procDic[process];  

    for(std::string filename: filenames){
        TFile* file = TFile::Open(filename.c_str());
     
        TTreeReader reader("Events", file);

        TTreeReaderValue<std::vector<Lepton>> electrons(reader, "electron");
        TTreeReaderValue<std::vector<Jet>> jets(reader, "jet");
        TTreeReaderValue<Quantities> quantities(reader, "quantities");
        TTreeReaderValue<TLorentzVector> MET(reader, "MET");

        TTreeReaderValue<float> lumi;
        TTreeReaderValue<float> xSec;
        TTreeReaderValue<float> puWeight;
        TTreeReaderValue<float> genWeight;

        float nGen = 1.;

        if(proc != DATA){
            lumi = TTreeReaderValue<float>(reader, "lumi");
            xSec = TTreeReaderValue<float>(reader, "xsec");
            puWeight = TTreeReaderValue<float>(reader, "puWeight");
            genWeight = TTreeReaderValue<float>(reader, "genWeight");
    
            TH1F* nGenHist = (TH1F*)file->Get("nGen");
            nGen = nGenHist->Integral();
        } 

        std::vector<std::function<bool (Event)>> cuts; 
        
        for(std::string cutstring: cutstrings){
            cuts.push_back(cutValues[cutstring]);
        }

        while(reader.Next()){    
            std::vector<Lepton> ele = *electrons;

            Event event = {*electrons, *jets, *quantities, *MET};
       
            bool cut = true;
            
            for(std::function<bool (Event)> cutValue: cuts){
                cut *= cutValue(event);
            }
            
            if(cut){
                for(std::pair<TH1F*, std::function<float (Event)>> hist: histograms){
                    if(proc != DATA){
                        float weight = GetWeight(event, {*xSec, (*lumi)*(float)1e3, 1.f/nGen, *puWeight, *genWeight});

                        hist.first->Fill(hist.second(event), weight);
                    }
                    
                    else{
                        hist.first->Fill(hist.second(event));
                    }

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
