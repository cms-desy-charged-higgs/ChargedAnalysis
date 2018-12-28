#include "JetMETCorrections/Modules/interface/JetResolution.h"

float jerResolution(std::string &inTXT, float jetPt, float jetEta, float rho){
    JME::JetResolution resolution = JME::JetResolution(inTXT.c_str());

    JME::JetParameters jet;
    jet.setJetPt(jetPt);
    jet.setJetEta(jetEta);
    jet.setRho(rho); 
    
    return resolution.getResolution(jet);
}

float jerSF(std::string &inTXT, float rho, float jetEta, float jetPt){
    JME::JetResolutionScaleFactor resolutionSF = JME::JetResolutionScaleFactor(inTXT.c_str());
    
    JME::JetParameters jet;
    jet.setJetPt(jetPt);
    jet.setJetEta(jetEta);
    jet.setRho(rho);    
    
    return resolutionSF.getScaleFactor(jet);
}
