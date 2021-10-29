/**
* @file treeappender.h
* @brief Header file for TreeAppender class
*/

#ifndef TREEAPPENDER_H
#define TREEAPPENDER_H

#include <string>
#include <vector>
#include <map>
#include <experimental/source_location>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>

#include <ChargedAnalysis/Utility/include/parser.h>
#include <ChargedAnalysis/Utility/include/extension.h>
#include <ChargedAnalysis/Utility/include/rootutil.h>

/**
* @brief Class for appending quantities on existing ROOT TTrees 
*/

class TreeAppender{
    private:
        std::string fileName, treeName; 
        int era;
        std::vector<std::string> appendFunctions;
        
    public:
        /**
        * @brief Default constructor
        */
        TreeAppender();

        /**
        * @brief Constructor
        * @param fileName File name of input ROOT file
        * @param treeName Name of TTree where a quantity should be appended
        * @param appendFunctions Vector of strings with name of function from Extension namespace, which will be used for the append
        */
        TreeAppender(const std::string& fileName, const std::string& treeName, const int& era, const std::vector<std::string>& appendFunctions);

        /**
        * @brief Function which will execute the appending
        * @param outName Name of ROOT file which contains appended TTree
        */
        void Append(const std::string& outName, const int& entryStart, const int& entryEnd, Parser& parser);
};

#endif
