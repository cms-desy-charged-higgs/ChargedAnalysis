#include <ChargedAnalysis/Analysis/include/utils.h>

std::vector<std::string> Utils::SplitString(const std::string& splitString, const std::string& delimeter){
    //Function which handles splitting of string input
    std::vector<std::string> splittedString;
    std::string string;
    std::istringstream splittedStream(splitString);
    while (std::getline(splittedStream, string, delimeter.c_str()[0])){
        splittedString.push_back(string);
    }

    return splittedString;
}

void Utils::ProgressBar(const int& progress, const std::string& process){
    std::string progressBar = "["; 

    for(int i = 0; i < progress; i++){
        if(i%2) progressBar += "#";
    }

    for(int i = 0; i < 100 - progress; i++){
        if(i%2) progressBar += " ";
    }

    progressBar = progressBar + "] " + "Progress of process " + process + ": " + std::to_string(progress) + "%";
    std::cout << "\r" << progressBar << std::flush;

    if(progress == 100) std::cout << std::endl;
}


Utils::RunTime::RunTime(){
    start = std::chrono::steady_clock::now();
}

float Utils::RunTime::Time(){
    end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
}

int Utils::FindInVec(const std::vector<std::string>& vect, const std::string& itemToFind){
    int position = -1.;

    for(unsigned int i=0; i < vect.size(); i++){
        if(vect[i].find(itemToFind) != std::string::npos){
            position=i;
        }
    }

    return position;
}

int Utils::Ratio(const float& num, const float& dem){
    return num/dem;
}
