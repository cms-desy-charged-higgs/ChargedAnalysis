#include <ChargedAnalysis/Analysis/include/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(const std::string &process, const std::vector<std::string> &xParameters, const std::vector<std::string> &yParameters, const std::vector<std::string> &cutStrings, const std::string &outname, const std::string &channel, const std::string& saveMode):
    process(process),
    xParameters(xParameters),
    yParameters(yParameters),
    cutStrings(cutStrings),
    outname(outname),
    channel(channel){
}

void TreeReader::PrepareLoop(TFile* outFile){
    for(std::string& xParameter: xParameters){
        //Functor structure and arguments
        Function function;
        FuncArgs arg;

        //Histogram
        std::string histName = ""; 

        for(std::string parameterInfo: Utils::SplitString<std::string>(xParameter, "/")){
            std::vector<std::string> info = Utils::SplitString<std::string>(parameterInfo, ":");

            //Which function for property to call e.g. pt -> TreeFunction::Pt
            if(info[0] == "f"){
                function.func = TreeFunction::funcMap[info[1]].first;
                histName = info[1];
            }

            if(info[0] == "p"){
                std::vector<std::string> particles = Utils::SplitString<std::string>(info[1], ",");
                for(std::string& particle: particles){
                    std::vector<std::string> part = Utils::SplitString<std::string>(particle, "_");
                    arg.parts.push_back(TreeFunction::partMap[part[0]].first);
                    arg.index.push_back(std::stoi(part[1]) - 1);
                }

                histName += "_" + Utils::Join("_", particles);
            }

            //Binning, e.g 30,0,10 -> 30 bins, xmin 0, xmax 30
            if(info[0] == "b"){
                std::vector<float> bins = Utils::SplitString<float>(info[1], ",");

                TH1F* hist = new TH1F(histName.c_str(), histName.c_str(), bins[0], bins[1], bins[2]);
                hist->SetDirectory(outFile);
                hists.push_back(hist);
            }
        }

        args.push_back(arg);
        functions.push_back(function);
    }
}

void TreeReader::EventLoop(const std::string &fileName, const int &entryStart, const int &entryEnd){
    TFile* inputFile = TFile::Open(fileName.c_str(), "READ");
    TTree* inputTree = (TTree*)inputFile->Get(channel.c_str());

    TH1::AddDirectory(kFALSE);
    gROOT->SetBatch(kTRUE);

    TFile* outFile = TFile::Open(outname.c_str(), "RECREATE");
    PrepareLoop(outFile);

    std::map<Particle, std::vector<float>*> Px, Py, Pz, E, looseSF, mediumSF, tightSF, triggerSF, recoSF, loosebTagSF, mediumbTagSF, tightbTagSF, FatJetIdx, isFromh, oneSubJettiness, twoSubJettiness, threeSubJettiness;
    float MET_Px, MET_Py, HT, eventNumber, nTrue, nGen;

    std::map<std::string, std::map<Particle, std::vector<float>*>&> properties = {
        {"Px", Px},
        {"Py", Py},
        {"Pz", Pz},
        {"E", E},
    };

    std::map<std::string, Particle> particles = {
        {"Electron", ELECTRON},
        {"Muon", MUON},
        {"Jet", JET},
        {"FatJet", FATJET},
    };

    for(std::pair<const std::string, Particle>& part: particles){
        for(std::pair<const std::string, std::map<Particle, std::vector<float>*>&> property: properties){
            std::string branchName = Utils::Join("_", {part.first, property.first});

            if(inputTree->GetListOfBranches()->Contains(branchName.c_str())){
                property.second[part.second] = new std::vector<float>();
                inputTree->SetBranchAddress(branchName.c_str(), &property.second[part.second]);
            }
        }
    }

    Event event;

    for (int i = entryStart; i < entryEnd; i++){
        //Load event and fill event class
        inputTree->GetEntry(i);

        event.particles.clear();

        for(const Particle part: {ELECTRON, MUON, JET, FATJET}){
            for(int j=0; j < Px[part]->size(); j++){
                event.particles[part].push_back(ROOT::Math::PxPyPzEVector(Px[part]->at(j), Py[part]->at(j), Pz[part]->at(j), E[part]->at(j)));
            }
        }

        for(int j=0; j < hists.size(); j++){
            float xValue = functions[j](event, args[j]);
            hists[j]->Fill(xValue);
        }
    }

    std::cout << "Loop finished" << std::endl;
    outFile->cd();
    for(TH1F* hist: hists){hist->Write(); delete hist;}

    delete inputTree; delete inputFile; delete outFile;
}
