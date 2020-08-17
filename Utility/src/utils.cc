#include <ChargedAnalysis/Utility/include/utils.h>

template <typename T>
std::vector<T> Utils::SplitString(const std::string& splitString, const std::string& delimeter){
    T value;
    std::vector<T> values;

    std::size_t current, previous = 0;
    current = splitString.find(delimeter);

    while (current != std::string::npos){
        std::istringstream iss(splitString.substr(previous, current - previous));
        iss >> value;
        values.push_back(value);

        previous = current + 1;
        current = splitString.find(delimeter, previous);
    }

    std::istringstream iss(splitString.substr(previous, current - previous));
    iss >> value;
    values.push_back(value);

    return values;
}

template std::vector<std::string> Utils::SplitString(const std::string&, const std::string&);
template std::vector<int> Utils::SplitString(const std::string&, const std::string&);
template std::vector<float> Utils::SplitString(const std::string&, const std::string&);

template <typename T>
std::string Utils::Format(const std::string& label, const std::string& initial, const T& replace, const bool& ignoreMissing){
    std::string result = initial;

    std::stringstream repl;
    repl << replace;

    if(result.find(label) != std::string::npos){
        result.replace(result.find(label), 1, repl.str());
    }

    else{
        if(!ignoreMissing) throw std::out_of_range("Did not found '" + label + "' in string '" + result + "'");
    }

    return result;
}

template std::string Utils::Format(const std::string&, const std::string&, const std::string&, const bool&);
template std::string Utils::Format(const std::string&, const std::string&, const int&, const bool&);
template std::string Utils::Format(const std::string&, const std::string&, const float&, const bool&);

std::string Utils::Join(const std::string& delimeter, const std::vector<std::string> strings){
    std::string out;

    for(std::vector<std::string>::const_iterator it = strings.begin(); it != strings.end(); ++it){
        out += *it;

        if(it != strings.end() - 1) out += delimeter;
    }

    return out;
}

template <typename T>
std::vector<T> Utils::Merge(const std::vector<T>& vec1, const std::vector<T>& vec2){
    std::vector<T> vec = vec1;
    vec.insert(vec.end(), vec2.begin(), vec2.end());

    return vec;
}

template std::vector<std::string> Utils::Merge(const std::vector<std::string>&, const std::vector<std::string>&);
template std::vector<int> Utils::Merge(const std::vector<int>&, const std::vector<int>&);
template std::vector<float> Utils::Merge(const std::vector<float>&, const std::vector<float>&);

template <typename T>
int Utils::Find(const std::string& string, const T& itemToFind){
    int position = -1.;

    std::stringstream item;
    item << itemToFind;

    if(string.find(item.str()) != std::string::npos){
        position=string.find(item.str());
    }

    return position;
}

template int Utils::Find(const std::string&, const int&);
template int Utils::Find(const std::string&, const float&);
template int Utils::Find(const std::string&, const std::string&);

void Utils::ProgressBar(const int& progress, const std::string& addInfo){
    std::string progressBar = "["; 

    for(int i = 0; i <= progress; i++){
        if(i%10 == 0) progressBar += "#";
    }

    for(int i = 0; i < 100 - progress; i++){
        if(i%10 == 0) progressBar += " ";
    }

    progressBar = progressBar + "] " + addInfo;

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

unsigned int Utils::BitCount(unsigned int num){
    unsigned int count = 0; 
    while (num) { 
        count += num & 1; 
        num >>= 1; 
    } 
    return count; 
}

float Utils::CheckZero(const float& input){
    return input == 0. ? 1: input;
}

void Utils::CopyToCache(const std::string inFile, const std::string outPath){
    std::string gfalPrefix = "srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=";
    std::string dCachePath = "/pnfs/desy.de/cms/tier2/store/user/dbrunner/";

    std::string fileName = Utils::SplitString<std::string>(inFile, "/").back();

    std::system(("gfal-mkdir -vp " + gfalPrefix + dCachePath +  outPath).c_str());
    std::system(("gfal-copy -vf " + inFile + " " + gfalPrefix + dCachePath +  outPath + "/" + fileName).c_str());
    std::system(("rm -fv " + inFile).c_str());
    std::system(("ln -sv " + dCachePath +  outPath + "/" + fileName + " " + inFile).c_str());
}

TGraph* Utils::GetROC(const std::vector<float>& pred, const std::vector<int>& target, const int& nPoints){
    TGraph* ROC = new TGraph();

    for(int i = 1; i <= nPoints; i++){
        int truePositive = 0, trueTotal = 0, falsePositive = 0, falseTotal = 0;
        float thresHold = (float)i/nPoints;

        for(int j = 0; j < pred.size(); j++){
            if(target[j] == 0){
                falseTotal++;
                if(pred[j] > thresHold) falsePositive++;
            } 

            if(target[j] == 1){
                trueTotal++;
                if(pred[j] > thresHold) truePositive++;
            } 
        }

        ROC->SetPoint(i, (float)falsePositive/falseTotal, (float)truePositive/trueTotal);
    }

    return ROC;
}

void Utils::DrawScore(const torch::Tensor pred, const torch::Tensor truth, const std::string& scorePath){
    TCanvas* canvas = new TCanvas("canvas", "canvas", 1000, 800);
    canvas->Draw();

    TH1F* higgsHist = new TH1F("Higgs", "Higgs", 30, 0, 1);
    higgsHist->SetLineColor(kBlue+1);
    higgsHist->SetFillStyle(3335);
    higgsHist->SetFillColor(kBlue);
    higgsHist->SetLineWidth(4);

    TH1F* topHist = new TH1F("Top", "Top", 30, 0, 1);
    topHist->SetLineColor(kRed+1);
    topHist->SetFillStyle(3353);
    topHist->SetFillColor(kRed);
    topHist->SetLineWidth(4);

    TGraph* ROC;
    TLatex* rocText;
    
    Plotter::SetPad(canvas);
    Plotter::SetStyle();
    Plotter::SetHist(canvas, topHist);

    for(unsigned int k=0; k < pred.size(0); k++){
        if(truth[k].item<float>() == 0){
            topHist->Fill(pred[k].item<float>());
        }

        else{
            higgsHist->Fill(pred[k].item<float>());
        }
    }

    topHist->DrawNormalized("HIST");
    higgsHist->DrawNormalized("HIST SAME");
    Plotter::DrawHeader(canvas, "All channel", "Work in Progress");

    canvas->SaveAs((scorePath + "/score.pdf").c_str());

    delete canvas; delete topHist; delete higgsHist;
}
