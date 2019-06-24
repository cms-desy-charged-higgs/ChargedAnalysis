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

        std::string histdir;
        std::vector<std::string> xParameters;
        std::vector<std::string> yParameters;

        std::string channel;
        std::map<std::string, Processes> procDic;
        std::map<std::string, std::string> channelHeader;

        void DrawHeader(const bool &twoPads, const std::string &titleText, const std::string &cmsText);
        void SetStyle();
        void SetPad(TPad* pad);
        void SetHist(TH1* frameHist);

    public:
        virtual ~Plotter();
        Plotter();
        Plotter(std::string &histdir, std::vector<std::string> &xParameters, std::string &channel);
        Plotter(std::string &histdir, std::vector<std::string> &xParameters, std::vector<std::string> &yParameters, std::string &channel);

        virtual void ConfigureHists(std::vector<std::string> &processes) = 0;
        virtual void Draw(std::vector<std::string> &outdirs) = 0;
        
};

#endif
