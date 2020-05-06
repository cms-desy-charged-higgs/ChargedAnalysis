#include <ChargedAnalysis/Analysis/include/treeparser.h>

TreeParser::TreeParser(){}

void TreeParser::GetFunction(const std::string& parameter, TreeFunction& func){
    if(Utils::Find<std::string>(parameter, "f:") == -1) throw std::runtime_error("No function key 'f' in '" + parameter + "'");

    std::string funcName; float value = -999.;

    std::string funcLine = parameter.substr(parameter.find("f:")+2, parameter.substr(parameter.find("f:")).find("/")-2);
    
    for(std::string& funcParam: Utils::SplitString<std::string>(funcLine, ",")){
        std::vector<std::string> fInfo = Utils::SplitString<std::string>(funcParam, "=");

        if(fInfo[0] == "n") funcName = fInfo[1];
        else if(fInfo[0] == "v") value = std::stoi(fInfo[1]);
        else throw std::runtime_error("Invalid key '" + fInfo[0] + "' in parameter '" +  funcLine + "'");
    }

    func.SetFunction(funcName, value); 
}

void TreeParser::GetParticle(const std::string& parameter, TreeFunction& func){
    if(Utils::Find<std::string>(parameter, "p:") == -1) return;

    std::string partLine = parameter.substr(parameter.find("p:")+2, parameter.substr(parameter.find("p:")).find("/")-2);

    for(std::string& particle: Utils::SplitString<std::string>(partLine, "~")){
        std::string part = ""; std::string wp = ""; int idx = 0;

        for(const std::string& partParam: Utils::SplitString<std::string>(particle, ",")){
            std::vector<std::string> pInfo = Utils::SplitString<std::string>(partParam, "=");

            if(pInfo[0] == "n") part = pInfo[1];
            else if (pInfo[0] == "wp") wp = pInfo[1];
            else if (pInfo[0] == "i") idx = std::atoi(pInfo[1].c_str());
            else throw std::runtime_error("Invalid key '" + pInfo[0] + "' in parameter '" +  partLine + "'");
        }

        func.SetP1(part, idx, wp);
    }
}

void TreeParser::GetCut(const std::string& parameter, TreeFunction& func){
    if(Utils::Find<std::string>(parameter, "c:") == -1) throw std::runtime_error("No cut key 'c' in '" + parameter + "'");

    std::string comp; float compValue = -999.;

    std::string cutLine = parameter.substr(parameter.find("c:")+2, parameter.substr(parameter.find("c:")).find("/")-2);

    for(std::string& cutParam: Utils::SplitString<std::string>(cutLine, ",")){
        std::vector<std::string> cInfo = Utils::SplitString<std::string>(cutParam, "=");

        if(cInfo[0] == "n") comp = cInfo[1];
        else if(cInfo[0] == "v") compValue = std::stof(cInfo[1]);
        else throw std::runtime_error("Invalid key '" + cInfo[0] + "' in parameter '" +  cutLine + "'");
    }

    func.SetCut(comp, compValue);
}

void TreeParser::GetBinning(const std::string& parameter, TH1* hist){
    if(Utils::Find<std::string>(parameter, "h:") == -1) throw std::runtime_error("No hist key 'h' in '" + parameter + "'");

    std::string histLine = parameter.substr(parameter.find("h:")+2, parameter.substr(parameter.find("h:")).find("/")-2);

    int bins = 30; float xlow = 0; float xhigh = 1; 

    for(std::string& histParam: Utils::SplitString<std::string>(histLine, ",")){
        std::vector<std::string> hInfo = Utils::SplitString<std::string>(histParam, "=");

        if(hInfo[0] == "nxb") bins = std::stof(hInfo[1]);
        else if(hInfo[0] == "xl") xlow = std::stof(hInfo[1]);
        else if(hInfo[0] == "xh") xhigh = std::stof(hInfo[1]);
        else throw std::runtime_error("Invalid key '" + hInfo[0] + " in parameter '" +  histLine + "'");
    }

    hist->SetBins(bins, xlow, xhigh);
}
