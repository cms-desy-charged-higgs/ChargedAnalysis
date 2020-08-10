#include <ChargedAnalysis/Utility/include/mathutil.h>

float MUtil::DeltaR(const float& eta1, const float& phi1, const float& eta2, const float& phi2){
    return std::sqrt(std::pow(eta1 - eta2, 2) + std::pow(phi1 - phi2, 2));
}

float MUtil::DeltaPhi(const float& phi1, const float& phi2){
    return std::acos(std::cos(phi1)*std::cos(phi2) + std::sin(phi1)*std::sin(phi2));
}
