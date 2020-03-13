#include <ChargedAnalysis/Analysis/include/treefunction.h>

Utils::Bimap<std::string, float(*)(Event&, FuncArgs&)> TreeFunction::functions = {
    {"mass", &TreeFunction::Mass},
    {"pt", &TreeFunction::Pt},
    {"phi", &TreeFunction::Phi},
    {"eta", &TreeFunction::Eta},
    {"dPhi", &TreeFunction::DeltaPhi},
    {"dR", &TreeFunction::DeltaR},
    {"N", &TreeFunction::NParticle},
    {"HT", &TreeFunction::HadronicEnergy},
    {"const", &TreeFunction::ConstantNumber},
    {"evNr", &TreeFunction::EventNumber},
};

Utils::Bimap<float(*)(Event&, FuncArgs&), std::string> TreeFunction::funcLabels = {
    {&TreeFunction::Mass, "m(@) [GeV]"},
    {&TreeFunction::Pt, "p_{T}(@) [GeV]"},
    {&TreeFunction::Phi, "#phi(@) [rad]"},
    {&TreeFunction::Eta, "#eta(@) [rad]"},
    {&TreeFunction::DeltaPhi, "#Delta#phi(@, @) [rad]"},
    {&TreeFunction::DeltaR, "#Delta R(@, @) [rad]"},
    {&TreeFunction::NParticle, "N(@)"},
    {&TreeFunction::HadronicEnergy, "H_{T}"},
    {&TreeFunction::ConstantNumber, ""},
    {&TreeFunction::EventNumber, ""},
};

Utils::Bimap<std::string, Particle> TreeFunction::particles = {
    {"e", ELECTRON},
    {"mu", MUON},
    {"j", JET},
    {"sj", SUBJET},
    {"fj", FATJET},
    {"bj", BJET},
    {"bsj", BSUBJET},
    {"met", MET},
};

Utils::Bimap<Particle, std::string> TreeFunction::partLabels = {
    {ELECTRON, "e_{@}"},
    {MUON, "#mu_{@}"},
    {JET, "j_{@}"},
    {SUBJET, "j^{sub}_{@}"},
    {FATJET, "j_{@}^{AK8}"},
    {BJET, "b_{@}"},
    {BSUBJET, "b^{sub}_{@}"},
    {MET, "#vec{p}_{T}^{miss}"},
};

Utils::Bimap<std::string, WP> TreeFunction::workingPoints = {
    {"l", LOOSE},
    {"m", MEDIUM},
    {"t", TIGHT},
};

Utils::Bimap<std::string, Comparison> TreeFunction::comparisons = {
    {"bigger", BIGGER},
    {"smaller", SMALLER},
    {"equal", EQUAL},
    {"divisible", DIVISIBLE},
    {"notdivisible", NOTDIVISIBLE},
};


//Function for returning value of wished quantity
float TreeFunction::Mass(Event& event, FuncArgs& args){
    return event.particles.at(args.parts[0]).at(args.wp[0]).at(args.index[0]).M();
}

float TreeFunction::Pt(Event& event, FuncArgs& args){
    return event.particles.at(args.parts[0]).at(args.wp[0]).at(args.index[0]).Pt();
}

float TreeFunction::Phi(Event& event, FuncArgs& args){
    return event.particles.at(args.parts[0]).at(args.wp[0]).at(args.index[0]).Phi();
}

float TreeFunction::Eta(Event& event, FuncArgs& args){
    return event.particles.at(args.parts[0]).at(args.wp[0]).at(args.index[0]).Eta();
}

float TreeFunction::DeltaPhi(Event& event, FuncArgs& args){
    return ROOT::Math::VectorUtil::DeltaPhi(event.particles.at(args.parts[0]).at(args.wp[0]).at(args.index[0]), event.particles.at(args.parts[1]).at(args.wp[1]).at(args.index[1]));
}

float TreeFunction::DeltaR(Event& event, FuncArgs& args){
    return ROOT::Math::VectorUtil::DeltaR(event.particles.at(args.parts[0]).at(args.wp[0]).at(args.index[0]), event.particles.at(args.parts[1]).at(args.wp[1]).at(args.index[1]));
}

float TreeFunction::ConstantNumber(Event &event, FuncArgs& args){
    return args.value;
}

float TreeFunction::NParticle(Event &event, FuncArgs& args){
    if(event.SF.count(args.parts[0])){
        for(float& SF: event.SF.at(args.parts[0]).at(args.wp[0])){
            event.weight *= SF;
        }
    }

    if(event.particles.count(args.parts[0])){
        if(event.particles.at(args.parts[0]).count(args.wp[0])){
            return event.particles.at(args.parts[0]).at(args.wp[0]).size();
        }

        else return 0;
    }

    else return 0;
}

float TreeFunction::HadronicEnergy(Event &event, FuncArgs& args){
    float HT=0;

    if(event.particles.count(JET)){
        for(ROOT::Math::PxPyPzEVector& jet: event.particles.at(JET).at(NONE)){HT+= jet.Pt();}
    }

    return HT;
}

float TreeFunction::EventNumber(Event &event, FuncArgs& args){
    return Utils::BitCount(int(event.eventNumber));
}

/*
float TreeReader::Subtiness(Event &event, Hist &hist){
    return event.particles[hist.parts[0]][hist.indeces[0]-1].subtiness[hist.funcValue];
}

float TreeReader::BDTScore(Event &event, Hist &hist){
    std::vector<float> paramValues;

    for(Hist funcs: bdtFunctions){
        paramValues.push_back((this->*funcDir[funcs.func])(event, funcs));
    }

    paramValues.push_back(hist.funcValue);

    float score = (int)EventNumber(event, hist) % 2 == 0 ? oddClassifier.Evaluate(paramValues) : evenClassifier.Evaluate(paramValues);

    return score;
}

float TreeReader::HTag(Event &event, Hist &hist){
    return event.hTag[hist.indeces[0]-1];
}

float TreeReader::NSigParticle(Event &event, Hist &hist){
    int nSigPart = 0;

    if(hist.parts[0] == ELECTRON or hist.parts[0] == MUON){
        for(const RecoParticle &part: event.particles[hist.parts[0]]){
            nSigPart+= part.isFromSignal;
        }

        return nSigPart;
    }

    else if(hist.parts[0] == JET or hist.parts[0] == SUBJET or hist.parts[0] == FATJET){
        for(const RecoParticle &part: event.particles[hist.parts[0]]){
            nSigPart+= part.isFromSignal != -1;
        }

        return nSigPart;
    }

    return nSigPart;
}


//Functions for reconstruct mother final state particles
void TreeReader::WBoson(Event &event){
    ROOT::Math::PxPyPzEVector lep = event.particles[MUON].size() != 0 ? event.particles[MUON][0].LV : event.particles[ELECTRON][0].LV;

    float pXNu = event.particles[MET][0].LV.Px();
    float pYNu = event.particles[MET][0].LV.Py();
    float mW = 80.399;
    
    //Analytic solutions to quadratic problem
    float pZNu1 = (-lep.E()*std::sqrt(-4*lep.E()*lep.E()*pXNu*pXNu - 4*lep.E()*lep.E()*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*lep.Px()*pXNu + 4*mW*mW*lep.Py()*pYNu + 4*lep.Px()*lep.Px()*pXNu*pXNu + 8*lep.Px()*pXNu*lep.Py()*pYNu + 4*pXNu*pXNu*lep.Pz()*lep.Pz() + 4*lep.Py()*lep.Py()*pYNu*pYNu + 4*pYNu*pYNu*lep.Pz()*lep.Pz())/2 + mW*mW*lep.Pz()/2 + lep.Px()*pXNu*lep.Pz() + lep.Py()*pYNu*lep.Pz())/(lep.E()*lep.E() - lep.Pz()*lep.Pz());

    float pZNu2 = (lep.E()*std::sqrt(-4*lep.E()*lep.E()*pXNu*pXNu - 4*lep.E()*lep.E()*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*lep.Px()*pXNu + 4*mW*mW*lep.Py()*pYNu + 4*lep.Px()*lep.Px()*pXNu*pXNu + 8*lep.Px()*pXNu*lep.Py()*pYNu + 4*pXNu*pXNu*lep.Pz()*lep.Pz() + 4*lep.Py()*lep.Py()*pYNu*pYNu + 4*pYNu*pYNu*lep.Pz()*lep.Pz())/2 + mW*mW*lep.Pz()/2 + lep.Px()*pXNu*lep.Pz() + lep.Py()*pYNu*lep.Pz())/(lep.E()*lep.E() - lep.Pz()*lep.Pz());

    //Neutrino candidates from solutions above
    ROOT::Math::PxPyPzEVector v1(pXNu, pYNu, pZNu1, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu1*pZNu1));
    ROOT::Math::PxPyPzEVector v2(pXNu, pYNu, pZNu2, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu2*pZNu2));

    //Take neutrino which gives physical reasonabel result
    RecoParticle WPart;
    WPart.LV = abs((lep + v1).M() - mW) < abs((lep + v2).M() - mW) ? lep + v1 : lep + v2;

    event.particles[W].push_back(WPart);
}

void TreeReader::Higgs(Event &event){
    //Vector of candPairs
    typedef std::vector<std::vector<RecoParticle>> hCandVec;

    hCandVec hCands; 
    
    //Intermediate step to save all possible combinations of two jets from jet collection
    std::vector<std::pair<int, int>> combi;
    
    for(unsigned int i = 0; i < event.particles[JET].size(); i++){
        for(unsigned int j = 0; j < i; j++){
            combi.push_back({i, j});
        }
    }

    //If 4 jets and no fat jets
    if(event.particles[JET].size() >= 4){
        //Construct all pairs of possible jet pairs
        for(unsigned int i = 0; i < combi.size(); i++){
            for(unsigned int j = 0; j < i; j++){
                //Check if not same jet in both pair
                std::set<int> check = {combi[i].first, combi[i].second, combi[j].first, combi[j].second};
                
                if(check.size() == 4){
                    hCands.push_back({event.particles[JET][combi[i].first], event.particles[JET][combi[i].second], event.particles[JET][combi[j].first], event.particles[JET][combi[j].second]});
                }
            }
        }
    }

    //If 2 jets and one fat jet
    else if(event.particles[JET].size() >= 2 and event.particles[FATJET].size() == 1){
        for(std::pair<int, int> jetIndex: combi){
            hCands.push_back({event.particles[FATJET][0], event.particles[JET][jetIndex.first], event.particles[JET][jetIndex.second]});
        }
    }

    //If 2 fat jets
    else if(event.particles[FATJET].size() == 2){
        hCands.push_back({event.particles[FATJET][0], event.particles[FATJET][1]});
    }

    //Sort candPairs for mass diff of jet pairs, where index 0 is the best pair
    std::function<bool(std::vector<RecoParticle>, std::vector<RecoParticle>)> sortFunc = [&](std::vector<RecoParticle> cands1, std::vector<RecoParticle> cands2){
        if(cands1.size()==4){
            return std::abs((cands1[0].LV+cands1[1].LV).M() - (cands1[2].LV+cands1[3].LV).M()) < std::abs((cands2[0].LV+cands2[1].LV).M() - (cands2[2].LV+cands2[3].LV).M());
        }

        else if(cands1.size()==3){
            return std::abs(cands1[0].LV.M() - (cands1[1].LV+cands1[2].LV).M()) < std::abs(cands2[0].LV.M() - (cands2[1].LV+cands2[2].LV).M());
        }

        else return true;
    };

    std::function<bool(RecoParticle, RecoParticle)> sortPt = [&](RecoParticle p1, RecoParticle p2){
        return p1.LV.Pt() > p2.LV.Pt();
    };

    std::sort(hCands.begin(), hCands.end(), sortFunc);

    //Check if W Boson alread reconstructed
    if(event.particles[W].empty()) WBoson(event);

    //Reconstruct charge Higgs with smallest dR between h and W
    RecoParticle hCand1 = hCands[0].size()==4 ? hCands[0][0] + hCands[0][1]: hCands[0][0];

    RecoParticle hCand2 = hCands[0].size()==4 ? hCands[0][2] + hCands[0][3]: hCands[0].size()==3 ? hCands[0][1] + hCands[0][2] : hCands[0][1];

    RecoParticle Hc1 = event.particles[W][0] + hCand1;
    RecoParticle Hc2 = event.particles[W][0] + hCand2;

    if(ROOT::Math::VectorUtil::DeltaPhi(Hc1.LV, hCand2.LV) > ROOT::Math::VectorUtil::DeltaPhi(Hc2.LV, hCand1.LV) and ROOT::Math::VectorUtil::DeltaPhi(Hc1.LV, hCand1.LV) < ROOT::Math::VectorUtil::DeltaPhi(Hc2.LV, hCand2.LV)){
        event.particles[H1JET] = hCands[0].size()==4 ? std::vector<RecoParticle>{hCands[0][0], hCands[0][1]} : std::vector<RecoParticle>{hCands[0][0]};
        std::sort(event.particles[H1JET].begin(), event.particles[H1JET].end(), sortPt);
        event.particles[h].push_back(hCands[0].size()==4 ? hCands[0][0]+hCands[0][1] : hCands[0][0]);

        event.particles[H2JET] = hCands[0].size()==4 ? std::vector<RecoParticle>{hCands[0][2], hCands[0][3]} : hCands[0].size()==3 ? std::vector<RecoParticle>{hCands[0][1], hCands[0][2]} : std::vector<RecoParticle>{hCands[0][1]};
        std::sort(event.particles[H2JET].begin(), event.particles[H2JET].end(), sortPt);
        event.particles[h].push_back(hCands[0].size()==4 ? hCands[0][2]+hCands[0][3] : hCands[0].size()==3 ? hCands[0][1]+hCands[0][2] : hCands[0][1]);

        
        event.particles[HC].push_back(Hc1);
    }

    else{
        event.particles[H1JET] =  hCands[0].size()==4 ? std::vector<RecoParticle>{hCands[0][2], hCands[0][3]} :  hCands[0].size()==3 ? std::vector<RecoParticle>{hCands[0][1], hCands[0][2]} : std::vector<RecoParticle>{hCands[0][1]};
        event.particles[h].push_back(hCands[0].size()==4 ? hCands[0][2]+hCands[0][3] : hCands[0].size()==3 ? hCands[0][1]+hCands[0][2] : hCands[0][1]);

        event.particles[H2JET] = hCands[0].size()==4 ? std::vector<RecoParticle>{hCands[0][0], hCands[0][1]} : std::vector<RecoParticle>{hCands[0][0]};
        event.particles[h].push_back(hCands[0].size()==4 ? hCands[0][0]+hCands[0][1] : hCands[0][0]);

        event.particles[HC].push_back(Hc2);
    }
}

*/
