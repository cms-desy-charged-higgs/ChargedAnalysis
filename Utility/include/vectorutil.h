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
    template <typename outType, typename inType>
    std::vector<outType> Transform(const std::vector<inType>& vec, const Invocable<inType, outType>& function){
        std::vector<outType> out;

        for(const inType& component : vec){
            out.push_back(function(component));
        }
    
        return out;
    }

    template <typename outType, typename inType>
    std::vector<outType> Transform(const std::initializer_list<inType>&& vec, const Invocable<inType, outType>& function){
        std::vector<inType> out; 
        out.insert(out.end(), std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()));

        return Transform<outType, inType>(out, function);
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
    template <typename T, typename... Vectors>
    std::vector<T> Merge(const std::vector<T>& vec = {}, Vectors&&... toMerge){
        std::vector<T> out = vec;
  
        if(!vec.empty()){
            std::vector<T> merge = Merge<T>(std::forward<Vectors>(toMerge)...);
            out.insert(out.end(), merge.begin(), merge.end());  
        }
    
        return out;
    }

    /**
    * @brief Overload of Merge function with std::initializer_list
    * See Merge() 
    */
    template <typename T, typename... Vectors>
    std::vector<T> Merge(const std::initializer_list<T>&& vec, Vectors&&... toMerge){
        std::vector<T> out; 
        out.insert(out.end(), std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()));

        return Merge<T>(out, std::forward<Vectors>(toMerge)...);
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

        for(int step=0; step <= steps; step++){
            out.push_back(start+step*(end-start)/steps);
        }        

        return out;
    }
};
