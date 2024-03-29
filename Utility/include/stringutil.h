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
#include <experimental/source_location>

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
    * std::vector<int> pos = StrUtil::Find("Where is my purse?", "purse"); //output "{12}"
    * @endcode
    *
    * @param string String which will be searched
    * @param itemToFind Streamble object which will searched for
    * @return Vector with positions of all occurences, empty if not occurence
    */
    template <Streamable T>
    std::vector<int> Find(const std::string& string, const T& itemToFind){
        std::vector<int> position;

        std::stringstream item;
        item << itemToFind;

        std::size_t pos = string.find(item.str());
        
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
    * @brief Give a nice pretty printed error message string
    *
    * Example:
    * @code
    * <std::string> m = StrUtil::Pretty(location, "You did", " something wrong"!);
    * @endcode
    *
    * @param location Standard library object containing the file positions
    * @param errorMessage mergeObjects Error message string pieces
    * @return Nice formatted string
    */
    template <Streamable... Args>
    std::string PrettyError(const std::experimental::source_location& location, Args&&... errorMessage){
        std::string message = Merge("\n    \033[1mWHERE\033[0m: File '", location.file_name(), "' in fuction '", location.function_name(), "' in line '", location.line(), "'\n");

        message = Merge(message, "    ", "\033[1;31mERROR\033[0m: ", errorMessage...);
        return message;
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

        if(occurences.empty()) return initial;
        if(occurences.size() < (... +  (sizeof(replace)/sizeof(replace)))) return initial;

        std::vector<std::string> toReplace;
        (toReplace.push_back(static_cast<std::ostringstream&&>(std::ostringstream() << replace).str()), ...);

        for(int i = 0, shift = 0; i < toReplace.size(); i++){
            result.replace(occurences[i]+shift, label.size(), toReplace[i]);
            shift+=toReplace[i].size()-label.size();
        }

        return result;
    }

    /**
    * @brief Split string by delimiter
    *
    * Example:
    * @code
    * std::vector<std::string> s = StrUtil::Split("Hello World !", " "); //output {"Hello", "World", "!"}
    * @endcode
    *
    * @param toSplit String to split
    * @param label Delimiter string to search for
    * @return Vector with splitted string
    */
    template <typename T = std::string>
    std::vector<T> Split(const std::string& toSplit, const std::string& label){
        std::vector<int> positions = Find(toSplit, label);
        std::vector<T> splittedString(positions.size() + 1, T());      

        if(positions.size() == 0) return splittedString;

        for(std::size_t pos = 0; pos < positions.size() + 1; ++pos){
            T split;
            std::stringstream splitStr;

            if(pos == 0){
                splitStr << toSplit.substr(0, positions[pos]);
                splitStr >> split;
                splittedString[pos] = split;
            }
            
            else if(pos == positions.size()){
                splitStr << toSplit.substr(positions.back() + 1);
                splitStr >> split;
                splittedString[pos] = split;    
            }
            
            else{
                splitStr << toSplit.substr(positions[pos - 1] + 1, positions[pos] - positions[pos - 1] - 1);
                splitStr >> split;
                splittedString[pos] = split;    
            }
        }
        
        return splittedString;
    }

    /**
    * @brief Merge string with delimiter, python like string join function
    *
    * Example:
    * @code
    * <std::string> s = StrUtil::Join(" ", "Hello", "World", "!"); //output "Hello World !"
    * @endcode
    *
    * @param delimeter Delimeter string which is used between each sstring
    * @param mergeObjects String to be merged
    * @return Merged string
    */
    template <Streamable... Args>
    std::string Join(const std::string& delimeter, Args&&... mergeObjects){
        std::stringstream result;

        ((result << std::forward<Args>(mergeObjects) << delimeter), ...);
        return result.str().substr(0, result.str().size() - 1);
    }

    /**
    * @brief Capitilize given string
    *
    * Example:
    * @code
    * <std::string> s = StrUtil::Capatilize("hello World!"); //output "Hello World !"
    * @endcode
    *
    * @param input Input string
    * @return Capitilized  string
    */

    template <Streamable T = std::string>
    std::string Capitilize(const T& input){
        std::string result = input;
        result[0] = std::toupper(result[0]);

        return result;
    }
};

/**
* @brief Overload of stream operator for vector
*/

template<StrUtil::Streamable T>
std::ostream& operator<<(std::ostream& os, const std::vector<T> vec)
{
    for(int i; i < vec.size(); i++){
        if(i == 0) os << vec[i];
        else os << " " << vec[i];
    }

    return os;
}

#endif
