#ifndef NTUPLEREADER_H
#define NTUPLEREADER_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <experimental/source_location>

#include <TTree.h>
#include <TLeaf.h>

#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

bool inline Bigger(const float& v1, const float& v2){
    return v1 != -999 and v1 >= v2; 
}

bool inline Smaller(const float& v1, const float& v2){
    return v1 != -999 and v1 <= v2; 
}

bool inline Equal(const float& v1, const float& v2){
    return v1 != -999 and v1 == v2; 
}

class CompiledCut;

class CompiledFunc{
    private:
        float(*func)(TLeaf*, const int&);
        float(*funcWithWp)(TLeaf*, std::vector<CompiledCut>&, const int&);
        TLeaf* leaf;
        int partIdx;
        std::vector<CompiledCut> cuts;
        bool isIter;

    public:
        CompiledFunc(){}
        CompiledFunc(float(*func)(TLeaf*, const int&), TLeaf* leaf, const int& partIdx, const bool& isIter = false) : 
            func(func), leaf(leaf), partIdx(isIter ? 0 : partIdx), cuts({}), isIter(isIter) {}
        CompiledFunc(float(*funcWithWp)(TLeaf*, std::vector<CompiledCut>&, const int&), TLeaf* leaf, std::vector<CompiledCut>& cuts, const int& partIdx, const bool& isIter = false) : 
            funcWithWp(funcWithWp), leaf(leaf), cuts(cuts), partIdx(isIter ? 0 : partIdx), isIter(isIter) {}
        virtual bool GetPassed(){};
        virtual float Get(){
            if(cuts.empty()) return (*func)(leaf, partIdx);
            else return (*funcWithWp)(leaf, cuts, partIdx);
        }
        virtual void Next(){if(isIter) ++partIdx;}
        virtual void Reset(){if(isIter) partIdx = 0;}
};

using Particles = std::map<std::pair<std::string, std::string>, std::shared_ptr<CompiledFunc>>;

class CompiledCustomFunc : public CompiledFunc {
    private:
        float(*func)(Particles& parts);
        Particles parts;

    public:
        CompiledCustomFunc(float(*func)(Particles& parts), const Particles& parts) : func(func), parts(parts) {}
        float Get(){return (*func)(parts);}
        void Next(){for(auto& p : parts) p.second->Next();}
        void Reset(){for(auto& p : parts) p.second->Reset();};
};

class CompiledCut : public CompiledFunc {
    private:
        std::shared_ptr<CompiledFunc> func;
        bool(*op)(const float&, const float&);
        float comp;

    public:
        CompiledCut(){}
        CompiledCut(const std::shared_ptr<CompiledFunc>& func, bool(*op)(const float&, const float&), const float& comp) : func(func), op(op), comp(comp) {}
        bool GetPassed(){return op(func->Get(), comp);}
        void Next(){func->Next();}
        void Reset(){func->Reset();};
};

namespace Properties{
    float HT(Particles& parts);
    float dR(Particles& parts);
    float dPhi(Particles& parts);
    float diCharge(Particles& parts);
    float NParticles(Particles& parts);

    //Register all functions here!
    static std::map<std::string, float(*)(Particles&)> properties = {
        {"dR", &dR},
        {"dphi", &dPhi},
        {"N", &NParticles},
        {"HT", &HT},
        {"dicharge", &diCharge},
    };
};

namespace pt = boost::property_tree;

class NTupleReader{
    private:
        pt::ptree partInfo;
        pt::ptree funcInfo;

        pt::ptree function;
        std::vector<pt::ptree> particles;

        std::shared_ptr<TTree> inputTree;

        std::shared_ptr<CompiledFunc> func;
        std::shared_ptr<CompiledCut> cut;

        bool isCompiled = false;
        inline static int entry = 0; 

        static float GetEntry(TLeaf* leaf, const int& idx);
        static float GetEntryWithWP(TLeaf* leaf, std::vector<CompiledCut>& cuts, const int& idx);

        std::shared_ptr<CompiledFunc> compileBranchReading(const pt::ptree& func, const pt::ptree& part);
        std::shared_ptr<CompiledFunc> compileCustomFunction(const pt::ptree& func, const std::vector<pt::ptree>& parts);

    public:
        NTupleReader();
        NTupleReader(const std::shared_ptr<TTree>& inputTree);

        void AddParticle(const std::string& pAlias, const std::size_t& idx, const std::string& wp, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddFunction(const std::string& fInfo, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddFunctionByBranchName(const std::string& branchName, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddCut(const float&, const std::string& op, const std::experimental::source_location& location = std::experimental::source_location::current());

        void Compile(const std::experimental::source_location& location = std::experimental::source_location::current());

        float Get();
        bool GetPassed();
        std::string GetHistName();
        std::string GetAxisLabel();
        std::string GetCutName();

        static void SetEntry(const int& entry){NTupleReader::entry = entry;}

        //Helper function to get keys of ptree
        static std::vector<std::string> GetInfo(const pt::ptree& node, const bool GetInfo = true){
            std::vector<std::string> keys;

            for(std::pair<const std::string, pt::ptree> p : node){
                keys.push_back(GetInfo ? p.first : p.second.get_value<std::string>());
            }

            return keys;
        }

        //Helper function to get name of child tree with wished alias in it
        static std::string GetName(const pt::ptree& node, const std::string& alias){
           for(const std::string& name : NTupleReader::GetInfo(node)){
                if(node.get<std::string>(name + ".alias") == alias) return name;
           }
    
            return "";
        }
};

#endif
