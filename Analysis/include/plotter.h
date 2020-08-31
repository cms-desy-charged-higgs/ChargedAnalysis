#ifndef PLOTTER_H
#define PLOTTER_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include <TLatex.h>
#include <TGaxis.h>
#include <TError.h>
#include <TGraph.h>
#include <TROOT.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TH1.h>
#include <TH2F.h>
#include <TPad.h>

#include <ChargedAnalysis/Utility/include/plotutil.h>

class Plotter{
    protected:
        std::map<std::string, int> colors;

        std::string histdir;

    public:
        Plotter();
        Plotter(const std::string& histdir);

        virtual void ConfigureHists() = 0;
        virtual void Draw(std::vector<std::string> &outdirs) = 0;
        
};

#endif
