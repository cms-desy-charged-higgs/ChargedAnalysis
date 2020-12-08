#include <ChargedAnalysis/Utility/include/csv.h>

CSV::CSV(const std::string& fileName, const std::string& fileMode, const std::string& delim, const std::experimental::source_location& location) : 
    CSV(fileName, fileMode, {}, delim, location)
    {}

CSV::CSV(const std::string& fileName, const std::string& fileMode, const std::vector<std::string>& colNames, const std::string& delim, const std::experimental::source_location& location):
    mode(fileMode),
    delim(delim),
    colNames(colNames) {
    
    //Translate given file mode into std::fstream file modes
    std::map<std::string, std::ios_base::openmode> modes = {
                {"r", std::ios_base::in},
                {"w", std::ios_base::out},
                {"rw", std::ios_base::in | std::ios_base::out | std::ios_base::app},
                {"w+", std::ios_base::out | std::ios_base::trunc}
    };

    if(!modes.count(fileMode)){
        throw std::runtime_error(StrUtil::PrettyError(location, "Invalid file mode: '", fileMode, "'!"));
    }

    //Create directory if not already there
    if(StrUtil::Split(fileName, "/").size() > 1){
        std::string dir;

        for(const std::string s : VUtil::Slice(StrUtil::Split(fileName, "/"), 0, -1)){
            dir += s + "/";
        }
    
        if(!std::filesystem::exists(dir)) std::filesystem::create_directories(dir);
    }

    //Open file
    file.open(fileName, modes[fileMode]);
    
    if(!file.is_open()){
        throw std::runtime_error(StrUtil::PrettyError(location, "Could not open file: '", fileName, "'!"));
    }
    
    //Save in read mode for each line length of line for later read out
    if(!StrUtil::Find(fileMode, "r").empty()){
        std::string line;
        
        //Read out header (which is assumed to exist)
        std::getline(file, line);
        this->colNames = StrUtil::Split(line, delim);
        
        linePos.push_back(line.size() + 1);
        
        //Read line lenghts for data in file
        while (std::getline(file, line)){
            linePos.push_back(line.size() + linePos.back() + 1);
        }
        
        linePos.pop_back();
    }
    
    //Write header in write mode
    if(fileMode == "w" or fileMode == "w+"){
        file.clear();

        if(colNames.empty()){
            throw std::runtime_error(StrUtil::PrettyError(location, "No column names for write mode are given!"));
        }
        
        std::size_t lineSize = 0;

        for(const std::string name : VUtil::Slice(colNames, 0, -1)){
            file << name << delim;
            lineSize += name.size() + delim.size();
        }
        
        file << colNames.back() << std::endl;
        linePos.push_back(lineSize + colNames.back().size() + 1);
    }
}
