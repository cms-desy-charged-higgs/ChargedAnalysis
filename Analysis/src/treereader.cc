#include <ChargedAnalysis/Analysis/include/treereader.h>

//Constructor

TreeReader::TreeReader(){}

TreeReader::TreeReader(const std::vector<std::string> &parameters, const std::vector<std::string> &cutStrings, const std::string& outDir, const std::string &outFile, const std::string &channel, const std::vector<std::string>& systDirs, const std::vector<std::string>& scaleSysts, const int& era):
    parameters(parameters),
    cutStrings(cutStrings),
    outDir(outDir),
    outFile(outFile),
    channel(channel),
    systDirs(systDirs),
    scaleSysts(scaleSysts),
    era(era){}

void TreeReader::PrepareLoop(){
    TreeParser parser;

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
                //Functor structure and arguments
                TreeFunction function(inputFile, inputTree->GetName(), era);
        
                //Read in everything, orders matter
                parser.GetParticle(parameter, function);
                parser.GetFunction(parameter, function);

                if(!StrUtil::Find(parameter, "h:").empty()){                          
                    if(!function.hasYAxis()){
                        std::shared_ptr<TH1F> hist1D = std::make_shared<TH1F>();
                        parser.GetBinning(parameter, hist1D.get());

                        hist1D->SetDirectory(outFiles.back().get());
                        hist1D->SetName((function.GetName<Axis::X>()).c_str());       
                        hist1D->SetTitle((function.GetName<Axis::X>()).c_str());
                        hist1D->GetXaxis()->SetTitle(function.GetAxisLabel<Axis::X>().c_str());

                        if(scaleSyst == "") hists1D.push_back(std::move(hist1D));
                        else hists1DSyst.push_back(std::move(hist1D));

                        if(scaleSyst == "") hist1DFunctions.push_back(function);
                    }

                    else{
                        std::shared_ptr<TH2F> hist2D = std::make_shared<TH2F>();
                        parser.GetBinning(parameter, hist2D.get());

                        hist2D->SetDirectory(outFiles.back().get());
                        hist2D->SetName((function.GetName<Axis::X>() + "_VS_" + function.GetName<Axis::Y>()).c_str());       
                        hist2D->SetTitle((function.GetName<Axis::X>() + "_VS_" + function.GetName<Axis::Y>()).c_str());

                        hist2D->GetXaxis()->SetTitle(function.GetAxisLabel<Axis::X>().c_str());
                        hist2D->GetYaxis()->SetTitle(function.GetAxisLabel<Axis::Y>().c_str());

                        if(scaleSyst == "") hists2D.push_back(std::move(hist2D));
                        else hists2DSyst.push_back(std::move(hist2D));

                        if(scaleSyst == "") hist2DFunctions.push_back(function);    
                    }
                }
  
                if(!StrUtil::Find(parameter, "t:").empty() and scaleSyst == ""){
                    if(outTree == nullptr){
                        outTree = std::make_shared<TTree>(channel.c_str(), channel.c_str());
                        outTree->SetDirectory(outFiles.back().get());
                    }

                    branchNames.push_back(function.GetName<Axis::X>());
                    treeValues.push_back(1.);

                    treeFunctions.push_back(function);
                }

                if(!StrUtil::Find(parameter, "csv:").empty()){
                    CSVNames.push_back(function.GetName<Axis::X>());

                    CSVFunctions.push_back(function);
                }
            }
        }
    }

    //Declare branches of output tree if wished
    for(int i=0; i < branchNames.size(); i++){
        outTree->Branch(branchNames[i].c_str(), &treeValues[i]);
    }

    //Declare columns in CSV file if wished
    if(!CSVNames.empty()){
        frame = std::make_shared<Frame>();
        frame->InitLabels(CSVNames);
    }

    for(const std::string& cut: cutStrings){
        //Functor structure and arguments
        TreeFunction function(inputFile, inputTree->GetName());

        parser.GetParticle(cut, function);
        parser.GetFunction(cut, function);
        parser.GetCut(cut, function);

        function.SetSystematics(scaleSysts);
        cutFunctions.push_back(function);

        std::cout << "Cut will be applied: '" << function.GetCutLabel() << "'" << std::endl;
    }
}

void TreeReader::EventLoop(const std::string &fileName, const std::string& cleanJet){
    //Take time
    Utils::RunTime timer;

    //Get input tree
    inputFile.reset(Utils::CheckNull<TFile>(TFile::Open(fileName.c_str(), "READ")));
    inputTree.reset(Utils::CheckNull<TTree>(inputFile->Get<TTree>(channel.c_str())));

    TLeaf* nTrueInter = inputTree->GetLeaf("Misc_TrueInteraction");

    std::cout << "Read file: '" << fileName << "'" << std::endl;
    std::cout << "Read tree '" << channel << std::endl;

    gROOT->SetBatch(kTRUE);

    if(outFile.find("csv") != std::string::npos) StrUtil::Replace(outFile, "csv", "root");

    //Open output file and set up all histograms/tree and their function to call
    PrepareLoop();

    //Determine what to clean from jets
    std::vector<std::string> cleanInfo = Utils::SplitString<std::string>(cleanJet, "/");

    if(cleanInfo.size() != 1){
        for(int j=0; j < hist1DFunctions.size(); j++){
            hist1DFunctions[j].SetCleanJet<Axis::X>(cleanInfo[0], cleanInfo[1]);
        }

        for(int j=0; j < hist2DFunctions.size(); j++){
            hist2DFunctions[j].SetCleanJet<Axis::X>(cleanInfo[0], cleanInfo[1]);
            hist2DFunctions[j].SetCleanJet<Axis::Y>(cleanInfo[0], cleanInfo[1]);
        }

        for(int j=0; j < cutFunctions.size(); j++){
            cutFunctions[j].SetCleanJet<Axis::X>(cleanInfo[0], cleanInfo[1]);
        }

        for(int j=0; j < treeFunctions.size(); j++){
            treeFunctions[j].SetCleanJet<Axis::X>(cleanInfo[0], cleanInfo[1]);
        }

        for(int j=0; j < CSVFunctions.size(); j++){
            CSVFunctions[j].SetCleanJet<Axis::X>(cleanInfo[0], cleanInfo[1]);
        }
    }

    //Get number of generated events
    if(inputFile->GetListOfKeys()->Contains("nGen")){
        nGen = inputFile->Get<TH1F>("nGen")->Integral();
    }

    if(inputFile->GetListOfKeys()->Contains("xSec")){
        xSec = inputFile->Get<TH1F>("xSec")->GetBinContent(1);
    }

    if(inputFile->GetListOfKeys()->Contains("Lumi")){
        lumi = inputFile->Get<TH1F>("Lumi")->GetBinContent(1);
    }

    //Set all branch addresses needed for the event
    bool passed = true;
    bool isData = true;

    //Calculate pile up weight histogram
    std::shared_ptr<TH1F> pileUpWeight, pileUpWeightUp, pileUpWeightDown;

    if(inputFile->GetListOfKeys()->Contains("puMC")){
        isData = false;

        std::shared_ptr<TH1F> puMC(static_cast<TH1F*>(inputFile->Get("puMC")));
        std::shared_ptr<TH1F> puReal(static_cast<TH1F*>(inputFile->Get("pileUp")));
        std::shared_ptr<TH1F> puRealUp(static_cast<TH1F*>(inputFile->Get("pileUpUp")));
        std::shared_ptr<TH1F> puRealDown(static_cast<TH1F*>(inputFile->Get("pileUpDown")));

        pileUpWeight.reset(static_cast<TH1F*>(puReal->Clone()));
        pileUpWeightUp.reset(static_cast<TH1F*>(puRealUp->Clone()));
        pileUpWeightDown.reset(static_cast<TH1F*>(puRealDown->Clone()));

        pileUpWeight->Scale(1./pileUpWeight->Integral());
        pileUpWeightUp->Scale(1./pileUpWeightUp->Integral());
        pileUpWeightDown->Scale(1./pileUpWeightDown->Integral());
        puMC->Scale(1./puMC->Integral());

        pileUpWeight->Divide(puMC.get());
        pileUpWeightUp->Divide(puMC.get());
        pileUpWeightDown->Divide(puMC.get());
    }

    //Weight with all variations

    //Cutflow
    std::shared_ptr<TH1F> cutflow(inputFile->Get<TH1F>(("cutflow_" + channel).c_str()));
    cutflow->SetName("cutflow"); cutflow->SetTitle("cutflow");
    cutflow->Scale(1./nGen);

    for (int i = 0; i < inputTree->GetEntries(); i++){
        if(i % 100000 == 0 and i != 0){
            std::cout << "Processed events: " << i << " (" << i/timer.Time() << " eve/s)" << std::endl;
        }

        //Set Entry
        TreeFunction::SetEntry(i);
        nTrueInter->GetBranch()->GetEntry(i);

        //Multiply all common weights
        float weight = xSec*lumi/nGen;
        if(pileUpWeight!=nullptr) weight *= pileUpWeight->GetBinContent(pileUpWeight->FindBin(*(float*)nTrueInter->GetValuePointer()));

        //Check if event passed all cuts
        passed=true;

        for(int j=0; j < cutFunctions.size(); j++){
            passed = passed && cutFunctions[j].GetPassed();

            if(passed){
                weight *= cutFunctions[j].GetWeight();
                cutflow->Fill(cutFunctions[j].GetCutLabel().c_str(), weight);
            }

            else break;
        }

        if(!passed) continue;

        //Fill nominal histogram
        for(int j=0; j < hist1DFunctions.size(); j++){
            hists1D[j]->Fill(hist1DFunctions[j].Get<Axis::X>(), weight);
        }

        for(int j=0; j < hist2DFunctions.size(); j++){
            hists2D[j]->Fill(hist2DFunctions[j].Get<Axis::X>(), hist2DFunctions[j].Get<Axis::Y>(), weight);
        }

        //Fill histogram with systematic shifts
        for(int syst = 0; syst < scaleSysts.size() - 1; syst++){
            for(int shift = 0; shift < 2; shift++){
                weight = xSec*lumi/nGen;
                if(pileUpWeight!=nullptr) weight *= pileUpWeight->GetBinContent(pileUpWeight->FindBin(*(float*)nTrueInter->GetValuePointer()));

                for(int j=0; j < cutFunctions.size(); j++){ 
                    weight *= VUtil::At(cutFunctions[j].GetSystWeight(), MUtil::RowMajIdx({scaleSysts.size() - 1, 2}, {syst, shift}));
                }

                for(int j=0; j < hist1DFunctions.size(); j++){
                    VUtil::At(hists1DSyst, MUtil::RowMajIdx({scaleSysts.size() - 1, 2, hist1DFunctions.size()}, {syst, shift, j}))->Fill(hist1DFunctions[j].Get<Axis::X>(), weight);
                }

                for(int j=0; j < hist2DFunctions.size(); j++){
                    VUtil::At(hists2DSyst, MUtil::RowMajIdx({scaleSysts.size() - 1, 2, hist2DFunctions.size()}, {syst, shift, j}))->Fill(hist2DFunctions[j].Get<Axis::X>(), hist2DFunctions[j].Get<Axis::Y>(), weight);
                }   
            }
        }

        //Fill trees
        for(int j=0; j < treeFunctions.size(); j++){
            treeValues[j] = treeFunctions[j].Get<Axis::X>();
        }

        if(outTree != nullptr) outTree->Fill();

        //Fill CSV file
        if(frame != nullptr){
            std::vector<float> values;

            for(int j=0; j < CSVFunctions.size(); j++){
                values.push_back(CSVFunctions[j].Get<Axis::X>());
            }

            frame->AddColumn(values);
        }
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

    hist1DFunctions.clear();
    cutFunctions.clear();

    if(outTree != nullptr){
        outTree->Write(); 
        std::cout << "Saved tree: '" << outTree->GetName() << "' with " << outTree->GetEntries() << " entries" << std::endl;
        std::cout << "Branches which are saved in tree:" << std::endl;

        for(std::string& branchName: branchNames){
            std::cout << branchName << std::endl;
        }
    }

    //Remove empty bins and write cutflow
    int nonEmpty = 0;

    for(int i=0; i < cutflow->GetNbinsX(); i++){
        if(std::string(cutflow->GetXaxis()->GetBinLabel(i)) != "") nonEmpty++;
    }

    VUtil::At(outFiles, 0)->cd();
    cutflow->SetAxisRange(0, nonEmpty); 
    cutflow->Write();

    if(frame!=nullptr){
        frame->WriteCSV(StrUtil::Join("/", outDir, outFile));
    }

    std::cout << "Closed output file: '" << outFile << "'" << std::endl;
    std::cout << "Time passed for complete processing: " << timer.Time() << " s" << std::endl;
}
