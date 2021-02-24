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

class CompiledFunc{
    private:
        float(*func)(TLeaf*, const int&);
        float(*funcWithWp)(TLeaf*, std::vector<std::shared_ptr<CompiledFunc>>&, const int&, const std::size_t&);
        TLeaf* leaf;
        int partIdx;
        std::size_t partHash;
        std::vector<std::shared_ptr<CompiledFunc>> cuts;
        bool(*op)(const float&, const float&);
        float comp;
        bool isIter;

    public:
        CompiledFunc(){}
        CompiledFunc(float(*func)(TLeaf*, const int&), TLeaf* leaf, const int& partIdx, const bool& isIter = false) : 
            func(func), leaf(leaf), partIdx(isIter ? 0 : partIdx), cuts({}), isIter(isIter) {}
        CompiledFunc(float(*funcWithWp)(TLeaf*, std::vector<std::shared_ptr<CompiledFunc>>&, const int&, const std::size_t&), TLeaf* leaf, std::vector<std::shared_ptr<CompiledFunc>>& cuts, const int& partIdx, const std::size_t& partHash, const bool& isIter = false) : 
            funcWithWp(funcWithWp), leaf(leaf), cuts(cuts), partIdx(isIter ? 0 : partIdx), partHash(partHash), isIter(isIter) {}
        virtual float Get(){
            if(cuts.empty()) return (*func)(leaf, partIdx);
            else return (*funcWithWp)(leaf, cuts, partIdx, partHash);
        }
        virtual void AddCut(bool(*op)(const float&, const float&), const float& comp){this->op = op; this->comp = comp;};
        virtual bool GetPassed(){return op(Get(), comp);};
        virtual void Next(){if(isIter) ++partIdx;}
        virtual void Reset(){if(isIter) partIdx = 0;}
};

using Particles = std::map<std::pair<std::string, std::string>, std::shared_ptr<CompiledFunc>>;

class CompiledCustomFunc : public CompiledFunc {
    private:
        float(*func)(Particles& parts);
        Particles parts;
        bool(*op)(const float&, const float&);
        float comp;

    public:
        CompiledCustomFunc(float(*func)(Particles& parts), const Particles& parts) : func(func), parts(parts) {}
        void AddCut(bool(*op)(const float&, const float&), const float& comp){this->op = op; this->comp = comp;};
        float Get(){return (*func)(parts);}
        bool GetPassed(){return op(Get(), comp);};
        void Next(){for(auto& p : parts) p.second->Next();}
        void Reset(){for(auto& p : parts) p.second->Reset();};
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
        int era;

        std::shared_ptr<CompiledFunc> func;

        bool isCompiled = false;
        inline static int entry = 0; 
        inline static std::map<std::size_t, std::map<int, int>> idxCache = {};

        static float GetEntry(TLeaf* leaf, const int& idx);
        static float GetEntryWithWP(TLeaf* leaf, std::vector<std::shared_ptr<CompiledFunc>>& cuts, const int& idx, const std::size_t& hash);

        std::shared_ptr<CompiledFunc> compileBranchReading(pt::ptree& func, pt::ptree& part);
        std::shared_ptr<CompiledFunc> compileCustomFunction(pt::ptree& func, std::vector<pt::ptree>& parts);

    public:
        NTupleReader();
        NTupleReader(const std::shared_ptr<TTree>& inputTree, const int& era = 2017);

        void AddParticle(const std::string& pAlias, const int& idx, const std::string& wp, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddFunction(const std::string& fInfo, const std::vector<std::string>& values = {}, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddFunctionByBranchName(const std::string& branchName, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddCut(const float&, const std::string& op, const std::experimental::source_location& location = std::experimental::source_location::current());

        void Compile(const std::experimental::source_location& location = std::experimental::source_location::current());

        float Get();
        bool GetPassed();
        std::string GetHistName();
        std::string GetAxisLabel();
        std::string GetCutName();

        void Next();
        void Reset();
        static void SetEntry(const int& entry){NTupleReader::entry = entry; idxCache.clear();}

        //Helper function to get keys of ptree
        static std::vector<std::string> GetInfo(const pt::ptree& node, const bool GetInfo = true){
            std::vector<std::string> keys;

            for(const std::pair<const std::string, pt::ptree>& p : node){
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
