#include <ChargedAnalysis/Analysis/include/utils.h>

std::vector<std::string> Utils::SplitString(const std::string& splitString){
    std::istringstream iss(splitString);
    std::vector<std::string> results(std::istream_iterator<std::string>{iss},
                                 std::istream_iterator<std::string>());

    return results;
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
