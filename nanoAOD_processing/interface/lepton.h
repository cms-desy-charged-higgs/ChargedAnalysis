#ifndef LEPTON_H
#define LEPTON_H

#include "TLorentzVector.h"
#include "Rtypes.h"

struct Lepton {
    Lepton();

    TLorentzVector fourVec;
    Bool_t isLoose;
    Bool_t isMedium;
    Bool_t isTight;

    Float_t recoSF;
    Float_t looseMvaSF;
    Float_t mediumMvaSF;
    Float_t tightMvaSF;

    Float_t charge;
    Float_t isolation; 
};

#endif
