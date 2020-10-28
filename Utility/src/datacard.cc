#include <ChargedAnalysis/Utility/include/datacard.h>

Datacard::Datacard(){}

Datacard::Datacard(const std::vector<std::string>& backgrounds, const std::string& signal, const std::string& data, const std::string& channel, const std::string& outDir, const bool& useAsimov, const std::vector<std::string>& systematics) :
    backgrounds(backgrounds),
    signal(signal),
    data(data),
    channel(channel),
    outDir(outDir), 
    useAsimov(useAsimov),
    systematics(systematics)

   // normSysts = {{"Lumi"}}

    {}

void Datacard::GetHists(const std::string& histDir, const std::string& discriminant){
    TH1::AddDirectory(kFALSE);

    //Open file which saves all process shapes
    TFile* outFile = TFile::Open((outDir + "/shapes.root").c_str(), "RECREATE");
    TDirectory* dir = outFile->mkdir(channel.c_str());

    TH1F* bkgSum = nullptr;

    //Open file with shape and get histogram
    for(const std::string& syst : systematics){
        for(const std::string shift : {"Up", "Down"}){
            //Skip Down loop for nominal case
            if(syst == "" and shift == "Down") continue;

            std::string systName = syst == "" ? "" : StrUtil::Merge(syst, shift);
            std::string fileName = StrUtil::Merge(histDir, "/", signal, "/merged/", systName, "/", signal, ".root");

            //Open file with shape and get histogram
            TFile* procFile = Utils::CheckNull<TFile>(TFile::Open((fileName).c_str()));
            TH1F* hist = Utils::CheckNull<TH1F>(procFile->Get<TH1F>(discriminant.c_str()));
            hist->SetName(syst == "" ? signal.c_str() : StrUtil::Join("_", signal, systName).c_str());
            hist->SetTitle(syst == "" ? signal.c_str() : StrUtil::Join("_", signal, systName).c_str());

            //Save yield of histogram
            if(syst == "") rates[signal] = (float)hist->Integral();

            dir->cd();
            hist->Write();

            delete hist;
            delete procFile;
        }
    }

    for(const std::string& syst : systematics){
        for(const std::string shift : {"Up", "Down"}){
            for(std::string& process: backgrounds){
                //Skip Down loop for nominal case
                if(syst == "" and shift == "Down") continue;

                std::cout << syst << shift << std::endl;

                std::string systName = syst == "" ? "" : StrUtil::Merge(syst, shift);
                std::string fileName = StrUtil::Merge(histDir, "/", process, "/merged/", systName, "/", process, ".root");

                //Open file with shape and get histogram
                TFile* procFile = Utils::CheckNull<TFile>(TFile::Open((fileName).c_str()));
                TH1F* hist = Utils::CheckNull<TH1F>(procFile->Get<TH1F>(discriminant.c_str()));
                hist->SetName(syst == "" ? process.c_str() : StrUtil::Join("_", process, systName).c_str());
                hist->SetTitle(syst == "" ? process.c_str() : StrUtil::Join("_", process, systName).c_str());

                if(syst == ""){
                    //Save yield of histogram
                    rates[process] = (float)hist->Integral();
                    if(bkgSum == nullptr) bkgSum = (TH1F*)hist->Clone();
                    else bkgSum->Add(hist);
                }

                dir->cd();
                hist->Write();

                delete hist;
                delete procFile;
            }
        }
    }

    if(useAsimov){
        bkgSum->SetName("data_obs");
        bkgSum->SetTitle("data_obs");

        rates["data_obs"] = bkgSum->Integral();

        dir->cd();
        bkgSum->Write();
    }

    else{
        //Open file with shape and get histogram
        TFile* procFile = Utils::CheckNull<TFile>(TFile::Open((histDir + "/" + data + "/" + data + ".root").c_str()));
        TH1F* hist = Utils::CheckNull<TH1F>(procFile->Get<TH1F>(discriminant.c_str()));
        hist->SetName("data_obs");
        hist->SetTitle("data_obs");

        //Save yield of histogram
        rates["data_obs"] = (float)hist->Integral();

        dir->cd();
        hist->Write();

        delete hist;
        delete procFile;
    }
   
    delete outFile;
}

void Datacard::Write(){
    std::ofstream datacard;
    datacard.open(outDir + "/datacard.txt");

    std::string border = "--------------------------------------------------------------------------------";

    datacard << "imax 1 number of channel\n";
    datacard << "jmax " << backgrounds.size() << " number of backgrounds\n";
    datacard << "kmax " << (systematics.size() - 1) << " number of nuisance parameters\n";
    datacard << border << std::endl;

    for(std::string& process: backgrounds){
        datacard << "shapes " << process << " " << channel  << " shapes.root $CHANNEL/$PROCESS $CHANNEL/$PROCESS_$SYSTEMATIC\n";
    }

    datacard << "shapes " << signal << " " << channel  << " shapes.root $CHANNEL/$PROCESS $CHANNEL/$PROCESS_$SYSTEMATIC\n";
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
    processName << std::left << std::setw(25) << signal;
    rate << std::left << std::setw(25) << rates[signal];

    for(unsigned int i = 0; i < backgrounds.size(); i++){
        binNames << std::left << std::setw(i!=backgrounds.size()-1 ? 25 : channel.size()) << channel;
        if(i==backgrounds.size()-1) binNames << "\n";

        processNr << std::left << std::setw(i!=backgrounds.size()-1 ? 25 : std::ceil((i+1)/10.)) << i+1;
        if(i==backgrounds.size()-1) processNr << "\n";

        processName << std::left << std::setw(i!=backgrounds.size()-1 ? 25 : backgrounds[i].size()) << backgrounds[i];
        if(i==backgrounds.size()-1) processName << "\n";

        rate << std::left << std::setw(i!=backgrounds.size()-1 ? 25 : std::to_string(rates[backgrounds[i]]).size()) << rates[backgrounds[i]];
        if(i==backgrounds.size()-1) rate << "\n";
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

        for(unsigned int i = 0; i < backgrounds.size() + 1; i++){
            systLine << std::left << std::setw(i!=backgrounds.size() ? 25 : channel.size()) << "1";
            if(i==backgrounds.size()) systLine << "\n";
        }

        datacard << systLine.rdbuf();
    }

    datacard.close();
}
