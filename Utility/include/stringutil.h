/**
* @file stringutil.h
* @brief Header file for StrUtil namespace
*/

#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <iostream>
#include <string>
#include <sstream>
#include <functional>
#include <iomanip>

/**
* @brief Utility library to operator on C++ standard library strings
*/

namespace StrUtil{
    /**
    * @brief Concept that check if object is streamable
    */
    template <typename T>
    concept Streamable = requires(T x, std::stringstream stream) {stream << x;};

    /**
    * @brief Find first occurrence of streamable object in string
    *
    * Example:
    * @code
    * int pos = StrUtil::Find("Where is my purse?", "purse"); //output "12"
    * @endcode
    *
    * @param string String which will be searched
    * @param itemToFind Streamble object which will searched for
    * @return Position of first occurence, if nothing is find -1 is returned
    */
    template <Streamable T>
    std::vector<int> Find(const std::string& string, const T& itemToFind){
        std::vector<int> position;

        std::stringstream item;
        item << itemToFind;

        int pos = string.find(item.str());
        
        while(pos != std::string::npos){   
            position.push_back(pos);
            pos = string.find(item.str(), pos+1);
        }

        return position;
    }

    /**
    * @brief Merge sequence of streamable objects into one string
    *
    * Example:
    * @code
    * std::string s = StrUtil::Merge("Answer to", "all questions: ", 42); //output "Answer to all questions: 42"
    * @endcode
    *
    * @param mergeObjects Parameter pack with sequence of object which will be merged into one string
    * @return Merged string
    */
    template <int prec = 3, Streamable... Args>
    std::string Merge(Args&&... mergeObjects){
        std::stringstream result;
        result << std::setprecision(prec);
        (result << ... << std::forward<Args>(mergeObjects));
        return result.str();
    }

    /**
    * @brief Replace substring in a string with another substring
    *
    * Example:
    * @code
    * std::string s = StrUtil::Replace("Bye World", "Bye", "Hello"); //output "Hello World"
    * @endcode
    *
    * @param initial initial string on which the replacement will be performed
    * @param label Substring which will be replaced
    * @param replace Replacement for label, can be any streamable type
    * @param ignoreMissing If false, a missing label in the string will result in std::out_of_range error
    * @return Replaced string
    */
    template <Streamable... Args>
    std::string Replace(const std::string& initial, const std::string& label, Args&&... replace){
        std::string result = initial;

        std::vector<int> occurences = Find(initial, label);

        if(occurences.empty()) throw std::out_of_range(Merge("Did not found '", label, "' in string '", result, "'"));
        if(occurences.size() < (... +  (sizeof(replace)/sizeof(replace)))) throw std::out_of_range(Merge("String '", initial, "' has less occurences of label '", label, "' than items given for replacement"));


        std::vector<std::string> toReplace;
        (toReplace.push_back(static_cast<std::ostringstream&&>(std::ostringstream() << replace).str()), ...);

        for(int i = 0, shift = 0; i < toReplace.size(); i++){
            result.replace(occurences[i]+shift, label.size(), toReplace[i]);
            shift+=toReplace[i].size()-label.size();
        }

        return result;
    }
};

#endif
