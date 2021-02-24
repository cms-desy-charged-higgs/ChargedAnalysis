#include <ChargedAnalysis/Analysis/include/weighter.h>

Weighter::Weighter(){}

Weighter::Weighter(const std::shared_ptr<TFile>& inputFile, const std::shared_ptr<TTree>& inputTree, const int& era) : 
    inputFile(inputFile),
    inputTree(inputTree),
    era(era){

    float nGen = 1., xSec = 1., lumi = 1.;
    isData = true;

    //Read out baseline weights 
    if(inputFile->GetListOfKeys()->Contains("nGen")){
        nGen = RUtil::Get<TH1F>(inputFile.get(), "nGen")->Integral();
    }

    if(inputFile->GetListOfKeys()->Contains("xSec")){
        xSec = RUtil::Get<TH1F>(inputFile.get(), "xSec")->GetBinContent(1);
    }

    if(inputFile->GetListOfKeys()->Contains("Lumi")){
        lumi = RUtil::Get<TH1F>(inputFile.get(), "Lumi")->GetBinContent(1);
    }

    this->baseWeight = xSec*lumi/nGen;

    //Read out pile up histograms
    if(inputFile->GetListOfKeys()->Contains("puMC")){
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
        float eff;
    
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

    for(const std::string& sfName : NTupleReader::GetInfo(partInfo.get_child(pName + ".scale-factors"), false)){
        std::string sfWithWP = StrUtil::Replace(sfName, "[WP]", wp);
        std::string sfWithWPUpper = StrUtil::Replace(sfName, "[WP]", StrUtil::Capitilize(wp));

        std::vector<std::vector<NTupleReader>*> scaleFactors{&sf, &sfUp, &sfDown};
        std::vector<std::function<float(const int&)>*> bWeights{&bWeight, &bWeightUp, &bWeightDown};
        std::vector<std::string> shift{"", "Up", "Down"};

        for(int i = 0; i < 3; ++i){
            std::string branchName = RUtil::BranchExists(inputTree.get(), StrUtil::Replace(sfWithWP, "[SHIFT]", shift[i])) ? sfWithWP : sfWithWPUpper;

            if(partInfo.get<std::string>(pName + ".alias") != "bj"){
                scaleFactors.at(i)->push_back(std::move(NTupleReader(inputTree, era)));
                scaleFactors.at(i)->back().AddParticle(pAlias, 0, wp);
                scaleFactors.at(i)->back().AddFunctionByBranchName(StrUtil::Replace(branchName, "[SHIFT]", shift[i]));
                scaleFactors.at(i)->back().Compile();
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

                *bWeights[i] = std::bind(&Weighter::GetBJetWeight, std::placeholders::_1, effB, effC, effLight, bPt, RUtil::Get<TLeaf>(inputTree.get(), "Jet_Pt"), RUtil::Get<TLeaf>(inputTree.get(), "Jet_Eta"), RUtil::Get<TLeaf>(inputTree.get(), StrUtil::Replace(branchName, "[SHIFT]", shift[i])), RUtil::Get<TLeaf>(inputTree.get(), "Jet_TrueFlavour"));
            }
        }
    }
}

double Weighter::GetBaseWeight(const std::size_t& entry){
    if(isData) return 1.;
    double puWeight = 1.;

    if(pileUpWeight){
        std::size_t bin = RUtil::GetEntry<char>(RUtil::Get<TLeaf>(inputTree.get(), "Misc_TrueInteraction"), entry);
        puWeight = pileUpWeight->GetBinContent(pileUpWeight->FindBin(bin));
    }

    return this->baseWeight*puWeight;
}

double Weighter::GetPartWeight(const std::size_t& entry){
    if(isData) return 1.;
    double partWeight = 1.;

    for(int i = 0; i < sf.size(); ++i){
        sf[i].Reset();

        while(true){
            float weight = sf[i].Get();
            sf[i].Next();

            if(weight != -999.) partWeight *= weight != 0 ? weight : 1.;
            break;
        }
    }

    try{
        partWeight *= bWeight(entry);
    }

    catch(...){}
 
    return partWeight;
}

double Weighter::GetTotalWeight(const std::size_t& entry){
    if(isData) return 1.;
    return GetBaseWeight(entry) * GetPartWeight(entry);
}
