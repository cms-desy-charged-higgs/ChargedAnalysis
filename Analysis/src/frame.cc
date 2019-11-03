#include <ChargedAnalysis/Analysis/include/frame.h>

Frame::Frame(){}

Frame::Frame(const std::vector<std::string>& initLabels) : 
    labels(initLabels) {}

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

void Frame::ToCsv(const std::string& fileName){
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
}

