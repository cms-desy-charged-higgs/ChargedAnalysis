#include <ChargedAnalysis/Analysis/interface/treereader.h>
#include <ChargedAnalysis/Analysis/interface/plottertriggeff.h>
#include <ChargedAnalysis/Analysis/interface/plotter1D.h>
#include <ChargedAnalysis/Analysis/interface/plotter2D.h>
#include <ChargedAnalysis/Analysis/interface/plotterLimit.h>
#include <ChargedAnalysis/Analysis/interface/plotterPostfit.h>
#include <ChargedAnalysis/Analysis/interface/plotterCut.h>
#include <ChargedAnalysis/Analysis/interface/plotterGen.h>
#include <ChargedAnalysis/Analysis/interface/bdt.h>
#include <ChargedAnalysis/Analysis/interface/limit.h>

namespace{
    namespace{
        TreeReader reader;
        PlotterTriggEff plotterTriggEff;
        Plotter1D plotter1D;
        Plotter2D plotter2D;
        PlotterLimit plotterLimit;
        PlotterPostfit plotterPostfit;
        PlotterCut plotterCut;
        PlotterGen plotterGen;
        BDT bdt;
        Limit lim;
    }
}

