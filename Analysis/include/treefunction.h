#ifndef TREEFUNCTION_H
#define TREEFUNCTION_H

#include <vector>
#include <string>
#include <map>
#include <functional>

#include <Math/GenVector/VectorUtil.h>
#include <Math/Vector4Dfwd.h>

enum Particle{ELECTRON, MUON, JET, BJET, FATJET, BFATJET, SUBJET, BSUBJET, MET};
enum WP{NONE, LOOSE, MEDIUM, TIGHT};
enum Comparison{BIGGER, SMALLER, EQUAL};

struct Event{
    std::map<Particle, std::map<WP, std::vector<ROOT::Math::PxPyPzEVector>>> particles;
    float HT;
};

struct FuncArgs{
    std::vector<Particle> parts;
    std::vector<int> index;
    std::vector<WP> wp;
    float value;
    Comparison comp;
    float compValue;
};

struct Function{
    float(*func)(Event&, FuncArgs&);
    std::string funcName, partName, index, wp;

    float operator()(Event& event, FuncArgs& args){
        try{
            return func(event, args);
        }

        catch (const std::exception& e){
            throw std::runtime_error("Error in function '" + funcName + "': No particle '" + partName + "' with index '" + index + "' and working point '" + wp + "'");
        }
    }

    bool operator()(Event& event, FuncArgs& args, const bool& isCut){
        switch(args.comp){
            case BIGGER:
                return func(event, args) > args.compValue;

            case SMALLER:
                return func(event, args) < args.compValue;

            case EQUAL:
                return func(event, args) == args.compValue;
        }
    }
};

namespace TreeFunction{
    extern std::map<std::string, std::pair<float(*)(Event&, FuncArgs&), std::string>> funcMap;
    extern std::map<std::string, std::pair<Particle, std::string>> partMap;
    extern std::map<std::string, WP> workingPointMap;
    extern std::map<std::string, Comparison> comparisonMap;

    float Mass(Event& event, FuncArgs& args);
    float Phi(Event& event, FuncArgs& args);
    float Pt(Event& event, FuncArgs& args);
    float Eta(Event& event, FuncArgs& args);
    float DeltaPhi(Event& event, FuncArgs& args);
    float DeltaR(Event& event, FuncArgs& args);
    float NParticle(Event &event, FuncArgs& args);
    float ConstantNumber(Event &event, FuncArgs& args);
};

#endif
