#include <ChargedAnalysis/Analysis/include/plotterGen.h>

PlotterGen::PlotterGen() : Plotter() {}

PlotterGen::PlotterGen(std::string &filename) : 
    Plotter(),
    filename(filename),
    hists({}),
    valueFunctions({})
 {}

void PlotterGen::ConfigureHists(std::vector<std::string> &processes){
    
    //Define histograms
    std::vector<histConfig> histValues = {
        {"nMuon", 5, 0, 5, "Number of muons", [&](genEvent event){return event.nMuons;}},
        {"nEle", 5, 0, 5, "Number of electrons", [&](genEvent event){return event.nEle;}},
        {"h1Pt", 30, 0, 300, "p_{T}^{gen}(h_{1}) [GeV]", [&](genEvent event){return event.h1.Pt();}},
        {"h2Pt", 30, 0, 300, "p_{T}^{gen}(h_{2}) [GeV]", [&](genEvent event){return event.h2.Pt();}},
        {"WPt", 30, 0, 300, "p_{T}^{gen}(W^{#pm}) [GeV]", [&](genEvent event){return event.W.Pt();}},
        {"HcPt", 30, 0, 300, "p_{T}^{gen}(H^{#pm}) [GeV]", [&](genEvent event){return event.Hc.Pt();}},
        {"lEta", 30, -5, 5, "#eta^{gen}(l) [rad]", [&](genEvent event){return event.l.Eta();}},
        {"b1Eta", 30, -5, 5, "#eta^{gen}(b_{1}) [rad]", [&](genEvent event){return event.b1.Eta();}},
        {"b2Eta", 30, -5, 5, "#eta^{gen}(b_{2}) [rad]", [&](genEvent event){return event.b2.Eta();}},
        {"b3Eta", 30, -5, 5, "#eta^{gen}(b_{3}) [rad]", [&](genEvent event){return event.b3.Eta();}},
        {"b4Eta", 30, -5, 5, "#eta^{gen}(b_{4}) [rad]", [&](genEvent event){return event.b4.Eta();}},
        {"h1Eta", 30, -3, 3, "#eta^{gen}(h_{1}) [rad]", [&](genEvent event){return event.h1.Eta();}},
        {"h2Eta", 30, -3, 3, "#eta^{gen}(h_{2}) [rad]", [&](genEvent event){return event.h2.Eta();}},
        {"WPEta", 30, -3, 3, "#eta^{gen}(W^{#pm}) [rad]", [&](genEvent event){return event.W.Eta();}},
        {"HcEta", 30, -3, 3, "#eta^{gen}(H^{#pm}) [rad]", [&](genEvent event){return event.Hc.Eta();}},
        {"vlPt", 30, 0, 200, "p_{T}^{gen}(#nu_{l}) [GeV]", [&](genEvent event){return event.vl.Pt();}},
        {"lPt", 30, 0, 200, "p_{T}^{gen}(l) [GeV]", [&](genEvent event){return event.l.Pt();}},
        {"b1Pt", 30, 0, 300, "p_{T}^{gen}(b_{1}) [GeV]", [&](genEvent event){return event.b1.Pt();}},
        {"b2Pt", 30, 0, 150, "p_{T}^{gen}(b_{2}) [GeV]", [&](genEvent event){return event.b2.Pt();}},
        {"b3Pt", 30, 0, 300, "p_{T}^{gen}(b_{3}) [GeV]", [&](genEvent event){return event.b3.Pt();}},
        {"b4Pt", 30, 0, 250, "p_{T}^{gen}(b_{4}) [GeV]", [&](genEvent event){return event.b4.Pt();}},
        {"dPhib1b2", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(b_{1}, b_{2}) [rad]", [&](genEvent event){return event.b1.DeltaPhi(event.b2);}},
        {"dPhib3b4", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(b_{3}, b_{4}) [rad]", [&](genEvent event){return event.b3.DeltaPhi(event.b4);}},
        {"dRlb1", 30, 0, 6, "#DeltaR^{gen}(l, b_{1}) [rad]", [&](genEvent event){return event.l.DeltaR(event.b1);}},
        {"dRlb2", 30, 0, 6, "#DeltaR^{gen}(l, b_{2}) [rad]", [&](genEvent event){return event.l.DeltaR(event.b2);}},
        {"dRlb3", 30, 0, 6, "#DeltaR^{gen}(l, b_{3}) [rad]", [&](genEvent event){return event.l.DeltaR(event.b3);}},
        {"dRlb4", 30, 0, 6, "#DeltaR^{gen}(l, b_{4}) [rad]", [&](genEvent event){return event.l.DeltaR(event.b4);}},
        {"dRb1b2", 30, 0, 6, "#DeltaR^{gen}(b_{1}, b_{2}) [rad]", [&](genEvent event){return event.b1.DeltaR(event.b2);}},
        {"dRb3b4", 30, 0, 6, "#DeltaR^{gen}(b_{3}, b_{4}) [rad]", [&](genEvent event){return event.b3.DeltaR(event.b4);}},
        {"dPhih1W", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(h_{1}, W^{#pm}) [rad]", [&](genEvent event){return event.h1.DeltaPhi(event.W);}},
        {"dPhih2W", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(h_{2}, W^{#pm}) [rad]", [&](genEvent event){return event.h2.DeltaPhi(event.W);}},
        {"dRh1W", 30, 0, 4, "#DeltaR^{gen}(h_{1}, W^{#pm}) [rad]", [&](genEvent event){return event.h1.DeltaR(event.W);}},
        {"dRh2W", 30, 0, 2*TMath::Pi(), "#DeltaR^{gen}(h_{2}, W^{#pm}) [rad]", [&](genEvent event){return event.h2.DeltaR(event.W);}},
        {"dPhib1W", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(b_{1}, W^{#pm}) [rad]", [&](genEvent event){return event.b1.DeltaPhi(event.W);}},
        {"dPhib2W", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(b_{2}, W^{#pm}) [rad]", [&](genEvent event){return event.b2.DeltaPhi(event.W);}},
        {"dPhib3W", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(b_{3}, W^{#pm}) [rad]", [&](genEvent event){return event.b3.DeltaPhi(event.W);}},
        {"dPhib4W", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(b_{4}, W^{#pm}) [rad]", [&](genEvent event){return event.b4.DeltaPhi(event.W);}},
        {"dRh1Hc", 30, 0, 4, "#DeltaR^{gen}(h_{1}, H^{#pm}) [rad]", [&](genEvent event){return event.h1.DeltaR(event.Hc);}},
        {"dRh2Hc", 30, 0, 2*TMath::Pi(), "#DeltaR^{gen}(h_{2}, H^{#pm}) [rad]", [&](genEvent event){return event.h2.DeltaR(event.Hc);}},
        {"dPhih1h2", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(h_{1}, h_{2}) [rad]", [&](genEvent event){return event.h1.DeltaPhi(event.h2);}},
        {"dPhih1Hc", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(h_{1}, H^{#pm}) [rad]", [&](genEvent event){return event.h1.DeltaPhi(event.Hc);}},
        {"dPhih2Hc", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(h_{2}, H^{#pm}) [rad]", [&](genEvent event){return event.h2.DeltaPhi(event.Hc);}},
        {"dPhiWHc", 30, 0, TMath::Pi(), "#Delta#phi^{gen}(W^{#pm}, H^{#pm}) [rad]", [&](genEvent event){return event.W.DeltaPhi(event.Hc);}},
        {"Angleh1Hc", 30, 0, TMath::Pi(), "Angle^{gen} (h_{1}, W^{#pm}) [rad]", [&](genEvent event){return event.h1.Angle(event.W.Vect());}},
        {"Angleh2Hc", 30, 0, TMath::Pi(), "Angle^{gen}(h_{2}, W^{#pm}) [rad]", [&](genEvent event){return event.h2.Angle(event.W.Vect());}},
        {"mh1h2", 30, 0, 600, "m(h_{1}, h_{2}) [GeV]", [&](genEvent event){return (event.h1 + event.h2).M();}},
        {"mW", 30, 0, 200, "m(W^{#pm}) [GeV]", [&](genEvent event){return (event.l + event.vl).M();}},
        {"mh1", 30, 0, 200, "m(h_{1}) [GeV]", [&](genEvent event){return (event.b1 + event.b2).M();}},
        {"dPzlvl", 30, -50, 50, "#Deltap_{z}(l, #nu_{l}) [GeV]", [&](genEvent event){return event.l.Pz() - event.vl.Pz();}},
        {"dEtalvl", 30, -2, 2, "#Delta#eta(l, #nu_{l}) [GeV]", [&](genEvent event){return event.l.Eta() - event.vl.Eta();}},
        {"dElvl", 30, -50, 50, "#DeltaE(l, #nu_{l}) [GeV]", [&](genEvent event){return event.l.E() - event.vl.E();}},
    };

    for(histConfig config: histValues){
        TH1F* hist = new TH1F(config.name.c_str(), config.name.c_str(), config.nBins, config.Min, config.Max);
        hist->GetXaxis()->SetTitle(config.Label.c_str());
        hist->GetXaxis()->SetTitleSize(0.05);
        hist->GetYaxis()->SetTitleSize(0.05);
        hist->GetXaxis()->SetTitleOffset(1.1);
        hist->GetYaxis()->SetTitle("Events");
        hist->GetXaxis()->SetLabelSize(0.05);
        hist->GetYaxis()->SetLabelSize(0.05);

        hist->SetMinimum(1e-1);
        hist->SetLineColor(kBlack);
        hist->SetLineWidth(4);

        valueFunctions.push_back(config.parameterValue);
        hists.push_back(hist);
    }
}

void PlotterGen::FillHists(){
    //TTreeReader with gen information
    TFile* file = TFile::Open(filename.c_str());

    TTreeReader reader("Events", file); 
    TTreeReaderArray<float> GenM(reader, "GenPart_mass");
    TTreeReaderArray<float> GenPt(reader, "GenPart_pt");
    TTreeReaderArray<float> GenPhi(reader, "GenPart_phi");
    TTreeReaderArray<float> GenEta(reader, "GenPart_eta");
    TTreeReaderArray<int> GenPartIdxMother(reader, "GenPart_genPartIdxMother"); 
    TTreeReaderArray<int> GenID(reader, "GenPart_pdgId");  //37 = ChargedHiggs, 25 = small Higgs, 24 = W, b = 5, e = 11, mu = 13

    while(reader.Next()){
        //Call Event class 
        genEvent event;

        //Indeces of particle
        int indexh1 = -999;
        int indexh2 = -999;
        int indexW = -999;

        //Fill 4 four vectors
        for(unsigned index = 0; index < GenID.GetSize(); index++){
            if(GenID[index] == 25){
                if(abs(GenID[GenPartIdxMother[index]]) == 37){
                    event.h1.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);
                    indexh1 = index;
                }

                else{
                    event.h2.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);
                    indexh2 = index;
                }
            } 

            if(abs(GenID[index]) == 24){
                if(abs(GenID[GenPartIdxMother[index]]) == 37){
                    event.W.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);
                    indexW = index;
                }
            }

            if(abs(GenID[index]) == 37){
                event.Hc.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);
            }

            if(abs(GenID[index]) == 11 or abs(GenID[index]) == 13){
                if(abs(GenID[index]) == 11 and GenPt[index] > 20){
                    event.nEle++;
                }

                if(abs(GenID[index]) == 13 and GenPt[index] > 20){
                    event.nMuons++;
                }

                if(GenID[GenPartIdxMother[index]] == GenID[indexW]){
                    event.l.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);
                }
            }

            if(abs(GenID[index]) == 12 or abs(GenID[index]) == 14){
                if(GenID[GenPartIdxMother[index]] == GenID[indexW]){
                    event.vl.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);
                }
            }

            if(abs(GenID[index]) == 5){
                if(GenID[GenPartIdxMother[index]] == GenID[indexh1]){
                    if(event.b1 == TLorentzVector()){
                        event.b1.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);
                    }

                    else{
                        TLorentzVector bJet;
                        bJet.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);

                        if(event.b1.Pt() > bJet.Pt()){
                            event.b2 = bJet;
                        }

                        else{
                            event.b2 = event.b1;
                            event.b1 = bJet;
                        }
                    }
                }

                if(GenID[GenPartIdxMother[index]] == GenID[indexh2]){
                    if(event.b3 == TLorentzVector()){
                        event.b3.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);
                    }

                    else{
                        TLorentzVector bJet;
                        bJet.SetPtEtaPhiM(GenPt[index], GenEta[index], GenPhi[index], GenM[index]);

                        if(event.b3.Pt() > bJet.Pt()){
                            event.b4 = bJet;
                        }

                        else{
                            event.b4 = event.b3;
                            event.b3 = bJet;
                        }
                    }
                }
            }

        }

        //Fill hists
        for(unsigned int index = 0; index < hists.size(); index++){
            hists[index]->Fill(valueFunctions[index](event));
        }        
    }
}

void PlotterGen::Draw(std::vector<std::string> &outdirs){
    for(TH1F* hist: hists){
        TCanvas* canvas = new TCanvas("c", "c", 1000, 800);
        TPad* mainpad = new TPad("m", "m", 0., 0. , 0.95, 1);
        
        mainpad->SetLeftMargin(0.15);
        mainpad->SetRightMargin(0.1);
        mainpad->SetBottomMargin(0.12);
        mainpad->Draw();
        mainpad->cd();

        hist->Draw("HIST");
        this->DrawHeader(false, "", "Simulation");

        for(std::string outdir: outdirs){
            canvas->SaveAs((outdir + "/" + hist->GetName() + ".pdf").c_str());
            canvas->SaveAs((outdir + "/" + hist->GetName() + ".png").c_str());
        }

        delete mainpad;
        delete canvas; 
    }
}
