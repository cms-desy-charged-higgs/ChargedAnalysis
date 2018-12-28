#include "Rtypes.h"
#include "TLorentzVector.h"

struct Lepton {
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

