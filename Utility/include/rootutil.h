/**
* @file rootutil.h
* @brief Header file for RUtil namespace
*/

#ifndef ROOTUTIL_H
#define ROOTUTIL_H

#include <string>
#include <memory>
#include <experimental/source_location>

#include <ChargedAnalysis/Utility/include/stringutil.h>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TLeaf.h>

/**
* @brief Utility library to handle getter function for ROOT pointer objects
*/

namespace RUtil{
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
    T* Get(TFile* obj, const std::string& getName, const std::experimental::source_location& location = std::experimental::source_location::current()){
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
    * @brief Get data from given TLeaf with given entry number
    *
    * @param leaf TLeaf to be readed out
    * @param entry Entry number
    * @return Return data
    */
    template<typename T>
    const T& GetEntry(TLeaf* leaf, const int& entry){
        if(leaf->GetBranch()->GetReadEntry() != entry) leaf->GetBranch()->GetEntry(entry);
        T* out = static_cast<T*>(leaf->GetValuePointer());

        return *out;
    }

    /**
    * @brief Get vector data from given TLeaf with given entry number
    *
    * @param leaf TLeaf to be readed out
    * @param entry Entry number
    * @return Return vector data
    */
    template<typename T>
    const std::vector<T>& GetVecEntry(TLeaf* leaf, const int& entry){
        if(leaf->GetBranch()->GetReadEntry() != entry) leaf->GetBranch()->GetEntry(entry);
        std::vector<T>* out = static_cast<std::vector<T>*>(leaf->GetValuePointer());

        return *out;
    }
};

#endif
