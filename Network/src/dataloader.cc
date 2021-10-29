#include <ChargedAnalysis/Network/include/dataloader.h>

DataLoader::DataLoader(const DNNDataSet& sigSet, const std::vector<DNNDataSet>& bkgSets, const std::size_t& batchSize, const float& validation, const bool& optimize) :
    sigSet(sigSet),
    bkgSets(bkgSets),
    batchSize(batchSize){
    
    //Allows opening/reading of TFiles in local thread
    ROOT::EnableThreadSafety();

    //Pure number of events for signal and background
    sigSetMaxEventTrain = sigSet.Size()*(1-validation);
    bkgSetMaxEventTrain = std::vector<std::size_t>(bkgSets.size());
    std::size_t nMin = 1e10;

    for(std::size_t set = 0; set < bkgSets.size(); ++set){
        bkgSetMaxEventTrain[set] = bkgSets[set].Size()*(1-validation);
        if(nMin > bkgSets[set].Size()) nMin = bkgSets[set].Size();
    }

    if(nMin > sigSet.Size()) nMin = sigSet.Size();

    //Calculate number of batches
    nBatchesTrain = optimize ? 30 : (bkgSets.size() + 1.)*nMin/batchSize*(1 - validation);
    nBatchesVali = (bkgSets.size() + 1.)*nMin/batchSize*validation;
}

DataLoader::~DataLoader(){
    if(trainThread.joinable()) trainThread.join();
    if(valiThread.joinable()) valiThread.join();
}

void DataLoader::InitSets(DNNDataSet& sigSet, std::vector<DNNDataSet>& bkgSets){
    std::unique_lock<std::mutex> lock(mutex);
    sigSet.Init();

    for(DNNDataSet& set : bkgSets){
        set.Init();
    }
}

void DataLoader::TrainBatcher(DNNDataSet sigSet, std::vector<DNNDataSet> bkgSets){
    try{
        InitSets(sigSet, bkgSets);
    
        std::size_t sigPos = 0;
        std::vector<std::size_t> bkgPos(bkgSets.size(), 0);

        for(std::size_t i = 0; i < nBatchesTrain; ++i){
            std::vector<DNNTensor> batch;

            sigPos = std::experimental::randint(0, int(sigSetMaxEventTrain - batchSize));
            for(std::size_t set = 0; set < bkgSets.size(); ++set) bkgPos[set] = std::experimental::randint(0, int(bkgSetMaxEventTrain[set] - batchSize));

            for(std::size_t j = 0; j < batchSize; j += bkgSets.size() + 1){
                batch.push_back(std::move(sigSet.Get(sigPos)));
                ++sigPos;

                for(std::size_t set = 0; set < bkgSets.size(); ++set){
                    batch.push_back(std::move(bkgSets[set].Get(bkgPos[set])));
                    ++bkgPos[set];
                }
            }

            trainBatches.push_back(std::move(DNNDataSet::Merge(batch)));

            std::unique_lock<std::mutex> lock(mutex);
            if(trainBatches.size() >= 20 and i != nBatchesTrain - 1) condition.wait(lock, [&](){return trainBatches.size() < 20;});
            lock.unlock();
            condition.notify_all();
        }
    }

    //Catch exception and wait until main threads terminates
    catch(...){
        std::unique_lock<std::mutex> lock(mutex);
        exception = std::current_exception();
    }
}

void DataLoader::ValidationBatcher(DNNDataSet sigSet, std::vector<DNNDataSet> bkgSets){
    try{
        InitSets(sigSet, bkgSets);

        std::size_t sigPos = sigSetMaxEventTrain + 1;
        std::vector<std::size_t> bkgPos(bkgSets.size(), 0);
        for(std::size_t set = 0; set < bkgSets.size(); ++set) bkgPos[set] = bkgSetMaxEventTrain[set] + 1;

        for(std::size_t i = 0; i < nBatchesVali; ++i){
            std::vector<DNNTensor> batch;

            for(std::size_t j = 0; j < batchSize; j += bkgSets.size() + 1){
                if(sigPos < sigSet.Size()){
                    batch.push_back(std::move(sigSet.Get(sigPos)));
                    ++sigPos;
                }

                for(std::size_t set = 0; set < bkgSets.size(); ++set){
                    if(bkgPos[set] >= bkgSets[set].Size()) continue;

                    batch.push_back(std::move(bkgSets[set].Get(bkgPos[set])));
                    ++bkgPos[set];
                }
            }

            valiBatches.push_back(std::move(DNNDataSet::Merge(batch)));

            std::unique_lock<std::mutex> lock(mutex);
            if(valiBatches.size() >= 20 and i != nBatchesVali - 1) condition.wait(lock, [&](){return valiBatches.size() < 20;});
            lock.unlock();
            condition.notify_all();
        }
    }

    //Catch exception and wait until main threads terminates
    catch(...){
        std::unique_lock<std::mutex> lock(mutex);
        exception = std::current_exception();
        condition.notify_all();
    }
}

void DataLoader::InitEpoch(){
    std::unique_lock<std::mutex> lock(mutex);

    //Check for exception in reading threads
    if(exception) std::rethrow_exception(exception);

    if(trainThread.joinable()) trainThread.join();
    trainThread = std::thread(&DataLoader::TrainBatcher, this, sigSet, bkgSets);

    if(valiThread.joinable()) valiThread.join();
    valiThread = std::thread(&DataLoader::ValidationBatcher, this, sigSet, bkgSets);
}

DNNTensor DataLoader::GetBatch(const bool& isVali){
    //Everything thread unsafe, so lock
    std::unique_lock<std::mutex> lock(mutex);

    //Check for exception in reading threads
    if(exception) std::rethrow_exception(exception);

    //Pop front of queue
    DNNTensor b;

    if(isVali){
        if(valiBatches.size() == 0) condition.wait(lock, [&](){return valiBatches.size() != 0 or exception;});
        if(exception) std::rethrow_exception(exception);

        b = std::move(valiBatches.front());
        valiBatches.pop_front();
    }

    else{
        if(trainBatches.size() == 0) condition.wait(lock, [&](){return trainBatches.size() != 0 or exception;});
        if(exception) std::rethrow_exception(exception);

        b = std::move(trainBatches.front());
        trainBatches.pop_front();
    }

    //Unlock and notify reader threads
    lock.unlock();
    condition.notify_all();

    return b;
}
