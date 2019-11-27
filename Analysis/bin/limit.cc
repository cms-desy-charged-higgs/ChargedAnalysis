#include <string>
#include <vector>

#include <ChargedAnalysis/Analysis/include/utils.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    std::string mass = std::string(argv[1]);
    std::string limitDir = std::string(argv[2]);
    std::vector<std::string> channels = Utils::SplitString(std::string(argv[3]), " ");
    
    std::stringstream combineCard; combineCard << "combineCards.py ";

    //Calculate limit per channel
    for(std::string& channel: channels){
        std::string channelDir = limitDir + "/" + Utils::ChanPaths(channel);
        std::system(("combine " + channelDir + "/datacard.txt --mass " + mass).c_str());
        std::system(("mv higgsCombineTest*" + mass + ".root " + channelDir + "/limit.root").c_str());

        combineCard << " " <<  channel << "=" << channelDir << "/datacard.txt"; 
    }

    //Combine datacards and calculate combined limit
    combineCard << " > " << limitDir << "/datacard.txt";
    std::system(combineCard.str().c_str());

    std::cout << combineCard.str().c_str() << std::endl;

    std::system(("combine " + limitDir + "/datacard.txt --mass " + mass).c_str());
    std::system(("mv higgsCombineTest*" + mass + ".root " + limitDir + "/limit.root").c_str());
}
