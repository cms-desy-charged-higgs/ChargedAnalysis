#include <ChargedHiggs/analysis/interface/treereader.h>

//Function for returning value of wished quantity
float TreeReader::Mass(Event &event, Hist &hist){
    return GetParticle(event, hist.parts[0], hist.indeces[0]).M();
}

float TreeReader::Pt(Event &event, Hist &hist){
    return GetParticle(event, hist.parts[0], hist.indeces[0]).Pt();
}

float TreeReader::Phi(Event &event, Hist &hist){
    return GetParticle(event, hist.parts[0], hist.indeces[0]).Phi();
}

float TreeReader::Eta(Event &event, Hist &hist){
    return GetParticle(event, hist.parts[0], hist.indeces[0]).Eta();
}

float TreeReader::DeltaPhi(Event &event, Hist &hist){
    return GetParticle(event, hist.parts[0], hist.indeces[0]).DeltaPhi(GetParticle(event, hist.parts[1], hist.indeces[1]));
}

float TreeReader::DeltaR(Event &event, Hist &hist){
    return GetParticle(event, hist.parts[0], hist.indeces[0]).DeltaR(GetParticle(event, hist.parts[1], hist.indeces[1]));
}

float TreeReader::HadronicEnergy(Event &event, Hist &hist){
    return event.HT;
}

float TreeReader::NParticle(Event &event, Hist &hist){
    int nPart = 0;

    switch(hist.parts[0]){
        case ELECTRON:
            for(const Electron &ele: event.electrons){
                if(ele.*eleID[hist.func].first && ele.isolation < eleID[hist.func].second && ele.isTriggerMatched){
                    nPart++;

                    event.weight *= ele.*eleSF[hist.func] * ele.recoSF;
                }
            } 
        
            return nPart;

        case MUON:
            for(const Muon &muon: event.muons){
                if(muon.*muonID[hist.func].first && muon.*muonID[hist.func].first && muon.isTriggerMatched){
                    nPart++;

                    event.weight *= muon.*muonSF[hist.func].first * muon.*muonSF[hist.func].second;
                }
            } 
        
            return nPart;

        case JET:
            return event.jets.size();

        case FATJET:
            return event.fatjets.size();

        case BJET: case BFATJET:
            for(const Jet &jet: hist.parts[0] == BJET ? event.jets: event.fatjets){
                if(jet.*bJetID[hist.func]){
                    nPart++;

                    event.weight *= jet.*bJetSF[hist.func];
                }    
            }
    
            return nPart;
        default: return -999.;
    }
}

//Function evaluating cuts
bool TreeReader::Cut(Event &event, Hist &hist){
    switch(hist.cutValues.first){
        case EQUAL: return (this->*funcDir[hist.func])(event, hist) == hist.cutValues.second;
        case EQBIGGER: return (this->*funcDir[hist.func])(event, hist) >= hist.cutValues.second;
        case EQSMALLER: return (this->*funcDir[hist.func])(event, hist) <= hist.cutValues.second;
        case BIGGER: return (this->*funcDir[hist.func])(event, hist) > hist.cutValues.second;
        case SMALLER: return (this->*funcDir[hist.func])(event, hist) < hist.cutValues.second;
        default: return true;
    }
}

//Function called by functions above to get particle of desire
const TLorentzVector& TreeReader::GetParticle(Event &event, Particle &part, const int &index){
    switch(part){
        case ELECTRON: return (unsigned int)index <= event.electrons.size() ? event.electrons[index-1].fourVec : event.dummy;
        case MUON: return (unsigned int)index <= event.muons.size() ? event.muons[index-1].fourVec : event.dummy;
        case JET: return (unsigned int)index <= event.jets.size() ? event.jets[index-1].fourVec : event.dummy;
        case FATJET: return (unsigned int)index <= event.fatjets.size() ? event.fatjets[index-1].fourVec : event.dummy;
        case MET: return event.MET;
        case W: 
            if(event.W.Pt() == 0){
                WBoson(event);
                return event.W;
            }

            return event.W;
        case HC: 
            if(event.Hc.Pt() == 0){
                Higgs(event);
                return event.Hc;
            }

            return event.Hc;
        case h: 
            if(event.h.empty()){
                Higgs(event);
                return event.h[index-1];
            }

            return event.h[index-1];
        default: return event.dummy;
    }
}


//Functions for reconstruct mother final state particles
void TreeReader::WBoson(Event &event){
    TLorentzVector lep = event.muons.size() != 0 ? event.muons[0].fourVec : event.electrons[0].fourVec;

    float pXNu = event.MET.Pt()*std::cos(event.MET.Phi());
    float pYNu = event.MET.Pt()*std::sin(event.MET.Phi());
    float mW = 80.399;
    
    //Analytic solutions to quadratic problem
    float pZNu1 = (-lep.E()*std::sqrt(-4*lep.E()*lep.E()*pXNu*pXNu - 4*lep.E()*lep.E()*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*lep.Px()*pXNu + 4*mW*mW*lep.Py()*pYNu + 4*lep.Px()*lep.Px()*pXNu*pXNu + 8*lep.Px()*pXNu*lep.Py()*pYNu + 4*pXNu*pXNu*lep.Pz()*lep.Pz() + 4*lep.Py()*lep.Py()*pYNu*pYNu + 4*pYNu*pYNu*lep.Pz()*lep.Pz())/2 + mW*mW*lep.Pz()/2 + lep.Px()*pXNu*lep.Pz() + lep.Py()*pYNu*lep.Pz())/(lep.E()*lep.E() - lep.Pz()*lep.Pz());

    float pZNu2 = (lep.E()*std::sqrt(-4*lep.E()*lep.E()*pXNu*pXNu - 4*lep.E()*lep.E()*pYNu*pYNu + mW*mW*mW*mW + 4*mW*mW*lep.Px()*pXNu + 4*mW*mW*lep.Py()*pYNu + 4*lep.Px()*lep.Px()*pXNu*pXNu + 8*lep.Px()*pXNu*lep.Py()*pYNu + 4*pXNu*pXNu*lep.Pz()*lep.Pz() + 4*lep.Py()*lep.Py()*pYNu*pYNu + 4*pYNu*pYNu*lep.Pz()*lep.Pz())/2 + mW*mW*lep.Pz()/2 + lep.Px()*pXNu*lep.Pz() + lep.Py()*pYNu*lep.Pz())/(lep.E()*lep.E() - lep.Pz()*lep.Pz());

    //Neutrino candidates from solutions above
    TLorentzVector v1;
    v1.SetPxPyPzE(pXNu, pYNu, pZNu1, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu1*pZNu1));

    TLorentzVector v2;
    v2.SetPxPyPzE(pXNu, pYNu, pZNu2, std::sqrt(pXNu*pXNu + pYNu*pYNu + pZNu2*pZNu2));

    //Take neutrino which gives physical reasonabel result
    event.W = abs((lep + v1).M() - mW) < abs((lep + v2).M() - mW) ? lep + v1 : lep + v2;
}

void TreeReader::Higgs(Event &event){
    //Vector of candPairs
    typedef std::pair<TLorentzVector, TLorentzVector> hCands;
    std::vector<hCands> candPairs; 
    
    //Intermediate step to save all possible combinations of two jets from jet collection
    std::vector<std::pair<int, int>> combi;
    
    for(unsigned int i = 0; i < event.jets.size(); i++){
        for(unsigned int j = 0; j < i; j++){
            combi.push_back({i, j});
        }
    }

    //If 4 jets and no fat jets
    if(event.jets.size() >= 4){
        //Construct all pairs of possible jet pairs
        for(unsigned int i = 0; i < combi.size(); i++){
            for(unsigned int j = 0; j < i; j++){
                //Check if not same jet in both pair
                std::set<int> check = {combi[i].first, combi[i].second, combi[j].first, combi[j].second};
                
                if(check.size() == 4){
                    candPairs.push_back({event.jets[combi[i].first].fourVec + event.jets[combi[i].second].fourVec, event.jets[combi[j].first].fourVec + event.jets[combi[j].second].fourVec});
                }
            }
        }
    }

    //If 2 jets and one fat jet
    else if(event.jets.size() >= 2 and event.fatjets.size() == 1){
        for(std::pair<int, int> jetIndex: combi){
            candPairs.push_back({event.fatjets[0].fourVec, event.jets[jetIndex.first].fourVec + event.jets[jetIndex.second].fourVec});
        }
    }

    //If 2 fat jets
    else if(event.fatjets.size() == 2){
        candPairs.push_back({event.fatjets[0].fourVec, event.fatjets[1].fourVec});
    }

    //Not valid event
    else{
        event.h = {TLorentzVector(), TLorentzVector()};
        return;
    }

    //Sort candPairs for mass diff of jet pairs, where index 0 is the best pair
    std::function<bool(hCands, hCands)> sortFunc = [&](hCands cands1, hCands cands2){
        return std::abs(cands1.first.M() - cands1.second.M()) < std::abs(cands2.first.M() - cands2.second.M());
    };

    std::sort(candPairs.begin(), candPairs.end(), sortFunc);

    //Check if W Boson alread reconstructed
    if(event.W == TLorentzVector()) WBoson(event);

    //Reconstruct charge Higgs with smallest dR between h and W
    TLorentzVector Hc1 = event.W + candPairs[0].first;
    TLorentzVector Hc2 = event.W + candPairs[0].second;

    if(Hc1.DeltaPhi(candPairs[0].second) > Hc2.DeltaPhi(candPairs[0].first) and Hc1.DeltaPhi(candPairs[0].first) < Hc2.DeltaPhi(candPairs[0].second)){
        event.Hc = Hc1;
        event.h = {candPairs[0].first, candPairs[0].second};
    }

    else{
        event.Hc = Hc2;
        event.h = {candPairs[0].second, candPairs[0].first};
    }
}
