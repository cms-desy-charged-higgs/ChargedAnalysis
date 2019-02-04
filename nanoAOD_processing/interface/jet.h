#ifndef JET_H
#define JET_H 

#include <Rtypes.h>
#include "TLorentzVector.h"

struct Jet {
    Jet();

    TLorentzVector fourVec;
    Bool_t isLooseB;
    Bool_t isMediumB;
    Bool_t isTightB;
    Float_t bTagSF;
};

#endif
