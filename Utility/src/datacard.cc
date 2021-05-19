#include <ChargedAnalysis/Utility/include/datacard.h>

Datacard::Datacard(){}

Datacard::Datacard(const std::string& outDir, const std::string& channel, const std::vector<std::string>& bkgProcesses, const std::map<std::string, std::vector<std::string>>& bkgFiles, const std::string& sigProcess, const std::map<std::string, std::string>& sigFiles, const std::string& data, const std::string& dataFile, const std::vector<std::string>& systematics) :
    outDir(outDir),
    channel(channel),
    bkgProcesses(bkgProcesses),
    bkgFiles(bkgFiles),
    sigProcess(sigProcess),
    sigFiles(sigFiles),
    data(data), 
    dataFile(dataFile),
    systematics(systematics){}

void Datacard::GetHists(const std::string& discriminant){
    TH1::AddDirectory(kFALSE);

    //Open file which saves all process shapes
    std::shared_ptr<TFile> outFile = std::make_shared<TFile>((outDir + "/shapes.root").c_str(), "RECREATE");
    TDirectory* dir = outFile->mkdir(channel.c_str());

    std::shared_ptr<TH1F> bkgSum;

    //Open file with shape and get histogram
    for(const std::string& syst : systematics){
        for(const std::string shift : {"Up", "Down"}){
            //Skip Down loop for nominal case
            if(syst == "" and shift == "Down") continue;

            std::string systName = syst == "" ? "" : StrUtil::Merge(syst, shift);

            for(std::size_t i = 0; i < bkgProcesses.size(); ++i){
                std::shared_ptr<TFile> file = RUtil::Open(bkgFiles[systName].at(i));
                std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), discriminant);

                hist->SetName(syst == "" ? bkgProcesses.at(i).c_str() : StrUtil::Join("_", bkgProcesses.at(i), systName).c_str());
                hist->SetTitle(syst == "" ? bkgProcesses.at(i).c_str() : StrUtil::Join("_", bkgProcesses.at(i), systName).c_str());

                //Save yield of histogram
                if(syst == ""){
                    rates[bkgProcesses.at(i)] = hist->Integral();
                    if(bkgSum == nullptr) bkgSum = RUtil::CloneSmart<TH1F>(hist.get());
                    else bkgSum->Add(hist.get());
                }

                dir->cd();
                hist->Write();
            }

            std::shared_ptr<TFile> file = RUtil::Open(sigFiles[systName]);
            std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), discriminant);

            hist->SetName(syst == "" ? sigProcess.c_str() : StrUtil::Join("_", sigProcess, systName).c_str());
            hist->SetTitle(syst == "" ? sigProcess.c_str() : StrUtil::Join("_", sigProcess, systName).c_str());

            //Save yield of histogram
            if(syst == "") rates[sigProcess] = hist->Integral();

            dir->cd();
            hist->Write();           
        }
    }

    if(dataFile == ""){
        std::shared_ptr<TH1F> data = RUtil::CloneSmart(bkgSum.get());
        data->Reset();
        data->SetName("data_obs");
        data->SetTitle("data_obs");

        rates["data_obs"] = 0;

        dir->cd();
        data->Write();
    }

    else{
        //Open file with shape and get histogram
        std::shared_ptr<TFile> file = RUtil::Open(dataFile);
        std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), discriminant);
        hist->SetName("data_obs");
        hist->SetTitle("data_obs");

        //Save yield of histogram
        rates["data_obs"] = hist->Integral();

        dir->cd();
        hist->Write();
    }
}

void Datacard::Write(){
    std::ofstream datacard;
    datacard.open(outDir + "/datacard.txt");

    std::string border = "--------------------------------------------------------------------------------";

    datacard << "imax 1 number of bins\n";
    datacard << "jmax " << bkgProcesses.size() << " number of backgrounds\n";
    datacard << "kmax * number of nuisance parameters\n";
    datacard << border << std::endl;

    for(std::string& process: bkgProcesses){
        datacard << "shapes " << process << " " << channel  << " shapes.root $CHANNEL/$PROCESS $CHANNEL/$PROCESS_$SYSTEMATIC\n";
    }

    datacard << "shapes " << sigProcess << " " << channel  << " shapes.root $CHANNEL/$PROCESS $CHANNEL/$PROCESS_$SYSTEMATIC\n";
    datacard << "shapes " << "data_obs" << " " << channel  << " shapes.root $CHANNEL/$PROCESS $CHANNEL/$PROCESS_$SYSTEMATIC\n";

    datacard << border << std::endl;

    datacard << std::left << std::setw(40) << "bin" << channel << "\n";
    datacard << std::left << std::setw(40) << "observation" << rates["data_obs"] << "\n";  

    datacard << border << std::endl;

    std::stringstream binNames; binNames << std::left << std::setw(40) << "bin";
    std::stringstream processNr; processNr << std::left << std::setw(40) << "process";
    std::stringstream processName; processName << std::left << std::setw(40) << "process";
    std::stringstream rate; rate << std::left << std::setw(40) << "rate";

    binNames << std::left << std::setw(25) << channel;
    processNr << std::left << std::setw(25) << -1;
    processName << std::left << std::setw(25) << sigProcess;
    rate << std::left << std::setw(25) << rates[sigProcess];

    for(unsigned int i = 0; i < bkgProcesses.size(); i++){
        binNames << std::left << std::setw(i!=bkgProcesses.size()-1 ? 25 : channel.size()) << channel;
        if(i==bkgProcesses.size()-1) binNames << "\n";

        processNr << std::left << std::setw(i!=bkgProcesses.size()-1 ? 25 : std::ceil((i+1)/10.)) << i+1;
        if(i==bkgProcesses.size()-1) processNr << "\n";

        processName << std::left << std::setw(i!=bkgProcesses.size()-1 ? 25 : bkgProcesses[i].size()) << bkgProcesses[i];
        if(i==bkgProcesses.size()-1) processName << "\n";

        rate << std::left << std::setw(i!=bkgProcesses.size()-1 ? 25 : std::to_string(rates[bkgProcesses[i]]).size()) << rates[bkgProcesses[i]];
        if(i==bkgProcesses.size()-1) rate << "\n";
    }

    datacard << binNames.rdbuf(); 
    datacard << processNr.rdbuf(); 
    datacard << processName.rdbuf(); 
    datacard << rate.rdbuf();

    datacard << border << std::endl;

    for(const std::string syst : systematics){
        if(syst == "") continue;

        std::stringstream systLine;
        systLine << std::left  << std::setw(20) << syst << std::setw(20) << "shape";

        for(unsigned int i = 0; i < bkgProcesses.size() + 1; i++){
            systLine << std::left << std::setw(i!=bkgProcesses.size() ? 25 : channel.size()) << "1";
            if(i==bkgProcesses.size()) systLine << "\n";
        }

        datacard << systLine.rdbuf();
    }

    datacard.close();
}
