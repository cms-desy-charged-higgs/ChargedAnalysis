#include <ChargedHiggs/Analysis/interface/plotterLimit.h>

PlotterLimit::PlotterLimit() : Plotter(){}

PlotterLimit::PlotterLimit(std::string &limitDir, std::vector<int> masses) : 
    Plotter(),
    limitDir(limitDir),
    masses(masses){

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

        theory = new TGraph();
        expected = new TGraph();
        sigmaOne = new TGraphAsymmErrors();
        sigmaTwo = new TGraphAsymmErrors();
    }


void PlotterLimit::ConfigureHists(std::vector<std::string> &processes){
    for(unsigned int i=0; i < masses.size(); i++){
        //Read file and tree with limits
        TFile* limitFile = TFile::Open((limitDir + "/" + std::to_string(masses[i]) + "/limit.root").c_str(), "READ");
        TTree* limitTree = (TTree*)limitFile->Get("limit");

        //Set branch for reading limits
        std::vector<double> limitValues;

        TTreeReader reader(limitTree);
        TTreeReaderValue<double> limitValue(reader, "limit");       

        //Values
        for(int j=0; j < 5; j++){
            reader.SetEntry(j);
            limitValues.push_back(*limitValue*xSecs[masses[i]]);
        }

        theory->SetPoint(i, masses[i], xSecs[masses[i]]);
        expected->SetPoint(i, masses[i], limitValues[2]);

        sigmaOne->SetPoint(i, masses[i], limitValues[2]);
        sigmaOne->SetPointEYlow(i, limitValues[2] - limitValues[1]);
        sigmaOne->SetPointEYhigh(i, limitValues[3] - limitValues[1]);     

        sigmaTwo->SetPoint(i, masses[i], limitValues[2]);
        sigmaTwo->SetPointEYlow(i, limitValues[2] - limitValues[0]);
        sigmaTwo->SetPointEYhigh(i, limitValues[4] - limitValues[1]); 

        delete limitTree;
        delete limitFile;
    }

    //Marker/lines styles
	expected->SetLineWidth(3);

	theory->SetLineWidth(3);
	theory->SetLineStyle(2);

	sigmaOne->SetFillColor(kGreen);
	sigmaOne->SetFillStyle(1001);

	sigmaTwo->SetFillColor(kYellow);
	sigmaTwo->SetFillStyle(1001);
}

void PlotterLimit::Draw(std::vector<std::string> &outdirs){
    TCanvas* canvas = new TCanvas("canvas",  "canvas", 1000, 800);
    TPad* mainpad = new TPad("mainpad", "mainpad", 0., 0. , 0.95, 1.);
    TPad* legendpad = new TPad("legendpad", "legendpad", 0.87, 0.3 , 1., 0.8);

    this->SetStyle();

    //TLegend
    TLegend* legend = new TLegend(0.0, 0.0, 1.0, 1.0);

    //Draw main pad
    this->SetPad(mainpad);
    mainpad->Draw();
    mainpad->cd();

    //Draw all the things
    sigmaTwo->Draw("A3");
    sigmaOne->Draw("3");
    expected->Draw("L");
    theory->Draw("L");

    //Add legend information
    legend->AddEntry(expected, "Expected", "L");
    legend->AddEntry(theory, "Theo. prediction", "L");
    legend->AddEntry(sigmaOne, "68% expected", "F");
    legend->AddEntry(sigmaTwo, "95% expected", "F");
    
    //Configure labels
    this->SetHist(sigmaTwo->GetHistogram());
    sigmaTwo->GetHistogram()->GetXaxis()->SetTitle("m(H^{#pm}) [GeV]");
    sigmaTwo->GetHistogram()->GetYaxis()->SetTitle("95% CL Limit on #sigma(pp #rightarrow H^{#pm}h #rightarrow lb#bar{b}b#bar{b}) [pb]");

    //Draw legend
    canvas->cd();
    legendpad->Draw();
    legendpad->cd();

    legend->SetTextSize(0.1);
    legend->Draw();

    mainpad->cd();

    this->DrawHeader(false, "All channel", "Work in progress");

    //Set range
    sigmaTwo->SetMinimum(theory->GetMinimum()*1e-2);
    sigmaTwo->GetHistogram()->GetXaxis()->SetRangeUser(masses[0], masses.back());

    //Save canvas in non-log and log scale
    mainpad->SetLogy(1);

    for(std::string outdir: outdirs){
        canvas->SaveAs((outdir + "/limit.pdf").c_str());
        canvas->SaveAs((outdir + "/limit.png").c_str());
    }
}
