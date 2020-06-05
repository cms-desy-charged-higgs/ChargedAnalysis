#include <ChargedAnalysis/Utility/include/parser.h>
#include <typeinfo>

Parser::Parser(int argc, char** argv){
    int counter=1;

    while(counter != argc){
        std::size_t pos = std::string(argv[counter]).find("--");

        //Variables for saving option and values 
        std::string option = std::string(argv[counter]).substr(2);
        std::vector<std::string> values;

        counter++;

        //Break if last statement is bool option like --help
        if(counter == argc){
            options[option] = values;
            break;
        }

        //Catch all values one option
        while(std::string(argv[counter]).find("--") == std::string::npos){
            values.push_back(std::string(argv[counter]));

            counter++;

            //Break if this is last option
            if(counter == argc) break;
        }

        //Save option and values in a map
        options[option] = values;
    }
}

template <typename T> 
T Parser::GetValue(const std::string& option, const T& defaultValue){   
    try{
        return GetValue<T>(option);
    }
    
    catch(...){
        return defaultValue;
    }
}

template int Parser::GetValue(const std::string&, const int&);
template float Parser::GetValue(const std::string&, const float&);
template double Parser::GetValue(const std::string&, const double&);
template std::string Parser::GetValue(const std::string&, const std::string&);


template <typename T> 
T Parser::GetValue(const std::string& option){
    T value;
    std::string type = typeid(value).name();
    std::string data;

    if(options.find(option) != options.end()){
        if(options[option].size() == 0) data = "1";
        else data = options[option][0];
    }

    else{
        if(type == "b"){
            data = "0";
        }

        else throw std::invalid_argument("No parser argument: " + option);
    }

    std::istringstream iss(data);
    iss >> value;

    return value;
}

template bool Parser::GetValue(const std::string&);
template int Parser::GetValue(const std::string&);
template float Parser::GetValue(const std::string&);
template double Parser::GetValue(const std::string&);
template std::string Parser::GetValue(const std::string&);

template <typename T> 
std::vector<T> Parser::GetVector(const std::string& option){
    std::vector<T> values;

    if(options.find(option) == options.end()){
        throw std::invalid_argument("No parser argument: " + option);
    }

    for(const std::string& data: options[option]){
        T value;
        std::istringstream iss(data);
        iss >> value;
        values.push_back(value);
    }

    return values;
}

template std::vector<int> Parser::GetVector(const std::string&);
template std::vector<float> Parser::GetVector(const std::string&);
template std::vector<double> Parser::GetVector(const std::string&);
template std::vector<std::string> Parser::GetVector(const std::string&);

template <typename T> 
std::vector<T> Parser::GetVector(const std::string& option, const std::vector<T>& defaultValue){
    try{
        return GetVector<T>(option);
    }
    
    catch(...){
        return defaultValue;
    }
}

template std::vector<int> Parser::GetVector(const std::string&, const std::vector<int>&);
template std::vector<float> Parser::GetVector(const std::string&, const std::vector<float>&);
template std::vector<double> Parser::GetVector(const std::string&, const std::vector<double>&);
template std::vector<std::string> Parser::GetVector(const std::string&, const std::vector<std::string>&);
