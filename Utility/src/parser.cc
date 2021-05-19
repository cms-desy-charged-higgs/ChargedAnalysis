#include <ChargedAnalysis/Utility/include/parser.h>

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
