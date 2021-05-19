#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <experimental/source_location>

#include <ChargedAnalysis/Utility/include/stringutil.h>

class Parser{
    private:
        std::map<std::string, std::vector<std::string>> options;

    public:
        Parser(int argc, char** argv);

        template <typename T = std::string>
        T GetValue(const std::string& option, const std::experimental::source_location& location = std::experimental::source_location::current()){
            T value;
            std::string type = typeid(value).name();
            std::string data;

            if(options.find(option) != options.end()){
                if(options[option].size() == 0) data = type == "b" ? "1" : "";
                else data = options[option][0];
            }

            else{
                if(type == "b"){
                    data = "0";
                }

                else throw std::runtime_error(StrUtil::PrettyError(location, "No parser argument: ", option));
            }

            std::istringstream iss(data);
            iss >> value;

            return value;
        }

        template <typename T = std::string, typename U>
        T GetValue(const std::string& option, const U& defaultValue, const std::experimental::source_location& location = std::experimental::source_location::current()){
            try{
                return GetValue<T>(option);
            }
            
            catch(...){
                return defaultValue;
            }
        }

        template <typename T = std::string>
        std::vector<T> GetVector(const std::string& option, const std::experimental::source_location& location = std::experimental::source_location::current()){
            std::vector<T> values;

            if(options.find(option) == options.end()){
                throw std::runtime_error(StrUtil::PrettyError(location, "No parser argument: ", option));
            }

            for(const std::string& data: options[option]){
                T value;
                std::istringstream iss(data);
                iss >> value;
                values.push_back(value);
            }

            return values;
        }

        template <typename T = std::string, typename U>
        std::vector<T> GetVector(const std::string& option, const std::initializer_list<U>& defaultValue, const std::experimental::source_location& location = std::experimental::source_location::current()){
            try{
                return GetVector<T>(option);
            }
            
            catch(...){
                std::vector<T> defaultV;
                for(const U& v : defaultValue) defaultV.push_back(static_cast<T>(v));

                return defaultV;
            }
        }
};

#endif
