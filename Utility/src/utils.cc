#include <ChargedAnalysis/Utility/include/utils.h>

std::string Utils::ChanPaths(const std::string& channel){
    std::map<std::string, std::string> chanMap = {
            {"e4j", "Ele4J"},
            {"mu4j", "Muon4J"},
            {"e2j1fj", "Ele2J1FJ"},
            {"mu2j1fj", "Muon2J1FJ"},
            {"e2fj", "Ele2FJ"},
            {"mu2fj", "Muon2FJ"},
    };

    return chanMap[channel];
};

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

template std::vector<std::string> Utils::SplitString(const std::string& splitString, const std::string& delimeter);
template std::vector<int> Utils::SplitString(const std::string& splitString, const std::string& delimeter);
template std::vector<float> Utils::SplitString(const std::string& splitString, const std::string& delimeter);


std::string Utils::Join(const std::string& delimeter, const std::vector<std::string> strings){
    std::string out;

    for(std::vector<std::string>::const_iterator it = strings.begin(); it != strings.end(); ++it){
        out += *it;

        if(it != strings.end() - 1) out += delimeter;
    }

    return out;
}

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

int Utils::FindInVec(const std::vector<std::string>& vect, const std::string& itemToFind){
    int position = -1.;

    for(unsigned int i=0; i < vect.size(); i++){
        if(vect[i].find(itemToFind) != std::string::npos){
            position=i;
        }
    }

    return position;
}

unsigned int Utils::BitCount(unsigned int num){
    unsigned int count = 0; 
    while (num) { 
        count += num & 1; 
        num >>= 1; 
    } 
    return count; 
}

TGraph* Utils::GetROC(const torch::Tensor pred, const torch::Tensor target, const int& nPoints){
    TGraph* ROC = new TGraph();

    std::vector<float> p(pred.size(0), 1);
    std::vector<float> t(pred.size(0), 1);

    for(int i = 0; i < pred.size(0); i++){
        p[i] = pred[i].item<float>();
        t[i] = target[i].item<float>();
    }

    for(int i = 1; i <= nPoints; i++){
        int truePositive = 0;
        int trueTotal = 0;
        int falsePositive = 0;
        int falseTotal = 0;
        float thresHold = (float)i/nPoints;

        for(int j = 0; j < pred.size(0); j++){
            if(t[j] == 0){
                falseTotal++;
                if(p[j] > thresHold) falsePositive++;
            } 

            if(t[j] == 1){
                trueTotal++;
                if(p[j] > thresHold) truePositive++;
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
    Plotter::SetHist(topHist);

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
    Plotter::DrawHeader(false, "All channel", "Work in Progress");

    canvas->SaveAs((scorePath + "/score.pdf").c_str());

    delete canvas; delete topHist; delete higgsHist;
}
