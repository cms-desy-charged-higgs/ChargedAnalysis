#include <ChargedAnalysis/Analysis/include/histmaker.h>

HistMaker::HistMaker(const std::vector<std::string>& parameters, const std::vector<std::string>& regions, const std::map<std::string, std::vector<std::string>>& cutStrings, const std::map<std::string, std::string>& outDir, const std::string& outFile, const std::string &channel, const std::map<std::string, std::vector<std::string>>& systDirs, const std::vector<std::string>& scaleSysts, const int& era):
    parameters(parameters),
    regions(regions),
    cutStrings(cutStrings),
    outDir(outDir),
    outFile(outFile),
    channel(channel),
    systDirs(systDirs),
    scaleSysts(scaleSysts),
    era(era){}

void HistMaker::PrepareHists(const std::shared_ptr<TFile>& inFile, const std::shared_ptr<TTree> inTree, const std::experimental::source_location& location){
    Decoder parser;
    
    for(unsigned int i = 0; i < regions.size(); ++i){
        std::string region = regions[i];

        for(const std::string& scaleSyst : VUtil::Merge(std::vector<std::string>{""}, scaleSysts)){
            for(const std::string shift : {"Up", "Down"}){
                //Skip Down loop for nominal case
                if(scaleSyst == "" and shift == "Down") continue;

                std::string systName = scaleSyst != "" ? StrUtil::Merge(scaleSyst, shift) : "";
                std::string outName;

                //Change outdir for systematic
                if(scaleSyst == ""){
                    outName = StrUtil::Join("/", outDir[region], outFile);
                    std::system(StrUtil::Merge("mkdir -p ", outDir[region]).c_str());
                }
                else{
                    for(const std::string dir : systDirs[region]){
                        std::system(StrUtil::Merge("mkdir -p ", dir).c_str());
                        if(!StrUtil::Find(dir, systName).empty()) outName = StrUtil::Join("/", dir, outFile);
                    }
                }

                outFiles.push_back(std::make_shared<TFile>(outName.c_str(), "RECREATE"));

                //Histograms for event count
                if(scaleSyst == ""){
                    eventCount.push_back(std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1));
                    eventCount.back()->SetDirectory(outFiles.back().get());
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
                    NTupleReader paramX(inputTree, era), paramY(inputTree, era);
            
                    //Read in everything, orders matter
                    if(!StrUtil::Find(parameter, "n=N").empty()){
                        std::shared_ptr<Weighter> w = std::make_shared<Weighter>(inFile, inTree, era);
                        parser.GetParticle(parameter, paramX, w.get());
                        paramX.AddWeighter(w);
                    }

                    else parser.GetParticle(parameter, paramX);
                    parser.GetFunction(parameter, paramX);

                    paramX.Compile();

                    if(!parser.hasYInfo(parameter)){
                        std::shared_ptr<TH1F> hist1D = std::make_shared<TH1F>();
                        parser.GetBinning(parameter, hist1D.get());

                        hist1D->SetDirectory(outFiles.back().get());
                        hist1D->SetName(paramX.GetHistName().c_str());       
                        hist1D->SetTitle(paramX.GetHistName().c_str());
                        hist1D->GetXaxis()->SetTitle(paramX.GetAxisLabel().c_str());

                        if(scaleSyst == "") hists1D.push_back(std::move(hist1D));
                        else{
                            if(shift == "Up") hists1DSystUp.push_back(std::move(hist1D));
                            if(shift == "Down") hists1DSystDown.push_back(std::move(hist1D));
                        }

                        if(scaleSyst == "" and i == 0) hist1DFunctions.push_back(paramX);
                    }

                    else{
                        parser.GetParticle(parameter, paramY, nullptr, true);
                        parser.GetFunction(parameter, paramY, true);

                        paramY.Compile();

                        std::shared_ptr<TH2F> hist2D = std::make_shared<TH2F>();
                        parser.GetBinning(parameter, hist2D.get(), true);

                        hist2D->SetDirectory(outFiles.back().get());
                        hist2D->SetName((paramX.GetHistName() + "_VS_" + paramY.GetHistName()).c_str());       
                        hist2D->SetTitle((paramX.GetHistName() + "_VS_" + paramY.GetHistName()).c_str());

                        hist2D->GetXaxis()->SetTitle(paramX.GetAxisLabel().c_str());
                        hist2D->GetYaxis()->SetTitle(paramY.GetAxisLabel().c_str());

                        if(scaleSyst == "") hists2D.push_back(std::move(hist2D));
                        else{
                            if(shift == "Up") hists2DSystUp.push_back(std::move(hist2D));
                            if(shift == "Down") hists2DSystDown.push_back(std::move(hist2D));
                        }

                        if(scaleSyst == "" and i == 0) hist2DFunctions.push_back({paramX, paramY});
                    }
                }
            }
        }

        std::cout << std::endl << "Cuts for region '" << region << "'" << std::endl;

        for(const std::string& cutString: cutStrings[region]){
            //Functor structure and arguments
            NTupleReader cut(inputTree, era);

            if(!StrUtil::Find(cutString, "n=N").empty()) parser.GetParticle(cutString, cut, &weight[i]);
            else parser.GetParticle(cutString, cut);
            parser.GetFunction(cutString, cut);
            parser.GetCut(cutString, cut);
            cut.Compile();

            cutFunctions.push_back(cut);

            std::cout << "Cut will be applied: '" << cut.GetCutName() << "'" << std::endl;
        }
    }
}

void HistMaker::Produce(const std::string& fileName, const int& eventStart, const int& eventEnd, const std::string& bkgYieldFac, const std::string& bkgType, const std::vector<std::string>& bkgYieldFacSyst){
    //Get input tree
    inputFile = RUtil::Open(fileName);
    inputTree = RUtil::GetSmart<TTree>(inputFile.get(), channel);

    weight = std::vector<Weighter>(regions.size(), Weighter(inputFile, inputTree, era));

    std::cout << "Read file: '" << fileName << "'" << std::endl;
    std::cout << "Read tree '" << channel << "'" << std::endl;
    std::cout << "----------------------------------" << std::endl;

    gROOT->SetBatch(kTRUE);
    TH1::SetDefaultSumw2();

    //Open output file and set up all histograms/tree and their function to call
    PrepareHists(inputFile, inputTree);

    std::cout << "----------------------------------" << std::endl; 

    //Set all branch addresses needed for the event
    bool isData = true;

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
    
    //Cutflow/Event count histogram
    std::shared_ptr<TH1F> cutflow = RUtil::CloneSmart(RUtil::Get<TH1F>(inputFile.get(), "cutflow_" + channel));
    cutflow->SetName("cutflow"); cutflow->SetTitle("cutflow");
  //  cutflow->Scale(1./weight.GetNGen());

    StopWatch timer; 
    timer.Start();
    timer.SetTimeMark();
    int nTimeMarks = 0;

    bool allFailed = false;
    std::vector<double> wght(regions.size(), 1.), systWeight(regions.size(), 1.);
    std::vector<bool> passed(regions.size(), true);

    std::cout << std::endl << "Start processing tree at event '" << eventStart << "'" << std::endl;

    for (int i = eventStart; i < eventEnd; ++i){
        if(i % 10000 == 0 and i != eventStart){
            std::cout << "Processed events: " << i-eventStart << " (" << 10000./(timer.SetTimeMark() - timer.GetTimeMark(nTimeMarks)) << " eve/s)" << std::endl;
            ++nTimeMarks;
        }

        //Set Entry
        NTupleReader::SetEntry(i);

        //Check if event passed all cuts
        allFailed = true;
        int offSet = 0;

        for(unsigned int region = 0; region < regions.size(); ++region){
            int nCuts = cutStrings[regions[region]].size();
            passed[region] = true;

            for(unsigned int j = 0; j < nCuts; ++j){
                passed[region] = passed[region]*cutFunctions.at(j + offSet).GetPassed();

                if(!passed[region]) break;
            }

            offSet += nCuts;
            allFailed = allFailed and !passed[region];
        }

        if(allFailed) continue;
        
        //Fill histograms
        for(unsigned int syst = 0; syst <= scaleSysts.size(); ++syst){
            std::string systName = syst == 0 ? "" : scaleSysts[syst - 1];

            for(const std::string& shift : {"Up", "Down"}){
                if(syst == 0 and shift == "Down") continue;

                for(unsigned int region = 0; region < regions.size(); ++region){
                    if(!passed[region]) continue;
                    wght[region] = weight[region].GetTotalWeight(i);
                    eventCount[region]->Fill(0., wght[region]);            

                    if(syst != 0){
                        if(!bkgYieldFac.empty()) systWeight[region] = (shift == "Up" ? bkgYUp[syst] : bkgYDown[syst]) * weight[region].GetTotalWeight(i, systName, shift);
                        else systWeight[region] = weight[region].GetTotalWeight(i, systName, shift);

                        if(shift == "Up") eventCountSystUp[syst + region*(scaleSysts.size()+1)]->Fill(0., systWeight[region]);
                        else eventCountSystDown[syst + region*(scaleSysts.size()+1)]->Fill(0., systWeight[region]);
                    }
                }

                for(unsigned int j=0; j < hist1DFunctions.size(); ++j){
                    float value = hist1DFunctions.at(j).Get();
                    float indW = 1; //hist1DFunctions[j].GetWeight();

                    for(unsigned int region = 0; region < regions.size(); ++region){
                        if(!passed[region]) continue;
                        unsigned int index = j + region*hist1DFunctions.size();
        
                        if(syst == 0) hists1D[index]->Fill(value, wght[region]*indW);

                        else{
                            if(shift == "Up") hists1DSystUp[index]->Fill(value, systWeight[region]*indW);
                            else hists1DSystDown[index]->Fill(value, systWeight[region]*indW);
                        }
                    }
                }

                for(unsigned int j=0; j < hist2DFunctions.size(); ++j){
                    float value1 = hist2DFunctions[j].first.Get();
                    float value2 = hist2DFunctions[j].second.Get();
                    float indW = 1;//= hist2DFunctions[j].first.GetWeight()*hist2DFunctions[j].second.GetWeight();

                    for(unsigned int region = 0; region < regions.size(); ++region){
                        if(!passed[region]) continue;
                        unsigned int index = j + region*hist2DFunctions.size();

                        if(syst == 0)  hists2D.at(index)->Fill(value1, value2, wght[region] * indW);
                    
                        else{
                            if(shift == "Up") hists2DSystUp[index]->Fill(value1, value2, systWeight[region]*indW);
                            else hists2DSystDown[index]->Fill(value1, value2, systWeight[region]*indW);
                        }
                    }
                }              
            }
        }
    }

    std::cout << "----------------------------------" << std::endl;

    std::string currentFile = "";

    //Write all histograms
    for(std::shared_ptr<TH1F>& hist: VUtil::Merge(eventCount, hists1D)){
        if(currentFile == ""){
            currentFile = hist->GetDirectory()->GetName();
            std::cout << std::endl << "Histograms saved in file '" << currentFile << "'" << std::endl;
        }

        else if(currentFile != hist->GetDirectory()->GetName()){
            currentFile = hist->GetDirectory()->GetName();
            std::cout << std::endl << "Histograms saved in file '" << currentFile << "'" << std::endl;
        }

        hist->GetDirectory()->cd();
        hist->Write();
        std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
    }

    currentFile = "";

    //Write all histograms
    for(std::shared_ptr<TH2F>& hist: hists2D){
        if(currentFile == ""){
            currentFile = hist->GetDirectory()->GetName();
            std::cout << std::endl << "Histograms saved in file '" << currentFile << "'" << std::endl;
        }

        else if(currentFile != hist->GetDirectory()->GetName()){
            currentFile = hist->GetDirectory()->GetName();
            std::cout << std::endl << "Histograms saved in file '" << currentFile << "'" << std::endl;
        }

        hist->GetDirectory()->cd();
        hist->Write();
        std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
    }

    //eventCount->GetDirectory()->cd();
    //eventCount->Write();
    //cutflow->Write();

    for(const std::string syst : scaleSysts){
     //   eventCountSystUp[syst]->GetDirectory()->cd();
     //   eventCountSystUp[syst]->Write();

      //  eventCountSystDown[syst]->GetDirectory()->cd();
     //   eventCountSystDown[syst]->Write();

        for(std::shared_ptr<TH1F>& hist: VUtil::Merge(hists1DSystUp, hists1DSystDown)){
            hist->GetDirectory()->cd();
            hist->Write();
            std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
        }

        for(std::shared_ptr<TH2F>& hist: VUtil::Merge(hists2DSystUp, hists2DSystDown)){
            hist->GetDirectory()->cd();
            hist->Write();
            std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
        }
    }

    std::cout << std::endl << "Time passed for complete processing: " << timer.GetTime() << " s" << std::endl;
}
