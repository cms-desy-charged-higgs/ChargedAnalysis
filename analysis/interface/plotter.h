#ifndef PLOTTER_H
#define PLOTTER_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <utility>
#include <algorithm>

#include "TError.h"
#include "TROOT.h"
#include "TFile.h"
#include "TH1F.h"
#include "TGraph.h"
#include "THStack.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TStyle.h"
#include "TLatex.h"
#include "Rtypes.h"

class Plotter{
    
    private:
        std::string histdir;

        std::map<std::string, std::vector<TH1F*>> background;
        std::map<std::string, TH1F*> data;
        std::map<std::string, int> colors;
        std::map<std::string, std::string> xLabels;

    public:
        Plotter();
        Plotter(std::string &histdir);
        void ConfigureHists(std::vector<std::string> &parameters, std::vector<std::string> &processes);
        void Draw(std::vector<std::string> &outdirs);
        
};

#endif
