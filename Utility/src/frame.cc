#include <ChargedAnalysis/Utility/include/frame.h>

Frame::Frame(){
}

Frame::Frame(const std::string& inFile){
    //Read file
    std::ifstream myFile(inFile, std::ifstream::in);
    std::string line;

    if(!myFile.is_open()){
        throw std::runtime_error("File can not be opened: " + inFile);
    }

    std::cout << "Read file: " << inFile << std::endl;

    //Get header
    std::getline(myFile, line);
    labels = Utils::SplitString<std::string>(line, "\t");

    //Read other columns
    while(std::getline(myFile, line)){
        std::vector<float> column = Utils::SplitString<float>(line, "\t");
        this->AddColumn(column);
    }
    
    myFile.close();
}

Frame::Frame(const std::vector<std::string>& inFiles){
    for(std::vector<std::string>::const_iterator file = inFiles.begin(); file != inFiles.end(); ++file){
        //Read file
        std::ifstream myFile(*file, std::ifstream::in);
        std::string line;

        if(!myFile.is_open()){
            throw std::runtime_error("File can not be opened: " + *file);
        }

        //Get header
        std::getline(myFile, line);
        if(file == inFiles.begin()){
            labels = Utils::SplitString<std::string>(line, "\t");
        }

        //Read other columns
        while(std::getline(myFile, line)){
            std::vector<float> column = Utils::SplitString<float>(line, "\t");
            this->AddColumn(column);
        }
        
        myFile.close();
    }
}

void Frame::InitLabels(const std::vector<std::string>& initLabels){
    this->labels = initLabels;  
}

int Frame::GetNLabels(){return labels.size();}

bool Frame::AddColumn(const std::vector<float>& column){
    if(column.size() != labels.size()){
        std::cout << "Length of given column (" << column.size() << ") does not match number of avaiable labels (" << labels.size() << ")!";

        return false;
    }

    columns.push_back(column);

    return true;
}

bool Frame::AddRow(const std::string& label, const std::vector<float>& row){
    if(row.size() != columns[0].size()){
        std::cout << "Length of given row (" << row.size() << ") does not match number of elements of each row in this Frame (" << columns[0].size() << ")!";

        return false;
    }

    labels.push_back(label);

    for(unsigned int i=0; i<row.size(); i++){
        columns[i].push_back(row[i]);
    }

    return true;
}

bool Frame::DoSort(std::vector<float>& column1, std::vector<float>& column2, const int& index, const bool& ascending){
    if(ascending){
        return column1[index] < column2[index];
    }

    else{
        return column1[index] > column2[index];
    }
}

void Frame::Sort(const std::string& label, const bool& ascending){
    int rowIndex = std::distance(labels.begin(), std::find(labels.begin(), labels.end(), label));
    
    std::sort(columns.begin(), columns.end(), std::bind(DoSort, std::placeholders::_1, std::placeholders::_2, rowIndex, ascending));
}

void Frame::WriteCSV(const std::string& fileName){
    std::ofstream myFile;
    myFile.open(fileName);

    std::string header;

    for(std::string& label: labels){
        header+= label + "\t";
    }

    header.replace(header.end()-1, header.end(), "\n");
    myFile << header;

    for(std::vector<float>& column: columns){
        std::string columnLine;

        for(float& value: column){
            columnLine+= std::to_string(value) + "\t";
        }

        columnLine.replace(columnLine.end()-1, columnLine.end(), "\n");
        myFile << columnLine;
    }
    
    myFile.close();

    std::cout << "Merged file: " << fileName << std::endl;
}
