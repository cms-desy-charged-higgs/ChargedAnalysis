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
#include <TMath.h>
#include <Math/Vector4D.h>

#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

class CompiledFunc{
    private:
        //Function signatures for different possibilities
        std::function<float(const int&, const int&)> func;

        int partIdx;
        bool isIter;

        //Boolean
        std::function<bool(const float&, const float&)> op;
        float comp;

    public:
        CompiledFunc(){}
        CompiledFunc(std::function<float(const int&, const int&)>& func, const int& partIdx = -1, const bool& isIter = false) : 
            func(func), partIdx(isIter ? 0 : partIdx), isIter(isIter) {}

        void AddCut(std::function<bool(const float&, const float&)> op, const float& comp){this->op = op; this->comp = comp;};

        float Get(const int& entry){return func(entry, partIdx);}
        bool GetPassed(const int& entry){return op(Get(entry), comp);};

        void Next(){if(isIter) ++partIdx;}
        void Reset(){if(isIter) partIdx = 0;}
};

namespace Properties{
    float HT(const int& entry, std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs);
    float dR(const int& entry, std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs);
    float dPhi(const int& entry, std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs);
    float diCharge(const int& entry, std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs);
    float NParticles(const int& entry, std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs);
    float LP(const int& entry, std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs);
    float diMass(const int& entry, std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs);
    float isGenMatched(const int& entry, std::map<std::string, std::shared_ptr<CompiledFunc>>& funcs);
};

namespace pt = boost::property_tree;

using NCache = std::map<std::size_t, std::map<int, int>>;

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
        std::shared_ptr<NCache> cache;

        static float GetEntry(TLeaf* leaf, const int& entry, const int& idx);
        static float GetEntryWithWP(TLeaf* leaf, std::vector<std::shared_ptr<CompiledFunc>>& cuts, const std::size_t& hash, const std::shared_ptr<NCache>& cache, const int& entry, const int& idx);

        std::shared_ptr<CompiledFunc> compileBranchReading(pt::ptree& func, pt::ptree& part);
        std::shared_ptr<CompiledFunc> compileCustomFunction(pt::ptree& func, std::vector<pt::ptree>& parts);

    public:
        NTupleReader();
        NTupleReader(const std::shared_ptr<TTree>& inputTree, const int& era = 2017, const std::shared_ptr<NCache>& cache = nullptr);

        void AddParticle(const std::string& pAlias, const int& idx, const std::string& wp, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddFunction(const std::string& fInfo, const std::vector<std::string>& values = {}, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddFunctionByBranchName(const std::string& branchName, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddCut(const float&, const std::string& op, const std::experimental::source_location& location = std::experimental::source_location::current());

        void Compile(const std::experimental::source_location& location = std::experimental::source_location::current());

        float Get(const int& entry);
        bool GetPassed(const int& entry);

        std::string GetHistName();
        std::string GetAxisLabel();
        std::string GetCutName();

        void Next();
        void Reset();

        //Helper function to get keys of ptree
        static std::vector<std::string> GetInfo(const pt::ptree& node, const bool& GetInfo = true){
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

        template <typename T>
        static void PutVector(pt::ptree& tree, const std::string& name, const std::vector<T>& vector){
            pt::ptree children;
            
            for(const T& v : vector){
                pt::ptree child;
                child.put("", v);
                children.push_back(std::make_pair("", child));
            }

            tree.add_child(name, children);
        }
};

#endif
