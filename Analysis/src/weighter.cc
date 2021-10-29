#include <ChargedAnalysis/Analysis/include/weighter.h>

Weighter::Weighter(){}

Weighter::Weighter(const std::shared_ptr<TFile>& inputFile, const std::shared_ptr<TTree>& inputTree, const int& era) : 
    inputFile(inputFile),
    inputTree(inputTree),
    era(era){

    isData = true;

    //Read out baseline weights 
    if(inputFile->GetListOfKeys()->Contains("nGen")){
        nGen = RUtil::Get<TParameter<float>>(inputFile.get(), "nGen")->GetVal();
    }

    if(inputFile->GetListOfKeys()->Contains("Lumi")){
        lumi = RUtil::Get<TParameter<float>>(inputFile.get(), "Lumi")->GetVal();
    }

    if(inputFile->GetListOfKeys()->Contains("xSec")){
        xSec = RUtil::Get<TParameter<float>>(inputFile.get(), "xSec")->GetVal();
    }

    //Stitching weights
 /*   if(inputFile->GetListOfKeys()->Contains("nGenStitched")){
        nGen = 1.;

        for(int i = 0; i < 5; ++i){
            stitchedWeights.push_back(RUtil::Get<TParameter<float>>(inputFile.get(), StrUtil::Merge("Weight_", i, "Jets"))->GetVal());
        }

        nPartons = RUtil::Get<TLeaf>(inputTree.get(), "Misc_NParton");
    } */
        
    this->baseWeight = lumi/nGen;

    //Read out pile up histograms
    if(inputFile->GetListOfKeys()->Contains("pileUp")){
        isData = false;
        std::shared_ptr<TH1F> puMC = RUtil::GetSmart<TH1F>(inputFile.get(), "puMC");

        pileUpWeight = RUtil::CloneSmart<TH1F>(RUtil::Get<TH1F>(inputFile.get(), "pileUp"));
        pileUpWeightUp = RUtil::CloneSmart<TH1F>(RUtil::Get<TH1F>(inputFile.get(), "pileUpUp"));
        pileUpWeightDown = RUtil::CloneSmart<TH1F>(RUtil::Get<TH1F>(inputFile.get(), "pileUpDown"));

        pileUpWeight->Scale(1./pileUpWeight->Integral());
        pileUpWeightUp->Scale(1./pileUpWeightUp->Integral());
        pileUpWeightDown->Scale(1./pileUpWeightDown->Integral());
        puMC->Scale(1./puMC->Integral());

        pileUpWeight->Divide(puMC.get());
        pileUpWeightUp->Divide(puMC.get());
        pileUpWeightDown->Divide(puMC.get());
    }
}

double Weighter::GetBJetWeight(const int& entry, TH2F* effB, TH2F* effC, TH2F* effLight, NTupleFunction& bPt, TLeaf* pt, TLeaf* eta, TLeaf* sf, TLeaf* flavour){
    double wData = 1., wMC = 1.;
    float jetPt, jetEta, trueFlav, scaleFactor, eff;

    int size = RUtil::GetLen(pt, entry);
    int bIdx = 0;

    std::cout << "Event start" << std::endl;

    for(std::size_t i = 0; i < size; ++i){
        eff = 1.;
        scaleFactor = RUtil::GetEntry<float>(sf, entry, i);
        trueFlav = RUtil::GetEntry<int>(flavour, entry, i);

        jetPt = RUtil::GetEntry<float>(pt, entry, i);
        jetEta = RUtil::GetEntry<float>(eta, entry, i);
        
        if(std::abs(trueFlav) == 5) eff = effB->GetBinContent(effB->FindBin(jetPt, jetEta));
        else if(std::abs(trueFlav) == 4) eff = effC->GetBinContent(effC->FindBin(jetPt, jetEta));
        else eff = effLight->GetBinContent(effLight->FindBin(jetPt, jetEta));

        if(eff == 0 or eff == 1) continue;

        if(bPt.Get(bIdx) == jetPt){
            wData *= scaleFactor * eff;
            wMC *= eff;
            ++bIdx;
        }
    
        else{
            wData *= (1 - scaleFactor*eff);
            wMC *= (1 - eff);
        }
    }

    std::cout << "Event end" << std::endl;

    return wData/wMC;
}

void Weighter::AddParticle(const std::string& pAlias, const std::string& wp, NTupleReader& reader, const std::experimental::source_location& location){
    if(isData) return;

    //Read out information of SF branches for wished particle
    pt::ptree partInfo;
    pt::json_parser::read_json(StrUtil::Merge(std::getenv("CHDIR"), "/ChargedAnalysis/Analysis/data/particle.json"), partInfo);
    
    //Check if sub function configured
    std::string pName = NTupleReader::GetName(partInfo, pAlias);

    if(pName == "") throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown particle alias: '", pAlias, "'!"));

    if(!partInfo.get_child_optional(pName + ".scale-factors")) return;

    std::vector<std::string> systematics = NTupleReader::GetKeys(partInfo.get_child(pName + ".scale-factors"));
    std::vector<std::string> sfNames = NTupleReader::GetVector(partInfo.get_child(pName + ".scale-factors"));

    for(int i = 0; i < sfNames.size(); ++i){
        for(const std::string shift : {"", "Up", "Down"}){
            std::string branchName = StrUtil::Replace(StrUtil::Replace(sfNames.at(i), "[WP]", wp), "[SHIFT]", shift);

            if(!RUtil::BranchExists(inputTree.get(), branchName)){
                std::cout << "Unknown branch '" << branchName << "', it will be skipped" << std::endl;
                continue;
            }

            if(partInfo.get<std::string>(pName + ".alias") != "bj"){
                if(shift == "") this->systematics.push_back(systematics.at(i));

                NTupleFunction scaleFactor = reader.BuildFunc();
                
                scaleFactor.AddParticle(pAlias, 0, wp);
                scaleFactor.AddFunctionByBranchName(branchName);
                scaleFactor.Compile();

                if(shift == "") sf.push_back(std::move(scaleFactor));
                else if(shift == "Up") sfUp.push_back(std::move(scaleFactor));
                else if(shift == "Down") sfDown.push_back(std::move(scaleFactor));
            }

            else{
                std::string wpName = StrUtil::Capitilize(wp);

                TH2F* effB = RUtil::Clone<TH2F>(RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@BbTagDeepCSV", "@", wpName)));
                effB->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueB"));
                TH2F* effC = RUtil::Clone<TH2F>(RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@CbTagDeepCSV", "@", wpName)));
                effC->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueC"));
                TH2F* effLight = RUtil::Clone<TH2F>(RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@LightbTagDeepCSV", "@", wpName)));
                effLight->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueLight"));

                NTupleFunction bPt = reader.BuildFunc();
                bPt.AddParticle("bj", 0, wp);
                bPt.AddFunction("pt");
                bPt.Compile();

                TLeaf* jetPt = RUtil::Get<TLeaf>(inputTree.get(), "Jet_Pt");
                TLeaf* jetEta = RUtil::Get<TLeaf>(inputTree.get(), "Jet_Eta");
                TLeaf* sf = RUtil::Get<TLeaf>(inputTree.get(), StrUtil::Replace(branchName, "[SHIFT]", shift));
                TLeaf* partonFlav = RUtil::Get<TLeaf>(inputTree.get(), "Jet_PartonFlavour");

                std::function<float(const int&)> bW = [=, bPt = std::move(bPt)](const int& entry) mutable {
                return Weighter::GetBJetWeight(entry, effB, effC, effLight, bPt, jetPt, jetEta, sf, partonFlav);};
            
                if(shift == "") bWeight = bW;
                else if(shift == "Up") bWeightUp = bW;
                else if(shift == "Down") bWeightDown = bW;
            }
        }
    }
}

int Weighter::GetNWeights(){return sf.size() + bool(bWeight);}

double Weighter::GetBaseWeight(const std::size_t& entry, const std::string& sysShift){
    if(isData) return 1.;
    double puWeight = 1., stitchWeight = xSec;

    if(pileUpWeight){
        std::shared_ptr<TH1F>& pileUp = sysShift == "" ? pileUpWeight : sysShift == "Up" ? pileUpWeightUp : pileUpWeightDown;

        std::size_t bin = RUtil::GetEntry<short>(RUtil::Get<TLeaf>(inputTree.get(), "Weight_nTrueInt"), entry);
        puWeight = pileUp->GetBinContent(pileUpWeight->FindBin(bin));
    }

   /* if(nPartons != nullptr){
        short n = RUtil::GetEntry<short>(nPartons, entry);

        stitchWeight = stitchedWeights.at(n);
    } */

    return this->baseWeight*puWeight*stitchWeight;
}

double Weighter::GetPartWeight(const std::size_t& entry, const std::string& syst, const std::string& sysShift){
    if(isData) return 1.;

    double partWeight = 1.;

    int systIdx;

    if(syst == "Nominal" or syst == "PileUp") systIdx = -1;
    else if(syst == "BJet") systIdx = -2;
    else{
        try{
            systIdx = VUtil::Find(systematics, syst).at(0);
        }
    
        catch(...){
            StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown systematic '", syst, "'!");
        }
    }

    for(int idx = 0; idx < sf.size(); ++idx){
        std::vector<NTupleFunction>* scale;

        if(systIdx != idx) scale = &sf;
        else{
            if(sysShift == "Up") scale = &sfUp;
            else scale = &sfDown;
        }

        for(int k = 0;; ++k){
            float weight = scale->at(idx).Get(k);

            if(weight != -999.) partWeight *= weight != 0 ? weight : 1.;
            else break;
        }   
    }

    if(bWeight or bWeightUp or bWeightDown){
        if(systIdx != -2) partWeight *= bWeight(entry);
        else{
            if(sysShift == "Up") partWeight *= bWeightUp(entry);
            else partWeight *= bWeightDown(entry);
        }
    }

    return partWeight;
}

double Weighter::GetTotalWeight(const std::size_t& entry, const std::string& syst, const std::string& sysShift){
    if(isData) return 1.;
    return GetBaseWeight(entry, syst == "PileUp" ? sysShift : "") * GetPartWeight(entry, syst, sysShift);
}
