#include "JetMETCorrections/Modules/interface/JetResolution.h"

class JetSmearer{

    private:
        JME::JetResolution jetReso;
        JME::JetResolutionScaleFactor jetResoSF;
        JME::JetParameters jet;
    
    public:
        JetSmearer(std::string &resoTXT, std::string &resoSFTXT){
            jetReso = JME::JetResolution(resoTXT.c_str());
            jetResoSF = JME::JetResolutionScaleFactor(resoSFTXT.c_str());
        }

        void SetJet(float pt, float eta, float rho){
            jet.setJetPt(pt);
            jet.setJetEta(eta);
            jet.setRho(rho);
        }

        float GetReso(){
            return jetReso.getResolution(jet);
        }

        float GetResoSF(){
            return jetResoSF.getScaleFactor(jet);
        } 

};
