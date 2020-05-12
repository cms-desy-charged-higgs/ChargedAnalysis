#include <ChargedAnalysis/Analysis/include/treeappender.h>

TreeAppender::TreeAppender() {}

TreeAppender::TreeAppender(const std::string& oldFile, const std::string& oldTree, const std::string& newFile, const std::vector<std::string>& branchNames, const std::string& dCacheDir) :
        oldFile(oldFile), 
        oldTree(oldTree),
        newFile(newFile),
        branchNames(branchNames),
        dCacheDir(dCacheDir){}

std::vector<float> TreeAppender::HScore(const int& FJindex, const int& length){
    //Vector with final score (FatJetIndex/Score per event)
    std::vector<float> tagValues(length, -999.);

    //Tagger
    torch::Device device(torch::kCPU);
    std::vector<std::shared_ptr<HTagger>> tagger(2, std::make_shared<HTagger>(7, 140, 1, 130, 57, 0.06, device));

    torch::load(tagger[0], std::string(std::getenv("CHDIR")) + "/DNN/Model/Even/htagger.pt");
    torch::load(tagger[1], std::string(std::getenv("CHDIR")) + "/DNN/Model/Odd/htagger.pt");

    //Get data set
    HTagDataset data = HTagDataset({oldFile + "/"  + oldTree}, FJindex, device, true);
    std::vector<int> entries;

    for(int i = 0; i < length; i++){entries.push_back(i);}
    std::vector<int>::iterator entry = entries.begin();
    int batchSize = length > 2500 ? 2500 : length;
    int counter = 0;
    
    while(entry != entries.end()){
        //For right indexing
        std::vector<int> evenIndex, oddIndex;

        //Put signal + background in one vector and split by even or odd numbered event
        std::vector<HTensor> evenTensors;
        std::vector<HTensor> oddTensors;

        for(int i = 0; i < batchSize; i++){
            HTensor tensor = data.get(*entry);

            if(tensor.isEven.item<bool>() == true){
                evenTensors.push_back(tensor);
                evenIndex.push_back(counter);
            }

            else{
                oddTensors.push_back(tensor);
                oddIndex.push_back(counter);
            }

            counter++;
            ++entry;
            if(entry == entries.end()) break; 
        }

        //Prediction
        torch::NoGradGuard no_grad;
        HTensor even, odd; 
        torch::Tensor evenPredict, oddPredict;      

        if(evenTensors.size() != 0){
            even = HTagDataset::PadAndMerge(evenTensors);
            evenPredict = tagger[1]->forward(even.charged, even.neutral, even.SV);
        }  

        if(oddTensors.size() != 0){    
            odd = HTagDataset::PadAndMerge(oddTensors);
            oddPredict = tagger[0]->forward(odd.charged, odd.neutral, odd.SV);
        }     

        //Put all predictions back in order again
        for(int j = 0; j < evenTensors.size(); j++){
            tagValues[evenIndex[j]] = evenPredict[j].item<float>();
        }

        for(int j = 0; j < oddTensors.size(); j++){
            tagValues[oddIndex[j]] = oddPredict[j].item<float>();
        }
    }

    return tagValues;
}

std::map<int, std::vector<float>> TreeAppender::DNNScore(const std::vector<int>& masses, TTree* oldT){
    std::map<int, std::vector<float>> values;

    for(const int& mass: masses){
        values[mass] = std::vector<float>(oldT->GetEntries(), -999.);
    }


    /*

    std::string dnnPath = std::string(std::getenv("CHDIR")) + "/DNN/Analysis/"; 

    Event event;
    TreeReader reader;

    reader.PrepareEvent<TTree*>(oldT);

    std::vector<Function> functions;
    std::vector<FuncArgs> args;

    std::ifstream params(dnnPath + "/Even/" + Utils::ChanPaths(oldTree) + "/parameter.txt"); 
    std::string parameter;
  
    while(getline(params, parameter)){
        Function func; FuncArgs arg;

        reader.GetFunction(parameter, func, arg);
        reader.GetParticle(parameter, arg);

        functions.push_back(func);
        args.push_back(arg);
    }
    params.close();

    //Tagger
    torch::Device device(torch::kCPU);
    std::vector<std::shared_ptr<DNNModel>> model(2, std::make_shared<DNNModel>(functions.size(), 2*functions.size(), 2, 0.1, device));

    torch::load(model[0], dnnPath + "/Even/" + Utils::ChanPaths(oldTree) + "/model.pt");
    torch::load(model[1], dnnPath + "/Odd/" + Utils::ChanPaths(oldTree) + "/model.pt");

    std::vector<int> entries;
    for(int i = entryStart; i < entryEnd; i++){entries.push_back(i);}
    std::vector<int>::iterator entry = entries.begin();
    int batchSize = entryEnd - entryStart > 2500 ? 2500 : entryEnd - entryStart;
    int counter = 0;

    while(entry != entries.end()){
        //For right indexing
        std::vector<int> evenIndex, oddIndex;

        //Put signal + background in one vector and split by even or odd numbered event
        std::map<int, std::vector<torch::Tensor>> evenTensors;
        std::map<int, std::vector<torch::Tensor>> oddTensors;

        for(int j = 0; j < batchSize; j++){
            oldT->GetEntry(*entry);

            //Fill event class with particle content
            reader.SetEvent(event, JET, NONE);
            std::vector<float> paramValues;

            for(int i=0; i < functions.size(); i++){
                paramValues.push_back(functions[i](event, args[i]));
            }

            if((int)TreeFunction::EventNumber(event, args[0]) % 2 == 0){
                for(const int& mass: masses){
                    paramValues.push_back(mass);
                    evenTensors[mass].push_back(torch::from_blob(paramValues.data(), {1, paramValues.size()}).clone().to(device));
                    paramValues.pop_back();
                }

                evenIndex.push_back(counter);
            }

            else{
                for(const int& mass: masses){
                    paramValues.push_back(mass);
                    oddTensors[mass].push_back(torch::from_blob(paramValues.data(), {1, paramValues.size()}).clone().to(device));
                    paramValues.pop_back();
                }

                oddIndex.push_back(counter);
            }

            counter++;
            ++entry;
            if(entry == entries.end()) break; 
        }

        for(const int& mass: masses){
            //Prediction
            torch::NoGradGuard no_grad;
            torch::Tensor even, odd; 
            torch::Tensor evenPredict, oddPredict;

            if(evenTensors[mass].size() != 0){    
                even = torch::cat(evenTensors[mass], 0);
                evenPredict = model[1]->forward(even);
            }  

            if(oddTensors[mass].size() != 0){    
                odd = torch::cat(oddTensors[mass], 0);
                oddPredict = model[0]->forward(odd);
            }     

            //Put all predictions back in order again
            for(int j = 0; j < evenTensors[mass].size(); j++){
                values[mass][evenIndex[j]] = evenPredict[j].item<float>();
            }

            for(int j = 0; j < oddTensors[mass].size(); j++){
                values[mass][oddIndex[j]] = oddPredict[j].item<float>();
            }
        }
    }
    */

    return values;
}

std::map<int, std::vector<float>> TreeAppender::BDTScore(const std::vector<int>& masses, TTree* oldT){
    std::map<int, std::vector<float>> values;

    /*
    std::string bdtPath = std::string(std::getenv("CHDIR")) + "/BDT/"; 

    Event event;
    TreeReader reader;

    reader.PrepareEvent<TTree*>(oldT);

    std::vector<Function> functions;
    std::vector<FuncArgs> args;

    std::ifstream bdtParams(bdtPath + "/Even/" + Utils::ChanPaths(oldTree) + "/parameters.txt"); 
    std::string parameter;
  
    while(getline(bdtParams, parameter)){
        Function func; FuncArgs arg;

        reader.GetFunction(parameter, func, arg);
        reader.GetParticle(parameter, arg);

        functions.push_back(func);
        args.push_back(arg);
    
    }
    bdtParams.close();

    BDT evenClassifier, oddClassifier;

    evenClassifier.SetEvaluation(bdtPath + "/Even/" + Utils::ChanPaths(oldTree));
    oddClassifier.SetEvaluation(bdtPath + "/Odd/" + Utils::ChanPaths(oldTree));

    for (int i = entryStart; i < entryEnd; i++){
        oldT->GetEntry(i);

        //Fill event class with particle content
        reader.SetEvent(event, JET, NONE);

        std::vector<float> paramValues;

        for(int i=0; i < functions.size(); i++){
            paramValues.push_back(functions[i](event, args[i]));
        }

        for(const int& mass: masses){
            paramValues.push_back(mass);

            values[mass].push_back((int)TreeFunction::EventNumber(event, args[i]) % 2 == 0 ? oddClassifier.Evaluate(paramValues) : evenClassifier.Evaluate(paramValues));

            paramValues.pop_back();
        }
    }

    */

    return values;
}

void TreeAppender::Append(){
    at::set_num_interop_threads(1);
    at::set_num_threads(1);

    //Get old Tree
    TFile* oldF = TFile::Open(oldFile.c_str(), "READ");
    TTree* oldT = (TTree*)oldF->Get(oldTree.c_str());

    std::cout << "Read file: '" << oldFile << "'" << std::endl;
    std::cout << "Read tree '" << oldTree << "'" << std::endl;

    //Disables wished branches if they are already existing in old tree
    for(std::string& branchName: branchNames){
        if(oldT->GetListOfBranches()->Contains(branchName.c_str())){
            oldT->SetBranchStatus(branchName.c_str(), 0);
        }
    }

    //Clone Tree
    std::string tmpFile = "/tmp/tmp_" + std::to_string(getpid()) + ".root";

    TFile* newF = TFile::Open(tmpFile.c_str(), "RECREATE");
    TTree* newT = oldT->CloneTree(-1, "fast");

    //Set new branches
    std::map<std::string, TBranch*> branches;
    std::map<std::string, float> branchValues;
    std::map<std::string, std::vector<float>> values;
    std::vector<int> masses;

    for(std::string& branchName: branchNames){
        branchValues[branchName] = -999.;

        branches[branchName] = newT->Branch(branchName.c_str(), &branchValues[branchName]);

        if(branchName == "ML_HTagFJ1") values[branchName] = HScore(0, oldT->GetEntries());
        if(branchName == "ML_HTagFJ2") values[branchName] = HScore(1, oldT->GetEntries());

        if(Utils::Find<std::string>(branchName, "BDT") != -1.){
            values[branchName] = std::vector<float>();
            masses.push_back(std::stoi(branchName.substr(branchName.size()-3,3)));
        }

        if(Utils::Find<std::string>(branchName, "DNN") != -1.){
            values[branchName] = std::vector<float>();
            masses.push_back(std::stoi(branchName.substr(branchName.size()-3,3)));
        }
    }

    std::map<int, std::vector<float>> bdtScores;
    std::map<int, std::vector<float>> dnnScores;

    if(!masses.empty()){
        dnnScores = DNNScore(masses, oldT);

        for(const int& mass: masses){
            for(std::string& branchName: branchNames){
                if(Utils::Find<int>(branchName, mass) != -1.){
                    values[branchName] = dnnScores[mass];
                }
            }
        }
    } 

    //Fill branches
    for(int i=0; i < newT->GetEntries(); i++){
        if(i % 100000 == 0 and i != 0){
            std::cout << "Processed events: " << i << std::endl;
        }

        for(std::string& branchName: branchNames){
            branchValues[branchName] = values[branchName][i];       
            branches[branchName]->Fill();
        }
    }

    newF->cd();
    newT->Write();

    std::cout << "Sucessfully append branches " << branchNames << " in tree " << oldTree << std::endl;

    TList* keys = oldF->GetListOfKeys();

    for(int i=0; i < keys->GetSize(); i++){
        bool skipKey = false;

        TObject* obj = oldF->Get(keys->At(i)->GetName());
        if(obj->InheritsFrom(TTree::Class())) continue;

        obj->Write();
    }
    

    delete oldT;
    delete oldF;
    delete newT;
    delete newF;

    std::system(("mv -vf " + tmpFile + " " + newFile).c_str());

    if(dCacheDir != "") Utils::CopyToCache(newFile, dCacheDir);
}
