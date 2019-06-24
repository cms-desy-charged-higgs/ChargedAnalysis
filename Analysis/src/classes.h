#include <ChargedHiggs/Analysis/interface/treereader.h>
#include <ChargedHiggs/Analysis/interface/plottertriggeff.h>
#include <ChargedHiggs/Analysis/interface/plotter1D.h>
#include <ChargedHiggs/Analysis/interface/plotter2D.h>
#include <ChargedHiggs/Analysis/interface/plotterLimit.h>
#include <ChargedHiggs/Analysis/interface/plotterCut.h>
#include <ChargedHiggs/Analysis/interface/plotterGen.h>
#include <ChargedHiggs/Analysis/interface/bdt.h>
#include <ChargedHiggs/Analysis/interface/limit.h>

namespace{
    namespace{
        TreeReader reader;
        PlotterTriggEff plotterTriggEff;
        Plotter1D plotter1D;
        Plotter2D plotter2D;
        PlotterLimit plotterLimit;
        PlotterCut plotterCut;
        PlotterGen plotterGen;
        BDT bdt;
        Limit lim;
    }
}

