#ifndef ELECTRON_H
#define ELECTRON_H

#include "Rtypes.h"
#include "TLorentzVector.h"

struct Electron {
    TLorentzVector fourVec;
    Bool_t isMedium;
    Bool_t isTight;

    Float_t recoSF;
    Float_t mediumMvaSF;
    Float_t tightMvaSF;

    Float_t charge;
    Float_t isolation; 
};

#endif
