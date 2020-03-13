#ifndef TREEFUNCTION_H
#define TREEFUNCTION_H

#include <vector>
#include <string>
#include <map>
#include <iostream>

#include <Math/GenVector/VectorUtil.h>
#include <Math/Vector4Dfwd.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/src/bimap.cc>

enum Particle{ELECTRON, MUON, JET, BJET, FATJET, BFATJET, SUBJET, BSUBJET, MET};
enum WP{NONE, LOOSE, MEDIUM, TIGHT};
enum Comparison{BIGGER, SMALLER, EQUAL, DIVISIBLE, NOTDIVISIBLE};

struct Event{
    std::map<Particle, std::map<WP, std::vector<ROOT::Math::PxPyPzEVector>>> particles;
    std::map<Particle, std::map<WP, std::vector<float>>> SF;
    float weight;
    int eventNumber;
};

struct FuncArgs{
    std::vector<Particle> parts;
    std::vector<int> index;
    std::vector<WP> wp;
    float value;
    Comparison comp;
    float compValue;
};

namespace TreeFunction{
    extern Utils::Bimap<std::string, float(*)(Event&, FuncArgs&)> functions;
    extern Utils::Bimap<float(*)(Event&, FuncArgs&), std::string> funcLabels;
    extern Utils::Bimap<std::string, Particle> particles;
    extern Utils::Bimap<Particle, std::string> partLabels;
    extern Utils::Bimap<std::string, WP> workingPoints;
    extern Utils::Bimap<std::string, Comparison> comparisons;

    float Mass(Event& event, FuncArgs& args);
    float Phi(Event& event, FuncArgs& args);
    float Pt(Event& event, FuncArgs& args);
    float Eta(Event& event, FuncArgs& args);
    float DeltaPhi(Event& event, FuncArgs& args);
    float DeltaR(Event& event, FuncArgs& args);
    float NParticle(Event &event, FuncArgs& args);
    float ConstantNumber(Event &event, FuncArgs& args);
    float HadronicEnergy(Event &event, FuncArgs& args);
    float EventNumber(Event &event, FuncArgs& args);
};

struct Function{
    float(*func)(Event&, FuncArgs&);

    float operator()(Event& event, FuncArgs& args){
        try{
            return func(event, args);
        }

        catch (const std::exception& e){
            return -999.;
        }
    }

    bool operator()(Event& event, FuncArgs& args, const bool& isCut){
        switch(args.comp){
            case BIGGER:
                return this->operator()(event, args) > args.compValue;

            case SMALLER:
                return this->operator()(event, args) < args.compValue;

            case EQUAL:
                return this->operator()(event, args) == args.compValue;

            case DIVISIBLE:
                return int(this->operator()(event, args)) % int(args.compValue) == 0;

            case NOTDIVISIBLE:
                return int(this->operator()(event, args)) % int(args.compValue) != 0;
        }
    }
};

#endif
