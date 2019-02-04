#include <CondFormats/JetMETObjects/interface/JetResolutionObject.h>

class JetSmearer{

    private:
        JME::JetResolutionObject jetReso;
        JME::JetResolutionObject jetResoSF;
        JME::JetParameters jet;
    
    public:
        JetSmearer();
        JetSmearer(std::string &resoTXT, std::string &resoSFTXT){
            jetReso = JME::JetResolutionObject(resoTXT.c_str());
            jetResoSF = JME::JetResolutionObject(resoSFTXT.c_str());
        }

        void SetJet(float pt, float eta, float rho){
            jet.setJetPt(pt);
            jet.setJetEta(eta);
            jet.setRho(rho);
        }

        float GetReso(){
            const JME::JetResolutionObject::Record* record = jetReso.getRecord(jet);
            if (! record)
                return 1;

            return jetReso.evaluateFormula(*record, jet);
        }

        float GetResoSF(){
            const JME::JetResolutionObject::Record* record = jetResoSF.getRecord(jet);
            if (! record)
                return 1;

            const std::vector<float>& parameters_values = record->getParametersValues();

            return parameters_values[0]; //0 Nominal, 1 Down, 2 Up
        } 

};
