#include <ChargedAnalysis/Analysis/include/histmaker.h>

HistMaker::HistMaker(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string& outDir, const std::string &outFile, const std::string &channel, const std::vector<std::string>& systDirs, const std::vector<std::string>& scaleSysts, const std::string& scaleFactors, const int& era):
    parameters(parameters),
    cutStrings(cutStrings),
    outDir(outDir),
    outFile(outFile),
    channel(channel),
    systDirs(systDirs),
    scaleSysts(scaleSysts),
    scaleFactors(scaleFactors),
    era(era){}

void HistMaker::PrepareHists(const std::experimental::source_location& location){
    Decoder parser;

    if(VUtil::Find(scaleSysts, std::string("")).empty()) scaleSysts.insert(scaleSysts.begin(), "");
    
    for(const std::string& scaleSyst : scaleSysts){
        for(const std::string shift : {"Up", "Down"}){
            //Skip Down loop for nominal case
            if(scaleSyst == "" and shift == "Down") continue;

            std::string systName = StrUtil::Merge(scaleSyst, shift);
            std::string outName;

            //Change outdir for systematic
            if(scaleSyst.empty()) outName = StrUtil::Join("/", outDir, outFile);
            else{
                for(const std::string dir : systDirs){
                    if(!StrUtil::Find(dir, systName).empty()) outName = StrUtil::Join("/", dir, outFile);
                }
            }

            outFiles.push_back(std::make_shared<TFile>(outName.c_str(), "RECREATE"));

            for(const std::string& parameter: parameters){
                if(StrUtil::Find(parameter, "h:").empty()) throw std::runtime_error(StrUtil::PrettyError(location, "Missing key 'h:' in parameter '", parameter, "'!"));                       

                //Functor structure and arguments
                NTupleReader paramX(inputTree, era), paramY(inputTree, era);
        
                //Read in everything, orders matter
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

                    if(scaleSyst == "") hists1D.push_back(std::move(hist1D));
                    else hists1DSyst.push_back(std::move(hist1D));

                    if(scaleSyst == "") hist1DFunctions.push_back(std::move(paramX));
                }

                else{
                    parser.GetParticle(parameter, paramY);
                    parser.GetFunction(parameter, paramY);

                    paramY.Compile();

                    std::shared_ptr<TH2F> hist2D = std::make_shared<TH2F>();
                    parser.GetBinning(parameter, hist2D.get());

                    hist2D->SetDirectory(outFiles.back().get());
                    hist2D->SetName((paramX.GetHistName() + "_VS_" + paramY.GetHistName()).c_str());       
                    hist2D->SetTitle((paramX.GetHistName() + "_VS_" + paramY.GetHistName()).c_str());

                    hist2D->GetXaxis()->SetTitle(paramX.GetAxisLabel().c_str());
                    hist2D->GetYaxis()->SetTitle(paramY.GetAxisLabel().c_str());

                    if(scaleSyst == "") hists2D.push_back(std::move(hist2D));
                    else hists2DSyst.push_back(std::move(hist2D));

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

void HistMaker::Produce(const std::string& fileName){
    //Get input tree
    inputFile = RUtil::Open(fileName);
    inputTree = RUtil::GetSmart<TTree>(inputFile.get(), channel);

    weight = Weighter(inputFile, inputTree, era);

    std::cout << "Read file: '" << fileName << "'" << std::endl;
    std::cout << "Read tree '" << channel << std::endl;

    gROOT->SetBatch(kTRUE);
    TH1::SetDefaultSumw2();

    //Open output file and set up all histograms/tree and their function to call
    PrepareHists();

    //Set all branch addresses needed for the event
    bool passed = true, isData = true;

    //Data driven scale factors
    float sf = 1.;
    if(!scaleFactors.empty()){  
        std::string fileName = StrUtil::Split(scaleFactors, "+")[0];
        std::string process = StrUtil::Split(scaleFactors, "+")[1];
      
        CSV scaleFactors(fileName, "r", "\t");
        std::vector<std::string> processes = scaleFactors.GetColumn("Process");
        sf = scaleFactors.Get<float>(VUtil::Find(processes, process).at(0), "Factor");
    }
    
    //Cutflow/Event count histogram
    std::shared_ptr<TH1F> cutflow = RUtil::GetSmart<TH1F>(inputFile.get(), "cutflow_" + channel);
    cutflow->SetName("cutflow"); cutflow->SetTitle("cutflow");
  //  cutflow->Scale(1./nGen);

    std::shared_ptr<TH1F> eventCount = std::make_shared<TH1F>("EventCount", "EventCount", 1, 0, 1);

    double wght = 1.;

    StopWatch timer; 
    timer.Start();
    timer.SetTimeMark();

    for (int i = 0; i < inputTree->GetEntries(); ++i){
        if(i % 100000 == 0 and i != 0){
            std::cout << "Processed events: " << i << " (" << 100000./(timer.SetTimeMark() - timer.GetTimeMark(i/100000 -1)) << " eve/s)" << std::endl;
        }

        //Set Entry
        NTupleReader::SetEntry(i);

        //Check if event passed all cuts
        passed = true;
        wght = weight.GetBaseWeight(i) * sf;

        for(int j=0; j < cutFunctions.size(); j++){
            passed *= cutFunctions[j].GetPassed();

            if(passed){
                cutflow->Fill(cutFunctions[j].GetCutName().c_str(), wght);
            }

            else break;
        }

        if(!passed) continue;

        wght *= weight.GetPartWeight(i);
        
        //Fill nominal histogram
        for(int j=0; j < hist1DFunctions.size(); j++){
            hists1D[j]->Fill(hist1DFunctions[j].Get(), wght);
        }

        for(int j=0; j < hist2DFunctions.size(); j++){
            hists2D[j]->Fill(hist2DFunctions[j].first.Get(), hist2DFunctions[j].second.Get(), wght);
        }

        eventCount->Fill(0., wght);
    }

    //Write all histograms and delete everything
    for(std::shared_ptr<TH1F>& hist: VUtil::Merge(hists1D, hists1DSyst)){
        hist->GetDirectory()->cd();
        hist->Write();
        std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
    }

    for(std::shared_ptr<TH2F>& hist: VUtil::Merge(hists2D, hists2DSyst)){
        hist->GetDirectory()->cd();
        hist->Write();
        std::cout << "Saved histogram: '" << hist->GetName() << "' with " << hist->GetEntries() << " entries" << std::endl;
    }

    //Write cutflow/eventcount
    VUtil::At(outFiles, 0)->cd();

    cutflow->Write();
    eventCount->Write();

    std::cout << "Closed output file: '" << outFile << "'" << std::endl;
    std::cout << "Time passed for complete processing: " << timer.GetTime() << " s" << std::endl;
}
