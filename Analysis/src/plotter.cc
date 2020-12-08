#include <ChargedAnalysis/Analysis/include/plotter.h>

Plotter::Plotter() : Plotter("") {}

Plotter::Plotter(const std::string& histdir):
    histdir(histdir),
    colors({
        {"DYJ", kRed + -7}, 
        {"DYqq", kRed + -4}, 
        {"TT1L", kYellow -7}, 
        {"TT2L", kYellow +4}, 
        {"TTHad", kYellow +8}, 
        {"TTV", kOrange +2},            
        {"T", kGreen  + 2},             
        {"WJ", kCyan + 2},             
        {"QCD", kBlue -3},             
        {"VV", kViolet -3},
    })
    {}
