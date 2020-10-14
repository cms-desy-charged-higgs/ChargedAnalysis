#include <ChargedAnalysis/Utility/include/mathutil.h>

float MUtil::DeltaR(const float& eta1, const float& phi1, const float& eta2, const float& phi2){
    return std::sqrt(std::pow(eta1 - eta2, 2) + std::pow(phi1 - phi2, 2));
}

float MUtil::DeltaPhi(const float& phi1, const float& phi2){
    return std::acos(std::cos(phi1)*std::cos(phi2) + std::sin(phi1)*std::sin(phi2));
}

int MUtil::RowMajIdx(const std::vector<int>& dimensions, const std::vector<int>& indeces){
    int idx = 0;

    for(int i = 0; i < dimensions.size(); i++){
        int product = 1;

        for(int j = i + 1; j < dimensions.size(); j++){
            product *= dimensions.at(j);
        }
    
        idx += indeces.at(i) * product;
    }

    return idx;
}
