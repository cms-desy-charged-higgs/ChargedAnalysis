#include <string>
#include <vector>

#include <ChargedAnalysis/Utility/include/utils.h>
#include <ChargedAnalysis/Utility/include/parser.h>

int main(int argc, char* argv[]){
    //Parser arguments
    Parser parser(argc, argv);

    std::string mass = parser.GetValue<std::string>("mass");
    std::string limitDir = parser.GetValue<std::string>("limit-dir");
    std::vector<std::string> channels = parser.GetVector<std::string>("channels");
    
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
