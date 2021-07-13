#include <ChargedAnalysis/Utility/include/datacard.h>

Datacard::Datacard(){}

Datacard::Datacard(const std::string& outDir, const std::string& channel, const std::string era, const std::vector<std::string>& bkgProcesses, const std::map<std::string, std::map<std::string, std::vector<std::string>>>& bkgFiles, const std::string& sigProcess, const std::map<std::string, std::map<std::string, std::string>>& sigFiles, const std::string& data, const std::map<std::string, std::string>& dataFile, const std::vector<std::string>& systematics, const std::vector<std::string>& regions) :
    outDir(outDir),
    channel(channel),
    era(era),
    bkgProcesses(bkgProcesses),
    bkgFiles(bkgFiles),
    sigProcess(sigProcess),
    sigFiles(sigFiles),
    data(data), 
    dataFile(dataFile),
    systematics(systematics),
    regions(regions){}

void Datacard::GetHists(const std::string& discriminant, const bool& blind){
    TH1::AddDirectory(kFALSE);

    //Open file which saves all process shapes
    std::shared_ptr<TFile> outFile = std::make_shared<TFile>((outDir + "/shapes.root").c_str(), "RECREATE");

    //Open file with shape and get histogram
    for(const std::string& region : regions){
        TDirectory* dir = outFile->mkdir(StrUtil::Join("_", channel, era, region).c_str());

        std::shared_ptr<TH1F> bkgSum;

        for(const std::string& syst : systematics){
            for(const std::string shift : {"Up", "Down"}){
                //Skip Down loop for nominal case
                if(syst == "" and shift == "Down") continue;

                std::string systName = syst == "" ? "" : StrUtil::Merge(syst, shift);

                for(std::size_t i = 0; i < bkgProcesses.size(); ++i){
                    std::shared_ptr<TFile> file = RUtil::Open(bkgFiles[region][systName].at(i));
                    std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), discriminant);

                    hist->SetName(syst == "" ? bkgProcesses.at(i).c_str() : StrUtil::Join("_", bkgProcesses.at(i), systName).c_str());
                    hist->SetTitle(syst == "" ? bkgProcesses.at(i).c_str() : StrUtil::Join("_", bkgProcesses.at(i), systName).c_str());

                    //Save yield of histogram
                    if(syst == ""){
                        rates[{region, bkgProcesses.at(i)}] = hist->Integral();
                        if(bkgSum == nullptr) bkgSum = RUtil::CloneSmart<TH1F>(hist.get());
                        else bkgSum->Add(hist.get());
                    }

                    else{
                        float value = 1. + (hist->Integral() - rates[{region, bkgProcesses.at(i)}])/rates[{region, bkgProcesses.at(i)}];
                        if(std::isnan(value) or value <= 0) value = 1.;
                        if(value > 2.) value = 2.;

                        relSys[{region, bkgProcesses.at(i), syst, shift}] = value;
                    }

                    dir->cd();
                    hist->Write();
                }

                std::shared_ptr<TFile> file = RUtil::Open(sigFiles[region][systName]);
                std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), discriminant);

                hist->SetName(syst == "" ? sigProcess.c_str() : StrUtil::Join("_", sigProcess, systName).c_str());
                hist->SetTitle(syst == "" ? sigProcess.c_str() : StrUtil::Join("_", sigProcess, systName).c_str());

                //Save yield of histogram
                if(syst == "") rates[{region, sigProcess}] = hist->Integral();

                else{
                    float value = 1. + (hist->Integral() - rates[{region, sigProcess}])/rates[{region, sigProcess}];
                    if(std::isnan(value) or value == 0) value = 1.;
                    if(value > 2.) value = 2.;

                    relSys[{region, sigProcess, syst, shift}] = value;
                }

                dir->cd();
                hist->Write();           
            }
        }

        if(blind){
            std::shared_ptr<TH1F> data = RUtil::CloneSmart(bkgSum.get());
            data->SetName("data_obs");
            data->SetTitle("data_obs");

            rates[{region, "data_obs"}] = data->Integral();

            dir->cd();
            data->Write();
        }

        else{
            //Open file with shape and get histogram
            std::shared_ptr<TFile> file = RUtil::Open(dataFile[region]);
            std::shared_ptr<TH1F> hist = RUtil::GetSmart<TH1F>(file.get(), discriminant);
            hist->SetName("data_obs");
            hist->SetTitle("data_obs");

            //Save yield of histogram
            rates[{region, "data_obs"}] = hist->Integral();

            dir->cd();
            hist->Write();
        }
    }
}

void Datacard::Write(){
    std::ofstream datacard;
    datacard.open(outDir + "/datacard.txt");

    std::string border = "--------------------------------------------------------------------------------";
    std::string binName = StrUtil::Join("_", channel, era, "@");

    datacard << StrUtil::Replace("imax @ number of channels\n", "@", regions.size());
    datacard << "jmax " << bkgProcesses.size() << " number of backgrounds\n";
    datacard << "kmax * number of nuisance parameters\n";
    datacard << border << std::endl;

    for(const std::string& region : regions){
        for(std::string& process: bkgProcesses){
            datacard << "shapes " << process << " " << StrUtil::Replace(binName, "@", region) << " shapes.root $CHANNEL/$PROCESS $CHANNEL/$PROCESS_$SYSTEMATIC\n";
        }

        datacard << "shapes " << sigProcess << " " << StrUtil::Replace(binName, "@", region) << " shapes.root $CHANNEL/$PROCESS $CHANNEL/$PROCESS_$SYSTEMATIC\n";
        datacard << "shapes " << "data_obs" << " " << StrUtil::Replace(binName, "@", region)  << " shapes.root $CHANNEL/$PROCESS $CHANNEL/$PROCESS_$SYSTEMATIC\n";
    }

    datacard << border << std::endl;

    datacard << std::left << std::setw(30) << "bin";
    for(const std::string& region : regions) datacard << std::left << std::setw(25) << StrUtil::Replace(binName, "@", region);

    datacard << "\n" << std::left << std::setw(30) << "observation";
    for(const std::string& region : regions) datacard << std::left << std::setw(25) << rates[{region, "data_obs"}];

    datacard << "\n" << border << std::endl;

    std::stringstream binNames; binNames << std::left << std::setw(30) << "bin";
    std::stringstream processNr; processNr << std::left << std::setw(30) << "process";
    std::stringstream processName; processName << std::left << std::setw(30) << "process";
    std::stringstream rate; rate << std::left << std::setw(30) << "rate";

    for(const std::string& region : regions){
        binNames << std::left << std::setw(25) << StrUtil::Replace(binName, "@", region);
        processNr << std::left << std::setw(25) << -1;
        processName << std::left << std::setw(25) << sigProcess;
        rate << std::left << std::setw(25) << rates[{region, sigProcess}];

        for(unsigned int i = 0; i < bkgProcesses.size(); i++){
            binNames << std::left << std::setw(25) << StrUtil::Replace(binName, "@", region);
            processNr << std::left << std::setw(25) << i+1;
            processName << std::left << std::setw(25) << bkgProcesses[i];
            rate << std::left << std::setw(25) << rates[{region, bkgProcesses[i]}];
        }
    }

    binNames << "\n"; processNr << "\n"; processName << "\n"; rate << "\n";

    datacard << binNames.rdbuf(); 
    datacard << processNr.rdbuf(); 
    datacard << processName.rdbuf(); 
    datacard << rate.rdbuf();

    datacard << border << std::endl;

    for(const std::string syst : systematics){
        if(syst == "") continue;

        std::stringstream systLine;
        systLine << std::left << std::setw(20) << syst << std::setw(10) << "lnN";

        for(const std::string& region : regions){
            systLine << std::left << std::setw(25) << StrUtil::Merge<5>(relSys[{region, sigProcess, syst, "Up"}], "/", relSys[{region, sigProcess, syst, "Down"}]);

            for(unsigned int i = 0; i < bkgProcesses.size(); i++){
                systLine << std::left << std::setw(25) << StrUtil::Merge<5>(relSys[{region, bkgProcesses[i], syst, "Up"}], "/", relSys[{region, bkgProcesses[i], syst, "Down"}]);
            }
        }

        datacard << systLine.rdbuf();
        datacard << "\n";
    }

    datacard << border << std::endl;

    for(int i = 2; i < regions.size(); ++i){
        if(regions[i] == "Misc"){
            for(const std::string& bkg : bkgProcesses){

                if(VUtil::Find(regions, bkg).empty()){
                    datacard << "SRalpha" << StrUtil::Replace(binName, "@", regions[i]) << " rateParam " << StrUtil::Replace(binName, "@", "SR") << " " << bkg << " @0 alpha" << StrUtil::Replace(binName, "@", regions[i]) << "\n";
                    datacard << "VRalpha" << StrUtil::Replace(binName, "@", regions[i]) << " rateParam " << StrUtil::Replace(binName, "@", "VR") << " " << bkg << " @0 alpha" << StrUtil::Replace(binName, "@", regions[i]) << "\n";

                    for(int j = 2; j < regions.size(); ++j){
                        datacard << "alpha" << StrUtil::Replace(binName, "@", regions[i]) << " rateParam " << StrUtil::Replace(binName, "@", regions[j]) << " " << bkg << " 1\n";
                    }
                }
            }
        }

        else{
            datacard << "SRalpha" << StrUtil::Replace(binName, "@", regions[i]) << " rateParam " << StrUtil::Replace(binName, "@", "SR") << " " << regions[i] << " @0 alpha" << StrUtil::Replace(binName, "@", regions[i]) << "\n";
            datacard << "VRalpha" << StrUtil::Replace(binName, "@", regions[i]) << " rateParam " << StrUtil::Replace(binName, "@", "VR") << " " << regions[i] << " @0 alpha" << StrUtil::Replace(binName, "@", regions[i]) << "\n";

            for(int j = 2; j < regions.size(); ++j){
                datacard << "alpha" << StrUtil::Replace(binName, "@", regions[i]) << " rateParam " << StrUtil::Replace(binName, "@", regions[j]) << " " << regions[i] << " 1\n";
            }
        }
    }

    datacard.close();
}
