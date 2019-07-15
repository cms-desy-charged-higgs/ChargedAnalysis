#include <ChargedHiggs/Analysis/interface/treereader.h>

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

float TreeReader::Subtiness(Event &event, Hist &hist){
    std::map<float, float FatJet::*> subtiness = {
                                        {1., &FatJet::oneSubJettiness},    
                                        {2., &FatJet::twoSubJettiness},    
                                        {3., &FatJet::threeSubJettiness},
    };    
    
    if(hist.parts[0] == FATJET and !event.fatjets.empty()){
        return event.fatjets[hist.indeces[0]-1].*subtiness[hist.funcValue];
    }

    else{return -999.;}
}

float TreeReader::HadronicEnergy(Event &event, Hist &hist){
    return event.HT;
}

float TreeReader::EventNumber(Event &event, Hist &hist){
    return event.eventNumber;
}

float TreeReader::ConstantNumber(Event &event, Hist &hist){
    return hist.funcValue;
}

float TreeReader::PhiStar(Event &event, Hist &hist){
    

    TVector3 k =  TVector3(event.h1Jets[0].fourVec.Px(), event.h1Jets[0].fourVec.Py(), event.h1Jets[0].fourVec.Pz()).Cross(TVector3(event.h1Jets[1].fourVec.Px(), event.h1Jets[1].fourVec.Py(), event.h1Jets[1].fourVec.Pz()));
    TVector3 m = TVector3(event.h2Jets[0].fourVec.Px(), event.h2Jets[0].fourVec.Py(), event.h2Jets[0].fourVec.Pz()).Cross(TVector3(event.h2Jets[1].fourVec.Px(), event.h2Jets[1].fourVec.Py(), event.h2Jets[1].fourVec.Pz()));

    return std::acos(k.Dot(m)/(k.Mag()*m.Mag()));
}

float TreeReader::BDTScore(Event &event, Hist &hist){
    std::thread::id index = std::this_thread::get_id();
    std::vector<float> paramValues;

    for(Hist funcs: bdtFunctions[index]){
        paramValues.push_back((this->*funcDir[funcs.func])(event, funcs));
    }

    paramValues.push_back(hist.funcValue);

    float score = (int)EventNumber(event, hist) % 2 == 0 ? oddClassifier[index].Evaluate(paramValues) : evenClassifier[index].Evaluate(paramValues);

    return score;
}

float TreeReader::NSigParticle(Event &event, Hist &hist){
    int nSigPart = 0;

    switch(hist.parts[0]){
        case ELECTRON:
            for(Electron electron: event.electrons){
                if(electron.isFromHc) nSigPart++;
            }

            return nSigPart;

        case MUON:
            for(Muon muon: event.muons){
                nSigPart+= muon.isFromHc;
            }

            return nSigPart;

        case JET:
            for(Jet jet: event.jets){
                if(jet.isFromh1 or jet.isFromh2) nSigPart++;
            }

            return nSigPart;

        default: return -999.;
    }
}

float TreeReader::NParticle(Event &event, Hist &hist){
    int nPart = 0;
    int WP = hist.funcValue == -999. ? 1 : hist.funcValue;

    switch(hist.parts[0]){
        case ELECTRON:
            for(const Electron &ele: event.electrons){
                if(ele.*eleID[WP].first && ele.isolation < eleID[WP].second && ele.isTriggerMatched && ele.fourVec.Pt() > 35.){
                    nPart++;

                    event.weight *= ele.*eleSF[WP] * ele.recoSF;

                    for(unsigned int i = 0; i < event.jets.size(); i++){
                        if(event.jets[i].fourVec.DeltaR(ele.fourVec) < 0.4){
                            event.jets.erase(event.jets.begin()+i);
                            break;
                        }
                    }
                }
            } 
        
            return nPart;

        case MUON:
            for(const Muon &muon: event.muons){
                if(muon.*muonID[WP].first && muon.*muonID[WP].second && muon.isTriggerMatched && muon.fourVec.Pt() > 30.){
                    nPart++;
                    event.weight *= muon.*muonSF[WP].first * muon.*muonSF[WP].second;


                    for(unsigned int i = 0; i < event.jets.size(); i++){
                        if(event.jets[i].fourVec.DeltaR(muon.fourVec) < 0.4){
                            event.jets.erase(event.jets.begin()+i);
                            break;
                        }
                    }
                }
            } 
        
            return nPart;

        case JET:
            return event.jets.size();

        case SUBJET:
            return event.subjets.size();

        case FATJET:
            return event.fatjets.size();

        case BJET:
            for(const Jet &jet: event.jets){
                if(jet.*bJetID[WP]){
                    nPart++;
                    event.weight *= jet.*bJetSF[WP];
                }    
            }
            return nPart;

        case BSUBJET:
            for(const Jet &jet: event.subjets){
                if(jet.*bJetID[WP]){
                    nPart++;
                }    
            }
            return nPart;

        case BFATJET:
            for(const Jet &jet: event.fatjets){
                if(jet.*bJetID[WP]){
                    nPart++;
                    event.weight *= jet.*bJetSF[WP];
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
        case DIVISIBLE: return (int)(this->*funcDir[hist.func])(event, hist) % (int)hist.cutValues.second == 0;
        case NOTDIVISIBLE: return (int)(this->*funcDir[hist.func])(event, hist) % (int)hist.cutValues.second != 0;
        default: return true;
    }
}

//Function called by functions above to get particle of desire
const TLorentzVector& TreeReader::GetParticle(Event &event, Particle &part, const int &index){
    //Lambda function to construct small Higgs from h jets

    switch(part){
        case ELECTRON: return (unsigned int)index <= event.electrons.size() ? event.electrons[index-1].fourVec : event.dummy;
        case MUON: return (unsigned int)index <= event.muons.size() ? event.muons[index-1].fourVec : event.dummy;
        case JET: return (unsigned int)index <= event.jets.size() ? event.jets[index-1].fourVec : event.dummy;
        case SUBJET: return (unsigned int)index <= event.subjets.size() ? event.subjets[index-1].fourVec : event.dummy;
        case FATJET: return (unsigned int)index <= event.fatjets.size() ? event.fatjets[index-1].fourVec : event.dummy;
        case MET: return event.MET;
        case W: 
            if(event.W.empty() and index != 2){
                WBoson(event);
                return event.W[index-1];
            }

            else if(event.W.empty()){
                Top(event);
                return event.W[index-1];
            }

            else{
                return event.W[index-1];
            }

        case HC: 
            if(event.h1Jets.empty()){
                Higgs(event);
                return event.Hc;
            }

            return event.Hc;
        
        case GENHC:
            return event.genParts.Hc;

        case h: 
            if(event.h1Jets.empty()){
                Higgs(event);
                return event.h[index-1];
            }

            return event.h[index-1];

        case GENH:
            return index == 0 ? event.genParts.h1 : event.genParts.h2;

        case TOP:
            if(event.top.empty()){
                Top(event);
                return event.top[index-1];
            }

            return event.top[index-1];
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
    event.W.push_back(abs((lep + v1).M() - mW) < abs((lep + v2).M() - mW) ? lep + v1 : lep + v2);
}

void TreeReader::Higgs(Event &event){
    //Vector of candPairs
    typedef std::vector<std::vector<Jet>> hCandVec;

    hCandVec hCands; 
    
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
                    hCands.push_back({event.jets[combi[i].first], event.jets[combi[i].second], event.jets[combi[j].first], event.jets[combi[j].second]});
                }
            }
        }
    }

    //If 2 jets and one fat jet
    else if(event.jets.size() >= 2 and event.fatjets.size() == 1){
        for(std::pair<int, int> jetIndex: combi){
            hCands.push_back({event.fatjets[0], event.jets[jetIndex.first], event.jets[jetIndex.second]});
        }
    }

    //If 2 fat jets
    else if(event.fatjets.size() == 2){
        hCands.push_back({event.fatjets[0], event.fatjets[1]});
    }

    //Sort candPairs for mass diff of jet pairs, where index 0 is the best pair
    std::function<bool(std::vector<Jet>, std::vector<Jet>)> sortFunc = [&](std::vector<Jet> cands1, std::vector<Jet> cands2){
        if(cands1.size()==4){
            return std::abs((cands1[0].fourVec+cands1[1].fourVec).M() - (cands1[2].fourVec+cands1[3].fourVec).M()) < std::abs((cands2[0].fourVec+cands2[1].fourVec).M() - (cands2[2].fourVec+cands2[3].fourVec).M());
        }

        else if(cands1.size()==3){
            return std::abs(cands1[0].fourVec.M() - (cands1[1].fourVec+cands1[2].fourVec).M()) < std::abs(cands2[0].fourVec.M() - (cands2[1].fourVec+cands2[2].fourVec).M());
        }

        else return true;
    };

    std::function<bool(Jet, Jet)> sortPt = [&](Jet jet1, Jet jet2){
        return jet1.fourVec.Pt() > jet2.fourVec.Pt();
    };

    std::sort(hCands.begin(), hCands.end(), sortFunc);

    //Check if W Boson alread reconstructed
    if(event.W.empty()) WBoson(event);

    //Reconstruct charge Higgs with smallest dR between h and W
    TLorentzVector hCand1 = hCands[0].size()==4 ? hCands[0][0].fourVec + hCands[0][1].fourVec: hCands[0][0].fourVec;

    TLorentzVector hCand2 = hCands[0].size()==4 ? hCands[0][2].fourVec + hCands[0][3].fourVec: hCands[0].size()==3 ? hCands[0][1].fourVec + hCands[0][2].fourVec : hCands[0][1].fourVec;

    TLorentzVector Hc1 = event.W[0] + hCand1;
    TLorentzVector Hc2 = event.W[0] + hCand2;

    if(Hc1.DeltaPhi(hCand2) > Hc2.DeltaPhi(hCand1) and Hc1.DeltaPhi(hCand1) < Hc2.DeltaPhi(hCand2)){
        event.h1Jets = hCands[0].size()==4 ? std::vector<Jet>{hCands[0][0], hCands[0][1]} : std::vector<Jet>{hCands[0][0]};
        std::sort(event.h1Jets.begin(), event.h1Jets.end(), sortPt);
        event.h.push_back(hCands[0].size()==4 ? hCands[0][0].fourVec+hCands[0][1].fourVec : hCands[0][0].fourVec);

        event.h2Jets = hCands[0].size()==4 ? std::vector<Jet>{hCands[0][2], hCands[0][3]} : hCands[0].size()==3 ? std::vector<Jet>{hCands[0][1], hCands[0][2]} : std::vector<Jet>{hCands[0][1]};
        std::sort(event.h2Jets.begin(), event.h2Jets.end(), sortPt);
        event.h.push_back(hCands[0].size()==4 ? hCands[0][2].fourVec+hCands[0][3].fourVec : hCands[0].size()==3 ? hCands[0][1].fourVec+hCands[0][2].fourVec : hCands[0][1].fourVec);

        event.Hc = event.W[0] + event.h[0];
    }

    else{
        event.h1Jets =  hCands[0].size()==4 ? std::vector<Jet>{hCands[0][2], hCands[0][3]} :  hCands[0].size()==3 ? std::vector<Jet>{hCands[0][1], hCands[0][2]} : std::vector<Jet>{hCands[0][1]};
        event.h.push_back(hCands[0].size()==4 ? hCands[0][2].fourVec+hCands[0][3].fourVec : hCands[0].size()==3 ? hCands[0][1].fourVec+hCands[0][2].fourVec : hCands[0][1].fourVec);

        event.h2Jets = hCands[0].size()==4 ? std::vector<Jet>{hCands[0][0], hCands[0][1]} : std::vector<Jet>{hCands[0][0]};
        event.h.push_back(hCands[0].size()==4 ? hCands[0][0].fourVec+hCands[0][1].fourVec : hCands[0][0].fourVec);

        event.Hc = event.W[0] + event.h[0];
    }
}

void TreeReader::Top(Event &event){
    //Check if W Boson alread reconstructed
    if(event.W.empty()) WBoson(event);

    typedef std::tuple<TLorentzVector, TLorentzVector, TLorentzVector> topCand;
    typedef std::pair<TLorentzVector, TLorentzVector> WCand;

    //Sort top cands with mass, where index 0 is the best top
    std::function<bool(WCand, WCand, TLorentzVector)> sortTop2 = [&](WCand cand1, WCand cand2, TLorentzVector top1){
        return std::abs((cand1.first + cand1.second).M() - top1.M()) < std::abs((cand2.first + cand2.second).M() - top1.M());
    };

    //Sort top cands with mass, where index 0 is the best top
    std::function<bool(topCand, topCand)> sortTop = [&](topCand cand1, topCand cand2){
        return std::abs(std::get<0>(cand1).M() - (std::get<1>(cand1) + std::get<2>(cand1)).M()) < std::abs(std::get<0>(cand2).M() - (std::get<1>(cand2) + std::get<2>(cand2)).M());
    };

    std::vector<topCand> topCands;

    for(unsigned int i = 0; i < event.jets.size(); i++){
        TLorentzVector top1Cand = event.W[0] + event.jets[i].fourVec;
        std::vector<WCand> WCands;

        for(unsigned int j = 0; j < event.jets.size(); j++){
            for(unsigned int k = 0; k < j; k++){
                for(unsigned int m = 0; m < k; m++){
                    if(i!=j and i!=k and i!=m){
                        float alpha = 80.399/(event.jets[j].fourVec + event.jets[k].fourVec).M();

                        WCands.push_back({alpha*event.jets[j].fourVec + alpha*event.jets[k].fourVec, event.jets[m].fourVec});
                    } 
                }            
            }
        }

        std::sort(WCands.begin(), WCands.end(), std::bind(sortTop2, std::placeholders::_1, std::placeholders::_2, top1Cand));

        topCands.push_back({top1Cand,  WCands[0].first,  WCands[0].second});
    }

    std::sort(topCands.begin(), topCands.end(), sortTop);

    event.W.push_back(std::get<1>(topCands[0]));
    event.top = {std::get<0>(topCands[0]), std::get<1>(topCands[0])+std::get<2>(topCands[0])};
}
