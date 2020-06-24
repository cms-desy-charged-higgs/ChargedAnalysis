#include <ChargedAnalysis/Analysis/include/treeparser.h>

TreeParser::TreeParser(){}

void TreeParser::GetFunction(const std::string& parameter, TreeFunction& func){
    if(Utils::Find<std::string>(parameter, "f:") == -1) throw std::runtime_error("No function key 'f' in '" + parameter + "'");

    std::vector lines = Utils::SplitString<std::string>(parameter, "/");
    lines.erase(std::remove_if(lines.begin(), lines.end(), [&](std::string l){return Utils::Find<std::string>(l, "f:") == -1;}), lines.end());
    
    for(const std::string line : lines){
        std::string funcName; std::string value = ""; bool isX = true;

        std::string funcLine = line.substr(line.find("f:")+2);
        
        for(std::string& funcParam: Utils::SplitString<std::string>(funcLine, ",")){
            std::vector<std::string> fInfo = Utils::SplitString<std::string>(funcParam, "=");

            if(fInfo[0] == "n") funcName = fInfo[1];
            else if(fInfo[0] == "v") value = fInfo[1];
            else if(fInfo[0] == "ax") isX = fInfo[1] != "y";
            else throw std::runtime_error("Invalid key '" + fInfo[0] + "' in parameter '" +  funcLine + "'");
        }

        if(isX) func.SetFunction<Axis::X>(funcName, value);
        else{
            func.SetYAxis();
            func.SetFunction<Axis::Y>(funcName, value);
        }
    }
}

void TreeParser::GetParticle(const std::string& parameter, TreeFunction& func){
    if(Utils::Find<std::string>(parameter, "p:") == -1) return;

    std::vector lines = Utils::SplitString<std::string>(parameter, "/");
    lines.erase(std::remove_if(lines.begin(), lines.end(), [&](std::string l){return Utils::Find<std::string>(l, "p:") == -1;}), lines.end());

    for(const std::string line : lines){
        std::string partLine = line.substr(line.find("p:")+2);

        std::string part = ""; std::string wp = ""; int idx = 0; bool isX = true; int pos = 1;

        for(const std::string& partParam: Utils::SplitString<std::string>(partLine, ",")){
            std::vector<std::string> pInfo = Utils::SplitString<std::string>(partParam, "=");

            if(pInfo[0] == "n") part = pInfo[1];
            else if (pInfo[0] == "wp") wp = pInfo[1];
            else if (pInfo[0] == "i") idx = std::atoi(pInfo[1].c_str());
            else if (pInfo[0] == "pos") pos = std::atoi(pInfo[1].c_str());
            else if(pInfo[0] == "ax") isX = pInfo[1] != "y";
            else throw std::runtime_error("Invalid key '" + pInfo[0] + "' in parameter '" +  partLine + "'");
        }

        if(pos == 1){
            if(isX) func.SetP1<Axis::X>(part, idx, wp);

            else{
                func.SetYAxis();
                func.SetP1<Axis::Y>(part, idx, wp);
            } 
        }

        if(pos == 2){
            if(isX) func.SetP2<Axis::X>(part, idx, wp);

            else{
                func.SetYAxis();
                func.SetP2<Axis::Y>(part, idx, wp);
            } 
        }
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

    int xBins = 30, yBins = -1.; 
    float xlow = 0, xhigh = 1, ylow = 0, yhigh = 1; 

    for(std::string& histParam: Utils::SplitString<std::string>(histLine, ",")){
        std::vector<std::string> hInfo = Utils::SplitString<std::string>(histParam, "=");

        if(hInfo[0] == "nxb") xBins = std::stof(hInfo[1]);
        else if(hInfo[0] == "xl") xlow = std::stof(hInfo[1]);
        else if(hInfo[0] == "xh") xhigh = std::stof(hInfo[1]);
        else if(hInfo[0] == "nyb") yBins = std::stof(hInfo[1]);
        else if(hInfo[0] == "yl") ylow = std::stof(hInfo[1]);
        else if(hInfo[0] == "yh") yhigh = std::stof(hInfo[1]);
        else throw std::runtime_error("Invalid key '" + hInfo[0] + " in parameter '" +  histLine + "'");
    }

    if(yBins == -1.) hist->SetBins(xBins, xlow, xhigh);
    else hist->SetBins(xBins, xlow, xhigh, yBins, ylow, yhigh);
}
