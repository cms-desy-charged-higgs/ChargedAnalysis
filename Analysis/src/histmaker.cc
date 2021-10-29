#include <ChargedAnalysis/Analysis/include/histmaker.h>

HistMaker::HistMaker(const std::vector<std::string>& parameters, const std::vector<std::string>& regions, const std::map<std::string, std::vector<std::string>>& cutStrings, const std::map<std::string, std::string>& outDir, const std::string& outFile, const std::string &channel, const std::map<std::string, std::vector<std::string>>& systDirs, const std::vector<std::string>& scaleSysts, const std::string& fakeRateFile, const std::string& promptRateFile, const int& era):
    parameters(parameters),
    regions(regions),
    cutStrings(cutStrings),
    outDir(outDir),
    outFile(outFile),
    channel(channel),
    systDirs(systDirs),
    scaleSysts(scaleSysts),
    fakeRateFile(fakeRateFile),
    promptRateFile(promptRateFile),
    era(era){}

void HistMaker::PrepareHists(const std::shared_ptr<TFile>& inFile, const std::shared_ptr<TTree> inTree, NTupleReader& reader, const std::experimental::source_location& location){
    Decoder parser;
    bool isData = !RUtil::BranchExists(inTree.get(), "Electron_GenID");

    for(unsigned int i = 0; i < regions.size(); ++i){
        std::string region = regions[i];

        for(const std::string& scaleSyst : VUtil::Merge(std::vector<std::string>{"Nominal"}, scaleSysts)){
            for(const std::string shift : {"Up", "Down"}){
                //Skip Down loop for nominal case
                if(scaleSyst == "Nominal" and shift == "Down") continue;

                std::string systName = scaleSyst != "Nominal" ? StrUtil::Merge(scaleSyst, shift) : "Nominal";
                std::string outName, outNameForMisIDJ;

                //Change outdir for systematic
                if(scaleSyst == "Nominal"){
                    outName = StrUtil::Join("/", outDir[region], outFile);
                    std::system(StrUtil::Merge("mkdir -p ", outDir[region]).c_str());

                    if(isMisIDJ){
                        std::string procToReplace = !StrUtil::Find(channel, "Muon").empty() ? "SingleMu" : "SingleE";
                        outNameForMisIDJ = StrUtil::Replace(StrUtil::Join("/", outDir[region], outFile), procToReplace, "MisIDJ", "MisIDJ");
                        std::system(StrUtil::Merge("mkdir -p ", StrUtil::Replace(outDir[region], procToReplace, "MisIDJ")).c_str());
                    }
                }
                else{
                    for(const std::string dir : systDirs[region]){
                        std::system(StrUtil::Merge("mkdir -p ", dir).c_str());
                        if(!StrUtil::Find(dir, systName).empty()) outName = StrUtil::Join("/", dir, outFile);
                    }
                }

                outFiles.push_back(std::make_shared<TFile>(outName.c_str(), "RECREATE"));
                if(isMisIDJ) outFilesForMisIDJ.push_back(std::make_shared<TFile>(outNameForMisIDJ.c_str(), "RECREATE"));

                //Histograms for event count
                if(scaleSyst == "Nominal"){
                    eventCount.push_back(std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1));
                    eventCount.back()->SetDirectory(outFiles.back().get());

                    if(isMisIDJ){
                        eventCountMisIDJ.push_back(std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1));
                        eventCountMisIDJ.back()->SetDirectory(outFilesForMisIDJ.back().get());
                    }
                }

                else{
                    if(shift == "Up"){
                        eventCountSystUp.push_back(std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1));
                        eventCountSystUp.back()->SetDirectory(outFiles.back().get());
                    }

                    if(shift == "Down"){
                        eventCountSystDown.push_back(std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1));
                        eventCountSystDown.back()->SetDirectory(outFiles.back().get());
                    }
                }

                //Histograms for other parameters
                for(const std::string& parameter: parameters){
                    if(StrUtil::Find(parameter, "h:").empty()) throw std::runtime_error(StrUtil::PrettyError(location, "Missing key 'h:' in parameter '", parameter, "'!"));

                    //Functor structure and arguments
                    NTupleFunction paramX = reader.BuildFunc(), paramY = reader.BuildFunc();
            
                    //Read in everything, orders matter
                    if(!StrUtil::Find(parameter, "getSF").empty() and scaleSyst == "Nominal" and i == 0){
                        Weighter w(inFile, inTree, era);
                        parser.GetParticle(parameter, reader, w);
                        histPartWeight[hist1DFunctions.size()] = w;
                    }

                    parser.GetParticle(parameter, paramX);
                    parser.GetFunction(parameter, paramX);

                    paramX.Compile();

                    if(!parser.hasYInfo(parameter)){
                        std::shared_ptr<TH1F> hist1D = std::make_shared<TH1F>();
                        parser.GetBinning(parameter, hist1D.get());

                        hist1D->SetDirectory(outFiles.back().get());
                        hist1D->SetName(paramX.GetHistName().c_str());       
                        hist1D->SetTitle(paramX.GetHistName().c_str());
                        hist1D->GetXaxis()->SetTitle(paramX.GetAxisLabel().c_str());

                        if(scaleSyst == "Nominal"){
                            if(isMisIDJ) hists1DMisIDJ.push_back(RUtil::CloneSmart(hist1D.get()));
                            hists1D.push_back(std::move(hist1D));
                        }

                        else{
                            if(shift == "Up"){
                                hists1DSystUp.push_back(std::move(hist1D));
                            }
                            if(shift == "Down"){
                                hists1DSystDown.push_back(std::move(hist1D));
                            }
                        }

                        if(scaleSyst == "Nominal" and i == 0) hist1DFunctions.push_back(paramX);
                    }

                    else{
                        parser.GetParticle(parameter, paramY, true);
                        parser.GetFunction(parameter, paramY, true);

                        paramY.Compile();

                        std::shared_ptr<TH2F> hist2D = std::make_shared<TH2F>();
                        parser.GetBinning(parameter, hist2D.get(), true);

                        hist2D->SetDirectory(outFiles.back().get());
                        hist2D->SetName((paramX.GetHistName() + "_VS_" + paramY.GetHistName()).c_str());       
                        hist2D->SetTitle((paramX.GetHistName() + "_VS_" + paramY.GetHistName()).c_str());

                        hist2D->GetXaxis()->SetTitle(paramX.GetAxisLabel().c_str());
                        hist2D->GetYaxis()->SetTitle(paramY.GetAxisLabel().c_str());

                        if(scaleSyst == "Nominal"){
                            if(isMisIDJ) hists2DMisIDJ.push_back(RUtil::CloneSmart(hist2D.get()));
                            hists2D.push_back(std::move(hist2D));
                        }
                        else{
                            if(shift == "Up"){
                                hists2DSystUp.push_back(std::move(hist2D));
                            }
                            if(shift == "Down"){
                                hists2DSystDown.push_back(std::move(hist2D));
                            }
                        }

                        if(scaleSyst == "Nominal" and i == 0) hist2DFunctions.push_back({paramX, paramY});
                    }
                }
            }
        }

        std::cout << std::endl << "Cuts for region '" << region << "'" << std::endl;

        int nCuts = 0;

        for(const std::string& cutString: cutStrings[region]){
            //Functor structure and arguments
            NTupleFunction cut = reader.BuildFunc();
            
            try{
                parser.GetParticle(cutString, cut);
                parser.GetFunction(cutString, cut);
                parser.GetCut(cutString, cut);
                cut.Compile();
            }

            catch(const std::exception& e){
                if(isData) continue;
                else throw std::runtime_error(e.what());
            }

            if(!StrUtil::Find(cutString, "getSF").empty()){
                Weighter w(inFile, inTree, era);
                parser.GetParticle(cutString, reader, w);
                cutPartWeight[cutFunctions.size()] = std::move(w);
            }

            std::cout << "Cut will be applied: '" << cut.GetCutName() << "'" << std::endl;

            cutFunctions.push_back(std::move(cut));
            ++nCuts;
        }

        cutIdxRange.push_back(nCuts + (cutIdxRange.size() == 0 ? 0 : cutIdxRange.back()));
    }
}

void HistMaker::Produce(const std::string& fileName, const int& eventStart, const int& eventEnd, const std::string& bkgYieldFac, const std::string& bkgType, const std::vector<std::string>& bkgYieldFacSyst){
    //Get input tree
    inputFile = RUtil::Open(fileName);
    inputTree = RUtil::GetSmart<TTree>(inputFile.get(), channel);
    NTupleReader reader(inputTree, era);
    
    baseWeight = Weighter(inputFile, inputTree, era);

    std::cout << "Read file: '" << fileName << "'" << std::endl;
    std::cout << "Read tree '" << channel << "'" << std::endl;
    std::cout << "----------------------------------" << std::endl;

    gROOT->SetBatch(kTRUE);
    TH1::SetDefaultSumw2();

    //Open output file and set up all histograms/tree and their function to call
    isMisIDJ = promptRateFile != "";
    PrepareHists(inputFile, inputTree, reader);

    std::cout << "----------------------------------" << std::endl; 

    //Data driven scale factors
    float bkgY = 1.;
    std::vector<float> bkgYUp, bkgYDown;

    if(!bkgYieldFac.empty()){
        for(unsigned int syst = 0; syst <= scaleSysts.size(); ++syst){
            for(const std::string& shift : {"Up", "Down"}){
                std::string fName = bkgYieldFac;
                std::string systName = syst != 0 ? StrUtil::Merge(scaleSysts[syst], shift) : "";

                for(const std::string file : bkgYieldFacSyst){
                    if(!StrUtil::Find(file, systName).empty()){
                        fName = file;
                        break;
                    }
                }

                CSV bkgYieldFile(fName, "r", "\t");

                std::vector<std::string> processes = bkgYieldFile.GetColumn("Process");

                if(syst == 0){
                    bkgY = bkgYieldFile.Get<float>(VUtil::Find(processes, bkgType).at(0), "Factor");
                }

                else{
                    if(shift == "Up") bkgYUp.push_back(bkgYieldFile.Get<float>(VUtil::Find(processes, bkgType).at(0), "Factor"));
                    else bkgYDown.push_back(bkgYieldFile.Get<float>(VUtil::Find(processes, bkgType).at(0), "Factor"));
                }
            }
        }
    }

    //Prepare QCD estimatation if wished
    std::shared_ptr<TFile> fFile, pFile;
    std::shared_ptr<TH2F> promptRate, fakeRate;
    NTupleFunction lPt = reader.BuildFunc(), lEta = reader.BuildFunc();
    float fRate, pRate, wLoose = 1., wTight = 1.;
    std::function<float(float&, float&)> f1 = [&](float& f, float& p){return (f*p)/(p-f);};

    std::function<float(float&)> f2 = [&](float& p){return (1.-p)/p;};

    if(fakeRateFile != ""){
        fFile = RUtil::Open(fakeRateFile);
        pFile = RUtil::Open(promptRateFile);

        fakeRate = RUtil::CloneSmart(RUtil::Get<TH2F>(fFile.get(), "fakerate"));
        promptRate = RUtil::CloneSmart(RUtil::Get<TH2F>(pFile.get(), "promptrate"));

        Decoder parser;

        lPt.AddParticle(!StrUtil::Find(channel, "Ele").empty() ? "e" : "mu", 1, "loose");
        lPt.AddFunction("pt");
        lPt.Compile();

        lEta.AddParticle(!StrUtil::Find(channel, "Ele").empty() ? "e" : "mu", 1, "loose");
        lEta.AddFunction("eta");
        lEta.Compile();
    }
    
    //Cutflow/Event count histogram
    std::vector<std::shared_ptr<TH1F>> cutflows;

    for(unsigned int region = 0; region < regions.size(); ++region){
        cutflows.push_back(RUtil::CloneSmart(RUtil::Get<TH1F>(inputFile.get(), "Cutflow_" + channel)));
        cutflows.back()->SetName("cutflow"); cutflows.back()->SetTitle("cutflow");
    }

    StopWatch timer; 
    timer.Start();
    timer.SetTimeMark();
    int nTimeMarks = 0;

    std::vector<bool> passed(regions.size(), true);
    bool allFailed = false;
    double wght = 1., wghtMisIDJ = 1.;;

    std::vector<float> values1D(hist1DFunctions.size(), 1.);
    std::vector<std::pair<float, float>> values2D(hist2DFunctions.size(), {1., 1.});

    std::cout << std::endl << "Start processing tree at event '" << eventStart << "'" << std::endl;

    for (int entry = eventStart; entry < eventEnd; ++entry){
        if(entry % 10000 == 0 and entry != eventStart){
            std::cout << "Processed events: " << entry-eventStart << " (" << 10000./(timer.SetTimeMark() - timer.GetTimeMark(nTimeMarks)) << " eve/s)" << std::endl;
            ++nTimeMarks;
        }

        //Set entry
        reader.SetEntry(entry);

        //Check if event passed all cuts
        allFailed = true;

        for(unsigned int region = 0; region < regions.size(); ++region){
            passed[region] = true;

            for(unsigned int j = region == 0 ? 0 : cutIdxRange[region-1]; j < cutIdxRange[region]; ++j){
                passed[region] = passed[region] and cutFunctions.at(j).GetPassed();
                if(!passed[region]) break;
            }

            allFailed = allFailed and !passed[region];
        }

        if(allFailed) continue;

        if(fakeRate){
            float lpt = lPt.Get();
            float leta = lEta.Get();

            fRate = fakeRate->GetBinContent(fakeRate->FindBin(lpt, leta));
            pRate = promptRate->GetBinContent(promptRate->FindBin(lpt, leta));

            if(fRate == 0) fRate = fakeRate->GetBinContent(fakeRate->FindBin(79.9, leta));
            if(pRate == 0) pRate = promptRate->GetBinContent(promptRate->FindBin(199.9, leta));

            wLoose = f1(fRate, pRate);
            wTight = wLoose * f2(pRate);
        }
        
        //Fill histograms
        for(unsigned int j=0; j < hist1DFunctions.size(); ++j){
            values1D[j] = hist1DFunctions.at(j).Get();
        }

        for(unsigned int j=0; j < hist2DFunctions.size(); ++j){
            values2D[j] = {hist2DFunctions[j].first.Get(), hist2DFunctions[j].second.Get()};
        }

        for(unsigned int syst = 0; syst <= scaleSysts.size(); ++syst){
            std::string systName = syst == 0 ? "Nominal" : scaleSysts[syst - 1];

            for(const std::string& shift : {"Up", "Down"}){
                if(syst == 0 and shift == "Down") continue;

                for(unsigned int region = 0; region < regions.size(); ++region){
                    if(!passed[region]) continue;
                    wght = baseWeight.GetBaseWeight(entry, systName == "PileUp" ? shift : "") * bkgY;
                    wghtMisIDJ = (isMisIDJ and region % 2 == 0 ? wTight : wLoose);

                    for(unsigned int j = region == 0 ? 0 : cutIdxRange[region-1]; j < cutIdxRange[region]; ++j){
                        wght *= cutPartWeight.count(j) ? cutPartWeight.at(j).GetPartWeight(entry, systName, shift) : 1.;
                    }

                    if(syst == 0){
                        eventCount[region]->Fill(0., wght);
                        if(isMisIDJ) eventCountMisIDJ[region]->Fill(0., wghtMisIDJ);
                    }

                    else{
                        if(shift == "Up") eventCountSystUp[(syst - 1) + region*(scaleSysts.size())]->Fill(0., wght);
                        else eventCountSystDown[(syst - 1) + region*(scaleSysts.size())]->Fill(0., wght);
                    }

                    for(unsigned int j=0; j < hist1DFunctions.size(); ++j){
                        if(!passed[region]) continue;

                        unsigned int index = j + hist1DFunctions.size()*region;
                        unsigned int systIndex = j + hist1DFunctions.size()*((syst - 1) + scaleSysts.size()*region);

                        float pW = histPartWeight.count(j) ? histPartWeight[j].GetPartWeight(entry) : 1.;
        
                        if(syst == 0){
                            hists1D.at(index)->Fill(values1D[j], wght*pW);
                            if(isMisIDJ) hists1DMisIDJ.at(index)->Fill(values1D[j], wghtMisIDJ);
                        }

                        else{
                            if(shift == "Up") hists1DSystUp.at(systIndex)->Fill(values1D[j], wght*pW);
                            else hists1DSystDown.at(systIndex)->Fill(values1D[j], wght*pW);
                        }   
                    }

                    for(unsigned int j=0; j < hist2DFunctions.size(); ++j){
                        if(!passed[region]) continue;

                        unsigned int index = j + region*hist2DFunctions.size();
                        unsigned int systIndex = j + hist1DFunctions.size()*(region + regions.size()*syst);

                        if(syst == 0){
                            hists2D.at(index)->Fill(values2D[j].first, values2D[j].second, wght);
                            if(isMisIDJ) hists2DMisIDJ.at(index)->Fill(values2D[j].first, values2D[j].second, wghtMisIDJ);
                        }

                        else{
                            if(shift == "Up") hists2DSystUp.at(systIndex)->Fill(values2D[j].first, values2D[j].second, wght);
                            else hists2DSystDown.at(systIndex)->Fill(values2D[j].first, values2D[j].second, wght);
                        }   
                    }
                }
            }
        }
    }

    std::cout << "----------------------------------" << std::endl;

    for(unsigned int region = 0; region < regions.size(); ++region){
        if(isMisIDJ and region % 2 != 0) continue;

        eventCount[region]->GetDirectory()->cd();
        eventCount[region]->Write();

        std::cout << std::endl << "Histograms saved in file '" << eventCount[region]->GetDirectory()->GetName() << "'" << std::endl;

        if(isMisIDJ){
            eventCountMisIDJ[region]->GetDirectory()->cd();

            std::shared_ptr<TH1F> hist = RUtil::CloneSmart(eventCountMisIDJ[region + 1].get());
            hist->Add(eventCountMisIDJ[region].get(), -1);
            hist->Write();

            std::cout << std::endl << "Histograms saved in file '" << eventCountMisIDJ[region]->GetDirectory()->GetName() << "'" << std::endl;
        }

        //Write all histograms
        for(unsigned int j=0; j < hist1DFunctions.size(); ++j){
            unsigned int index = j + region*hist1DFunctions.size();

            hists1D.at(index)->GetDirectory()->cd();
            hists1D.at(index)->Write();

            std::cout << "Saved histogram: '" << hists1D.at(index)->GetName() << "' with " << hists1D.at(index)->GetEntries() << " entries" << std::endl;

            if(isMisIDJ){
                hists1DMisIDJ.at(index)->GetDirectory()->cd();
                std::shared_ptr<TH1F> hist = RUtil::CloneSmart(hists1DMisIDJ.at(j + (region + 1)*hist1DFunctions.size()).get());
                hist->Add(hists1DMisIDJ.at(index).get(), -1);

                hist->Write();
            }
        }

        for(unsigned int j=0; j < hist2DFunctions.size(); ++j){
            unsigned int index = j + region*hist2DFunctions.size();

            hists2D.at(index)->GetDirectory()->cd();
            hists2D.at(index)->Write();

            std::cout << "Saved histogram: '" << hists2D.at(index)->GetName() << "' with " << hists2D.at(index)->GetEntries() << " entries" << std::endl;

            if(isMisIDJ){
                hists2DMisIDJ.at(index)->GetDirectory()->cd();
                std::shared_ptr<TH2F> hist = RUtil::CloneSmart(hists2DMisIDJ.at(j + (region + 1)*hist2DFunctions.size()).get());
                hist->Add(hists2DMisIDJ.at(index).get(), -1);

                hist->Write();
            }
        }

        for(int syst = 0; syst < scaleSysts.size(); ++syst){
            for(const std::string shift : {"Up", "Down"}){
                std::shared_ptr<TH1F> count = shift == "Up" ? eventCountSystUp[syst + scaleSysts.size()*region] : eventCountSystDown[syst + scaleSysts.size()*region];

                std::cout << std::endl << "Histograms saved in file '" << count->GetDirectory()->GetName() << "'" << std::endl;
                count->GetDirectory()->cd();
                count->Write();

                for(unsigned int j=0; j < hist1DFunctions.size(); ++j){
                    unsigned int index = j + hist1DFunctions.size()*(syst + scaleSysts.size()*region);

                    std::shared_ptr<TH1F> hist = shift == "Up" ? hists1DSystUp.at(index) : hists1DSystDown.at(index);

                    hist->GetDirectory()->cd();
                    hist->Write();
                    std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
                }

                for(unsigned int j=0; j < hist2DFunctions.size(); ++j){
                    unsigned int index = j + hist2DFunctions.size()*(syst + scaleSysts.size()*region);

                    std::shared_ptr<TH2F> hist = shift == "Up" ? hists2DSystUp.at(index) : hists2DSystUp.at(index);

                    hist->GetDirectory()->cd();
                    hist->Write();
                    std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
                }
            }
        }
    }

    std::cout << std::endl << "Time passed for complete processing: " << timer.GetTime() << " s" << std::endl;
}
