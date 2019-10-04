#include <thread>
#include <unistd.h>
#include <sys/wait.h>

#include <ChargedAnalysis/Analysis/include/utils.h>
#include <ChargedAnalysis/Analysis/include/treereader.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    std::string process = std::string(argv[1]);
    std::vector<std::string> xParameters = Utils::SplitString(std::string(argv[2]));
    std::vector<std::string> yParameters = Utils::SplitString(std::string(argv[3]));
    std::vector<std::string> cutStrings = Utils::SplitString(std::string(argv[4]));
    std::string outname = std::string(argv[5]);
    std::string channel = std::string(argv[6]);
    bool saveTree = std::string(argv[7]) == "True" ? true : false;
    bool saveCsv = std::string(argv[8]) == "True" ? true : false;
    std::vector<std::string> fileNames = Utils::SplitString(std::string(argv[9]));
    float eventFraction = std::stof(std::string(argv[10]));

    //Measure execution time
    Utils::RunTime timer;

    //Get number of avaiable cores and calculate roughly equidistant ranges
    int nCores = (int)std::thread::hardware_concurrency();  
    std::vector<std::vector<std::pair<int, int>>> entries = TreeReader::EntryRanges(fileNames, nCores, channel, eventFraction);

    //Start progress bar at zero
    Utils::ProgressBar(0, process);

    //Create treereader instance
    int pid = -1;
    TreeReader reader(process, xParameters, yParameters, cutStrings, outname, channel, saveTree, saveCsv);

    for(unsigned int i = 0; i < fileNames.size(); i++){
        for(std::pair<int, int> range: entries[i]){
            if(range.first == -1.) continue;

            //Split main program in nCore child processes
            pid = fork();
            
            if(pid == 0){
                //Run event loop in child processes
                reader.EventLoop(fileNames[i], range.first, range.second);
                break;
            }
        }
        if(pid == 0){break;}
    }

    //Wait for child processes and merge output
    if(pid != 0){
        int nFinished=1;

        while(wait(NULL) != -1){
            Utils::ProgressBar(100*((float)nFinished/nCores), process);
            nFinished++;
        }

        reader.Merge();
        std::cout << "Created output for process: " << process << " (" << timer.Time() << " s)" << std::endl;
    }
}
