#include <ChargedAnalysis/Analysis/include/plotterLimit.h>

PlotterLimit::PlotterLimit() : Plotter(){}

PlotterLimit::PlotterLimit(std::string &limitDir, const std::vector<int>& masses, const std::vector<std::string>& channels) : 
    Plotter(),
    limitDir(limitDir),
    masses(masses),
    channels(channels){

        xSecs = {
                {200, 0.03208},
                {250, 0.0178},
                {300, 0.01062},
                {350, 0.006656},
                {400, 0.004334},
                {450, 0.002918},
                {500, 0.002019},
                {550, 0.001431},
                {600, 0.001063},
        };
    }


void PlotterLimit::ConfigureHists(){
    for(const std::string& channel : Utils::Merge<std::string>({""}, channels)){
        expected[channel] = new TGraph();
        sigmaOne[channel] = new TGraphAsymmErrors();
        sigmaTwo[channel] = new TGraphAsymmErrors();

        if(channel == "") theory = new TGraph();

        for(unsigned int i=0; i < masses.size(); i++){
            //Read file and tree with limits
            TFile* limitFile = TFile::Open((limitDir + "/" + std::to_string(masses[i]) + "/" + channel + "/limit.root").c_str(), "READ");
            TTree* limitTree = (TTree*)limitFile->Get("limit");

            //Set branch for reading limits
            std::vector<double> limitValues;
            
            TLeaf* limitValue = limitTree->GetLeaf("limit");       

            //Values
            for(int j=0; j < 5; j++){
                limitValue->GetBranch()->GetEntry(j);
                limitValues.push_back(*(double*)limitValue->GetValuePointer() * xSecs[masses[i]]);
            }

            expected[channel]->SetPoint(i, masses[i], limitValues[2]);

            sigmaOne[channel]->SetPoint(i, masses[i], limitValues[2]);
            sigmaOne[channel]->SetPointEYlow(i, limitValues[2] - limitValues[1]);
            sigmaOne[channel]->SetPointEYhigh(i, limitValues[3] - limitValues[1]);     

            sigmaTwo[channel]->SetPoint(i, masses[i], limitValues[2]);
            sigmaTwo[channel]->SetPointEYlow(i, limitValues[2] - limitValues[0]);
            sigmaTwo[channel]->SetPointEYhigh(i, limitValues[4] - limitValues[1]);

            if(channel == "") theory->SetPoint(i, masses[i], xSecs[masses[i]]);
       
            delete limitTree;
            delete limitFile;
        }
    }
}

void PlotterLimit::Draw(std::vector<std::string> &outdirs){
    TCanvas* canvas = new TCanvas("canvas",  "canvas", 1000, 1000);
    TPad* mainPad = new TPad("mainPad", "mainPad", 0., 0. , 1., 1.);

    PUtil::SetStyle();

    //Draw main pad
    PUtil::SetPad(mainPad);
    mainPad->Draw();
    mainPad->cd();

    //Marker/lines styles
    expected.at("")->SetLineWidth(3);

    theory->SetLineWidth(3);
    theory->SetLineStyle(2);

    sigmaOne.at("")->SetFillColor(kGreen);
    sigmaOne.at("")->SetFillStyle(1001);

    sigmaTwo.at("")->SetFillColor(kYellow);
    sigmaTwo.at("")->SetFillStyle(1001);

    float min = std::min(theory->GetHistogram()->GetMinimum(), sigmaTwo.at("")->GetHistogram()->GetMinimum());
    float max = std::max(theory->GetHistogram()->GetMaximum(), sigmaTwo.at("")->GetHistogram()->GetMaximum());

    //Set range
    sigmaTwo.at("")->SetMinimum(min*0.5);
    sigmaTwo.at("")->SetMaximum(max*1e1);
    sigmaTwo.at("")->GetHistogram()->GetXaxis()->SetRangeUser(masses[0], masses.back());

    //Draw all the things
    sigmaTwo.at("")->Draw("A3");
    sigmaOne.at("")->Draw("3");
    expected.at("")->Draw("L");
    theory->Draw("L");

    //TLegend
    TLegend* legend = new TLegend(0.0, 0.0, 1.0, 1.0);

    //Add legend information
    legend->AddEntry(expected.at(""), "Expected", "L");
    legend->AddEntry(theory, "Theo. prediction", "L");
    legend->AddEntry(sigmaOne.at(""), "68% expected", "F");
    legend->AddEntry(sigmaTwo.at(""), "95% expected", "F");
    
    //Configure labels
    PUtil::SetHist(mainPad, sigmaTwo.at("")->GetHistogram());
    sigmaTwo.at("")->GetHistogram()->GetXaxis()->SetTitle("m(H^{#pm}) [GeV]");
    sigmaTwo.at("")->GetHistogram()->GetYaxis()->SetTitle("95% CL Limit on #sigma(pp #rightarrow H^{#pm}h #rightarrow lb#bar{b}b#bar{b}) [pb]");

    mainPad->cd();

    PUtil::DrawHeader(mainPad, "All channel", "Work in progress");

    //Save canvas in non-log and log scale
    mainPad->SetLogy(1);

    //Draw legend
    PUtil::DrawLegend(mainPad, legend, 2);

    for(std::string outdir: outdirs){
        canvas->SaveAs((outdir + "/limit.pdf").c_str());
        canvas->SaveAs((outdir + "/limit.png").c_str());
    }

    //Draw for each channel
    mainPad->Clear();
    legend->Clear();
    
    TH1F* frameHist = dynamic_cast<TH1F*>(sigmaTwo.at("")->GetHistogram()->Clone());
    frameHist->Clear();
    frameHist->SetMaximum(max*1e2);
    frameHist->Draw();

    theory->Draw("L SAME");
    expected.at("")->Draw("L SAME");

    legend->AddEntry(expected.at(""), "Combined", "L");
    legend->AddEntry(theory, "Theo. prediction", "L");

    std::vector<int> colors = {kRed, kBlue, kGreen, kViolet, kOrange, kMagenta};
    std::vector<int>::iterator color = colors.begin();
    
    for(const std::string& channel : channels){
        expected[channel]->SetLineWidth(3);
        expected[channel]->SetLineColor(*color); ++color;

        legend->AddEntry(expected.at(channel), PUtil::GetChannelTitle(channel).c_str(), "L");

        expected.at(channel)->Draw("L SAME");
    }

    PUtil::DrawLegend(mainPad, legend, 3);

    for(std::string outdir: outdirs){
        canvas->SaveAs((outdir + "/limit_by_channel.pdf").c_str());
        canvas->SaveAs((outdir + "/limit_by_channel.png").c_str());
    }
}
