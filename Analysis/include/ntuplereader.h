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

namespace pt = boost::property_tree;

using Func = std::function<float(const int&, const int&)>;
using Cut = std::function<bool(const float&)>;

class NTupleReader;

class NTupleFunction {
    friend class NTupleReader;

    private:
        pt::ptree function;
        std::vector<pt::ptree> particles;
        std::size_t mainIdx;

        bool isCompiled = false, isValid = true;
        Func func;
        Cut cut;

        NTupleReader* reader;
        NTupleFunction(NTupleReader* reader) : reader(reader) {}

        Func compileBranchReading(const pt::ptree& part);
        Func compileCustomFunction(const std::vector<pt::ptree>& parts);

    public:
        NTupleFunction() = delete;

        void AddParticle(const std::string& pAlias, const std::size_t& idx, const std::string& wp, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddFunction(const std::string& fInfo, const std::vector<std::string>& values = {}, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddFunctionByBranchName(const std::string& branchName, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddCut(const float& value, const std::string& op, const std::experimental::source_location& location = std::experimental::source_location::current());
        void AddCut(const std::vector<float>& values, const std::string& op, const std::experimental::source_location& location = std::experimental::source_location::current());

        float Get();
        float Get(const std::size_t& idx);
        bool GetPassed();
        bool GetPassed(const std::size_t& idx);

        std::string GetHistName();
        std::string GetAxisLabel();
        std::string GetCutName();

        void Compile(const std::experimental::source_location& location = std::experimental::source_location::current());
};

class NTupleReader {
    friend class NTupleFunction;

    private:
        pt::ptree partInfo;
        pt::ptree funcInfo;

        TTree* inputTree;
        std::string chanPrefix;
        std::size_t era;

        std::size_t entry;
        std::map<std::pair<std::size_t, std::size_t>, std::size_t> idxCache;

        std::map<std::size_t, TLeaf*> partSize;
        std::map<std::size_t, std::vector<std::pair<Cut, Func>>> partRequirements;

        Cut CreateCut(const std::string& op, const float& compV);
        Cut CreateCut(const std::string& op, const std::vector<float>& compV);
        Func CreateFunc(const std::string& branchName, const pt::ptree& part);
        Func CreateCustomFunc(const std::string& customFunc, const std::vector<pt::ptree>& parts);

        std::size_t GetWPIndex(const std::size_t& entry, const std::size_t& partHash, const std::size_t& idx);

        pt::ptree SetPartWP(const pt::ptree& part, const std::string& wp, const std::size_t& idx);
        void RegisterParticle(const pt::ptree& part);

    public:
        NTupleReader(){}
        NTupleReader(const std::shared_ptr<TTree>& inputTree, const std::size_t& era = 2017) : 
                    inputTree(inputTree.get()), 
                    era(era), 
                    chanPrefix(!StrUtil::Find(inputTree->GetName(), "Ele").empty() ? "Ele" : "Muon"){

                pt::json_parser::read_json(StrUtil::Merge(std::getenv("CHDIR"), "/ChargedAnalysis/Analysis/data/particle.json"), partInfo);
                pt::json_parser::read_json(StrUtil::Merge(std::getenv("CHDIR"), "/ChargedAnalysis/Analysis/data/function.json"), funcInfo);
        }

        NTupleFunction BuildFunc(){return NTupleFunction(this);}
        void SetEntry(const std::size_t& entry){
            this->entry = entry;
            idxCache.clear();
        }

        float NParticles(const std::size_t& entry, const std::size_t& pHash, TLeaf* pSize);
        float HT(const std::size_t& entry, const std::size_t& jetHash, TLeaf* jetPt);
        float diCharge(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* ch1, TLeaf* ch2);
        float dPhi(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* phi1, TLeaf* phi2);
        float dR(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* phi1, TLeaf* phi2, TLeaf* eta1, TLeaf* eta2);
        float diMass(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* pt1, TLeaf* pt2, TLeaf* phi1, TLeaf* phi2, TLeaf* eta1, TLeaf* eta2);
        float ModifiedEntry(const int& entry, TLeaf* evNr);
        float isGenMatched(const int& entry, const std::size_t& p1Idx, const std::size_t& p1Hash, TLeaf* gID);
        float diMT(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, TLeaf* pt1, TLeaf* pt2, TLeaf* phi1, TLeaf* phi2);
        float dPhi3(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p3Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, const std::size_t& p3Hash, TLeaf* phi1, TLeaf* pt2, TLeaf* phi2, TLeaf* eta2, TLeaf* pt3, TLeaf* phi3, TLeaf* eta3);
        float dR3(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p2Idx, const std::size_t& p3Idx, const std::size_t& p1Hash, const std::size_t& p2Hash, const std::size_t& p3Hash, TLeaf* phi1, TLeaf* eta1, TLeaf* pt2, TLeaf* phi2, TLeaf* eta2, TLeaf* pt3, TLeaf* phi3, TLeaf* eta3);
        float LP(const std::size_t& entry, const std::size_t& p1Idx, const std::size_t& p1Hash, TLeaf* pt1, TLeaf* ptMet, TLeaf* phi1, TLeaf* phiMet);

        //Helper function to get keys of ptree
        static std::vector<std::string> GetKeys(const pt::ptree& node){
            std::vector<std::string> keys;

            for(const std::pair<const std::string, pt::ptree>& p : node){
                keys.push_back(p.first);
            }

            return keys;
        }

        template <typename T = std::string>
        static std::vector<T> GetVector(const pt::ptree& node){
            std::vector<T> values;

            for(const std::pair<const std::string, pt::ptree>& p : node){
                values.push_back(p.second.get_value<T>());
            }

            return values;
        }

        //Helper function to get name of child tree with wished alias in it
        static std::string GetName(const pt::ptree& node, const std::string& alias){
           for(const std::string& name : NTupleReader::GetKeys(node)){
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
