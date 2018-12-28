#include "Rtypes.h"
#include "TLorentzVector.h"

struct Jet {
    TLorentzVector fourVec;
    Bool_t isLooseB;
    Bool_t isMediumB;
    Bool_t isTightB;
    Float_t bTagSF;
};
