/**
* @file stringutil.h
* @brief Header file for StrUtil namespace
*/

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
    concept bool Streamable(){
        return requires(T x, std::stringstream stream) {stream << x;};
    };

    /**
    * @brief Replace substring in a string with another substring
    *
    * Example:
    * @code
    * std::string s = StrUtil::Replace("Bye World", "Bye", "Hello"); //output "Hello World"
    * @endcode
    *
    * @param[in] initial initial string on which the replacement will be performed
    * @param[in] label Substring which will be replaced
    * @param[in] replace Replacement for label, can be any streamable type
    * @param[in] ignoreMissing If false, a missing label in the string will result in std::out_of_range error
    * @return Replaced string
    */
    template <typename T>
    std::string Replace(const std::string& initial, const std::string& label, const T& replace, const bool& ignoreMissing = false){
        std::string result = initial;

        std::stringstream repl;
        repl << replace;

        if(result.find(label) != std::string::npos){
            result.replace(result.find(label), label.size(), repl.str());
        }

        else{
            if(!ignoreMissing) throw std::out_of_range("Did not found '" + label + "' in string '" + result + "'");
        }

        return result;
    }

    /**
    * @brief Merge sequence of streamable objects into one string
    *
    * Example:
    * @code
    * std::string s = StrUtil::Merge("Answer to", "all questions: ", 42); //output "Answer to all questions: 42"
    * @endcode
    *
    * @param[in] mergeObjects Parameter pack with sequence of object which will be merged into one string
    * @return Merged string
    */
    template <int prec = 3, typename... Args>
    std::string Merge(Args&&... mergeObjects){
        std::stringstream result;
        result << std::setprecision(prec);
        (result << ... << std::forward<Args>(mergeObjects));
        return result.str();
    }

    /**
    * @brief Find first occurrence of streamable object in string
    *
    * Example:
    * @code
    * int pos = StrUtil::Find("Where is my purse?", "purse"); //output "12"
    * @endcode
    *
    * @param[in] string String which will be searched
    * @param[in] itemToFind Streamble object which will searched for
    * @return Position of first occurence, if nothing is find -1 is returned
    */
    template <typename T>
    int Find(const std::string& string, const T& itemToFind){
        int position = -1;

        std::stringstream item;
        item << itemToFind;

        if(string.find(item.str()) != std::string::npos){
            position=string.find(item.str());
        }

        return position;
    }
};
