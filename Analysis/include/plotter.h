#ifndef PLOTTER_H
#define PLOTTER_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <utility>
#include <algorithm>
#include <cmath>
#include <functional>

#include <TLatex.h>
#include <TGaxis.h>
#include <TError.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TH1.h>
#include <TPad.h>
#include <TMath.h>

class Plotter{
    protected:
        enum Processes {BKG, DATA, SIGNAL};

        std::map<std::string, Processes> procDic;
        std::map<std::string, std::string> channelHeader;
        std::map<std::string, int> colors;

        std::string histdir;

    public:
        Plotter();
        Plotter(const std::string& histdir);

        static void DrawHeader(const bool &twoPads, const std::string &titleText, const std::string &cmsText);
        static void SetStyle();
        static void SetPad(TPad* pad);
        static void SetHist(TH1* frameHist);

        virtual void ConfigureHists() = 0;
        virtual void Draw(std::vector<std::string> &outdirs) = 0;
        
};

#endif
