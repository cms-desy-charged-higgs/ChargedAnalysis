#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <sstream>

class Parser{
    private:
        std::map<std::string, std::vector<std::string>> options;

    public:
        Parser(int argc, char** argv);

        template <typename T>
        T GetValue(const std::string& option);

        template <typename T>
        T GetValue(const std::string& option, const T& defaultValue);

        template <typename T>
        std::vector<T> GetVector(const std::string& option);

        template <typename T>
        std::vector<T> GetVector(const std::string& option, const std::vector<T>& defaultValue);
};

#endif
