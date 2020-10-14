#ifndef TREEFUNCTION_H
#define TREEFUNCTION_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cctype>
#include <tuple>
#include <memory>

#include <TLeaf.h>
#include <TTree.h>
#include <TFile.h>
#include <TH2F.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/mathutil.h>

enum class Axis{X, Y};

class TreeFunction{
    private:
        enum Particle{VACUUM, ELECTRON = 11, MUON = 13, BJET = 6, BSUBJET, JET, FATJET, SUBJET, MET = 12, HIGGS = 25, CHAREDHIGGS = 37, W = 24};
        enum WP{NOTCLEAN = -10, NONE, LOOSE, MEDIUM, TIGHT};
        enum Comparison{BIGGER, SMALLER, EQUAL, DIVISIBLE, NOTDIVISIBLE};

        enum Systematic{NOMINAL, BTAG, SUBBTAG, MUTRIGG, MUID, MUISO, ELERECO, ELEID};
        enum Shift{NON, UP, DOWN};

        void(TreeFunction::*funcPtr)();
        std::string inputValue;

        std::shared_ptr<TFile> inputFile;
        TTree* inputTree;
        int era;
        std::vector<Systematic> systematics;

        //Define another TreeFunction if wants to make 2D distributions
        std::shared_ptr<TreeFunction> yFunction;
    
        Comparison comp;
        float compValue;

        float value;
        float weight;
        std::vector<float> systWeights;

        std::string partLabel1 = "", partLabel2 = "", partName1 = "", partName2 = "", wpName1 = "", wpName2 = "";
        std::string name, axisLabel, cutLabel;

        inline static int entry = 0;

        std::map<std::string, std::tuple<void(TreeFunction::*)(), std::string>> funcInfo;
        std::map<std::string, std::tuple<Particle, std::string, std::string, std::string>> partInfo;
        std::map<std::string, std::tuple<WP, std::string>> wpInfo;
        std::map<std::string, std::tuple<Comparison, std::string>> comparisons;
        std::map<std::string, Systematic> systInfo;

        //General TLeafes
        std::vector<TLeaf*> quantities;

        //Particle specific
        TLeaf* particleID;
        TLeaf* motherID;
        TLeaf* nPart1; 
        TLeaf* nPart2;
        TLeaf* ID;
        TLeaf* Isolation;
        TLeaf* BScore;
        TLeaf* TrueFlavour;
        std::map<Systematic, TLeaf*> scaleFactors, scaleFactorsUp, scaleFactorsDown;

        //BTag Histo
        TH2F* effB;
        TH2F* effC;
        TH2F* effLight;

        //B Cut values
        std::map<int, float> looseBCut = {{2016, 0.2217}, {2017, 0.1522}, {2018, 0.1241}};
        std::map<int, float> mediumBCut = {{2016, 0.6321}, {2017, 0.4941}, {2018, 0.4184}};
        std::map<int, float> tightBCut = {{2016, 0.8953}, {2017, 0.8001}, {2018, 0.7527}};

        //Particle information
        Particle part1 = VACUUM, part2 = VACUUM;
        WP wp1 = NONE, wp2 = NONE;
        int idx1 = -1., idx2 = -1., genMother1 = -1., genMother2 = -1., realIdx1, realIdx2;

        WP whichWP(const Particle& part, const int& idx);
        int whichIndex(TLeaf* nPart, const Particle& part, const int& idx, const WP& wp, const int& genMother = -1.);
        bool isCleanJet(const int& idx);

        Particle cleanPart = VACUUM; WP cleanedWP = NONE;        
        TLeaf* cleanPhi;
        TLeaf* cleanEta;
        TLeaf* jetPhi;
        TLeaf* jetEta;

        //TreeFunction to get wished values
        void Pt();
        void Mt();
        void Phi();
        void Eta();
        void Mass();
        void DeltaR();
        void DeltaPhi();
        void JetSubtiness();
        void EventNumber();
        void HT();
        void NParticle();
        void HTag();
        void DNN();
        void DeepAK();

    public:
        TreeFunction(std::shared_ptr<TFile>& inputFile, const std::string& treeName, const int& era = 2017);
        ~TreeFunction();

        //Setter function
        template<Axis A>
        void SetP1(const std::string& part, const int& idx = 0, const std::string& wp = "", const int& genMother = -1.);

        template<Axis A>
        void SetP2(const std::string& part, const int& idx = 0, const std::string& wp = "", const int& genMother = -1.);

        template<Axis A>
        void SetFunction(const std::string& funcName, const std::string& inputValue = "");

        template<Axis A>
        void SetCleanJet(const std::string& part, const std::string& wp);

        void SetSystematics(const std::vector<std::string>& systNames);
        void SetCut(const std::string& comp, const float& compValue);

        //Getter function
        template<Axis A>
        const float Get();

        template<Axis A>
        const std::string GetAxisLabel();

        template<Axis A>
        const std::string GetName();
    
        const float GetWeight(const Systematic syst = NOMINAL, const Shift& shift = NON);
        const std::vector<float> GetSystWeight();

        const bool GetPassed();
        const std::string GetCutLabel();

        //Set y axis
        void SetYAxis();
        const bool hasYAxis();

        //Static function for global configuration of all instances
        static void SetEntry(const int& entry);
};

#endif A_
