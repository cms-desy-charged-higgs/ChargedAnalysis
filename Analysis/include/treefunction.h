#ifndef TREEFUNCTION_H
#define TREEFUNCTION_H

#include <vector>
#include <string>
#include <map>

#include <Math/GenVector/VectorUtil.h>
#include <Math/Vector4Dfwd.h>

enum Particle{ELECTRON, MUON, JET, FATJET, MET};
enum WP{NONE, LOOSE, MEDIUM, TIGHT};

struct Event{
    std::map<Particle, std::vector<ROOT::Math::PxPyPzEVector>> particles;
    float HT;
};

struct FuncArgs{
    std::vector<Particle> parts;
    std::vector<int> index;
};

struct Function{
    float(*func)(Event&, FuncArgs&);

    float operator()(Event& event, FuncArgs& args){
        return func(event, args);
    }
};

namespace TreeFunction{
    extern std::map<std::string, std::pair<float(*)(Event&, FuncArgs&), std::string>> funcMap;
    extern std::map<std::string, std::pair<Particle, std::string>> partMap;

    float Mass(Event& event, FuncArgs& args);
    float Phi(Event& event, FuncArgs& args);
    float Pt(Event& event, FuncArgs& args);
    float Eta(Event& event, FuncArgs& args);
    float DeltaPhi(Event& event, FuncArgs& args);
    float DeltaR(Event& event, FuncArgs& args);
};


#endif
