#ifndef CSV_H
#define CSV_H

#include <fstream>
#include <vector>
#include <string_view>
#include <map>
#include <filesystem>
#include <numeric>
#include <experimental/source_location>

#include <ChargedAnalysis/Utility/include/stringutil.h>
#include <ChargedAnalysis/Utility/include/vectorutil.h>

class CSV{
    private:
        std::string mode;
        std::string delim;
        std::fstream file;
        
        std::vector<std::size_t> linePos;
        std::vector<std::size_t> lineSize;
        std::vector<std::string> colNames; 
        
        std::size_t lineMinBuff = 0, lineMaxBuff = 0;
        std::vector<char> buffer;

        std::filebuf* GetBuffer();
        void WriteBuffer(std::filebuf* buffer);
        
    public:
        CSV(const std::string& fileName, const std::string& fileMode, const std::string& delim = ",", const std::experimental::source_location& location = std::experimental::source_location::current());
        CSV(const std::string& fileName, const std::string& fileMode, const std::vector<std::string>& colNames, const std::string& delim = ",", const std::experimental::source_location& location = std::experimental::source_location::current());
        ~CSV(){Close();}

        std::size_t GetNColumns(){return colNames.size();}
        std::size_t GetNRows(){return linePos.size();}

        bool Close(){
            if(file.is_open()){
                file.close();
                return true;
            }

            return false;
        }

        static void Merge(const std::string& outFile, const std::vector<std::string>& fileNames, const std::string& delim = ",", const std::experimental::source_location& location = std::experimental::source_location::current());

        template<typename T = std::string>
        std::vector<T> GetRow(const std::size_t& row, const std::experimental::source_location& location = std::experimental::source_location::current()){
            if(mode != "r" and mode != "rw"){
                throw std::runtime_error(StrUtil::PrettyError(location, "Can not read in '", mode, "' filemode!"));
            }

            if(row >= linePos.size()){
                throw std::runtime_error(StrUtil::PrettyError(location, "Line number '", row, " too large with '", linePos.size() ,"'number of lines in file!"));
            } 

            if(row < lineMinBuff or lineMaxBuff < row or row == 0){
                lineMinBuff = row;
                lineMaxBuff = lineMinBuff + 1000;

                buffer.resize(std::accumulate(lineSize.begin() + lineMinBuff, lineMaxBuff < lineSize.size() ? lineSize.begin() + lineMaxBuff : lineSize.end(),  0));

                file.clear();
                file.seekg(linePos.at(row));
                file.read(buffer.data(), buffer.size());
            }

            std::string_view line(buffer.data() + std::accumulate(lineSize.begin() + lineMinBuff, lineSize.begin() + row, 0), lineSize.at(row) - 1);

            //If more than one column, split by delimeter and return
            if(colNames.size() > 1){
                return StrUtil::Split<T>(std::string(line.data()), delim);
            }

            //If only one column, no need for splitting
            else{
                T out;
                std::stringstream rowStream;
                rowStream << line;
                rowStream >> out;

                return {out};
            }
        }
        
        template<typename T = std::string>
        T Get(const std::size_t& row, const std::size_t& column, const std::experimental::source_location& location = std::experimental::source_location::current()){
            if(column >= colNames.size()){
                throw std::runtime_error(StrUtil::PrettyError(location, "Column number '", column, " too large with '", colNames.size() ,"'number of columns in file!"));
            }

            return GetRow<T>(row, location).at(column);
        }
        
        template<typename T = std::string>
        T Get(const std::size_t& row, const std::string& columnName, const std::experimental::source_location& location = std::experimental::source_location::current()){
            std::size_t column;
        
            //Try to find column number
            try{
                column = VUtil::Find(colNames, columnName).at(0);
            }
            catch(std::out_of_range){
                throw std::runtime_error(StrUtil::PrettyError(location, "Unknown column name '", columnName, "'!"));
            }
            
            //Call get function with row/column number
            return Get<T>(row, column, location);
        }

        template<typename T = std::string>
        std::vector<T> GetColumn(const std::size_t& column, const std::experimental::source_location& location = std::experimental::source_location::current()){
            if(StrUtil::Find(mode, "r").empty()){
                throw std::runtime_error(StrUtil::PrettyError(location, "Can not read in '", mode, "' filemode!"));
            }

            std::vector<T> out;
            
            //Read out all rows and extract column info
            for(int i = 0; i < linePos.size(); ++i){
                out.push_back(Get<T>(i, column, location));
            } 

            return out;
        }

        template<typename T = std::string>
        std::vector<T> GetColumn(const std::string& columnName, const std::experimental::source_location& location = std::experimental::source_location::current()){
            std::size_t column;
        
            //Try to find column number
            try{
                column = VUtil::Find(colNames, columnName).at(0);
            }
            catch(std::out_of_range){
                throw std::runtime_error(StrUtil::PrettyError(location, "Unknown column name '", columnName, "'!"));
            }

            return GetColumn<T>(column, location);
        }

        template <typename... T>
        void WriteRow(T&&... data){
            //Jump to end of file
            file.clear();
            file.seekp(0, std::ios::end);

            //Append data
            std::stringstream stream;

            ((stream << std::forward<T>(data) << delim), ...);
            std::string result = stream.str();
            result.replace(result.size() - 1, 1, "\n");

            file << result;

            //Update file information
            linePos.push_back(linePos.back() + result.size());
        }
};

#endif
