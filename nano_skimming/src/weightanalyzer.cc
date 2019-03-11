#include <ChargedHiggs/nano_skimming/interface/weightanalyzer.h>

WeightAnalyzer::WeightAnalyzer(const float era, const float xSec):
    BaseAnalyzer(),
    era(era),
    xSec(xSec)
    {
        //Set lumi map
        lumis = {{2016, 35.87*1e3}, {2017, 41.37*1e3}};

        pileUpFiles = {
                {2017, filePath + "pileUp/data_pileUp2017.root"}, 
        };
    }

void WeightAnalyzer::BeginJob(TTreeReader &reader, TTree *tree, bool &isData){
    //Set data bool
    this->isData = isData;

    if(!this->isData){
        //Calculate pile up weights
        TFile* puFile = TFile::Open(pileUpFiles[era].c_str());
        TH1F* puData = (TH1F*)puFile->Get("pileup");

        TH1F* puMC = new TH1F("puMC", "puMC", 100, 0, 100);
        reader.GetTree()->Draw("Pileup_nTrueInt>>puMC");

        puData->Scale(1./puData->Integral(), "width");
        puMC->Scale(1./puMC->Integral(), "width");

        pileUpWeights = new TH1F("pileUpWeights", "pileUpWeights", 100, 0, 100);
        pileUpWeights->Divide(puData, puMC);

        //Initiliaze TTreeReaderValues
        genWeightValue = std::make_unique<TTreeReaderValue<float>>(reader, "Generator_weight");
        nPU = std::make_unique<TTreeReaderValue<float>>(reader, "Pileup_nTrueInt");

        nGen = new TH1F("nGen", "nGen", 100, 0, 2);
    }

    //Branches for output tree
    tree->Branch("lumi", &lumi);
    tree->Branch("xsec", &xSec);
    tree->Branch("genWeight", &genWeight);
    tree->Branch("puWeight", &puWeight);
}

bool WeightAnalyzer::Analyze(){
    //Set values if not data
    if(!this->isData){
        lumi = lumis[era];
        puWeight = pileUpWeights->GetBinContent(pileUpWeights->FindBin(*nPU->Get()));
        genWeight = *genWeightValue->Get();

        nGen->Fill(1);
    }

    return true;
}

void WeightAnalyzer::EndJob(TFile* file){
    if(!this->isData){
        if(!file->GetListOfKeys()->Contains("nGen")){
            nGen->Write();   
        }  
    }
}
