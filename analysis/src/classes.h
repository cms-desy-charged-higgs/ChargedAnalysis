#include <ChargedHiggs/analysis/interface/treereader.h>
#include <ChargedHiggs/analysis/interface/plottertriggeff.h>
#include <ChargedHiggs/analysis/interface/plotter1D.h>
#include <ChargedHiggs/analysis/interface/plotter2D.h>
#include <ChargedHiggs/analysis/interface/plotterLimit.h>
#include <ChargedHiggs/analysis/interface/plotterCut.h>
#include <ChargedHiggs/analysis/interface/plotterGen.h>
#include <ChargedHiggs/analysis/interface/bdt.h>
#include <ChargedHiggs/analysis/interface/limit.h>

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

