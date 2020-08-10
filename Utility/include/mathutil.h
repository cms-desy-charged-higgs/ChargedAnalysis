#ifndef MATHUTIL_H
#define MATHUTIL_H

#include <cmath>
#include <memory>
#include <functional>

#include <ChargedAnalysis/Utility/include/util.h>

/**
* @brief Math utility library to for calculating properties
*/

namespace MUtil{
    /**
    * @brief Calculates @f$\Delta @f$R between to particles
    *
    * @param eta1 Eta value of first particle
    * @param phi1 Phi value of first particle
    * @param eta2 Eta value of second particle
    * @param phi2 Phi value of second particle
    * @return Return @f$\Delta @f$R
    */
    float DeltaR(const float& eta1, const float& phi1, const float& eta2, const float& phi2);

    /**
    * @brief Calculates @f$\Delta\phi @f$ between to particles
    *
    * @param phi1 Phi value of first particle
    * @param phi2 Phi value of second particle
    * @return Return @f$\Delta\phi @f$
    */
    float DeltaPhi(const float& phi1, const float& phi2);
};

#endif
