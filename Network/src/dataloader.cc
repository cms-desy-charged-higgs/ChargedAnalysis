#include <ChargedAnalysis/Network/include/dataloader.h>

DataLoader::DataLoader(std::vector<DNNDataSet>& sigSets, std::vector<DNNDataSet>& bkgSets, const int& nClasses, const int& batchSize, const std::size_t& nThreads){
    //Allows opening/reading of TFiles in local thread
    ROOT::EnableThreadSafety();

    //Calculate weights to equalize pure number of events for signal and background
    std::vector<int> nBkg(nClasses - 1, 0);
    int nSig = 0, nBkgTotal = 0;
    
    for(DNNDataSet& set : sigSets) nSig += set.Size();
    
    for(DNNDataSet& set : bkgSets){        
        nBkg[set.GetClass()] += set.Size();
        nBkgTotal += set.Size();
    }

    float wSig = (nBkgTotal+nSig)/float(nSig);
    std::vector<float> wBkg(nClasses - 1, 1.);
    
    for(int i = 0; i < nClasses - 1; ++i){
        wBkg[i] = (nBkgTotal+nSig)/float(nBkg[i]);
    }

    //Calculate number of batches
    nBatches = nSig+nBkgTotal % batchSize == 0 ? (nSig+nBkgTotal)/batchSize - 1 : std::ceil((nSig+nBkgTotal)/batchSize);

    //Fill ranges of event number for each batch
    std::vector<std::vector<std::pair<int, int>>> sigBatches;
    std::vector<std::vector<std::pair<int, int>>> bkgBatches;

    for(DNNDataSet& set : sigSets){
        int bSize  = float(set.Size())*batchSize/(nBkgTotal+nSig);
        int skip = bSize > 1 ? 0 : std::ceil(1./bSize);
    
        std::vector<std::pair<int, int>> batches(nBatches);

        for(std::size_t i = 0; i < nBatches; ++i){
            if(skip == 0){
                batches.at(i) = {i*bSize, (i + 1)*bSize};
            }

            else if(i % skip == 0){
                batches.at(i/skip) = {i/skip*bSize, (i/skip + 1)*bSize};
            }
        }

        sigBatches.push_back(std::move(batches));
    }

    for(DNNDataSet& set : bkgSets){
        float bSize  = float(set.Size())*batchSize/(nBkgTotal+nSig);
        int skip = bSize > 1 ? 0 : std::ceil(1./bSize);
        
        std::vector<std::pair<int, int>> batches(nBatches);

        for(std::size_t i = 0; i < nBatches; ++i){
            if(skip == 0){
                batches.at(i) = {i*bSize, (i + 1)*bSize};
            }

            else if(i % skip == 0){
                batches.at(i/skip) = {i/skip*bSize, (i/skip + 1)*bSize};
            }
        }

        bkgBatches.push_back(std::move(batches));
    }
    
    //Class weights
    classWeights = torch::from_blob(VUtil::Append(wBkg, wSig).data(), {nClasses}).clone();

    //Define threads
    threads = std::vector<std::thread>(nThreads);
    std::size_t vecSize = sigBatches.size()/nThreads;

    for(std::size_t thread = 0; thread < nThreads; ++thread){
        //Start threads
        threads[thread] = std::thread(&DataLoader::ReadBatch, this, sigSets, bkgSets, sigBatches, bkgBatches);
    }
}

void DataLoader::ReadBatch(std::vector<DNNDataSet> sigSets, std::vector<DNNDataSet> bkgSets, const std::vector<std::vector<std::pair<int, int>>> sigBatches, std::vector<std::vector<std::pair<int, int>>> bkgBatches){
    //Init of dataset class (Open files, set up ntuplerreader)
    for(DNNDataSet& set : sigSets) set.Init();
    for(DNNDataSet& set : bkgSets) set.Init();

    //Wait for first initiazer call (if not already happended)
    std::unique_lock<std::mutex> lock(mutex);
    while(readOrder.size() == 0) condition.wait(lock);
    lock.unlock();

    while(true){
        //Get current batch idx to read and increment counter (thread unsafe, so lock up)
        std::unique_lock<std::mutex> lock(mutex);
        int idx = readOrder.at(nProcessed);
        ++nProcessed;
        lock.unlock();

        //Read data via ntuplereader used in dataset class (thread safe)
        std::vector<DNNTensor> batch;
        std::vector<int> batchChargedMass, batchNeutralMass;

        for(int k = 0; k < sigSets.size(); ++k){
            std::vector<DNNTensor> sigBatch = sigSets.at(k).GetBatch(sigBatches.at(k).at(idx).first, sigBatches.at(k).at(idx).second);

            for(int l = 0; l < sigBatch.size(); ++l){
                batchChargedMass.push_back(sigSets.at(k).chargedMass);
                batchNeutralMass.push_back(sigSets.at(k).neutralMass);
            }

            batch.insert(batch.end(), std::make_move_iterator(sigBatch.begin()), std::make_move_iterator(sigBatch.end()));
        }

        for(int k = 0; k < bkgSets.size(); ++k){
            std::vector<DNNTensor> bkgBatch = bkgSets.at(k).GetBatch(bkgBatches.at(k).at(idx).first, bkgBatches.at(k).at(idx).second);

            for(int l = 0; l < bkgBatch.size(); ++l){
                int chargedIndex = std::experimental::randint(0, (int)sigSets.size() -1);
                int neutralIndex = std::experimental::randint(0, (int)sigSets.size() -1);

                batchChargedMass.push_back(sigSets.at(chargedIndex).chargedMass);
                batchNeutralMass.push_back(sigSets.at(neutralIndex).neutralMass);
            }

            batch.insert(batch.end(), std::make_move_iterator(bkgBatch.begin()), std::make_move_iterator(bkgBatch.end()));
        }

        lock.lock();

        //Put batch in queue (thread unsafe, therefore lock)
        batchQueue.push_back(std::make_tuple(std::move(batch), std::move(batchChargedMass), std::move(batchNeutralMass)));

        while(batchQueue.size() >= 10*threads.size() or nProcessed == nBatches){ //Sleep while queue is full or epoch is done
            condition.wait(lock);
        }

        //Unlock and notify Batch returner function
        lock.unlock();
        condition.notify_all();
    }
}

void DataLoader::InitEpoch(const float& val){
    //Reset batch counter
    std::unique_lock<std::mutex> lock(mutex);
    nProcessed = 0;

    nBatchesTrain = int(nBatches*(1-val));

    //Shuffle order of drawn batch
    readOrder = VUtil::Range(0, int(nBatchesTrain - 1), int(nBatchesTrain));
    std::random_shuffle(readOrder.begin(), readOrder.end());

    //Normal read order for validation
    for(std::size_t batch = nBatchesTrain; batch < nBatches; ++batch) readOrder.push_back(batch);

    //Unlock and notify Batch returner function
    lock.unlock();
    condition.notify_all();
}

std::tuple<std::vector<DNNTensor>, std::vector<int>, std::vector<int>> DataLoader::GetBatch(){
    //Everything thread unsafe, so lock
    std::unique_lock<std::mutex> lock(mutex);

    //While queue is empty, wait for reader threads
    while(batchQueue.size() == 0){
        condition.wait(lock);
    }
    
    //Pop front of queue
    std::tuple<std::vector<DNNTensor>, std::vector<int>, std::vector<int>> b = batchQueue.front();
    batchQueue.pop_front();

    //Unlock and notify reader threads
    lock.unlock();
    condition.notify_all();

    return b;
}
