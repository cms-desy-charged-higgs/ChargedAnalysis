#include <ChargedAnalysis/Analysis/include/plotter.h>

Plotter::Plotter() : Plotter("") {}

Plotter::Plotter(const std::string& histdir):
    histdir(histdir),
    colors({
        {"DY+j", kRed + -7}, 
        {"DYqq", kRed + -4}, 
        {"TT-1L", kYellow -7}, 
        {"TT-2L", kYellow +4}, 
        {"TT-Had", kYellow +8}, 
        {"TT+V", kOrange +2},            
        {"T", kGreen  + 2},             
        {"W+j", kCyan + 2},             
        {"QCD", kBlue -3},             
        {"VV+VVV", kViolet -3},
    })
    {}
