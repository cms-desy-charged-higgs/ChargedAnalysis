#ifndef TREEFUNCTION
#define TREEFUNCTION

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cctype>
#include <tuple>

#include <TLeaf.h>
#include <TTree.h>
#include <TFile.h>
#include <TH2F.h>

#include <ChargedAnalysis/Utility/include/utils.h>

class TreeFunction{
    private:
        enum Particle{VACUUM, ELECTRON, MUON, BJET, BSUBJET, JET, FATJET, SUBJET, MET, HIGGS, CHAREDHIGGS, W};
        enum WP{NOTCLEAN = -10, NONE, LOOSE, MEDIUM, TIGHT};
        enum Comparison{BIGGER, SMALLER, EQUAL, DIVISIBLE, NOTDIVISIBLE};

        void(TreeFunction::*funcPtr)();
        float inputValue;

        Comparison comp;
        float compValue;

        float value;
        float weight;

        std::string partLabel1 = "", partLabel2 = "", partName1 = "", partName2 = "", wpName1 = "", wpName2 = "";
        std::string name, axisLabel, cutLabel;

        inline static int entry = 0;

        std::map<std::string, std::tuple<void(TreeFunction::*)(), std::string>> funcInfo;
        std::map<std::string, std::tuple<Particle, std::string, std::string, std::string>> partInfo;
        std::map<std::string, std::tuple<WP, std::string>> wpInfo;
        std::map<std::string, std::tuple<Comparison, std::string>> comparisons;

        //General TLeafes
        TFile* inputFile;
        TTree* inputTree;
        std::vector<TLeaf*> quantities;

        //Particle specific
        TLeaf* nPart;
        TLeaf* ID;
        TLeaf* Isolation;
        TLeaf* BScore;
        TLeaf* nTrueB;
        std::vector<TLeaf*> scaleFactors;

        //BTag Histo
        TH2F* effBTag = nullptr;

        //Particle information
        Particle part1 = VACUUM, part2 = VACUUM;
        WP wp1 = NONE, wp2 = NONE;
        int idx1 = -1., idx2 = -1., realIdx1, realIdx2;

        WP whichWP(const Particle& part, const int& idx);
        bool isCleanJet(const int& idx);

        Particle cleanPart = VACUUM; WP cleanedWP = NONE;        
        TLeaf* cleanPhi = nullptr; 
        TLeaf* cleanEta = nullptr;
        TLeaf* jetPhi = nullptr; 
        TLeaf* jetEta = nullptr;

        //TreeFunction to get wished values
        void Pt();
        void Phi();
        void Eta();
        void Mass();
        void JetSubtiness();
        void EventNumber();
        void HT();
        void NParticle();
        void HTag();
        void DNN();
        void DeepAK();

    public:
        TreeFunction(TFile* inputFile, const std::string& treeName);
        ~TreeFunction();

        //Setter function
        void SetP1(const std::string& part, const int& idx = 0, const std::string& wp = "");
        void SetP2(const std::string& part, const int& idx = 0, const std::string& wp = "");
        void SetCleanJet(const std::string& part, const std::string& wp);
        void SetCut(const std::string& comp, const float& compValue);
        void SetFunction(const std::string& funcName, const float& inputValue = -999.);

        //Getter function
        const float Get();
        const float GetWeight();
        const bool GetPassed();
        const std::string GetAxisLabel();
        const std::string GetCutLabel();
        const std::string GetName();

        //Static function for global configuration of all instances
        static void SetEntry(const int& entry);
};

#endif A_
