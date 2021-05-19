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
    if(inputFile->GetListOfKeys()->Contains("nGenStitched")){
        nGen = 1.;

        for(int i = 0; i < 5; ++i){
            stitchedWeights.push_back(RUtil::Get<TParameter<float>>(inputFile.get(), StrUtil::Merge("Weight_", i, "Jets"))->GetVal());
        }

        nPartons = RUtil::Get<TLeaf>(inputTree.get(), "Misc_NParton");
    }

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

double Weighter::GetBJetWeight(const int& entry, TH2F* effB, TH2F* effC, TH2F* effLight, NTupleReader bPt, TLeaf* pt, TLeaf* eta, TLeaf* sf, TLeaf* flavour){
    const std::vector<float> scaleFactor = RUtil::GetVecEntry<float>(sf, entry);
    const std::vector<int> trueFlav = RUtil::GetVecEntry<int>(flavour, entry);

    const std::vector<float> jetPt = RUtil::GetVecEntry<float>(pt, entry);
    const std::vector<float> jetEta = RUtil::GetVecEntry<float>(eta, entry);

    double wData = 1., wMC = 1.;
    bPt.Reset();

    for(std::size_t i = 0; i < scaleFactor.size(); ++i){
        float eff = 1.;
    
        if(std::abs(trueFlav.at(i)) == 5) eff = effB->GetBinContent(effB->FindBin(jetPt.at(i), jetEta.at(i)));
        else if(std::abs(trueFlav.at(i)) == 4) eff = effC->GetBinContent(effC->FindBin(jetPt.at(i), jetEta.at(i)));
        else eff = effLight->GetBinContent(effLight->FindBin(jetPt.at(i), jetEta.at(i)));

        if(eff == 0 or eff == 1) continue;

        if(bPt.Get() == jetPt.at(i)){
            wData *= scaleFactor.at(i) * eff;
            wMC *= eff;
            bPt.Next();
        }
    
        else{
            wData *= 1 - scaleFactor.at(i)*eff;
            wMC *= (1 - eff);
        }
    }

    return wData/wMC;
}

void Weighter::AddParticle(const std::string& pAlias, const std::string& wp, const std::experimental::source_location& location){
    if(isData) return;

    //Read out information of SF branches for wished particle
    pt::ptree partInfo;
    pt::json_parser::read_json(StrUtil::Merge(std::getenv("CHDIR"), "/ChargedAnalysis/Analysis/data/particle.json"), partInfo);
    
    //Check if sub function configured
    std::string pName = NTupleReader::GetName(partInfo, pAlias);

    if(pName == "") throw std::runtime_error(StrUtil::PrettyError(std::experimental::source_location::current(), "Unknown particle alias: '", pAlias, "'!"));

    if(!partInfo.get_child_optional(pName + ".scale-factors")) return;

    std::vector<std::string> systematics = NTupleReader::GetInfo(partInfo.get_child(pName + ".scale-factors"), true);
    std::vector<std::string> sfNames = NTupleReader::GetInfo(partInfo.get_child(pName + ".scale-factors"), false);

    for(int i = 0; i < sfNames.size(); ++i){
        std::string sfWithWP = StrUtil::Replace(sfNames.at(i), "[WP]", wp);
        std::string sfWithWPUpper = StrUtil::Replace(sfNames.at(i), "[WP]", StrUtil::Capitilize(wp));

        for(const std::string shift : {"", "Up", "Down"}){
            std::string branchName;

            if(RUtil::BranchExists(inputTree.get(), StrUtil::Replace(StrUtil::Replace(sfNames.at(i), "[WP]", wp), "[SHIFT]", shift))){
                branchName = StrUtil::Replace(StrUtil::Replace(sfNames.at(i), "[WP]", wp), "[SHIFT]", shift);
            }

            else if(RUtil::BranchExists(inputTree.get(), StrUtil::Replace(StrUtil::Replace(sfNames.at(i), "[WP]", StrUtil::Capitilize(wp)), "[SHIFT]", shift))){
                branchName = StrUtil::Replace(StrUtil::Replace(sfNames.at(i), "[WP]", StrUtil::Capitilize(wp)), "[SHIFT]", shift);
            }

            else continue;

            if(partInfo.get<std::string>(pName + ".alias") != "bj"){
                if(shift == "") this->systematics.push_back(systematics.at(i));

                std::vector<NTupleReader>& scaleFactor = shift == "" ? sf : shift == "Up" ? sfUp : sfDown;

                scaleFactor.push_back(std::move(NTupleReader(inputTree, era)));
                scaleFactor.back().AddParticle(pAlias, 0, wp);
                scaleFactor.back().AddFunctionByBranchName(branchName);
                scaleFactor.back().Compile();
            }

            else{
                std::string wpName = StrUtil::Capitilize(wp);

                TH2F* effB = RUtil::Clone<TH2F>(RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@BCSVbTag", "@", wpName)));
                effB->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueB"));
                TH2F* effC = RUtil::Clone<TH2F>(RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@CCSVbTag", "@", wpName)));
                effC->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueC"));
                TH2F* effLight = RUtil::Clone<TH2F>(RUtil::Get<TH2F>(inputFile.get(), StrUtil::Replace("n@LightCSVbTag", "@", wpName)));
                effLight->Divide(RUtil::Get<TH2F>(inputFile.get(), "nTrueLight"));

                NTupleReader bPt = NTupleReader(inputTree, era);
                bPt.AddParticle("bj", 0, wp);
                bPt.AddFunction("pt");
                bPt.Compile();

                std::function<float(const int&)>& bW = shift == "" ? bWeight : shift == "Up" ? bWeightUp : bWeightDown;

                bW = std::bind(&Weighter::GetBJetWeight, std::placeholders::_1, effB, effC, effLight, bPt, RUtil::Get<TLeaf>(inputTree.get(), "Jet_Pt"), RUtil::Get<TLeaf>(inputTree.get(), "Jet_Eta"), RUtil::Get<TLeaf>(inputTree.get(), StrUtil::Replace(branchName, "[SHIFT]", shift)), RUtil::Get<TLeaf>(inputTree.get(), "Jet_TrueFlavour"));
            }
        }
    }
}

int Weighter::GetNWeights(){return sf.size() + bool(bWeight);}

double Weighter::GetNGen(){return nGen;}

double Weighter::GetBaseWeight(const std::size_t& entry, const std::string& sysShift){
    if(isData) return 1.;
    double puWeight = 1., stitchWeight = xSec;

    if(pileUpWeight){
        std::shared_ptr<TH1F>& pileUp = sysShift == "" ? pileUpWeight : sysShift == "Up" ? pileUpWeightUp : pileUpWeightDown;

        std::size_t bin = RUtil::GetEntry<short>(RUtil::Get<TLeaf>(inputTree.get(), "Misc_TrueInteraction"), entry);
        puWeight = pileUp->GetBinContent(pileUpWeight->FindBin(bin));
    }

    if(nPartons != nullptr){
        short n = RUtil::GetEntry<short>(nPartons, entry);

        stitchWeight = stitchedWeights.at(n);
    }

    return this->baseWeight*puWeight*stitchWeight;
}

double Weighter::GetPartWeight(const std::size_t& entry, const std::string& syst, const std::string& sysShift){
    if(isData) return 1.;
    double partWeight = 1.;

    int systIdx;

    if(syst == "" or syst == "PileUp") systIdx = -1;
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
        NTupleReader& scale = systIdx != idx ? sf[idx] : sysShift == "Up" ? sfUp[idx] : sfDown[idx];
        scale.Reset();

        while(true){
            float weight = scale.Get();
            scale.Next();

            if(weight != -999.) partWeight *= weight != 0 ? weight : 1.;
            break;
        }   
    }

    if(bWeight){
        std::function<float(const int&)>& bScale = systIdx != -2 ? bWeight : sysShift == "Up" ? bWeightUp : bWeightDown;

        partWeight *= bScale(entry);
    }

    return partWeight;
}

double Weighter::GetTotalWeight(const std::size_t& entry, const std::string& syst, const std::string& sysShift){
    if(isData) return 1.;
    return GetBaseWeight(entry, syst == "PileUp" ? sysShift : "") * GetPartWeight(entry, syst, sysShift);
}
