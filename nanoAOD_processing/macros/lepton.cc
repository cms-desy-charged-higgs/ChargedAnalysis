#include "Rtypes.h"
#include "TLorentzVector.h"

struct Lepton {
    TLorentzVector fourVec;
    Bool_t isLoose;
    Bool_t isMedium;
    Bool_t isTight;
    Float_t charge;
};

