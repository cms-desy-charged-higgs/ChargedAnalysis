#include <ChargedAnalysis/Utility/include/csv.h>

CSV::CSV(const std::string& fileName, const std::string& fileMode, const std::string& delim, const std::experimental::source_location& location) : 
    CSV(fileName, fileMode, {}, delim, location)
    {}

CSV::CSV(const std::string& fileName, const std::string& fileMode, const std::vector<std::string>& colNames, const std::string& delim, const std::experimental::source_location& location):
    mode(fileMode),
    delim(delim),
    colNames(colNames){
    //Translate given file mode into std::fstream file modes
    std::map<std::string, std::ios_base::openmode> modes = {
                {"r", std::ios_base::in | std::ios_base::binary},
                {"w", std::ios_base::out | std::ios_base::binary},
                {"rw", std::ios_base::in | std::ios_base::out},
                {"w+", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary}
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
    if(fileMode == "r" or fileMode == "rw"){
        std::string line;
        
        //Read out header (which is assumed to exist)
        std::getline(file, line);
        if(line.size() == 0) throw std::runtime_error(StrUtil::PrettyError(location, "File is empty: '", fileName, "'!"));

        this->colNames = StrUtil::Split(line, delim);
        if(this->colNames.size() == 1) this->colNames = {line};
        
        linePos.push_back(line.size() + 1);
        
        //Read line lenghts for data in file
        while (std::getline(file, line)){
            linePos.push_back(line.size() + linePos.back() + 1);
            lineSize.push_back(line.size() + 1);
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

std::filebuf* CSV::GetBuffer(){
    //Get buffer without header
    file.clear();
   
    if(linePos.size() > 0){
        file.seekg(linePos.at(0));

        return file.rdbuf();
    }

    else return nullptr;
}

void CSV::WriteBuffer(std::filebuf* buffer){
    if(buffer != nullptr){
        file.clear();
        file.seekp(0, std::ios::end);
        file << buffer;
    }
}

void CSV::Merge(const std::string& outFile, const std::vector<std::string>& inputFiles, const std::string& delim, const std::experimental::source_location& location){
    if(inputFiles.empty()) throw std::runtime_error(StrUtil::PrettyError(location, "Empty list of input files is given!"));

    //Create directory if not already there
    if(StrUtil::Split(outFile, "/").size() > 1){
        std::string dir;

        for(const std::string s : VUtil::Slice(StrUtil::Split(outFile, "/"), 0, -1)){
            dir += s + "/";
        }
    
        if(!std::filesystem::exists(dir)) std::filesystem::create_directories(dir);
    }

    //Copy first file to output and return if only file
    std::filesystem::copy(inputFiles.at(0), outFile, std::filesystem::copy_options::overwrite_existing);
    std::cout << inputFiles.at(0) << std::endl;
    std::cout << outFile << std::endl;

    if(inputFiles.size() == 1) return;

    //Open output file and merge
    CSV out(outFile, "rw", delim);

    for(std::size_t idx = 1; idx < inputFiles.size(); ++idx){
        std::cout << inputFiles.at(idx) << std::endl;

        CSV toMerge(inputFiles.at(idx), "r", delim);
        out.WriteBuffer(toMerge.GetBuffer());
    }

    return;
}
