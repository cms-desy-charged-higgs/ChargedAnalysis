#include <ChargedAnalysis/Analysis/include/decoder.h>

Decoder::Decoder(){}

bool Decoder::hasYInfo(const std::string& parameter){
    return !StrUtil::Find(parameter, "ax=y").empty();
}

void Decoder::GetFunction(const std::string& parameter, NTupleReader& reader, const bool& readY, const std::experimental::source_location& location){
    if(StrUtil::Find(parameter, "f:").empty()) throw std::runtime_error(StrUtil::PrettyError(location, "No function key 'f:' in '",  parameter, "'!"));

    std::vector<std::string> lines = StrUtil::Split(parameter, "/");
    lines.erase(std::remove_if(lines.begin(), lines.end(), [&](std::string l){return StrUtil::Find(l, "f:").empty();}), lines.end());
    
    for(const std::string& line : lines){
        std::string funcName; std::vector<std::string> values; bool isY = false;

        std::string funcLine = line.substr(line.find("f:")+2);
        
        for(std::string& funcParam: StrUtil::Split(funcLine, ",")){
            std::vector<std::string> fInfo = funcParam != "" ? StrUtil::Split(funcParam, "=") : StrUtil::Split(funcLine, "=");

            if(fInfo[0] == "n") funcName = fInfo[1];
            else if(fInfo[0] == "v") values.push_back(fInfo[1]);
            else if(fInfo[0] == "ax") isY = "y" == fInfo[1];
            else throw std::runtime_error(StrUtil::PrettyError(location, "Invalid key '", fInfo[0], "' in parameter '", parameter, "'"));
        }

        if(isY == readY) reader.AddFunction(funcName, values);
    }
}

void Decoder::GetParticle(const std::string& parameter, NTupleReader& reader, const bool& readY, const std::experimental::source_location& location){
    std::vector<std::string> lines = StrUtil::Split(parameter, "/");
    lines.erase(std::remove_if(lines.begin(), lines.end(), [&](std::string l){return StrUtil::Find(l, "p:").empty();}), lines.end());

    for(const std::string line : lines){
        std::string partLine = line.substr(line.find("p:")+2);
        std::string part = ""; std::string wp = ""; int idx = 0; bool isY = false;

        for(const std::string& partParam: StrUtil::Split(partLine, ",")){
            std::vector<std::string> pInfo = partParam != "" ? StrUtil::Split(partParam, "=") : StrUtil::Split(partLine, "=");

            if(pInfo[0] == "n") part = pInfo[1];
            else if (pInfo[0] == "wp") wp = pInfo[1];
            else if (pInfo[0] == "i") idx = std::atoi(pInfo[1].c_str());
            else if(pInfo[0] == "ax") isY = "y" == pInfo[1];
            else throw std::runtime_error(StrUtil::PrettyError(location, "Invalid key '", pInfo[0], "' in parameter '", parameter, "'"));
        }

        if(isY == readY){
            reader.AddParticle(part, idx, wp);
        }
    }
}

void Decoder::GetParticle(const std::string& parameter, Weighter& weight, const bool& readY, const std::experimental::source_location& location){
    std::vector<std::string> lines = StrUtil::Split(parameter, "/");
    lines.erase(std::remove_if(lines.begin(), lines.end(), [&](std::string l){return StrUtil::Find(l, "p:").empty();}), lines.end());

    for(const std::string line : lines){
        std::string partLine = line.substr(line.find("p:")+2);

        std::string part = ""; std::string wp = ""; int idx = 0; bool isY = false;

        for(const std::string& partParam: StrUtil::Split(partLine, ",")){
            std::vector<std::string> pInfo = partParam != "" ? StrUtil::Split(partParam, "=") : StrUtil::Split(partLine, "=");

            if(pInfo[0] == "n") part = pInfo[1];
            else if (pInfo[0] == "wp") wp = pInfo[1];
            else if (pInfo[0] == "i") idx = std::atoi(pInfo[1].c_str());
            else if(pInfo[0] == "ax") isY = "y" == pInfo[1];
            else throw std::runtime_error(StrUtil::PrettyError(location, "Invalid key '", pInfo[0], "' in parameter '", parameter, "'"));
        }

        if(isY == readY){
            weight.AddParticle(part, wp);
        }
    }
}

void Decoder::GetBinning(const std::string& parameter, TH1* hist, const bool& isY, const std::experimental::source_location& location){
    if(StrUtil::Find(parameter, "h:").empty()) throw std::runtime_error(StrUtil::PrettyError(location, "No function key 'h:' in '",  parameter, "'!"));

    std::vector<std::string> lines = StrUtil::Split(parameter, "/");
    lines.erase(std::remove_if(lines.begin(), lines.end(), [&](std::string l){return StrUtil::Find(l, "h:").empty();}), lines.end());

    int xBins = 30, yBins = -1.; 
    float xlow = 0, xhigh = 1, ylow = 0, yhigh = 1;

    for(const std::string line : lines){
        std::string histLine = line.substr(line.find("h:")+2);

        for(std::string& histParam: StrUtil::Split(histLine, ",")){
            std::vector<std::string> hInfo = histParam != "" ? StrUtil::Split(histParam, "=") : StrUtil::Split(histLine, "=");

            if(hInfo[0] == "nxb") xBins = std::stof(hInfo[1]);
            else if(hInfo[0] == "xl") xlow = std::stof(hInfo[1]);
            else if(hInfo[0] == "xh") xhigh = std::stof(hInfo[1]);
            else if(hInfo[0] == "nyb") yBins = std::stof(hInfo[1]);
            else if(hInfo[0] == "yl") ylow = std::stof(hInfo[1]);
            else if(hInfo[0] == "yh") yhigh = std::stof(hInfo[1]);
            else throw std::runtime_error(StrUtil::PrettyError(location, "Invalid key '", hInfo[0], "' in parameter '", parameter, "'"));
        }
    }

    if(!isY) hist->SetBins(xBins, xlow, xhigh);
    else hist->SetBins(xBins, xlow, xhigh, yBins, ylow, yhigh);
}

void Decoder::GetCut(const std::string& parameter, NTupleReader& reader, const std::experimental::source_location& location){
    if(StrUtil::Find(parameter, "c:").empty()) throw std::runtime_error(StrUtil::PrettyError(location, "No function key 'c:' in '",  parameter, "'!"));

    std::string comp; float compValue = -999.;
    std::map<std::string, std::string> compMap = {{"equal", "=="}, {"bigger", ">="}, {"smaller", "<="}, {"modulo", "%"}};

    std::string cutLine = parameter.substr(parameter.find("c:")+2, parameter.substr(parameter.find("c:")).find("/")-2);

    for(std::string& cutParam: StrUtil::Split(cutLine, ",")){
        std::vector<std::string> cInfo = cutParam != "" ? StrUtil::Split(cutParam, "=") : StrUtil::Split(cutLine, "=");

        if(cInfo[0] == "n") comp = cInfo[1];
        else if(cInfo[0] == "v") compValue = std::stof(cInfo[1]);
        else throw std::runtime_error(StrUtil::PrettyError(location, "Invalid key '", cInfo[0], "' in parameter '", parameter, "'"));
    }

    reader.AddCut(compValue, VUtil::At(compMap,comp));
}
