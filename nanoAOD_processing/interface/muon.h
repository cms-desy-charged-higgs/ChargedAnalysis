#ifndef MUON_H
#define MUON_H

#include "Rtypes.h"
#include "TLorentzVector.h"

struct Muon {
    Muon();

    TLorentzVector fourVec;
    Bool_t isMedium;
    Bool_t isTight;
    Bool_t isLooseIso;
    Bool_t isTightIso;

    Float_t looseIsoMediumSF;
    Float_t tightIsoMediumSF;
    Float_t looseIsoTightSF;
    Float_t tightIsoTightSF;
    Float_t mediumSF;
    Float_t tightSF;
    Float_t triggerSF;

    Float_t charge;
};

#endif
