#include <ChargedAnalysis/Utility/include/datacard.h>

Datacard::Datacard(){}

Datacard::Datacard(const std::vector<std::string>& backgrounds, const std::string& signal, const std::string& data, const std::string& channel, const std::string& outDir, const bool& useAsimov) :
    backgrounds(backgrounds),
    signal(signal),
    data(data),
    channel(channel),
    outDir(outDir), 
    useAsimov(useAsimov)
    {}

void Datacard::GetHists(const std::string& histDir, const std::string& discriminant){
    TH1::AddDirectory(kFALSE);

    //Open file which saves all process shapes
    TFile* outFile = TFile::Open((outDir + "/shapes.root").c_str(), "RECREATE");
    TDirectory* dir = outFile->mkdir(channel.c_str());

    TH1F* bkgSum = NULL;

    //Open file with shape and get histogram
    TFile* procFile = TFile::Open((histDir + "/" + signal + ".root").c_str());
    TH1F* hist = (TH1F*)procFile->Get(discriminant.c_str());
    hist->SetName(signal.c_str());
    hist->SetTitle(signal.c_str());

    //Save yield of histogram
    rates[signal] = (float)hist->Integral();

    dir->cd();
    hist->Write();

    delete hist;
    delete procFile;

    for(std::string& process: backgrounds){
        //Open file with shape and get histogram
        TFile* procFile = TFile::Open((histDir + "/" + process + ".root").c_str());
        TH1F* hist = (TH1F*)procFile->Get(discriminant.c_str());
        hist->SetName(process.c_str());
        hist->SetTitle(process.c_str());

        //Save yield of histogram
        rates[process] = (float)hist->Integral();

        if(bkgSum == NULL) bkgSum = (TH1F*)hist->Clone();
        else bkgSum->Add(hist);

        dir->cd();
        hist->Write();

        delete hist;
        delete procFile;
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
        TFile* procFile = TFile::Open((histDir + "/" + "/" + data + ".root").c_str());
        TH1F* hist = (TH1F*)procFile->Get(discriminant.c_str());
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

    datacard.close();
}
