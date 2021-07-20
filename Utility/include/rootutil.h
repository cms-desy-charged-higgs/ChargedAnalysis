/**
* @file rootutil.h
* @brief Header file for RUtil namespace
*/

#ifndef ROOTUTIL_H
#define ROOTUTIL_H

#include <string>
#include <memory>
#include <vector>
#include <experimental/source_location>

#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>

#include <TFile.h>
#include <TDirectory.h>
#include <THStack.h>
#include <TTree.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <TH2.h>

/**
* @brief Utility library to handle getter function for ROOT pointer objects
*/

namespace RUtil{
    template <typename H>
    concept Hist2D = std::is_base_of<TH2, H>::value;

    /**
    * @brief Open ROOT TFile with exception handling in READ mode
    *
    * Example:
    * @code
    * std::shared_ptr<TFile> f = RUtil::Open("myFile.root");
    * @endcode
    *
    * @param fileName ROOT file name
    * @param location Standard library object containing the file positions
    * @return Shared ptr TFile opened in READ mode
    */

    std::shared_ptr<TFile> Open(const std::string& fileName, const std::experimental::source_location& location = std::experimental::source_location::current());

    bool BranchExists(TTree* tree, const std::string& branchName, const std::experimental::source_location& location = std::experimental::source_location::current());
    
    std::vector<std::string> ListOfContent(TDirectory* f, const std::experimental::source_location& location = std::experimental::source_location::current());

    /**
    * @brief Get object from TFile with exception handling
    *
    * Example:
    * @code
    * TH1F* h = RUtil::Get<TH1F>(myFile, "myHist");
    * @endcode
    *
    * @param obj Opened readable TFile
    * @param getName Object name to retrieve
    * @param location Standard library object containing the file positions
    * @return Pointer of the object
    */
    template<typename T>
    T* Get(TDirectory* obj, const std::string& getName, const std::experimental::source_location& location = std::experimental::source_location::current()){
        //Check if objects is not null pointer
        if(obj == nullptr) throw std::runtime_error(StrUtil::PrettyError(location, "TFile is a null pointer!"));

        T* out = static_cast<T*>(obj->Get(getName.c_str()));

        if(out == nullptr) throw std::runtime_error(StrUtil::PrettyError(location, "Object '", getName, "' in TFile '", obj->GetName(), "' is a null pointer!"));

        return out;
    }

    /**
    * @brief Get TLeaf from TTree with exception handling
    *
    * Example:
    * @code
    * TLeaf* h = RUtil::Get<TLeaf>(tree, "myLeaf");
    * @endcode
    *
    * @param obj TTree with the wished leaf
    * @param getName TLeaf name to retrieve
    * @param location Standard library object containing the file positions
    * @return Pointer of the TLeaf
    */
    template<typename T>
    T* Get(TTree* obj, const std::string& getName, const std::experimental::source_location& location = std::experimental::source_location::current()){
        //Check if objects is not null pointer
        if(obj == nullptr){
            throw std::runtime_error(StrUtil::PrettyError(location, "Null pointer is given!"));
        }

        T* out = obj->GetLeaf(getName.c_str());
 
        if(out == nullptr) throw std::runtime_error(StrUtil::PrettyError(location,  "Object '", getName, "' is a null pointer!"));

        return out;
    }

    /**
    * @brief Wrapper of the RUtil::Get function to return shared pointer
    *
    * @param obj TTree with the wished leaf
    * @param getName TLeaf name to retrieve
    * @param location Standard library object containing the file positions
    * @return Return shared pointer of wished pointer
    */
    template<typename outType, typename inType>
    std::shared_ptr<outType> GetSmart(inType* obj, const std::string& getName, const std::experimental::source_location& location = std::experimental::source_location::current()){
        return std::shared_ptr<outType>(RUtil::Get<outType>(obj, getName, location));
    }

    /**
    * @brief Clone ROOT object with exception handling
    *
    * Example:
    * @code
    * TH1F* h2 = RUtil::Clone<TH1F>(h1);
    * @endcode
    *
    * @param obj Object to clone
    * @param location Standard library object containing the file positions
    * @return Return shared pointer of wished pointer
    */
    template<typename T>
    T* Clone(T* obj, const std::experimental::source_location& location = std::experimental::source_location::current()){
        //Check if objects is not null pointer
        if(obj == nullptr){
            throw std::runtime_error(StrUtil::PrettyError(location, "Null pointer is given!"));
        }

        return static_cast<T*>(obj->Clone());
    }
    

    /**
    * @brief Wrapper of the RUtil::Clone function to return shared pointer
    *
    * @param obj Object to clone
    * @param location Standard library object containing the file positions
    * @return Return shared pointer of wished pointer
    */
    template<typename T>
    std::shared_ptr<T> CloneSmart(T* obj, const std::experimental::source_location& location = std::experimental::source_location::current()){
        return std::shared_ptr<T>(RUtil::Clone<T>(obj, location));
    }

    /**
    * @brief Wrapper of the RUtil::Clone function to return shared pointer
    *
    * @param obj Object to clone
    * @param location Standard library object containing the file positions
    * @return Return shared pointer of wished pointer
    */
    template<typename T>
    T* StackAt(THStack* stack, const int& idx, const std::experimental::source_location& location = std::experimental::source_location::current()){
        //Check if objects is not null pointer
        if(stack == nullptr){
            throw std::runtime_error(StrUtil::PrettyError(location, "Null pointer is given!"));
        }

        //Check if out of bounce
        if(idx >= stack->GetNhists()){
            throw std::runtime_error(StrUtil::PrettyError(location, "Given index '", idx, "' is bigger than number of histograms '", stack->GetNhists(), "' in the stack!"));
        }
    
        return static_cast<T*>(stack->GetStack()->At(idx));
    }
    
    /**
    * @brief Get data from given TLeaf with given entry number
    *
    * @param leaf TLeaf to be readed out
    * @param entry Entry number
    * @return Return data
    */
    template<typename T>
    const T GetEntry(TLeaf* leaf, const int& entry){
        if(leaf->GetBranch()->GetReadEntry() != entry) leaf->GetBranch()->GetEntry(entry);
        return leaf->GetValue();
    }

    /**
    * @brief Get vector data from given TLeaf with given entry number
    *
    * @param leaf TLeaf to be readed out
    * @param entry Entry number
    * @return Return vector data
    */
    template<typename T>
    const std::vector<T> GetVecEntry(TLeaf* leaf, const int& entry){
        if(leaf->GetBranch()->GetReadEntry() != entry) leaf->GetBranch()->GetEntry(entry);
        std::vector<T> v(leaf->GetLen());

        for(int i = 0; i < v.size(); ++i){
            v[i] = leaf->GetValue(i);
        }

        return v;
    }

    template <Hist2D H> 
    std::shared_ptr<H> Rebin2D(const std::shared_ptr<H>& hist, const std::vector<double>& xBins, const std::vector<double>& yBins){
        //Get current binning
        std::vector<double> currentX(hist->GetNbinsX() + 1, 0.), currentY(hist->GetNbinsY() + 1, 0.);

        for(unsigned int i = 1; i <= hist->GetNbinsX() + 1; ++i){
            currentX[i - 1] = hist->GetXaxis()->GetBinLowEdge(i);
        }

        for(unsigned int i = 1; i <= hist->GetNbinsY() + 1; ++i){
            currentY[i - 1] = hist->GetYaxis()->GetBinLowEdge(i);
        }

        if(xBins.size() > currentX.size()) throw std::runtime_error("New x-binning has more bins than current binning!");
        if(yBins.size() > currentY.size()) throw std::runtime_error("New y-binning has more bins than current binning!");

        //Create new hist
        std::shared_ptr<H> newHist = std::make_shared<H>("", "", xBins.size() - 1, xBins.data(), yBins.size() - 1, yBins.data());

        //Intermediate hist
        std::shared_ptr<H> xBinned = std::make_shared<H>("x", "x", xBins.size() - 1, xBins.data(), currentY.size() - 1, currentY.data());

        //Rebin x-axis first
        for(unsigned int y = 1; y < currentY.size(); ++y){
            std::shared_ptr<TH1D> projX(static_cast<TH1D*>(hist->ProjectionX("pr", y, y)->Rebin(xBins.size() - 1, "re", xBins.data())));

            for(unsigned int x = 1; x < xBins.size(); ++x){
                xBinned->SetBinContent(x, y, projX->GetBinContent(x));
                xBinned->SetBinError(x, y, projX->GetBinError(x));
            }
        }

        //Rebin y-axis second
        for(unsigned int x = 1; x < xBins.size(); ++x){
            std::shared_ptr<TH1D> projY(static_cast<TH1D*>(xBinned->ProjectionY("pr", x, x)->Rebin(yBins.size() - 1, "re", yBins.data())));

            for(unsigned int y = 1; y < yBins.size(); ++y){
                newHist->SetBinContent(x, y, projY->GetBinContent(y));
                newHist->SetBinError(x, y, projY->GetBinError(y));
            }
        }

        newHist->SetName(hist->GetName());
        newHist->SetTitle(hist->GetTitle());

        return newHist;
    }
};

#endif
