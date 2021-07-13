#include <ChargedAnalysis/Analysis/include/histmaker.h>

HistMaker::HistMaker(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string& outDir, const std::string &outFile, const std::string &channel, const std::vector<std::string>& systDirs, const std::vector<std::string>& scaleSysts, const int& era):
    parameters(parameters),
    cutStrings(cutStrings),
    outDir(outDir),
    outFile(outFile),
    channel(channel),
    systDirs(systDirs),
    scaleSysts(scaleSysts),
    era(era){}

void HistMaker::PrepareHists(const std::shared_ptr<TFile>& inFile, const std::shared_ptr<TTree> inTree, const std::experimental::source_location& location){
    Decoder parser;
    
    for(const std::string& scaleSyst : VUtil::Merge(std::vector<std::string>{""}, scaleSysts)){
        for(const std::string shift : {"Up", "Down"}){
            //Skip Down loop for nominal case
            if(scaleSyst == "" and shift == "Down") continue;

            std::string systName = scaleSyst != "" ? StrUtil::Merge(scaleSyst, shift) : "";
            std::string outName;

            //Change outdir for systematic
            if(scaleSyst == "") outName = StrUtil::Join("/", outDir, outFile);
            else{
                for(const std::string dir : systDirs){
                    std::system(StrUtil::Merge("mkdir -p ", dir).c_str());
                    if(!StrUtil::Find(dir, systName).empty()) outName = StrUtil::Join("/", dir, outFile);
                }
            }

            outFiles.push_back(std::make_shared<TFile>(outName.c_str(), "RECREATE"));

            //Histograms for event count
            if(scaleSyst == ""){
                eventCount = std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1);
                eventCount->SetDirectory(outFiles.back().get());
            }

            else{
                if(shift == "Up"){
                    eventCountSystUp[scaleSyst] = std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1);
                    eventCountSystUp[scaleSyst]->SetDirectory(outFiles.back().get());
                }

                if(shift == "Down"){
                    eventCountSystDown[scaleSyst] = std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1);
                    eventCountSystDown[scaleSyst]->SetDirectory(outFiles.back().get());
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
                        if(shift == "Up") hists1DSystUp[scaleSyst].push_back(std::move(hist1D));
                        if(shift == "Down") hists1DSystDown[scaleSyst].push_back(std::move(hist1D));
                    }

                    if(scaleSyst == "") hist1DFunctions.push_back(paramX);
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
                        if(shift == "Up") hists2DSystUp[scaleSyst].push_back(std::move(hist2D));
                        if(shift == "Down") hists2DSystDown[scaleSyst].push_back(std::move(hist2D));
                    }

                    if(scaleSyst == "") hist2DFunctions.push_back({paramX, paramY});
                }
            }
        }
    }

    for(const std::string& cutString: cutStrings){
        //Functor structure and arguments
        NTupleReader cut(inputTree, era);

        if(!StrUtil::Find(cutString, "n=N").empty()) parser.GetParticle(cutString, cut, &weight);
        else parser.GetParticle(cutString, cut);
        parser.GetFunction(cutString, cut);
        parser.GetCut(cutString, cut);
        cut.Compile();

        cutFunctions.push_back(cut);

        std::cout << "Cut will be applied: '" << cut.GetCutName() << "'" << std::endl;
    }
}

void HistMaker::Produce(const std::string& fileName, const int& eventStart, const int& eventEnd, const std::string& bkgYieldFac, const std::string& bkgType, const std::vector<std::string>& bkgYieldFacSyst){
    //Get input tree
    inputFile = RUtil::Open(fileName);
    inputTree = RUtil::GetSmart<TTree>(inputFile.get(), channel);

    weight = Weighter(inputFile, inputTree, era);

    std::cout << "Read file: '" << fileName << "'" << std::endl;
    std::cout << "Read tree '" << channel << std::endl;

    gROOT->SetBatch(kTRUE);
    TH1::SetDefaultSumw2();

    //Open output file and set up all histograms/tree and their function to call
    PrepareHists(inputFile, inputTree);

    //Set all branch addresses needed for the event
    bool passed = true, isData = true;

    //Data driven scale factors
    float bkgY = 1.;
    std::map<std::string, float> bkgYUp, bkgYDown;

    if(!bkgYieldFac.empty()){
        for(const std::string& scaleSyst : VUtil::Merge(std::vector<std::string>{""}, scaleSysts)){
            for(const std::string& shift : {"Up", "Down"}){
                std::string fName = bkgYieldFac;
                std::string systName = scaleSyst != "" ? StrUtil::Merge(scaleSyst, shift) : "";

                for(const std::string file : bkgYieldFacSyst){
                    if(!StrUtil::Find(file, systName).empty()){
                        fName = file;
                        break;
                    }
                }

                CSV bkgYieldFile(fName, "r", "\t");

                std::vector<std::string> processes = bkgYieldFile.GetColumn("Process");

                if(scaleSyst == ""){
                    bkgY = bkgYieldFile.Get<float>(VUtil::Find(processes, bkgType).at(0), "Factor");
                }

                else{
                    if(shift == "Up") bkgYUp[scaleSyst] = bkgYieldFile.Get<float>(VUtil::Find(processes, bkgType).at(0), "Factor");
                    else bkgYDown[scaleSyst] = bkgYieldFile.Get<float>(VUtil::Find(processes, bkgType).at(0), "Factor");
                }
            }
        }
    }
    
    //Cutflow/Event count histogram
    std::shared_ptr<TH1F> cutflow = RUtil::CloneSmart(RUtil::Get<TH1F>(inputFile.get(), "cutflow_" + channel));
    cutflow->SetName("cutflow"); cutflow->SetTitle("cutflow");
    cutflow->Scale(1./weight.GetNGen());

    double wght = 1., systWeight = 1.;

    StopWatch timer; 
    timer.Start();
    timer.SetTimeMark();
    int nTimeMarks = 0;

    for (int i = eventStart; i < eventEnd; ++i){
        if(i % 10000 == 0 and i != eventStart){
            std::cout << "Processed events: " << i-eventStart << " (" << 10000./(timer.SetTimeMark() - timer.GetTimeMark(nTimeMarks)) << " eve/s)" << std::endl;
            ++nTimeMarks;
        }

        //Set Entry
        NTupleReader::SetEntry(i);

        //Check if event passed all cuts
        passed = true;
        wght = weight.GetBaseWeight(i) * bkgY;

        for(int j=0; j < cutFunctions.size(); j++){
            passed *= cutFunctions[j].GetPassed();

            if(passed){
                cutflow->Fill(cutFunctions[j].GetCutName().c_str(), wght);
            }

            else break;
        }

        if(!passed) continue;

        wght *= weight.GetPartWeight(i);
        eventCount->Fill(0., wght);
        
        //Fill histograms
        for(const std::string& syst : VUtil::Merge(std::vector<std::string>{""}, scaleSysts)){
            for(const std::string& shift : {"Up", "Down"}){
                if(syst == "" and shift == "Down") continue;

                if(syst != ""){
                    if(!bkgYieldFac.empty()) systWeight = (shift == "Up" ? bkgYUp[syst] : bkgYDown[syst]) * weight.GetTotalWeight(i, syst, shift);
                    else systWeight = weight.GetTotalWeight(i, syst, shift);

                    if(shift == "Up") eventCountSystUp[syst]->Fill(0., systWeight);
                    else eventCountSystDown[syst]->Fill(0., systWeight);
                }

                for(int j=0; j < hist1DFunctions.size(); ++j){
                    float value = hist1DFunctions[j].Get();

                    if(syst == "") hists1D[j]->Fill(value, wght * hist1DFunctions[j].GetWeight());

                    else{
                        if(shift == "Up") hists1DSystUp.at(syst)[j]->Fill(value, systWeight);
                        else hists1DSystDown.at(syst)[j]->Fill(value, systWeight);
                    }
                }

                for(int j=0; j < hist2DFunctions.size(); ++j){
                    float value1 = hist2DFunctions[j].first.Get();
                    float value2 = hist2DFunctions[j].second.Get();

                    if(syst == "")  hists2D.at(j)->Fill(value1, value2, wght * hist2DFunctions[j].first.GetWeight());
            
                    else{
                        if(shift == "Up") hists2DSystUp.at(syst)[j]->Fill(value1, value2, systWeight);
                        else hists2DSystDown.at(syst)[j]->Fill(value1, value2, systWeight);
                    }
                }
            }
        }
    }

    //Write all histograms
    for(std::shared_ptr<TH1F>& hist: hists1D){
        hist->GetDirectory()->cd();
        hist->Write();
        std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
    }

    //Write all histograms
    for(std::shared_ptr<TH2F>& hist: hists2D){
        hist->GetDirectory()->cd();
        hist->Write();
        std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
    }

    eventCount->GetDirectory()->cd();
    eventCount->Write();
    cutflow->Write();

    for(const std::string syst : scaleSysts){
        eventCountSystUp[syst]->GetDirectory()->cd();
        eventCountSystUp[syst]->Write();

        eventCountSystDown[syst]->GetDirectory()->cd();
        eventCountSystDown[syst]->Write();

        for(std::shared_ptr<TH1F>& hist: VUtil::Merge(hists1DSystUp[syst], hists1DSystDown[syst])){
            hist->GetDirectory()->cd();
            hist->Write();
            std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
        }

        for(std::shared_ptr<TH2F>& hist: VUtil::Merge(hists2DSystUp[syst], hists2DSystUp[syst])){
            hist->GetDirectory()->cd();
            hist->Write();
            std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
        }
    }

    std::cout << "Time passed for complete processing: " << timer.GetTime() << " s" << std::endl;
}
