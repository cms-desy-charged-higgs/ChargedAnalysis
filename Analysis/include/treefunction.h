#ifndef TREEFUNCTION_H
#define TREEFUNCTION_H

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <iostream>

#include <Math/GenVector/VectorUtil.h>
#include <Math/Vector4Dfwd.h>
#include <TH2F.h>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/src/bimap.cc>

enum Particle{ELECTRON, MUON, JET, BJET, FATJET, SUBJET, BSUBJET, MET, NPART};
enum WP{LOOSE, MEDIUM, TIGHT, NONE, NWP};
enum Comparison{BIGGER, SMALLER, EQUAL, DIVISIBLE, NOTDIVISIBLE};

struct Event; struct Function; struct FuncArgs;

struct Event{
    const int NMAX=15;
    bool isData=true;

    std::vector<std::shared_ptr<ROOT::Math::PxPyPzEVector>> particles = std::vector<std::shared_ptr<ROOT::Math::PxPyPzEVector>>(NPART*NWP*NMAX, NULL);
    std::vector<float> SF = std::vector<float>(NPART*NWP*NMAX, 1.);
    std::vector<bool> isTrueB = std::vector<bool>(NPART*NWP*NMAX, 1.);
    std::vector<TH2F*> effBTag;
    std::vector<std::vector<float>> subtiness;
    float weight;
    int eventNumber;
    std::map<int, float> bdtScore, dnnScore;

    static int Index(const Particle& part, const WP& wp, const int& index){
        return part + NPART*(wp + NWP*index);
    }

    void Clear(){
        for(int i=0; i < particles.size(); i++){
            if(particles[i] != NULL) particles[i] = NULL;
        }

        this->subtiness.clear();
    }
};

struct FuncArgs{
    std::vector<Particle> parts;
    std::vector<int> index;
    std::vector<WP> wp;
    int value=-999.;
    Comparison comp;
    float compValue=-999.;
};

struct Function{
    float(*func)(Event&, FuncArgs&);

    float operator()(Event& event, FuncArgs& args){return func(event, args);}

    bool operator()(Event& event, FuncArgs& args, const bool& isCut){
        switch(args.comp){
            case BIGGER:
                return func(event, args) > args.compValue;

            case SMALLER:
                return func(event, args) < args.compValue;

            case EQUAL:
                return func(event, args) == args.compValue;

            case DIVISIBLE:
                return int(func(event, args)) % int(args.compValue) == 0;

            case NOTDIVISIBLE:
                return int(func(event, args)) % int(args.compValue) != 0;
        }
    }
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
    float Subtiness(Event &event, FuncArgs& args);
    float BDTScore(Event &event, FuncArgs& args);
    float DNNScore(Event &event, FuncArgs& args);
};

#endif
