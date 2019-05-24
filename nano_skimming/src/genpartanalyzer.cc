#include <ChargedHiggs/nano_skimming/interface/genpartanalyzer.h>

GenPartAnalyzer::GenPartAnalyzer():
    BaseAnalyzer(){}


void GenPartAnalyzer::BeginJob(TTreeReader &reader, TTree* tree, bool &isData){
    //Set data bool
    this->isData = isData;
    
    //Set TTreeReader for genpart and trigger obj from baseanalyzer
    SetCollection(reader, this->isData);

    //Set Branches of output tree
    tree->Branch("genPart", &genParts);
}


bool GenPartAnalyzer::Analyze(std::pair<TH1F*, float> &cutflow){
    if(!isData){
        //Fill 4 four vectors
        for(unsigned index = 0; index < genID->GetSize(); index++){
            if(genID->At(index) == 25){
                if(abs(genID->At(genMotherIdx->At(index))) == 37){
                    genParts.h1.SetPtEtaPhiM(genPt->At(index), genEta->At(index), genPhi->At(index), genMass->At(index));
                }

                else{
                    genParts.h2.SetPtEtaPhiM(genPt->At(index), genEta->At(index), genPhi->At(index), genMass->At(index));
                }
            } 

            if(abs(genID->At(index)) == 24){
                if(abs(genID->At(genMotherIdx->At(index))) == 37){
                    genParts.W.SetPtEtaPhiM(genPt->At(index), genEta->At(index), genPhi->At(index), genMass->At(index));
                }
            }

            if(abs(genID->At(index)) == 37){
                genParts.Hc.SetPtEtaPhiM(genPt->At(index), genEta->At(index), genPhi->At(index), genMass->At(index));
            }
        }
    }
    
    return true;
}


void GenPartAnalyzer::EndJob(TFile* file){
}
