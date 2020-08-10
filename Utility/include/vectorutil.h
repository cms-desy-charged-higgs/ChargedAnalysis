#ifndef VECTORUTIL_H
#define VECTORUTIL_H

#include <vector>
#include <functional>
#include <iostream>

/**
* @brief Utility library to operator on C++ standard library vectors
*/

namespace VUtil{
    /**
    * @brief Concept that checks if Func type is invocable and returs type T
    */
    template <typename Func, typename inType, typename outType>
    concept Invocable = std::is_invocable_r<outType, Func, inType>::value;

    /**
    * @brief Concept that checks object is a std::vector
    */
    template <typename Object, typename T>
    concept Vector = std::is_same<std::vector<T>, Object>::value;

    /**
    * @brief Perform python like map function with help of C++ lambdas.
    *
    * Example:
    * @code
    * std::vector<int> doubled = VUtil::Transform<int>({1, 2}, [&](int i){return 2*i;}); //output {2, 4}
    * @endcode
    *
    * @param vec Input vector of type inType, which will be used for the comprehension
    * @param function Generic callable function, which executables elementwise on the input vector  
    * @return Return vector of type outType
    */
    template <typename outType, typename inType, Invocable<inType, outType> Func>
    std::vector<outType> Transform(const std::vector<inType>& vec, const Func& function){
        std::vector<outType> out;

        for(const inType& component : vec){
            out.push_back(function(component));
        }
    
        return out;
    }

    /**
    * @brief Merge a list of vectors into one vector
    *
    * Example:
    * @code
    * std::vector v1 = {1, 2};
    * std::vector v2 = {3, 4};
    * std::vector<int> merged = VUtil::Merge(v1, v2); //output {1, 2, 3, 4}
    * @endcode
    *
    * @param vec Input vector which should be merged with other vectors
    * @param toMerge Parameter pack of vectors which should be merged with input vector
    * @return Return Merged vector
    */
    template <typename T, Vector<T>... Vectors>
    std::vector<T> Merge(const std::vector<T>& vec, Vectors&&... toMerge){
        std::vector<T> out = vec;
        (out.insert(out.end(), std::forward<Vectors>(toMerge).begin(), std::forward<Vectors>(toMerge).end()), ...);
    
        return out;
    }

    /**
    * @brief Merge values into vector
    *
    * Example:
    * @code
    * std::vector v = {1, 2};
    * std::vector<int> merged = VUtil::Merge(v, 3, 4); //output {1, 2, 3, 4}
    * @endcode
    *
    * @param vec Input vector which should be merged values
    * @param toMerge Parameter pack of values to be merged into the vector
    * @return Return Merged vector
    */
    template <typename T, typename... Args>
    std::vector<T> Merge(const std::vector<T>& vec, Args&&... toMerge){
        std::vector<T> out = vec; 
        (out.push_back(std::forward<Args>(toMerge)), ...);

        return out;
    }

    /**
    * @brief Produce a range of numbers
    *
    * Example:
    * @code
    * std::vector<int> range = VUtil::Range(1, 5, 5); //output {1, 2, 3, 4, 5}
    * @endcode
    *
    * @param start First element of range
    * @param end Last element of range
    * @param steps Number of steps between start and end
    * @return Return ranged vetor
    */
    template <typename T>
    std::vector<T> Range(const T& start, const T& end, const int& steps){
        std::vector<T> out;

        for(int step=0; step < steps; step++){
            out.push_back(start+step*(end-start)/(steps-1));
        }        

        return out;
    }
};

#endif
