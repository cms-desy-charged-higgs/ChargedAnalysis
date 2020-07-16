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

enum class Axis{X, Y};

class TreeFunction{
    private:
        enum Particle{VACUUM, ELECTRON, MUON, BJET, BSUBJET, JET, FATJET, SUBJET, MET, HIGGS, CHAREDHIGGS, W};
        enum WP{NOTCLEAN = -10, NONE, LOOSE, MEDIUM, TIGHT};
        enum Comparison{BIGGER, SMALLER, EQUAL, DIVISIBLE, NOTDIVISIBLE};

        void(TreeFunction::*funcPtr)();
        std::string inputValue;

        std::shared_ptr<TFile> inputFile;
        TTree* inputTree;

        //Define another TreeFunction if wants to make 2D distributions
        std::shared_ptr<TreeFunction> yFunction;
    
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
        std::vector<TLeaf*> quantities;

        //Particle specific
        TLeaf* nPart1; 
        TLeaf* nPart2;
        TLeaf* ID;
        TLeaf* Isolation;
        TLeaf* BScore;
        TLeaf* nTrueB;
        std::vector<TLeaf*> scaleFactors, scaleFactorsUp, scaleFactorsDown;

        //BTag Histo
        TH2F* effBTag;

        //Particle information
        Particle part1 = VACUUM, part2 = VACUUM;
        WP wp1 = NONE, wp2 = NONE;
        int idx1 = -1., idx2 = -1., realIdx1, realIdx2;

        WP whichWP(const Particle& part, const int& idx);
        int whichIndex(TLeaf* nPart, const Particle& part, const int& idx, const WP& wp);
        bool isCleanJet(const int& idx);

        Particle cleanPart = VACUUM; WP cleanedWP = NONE;        
        TLeaf* cleanPhi;
        TLeaf* cleanEta;
        TLeaf* jetPhi;
        TLeaf* jetEta;

        //TreeFunction to get wished values
        void Pt();
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
        TreeFunction(std::shared_ptr<TFile>& inputFile, const std::string& treeName);
        ~TreeFunction();

        //Setter function
        template<Axis A>
        void SetP1(const std::string& part, const int& idx = 0, const std::string& wp = "");

        template<Axis A>
        void SetP2(const std::string& part, const int& idx = 0, const std::string& wp = "");

        template<Axis A>
        void SetFunction(const std::string& funcName, const std::string& inputValue = "");

        template<Axis A>
        void SetCleanJet(const std::string& part, const std::string& wp);

        void SetCut(const std::string& comp, const float& compValue);

        //Getter function
        template<Axis A>
        const float Get();

        template<Axis A>
        const std::string GetAxisLabel();

        template<Axis A>
        const std::string GetName();

        const int GetNWeights();
        const float GetWeight();
        const bool GetPassed();
        const std::string GetCutLabel();

        //Set y axis
        void SetYAxis();
        const bool hasYAxis();

        //Static function for global configuration of all instances
        static void SetEntry(const int& entry);
};

#endif A_
