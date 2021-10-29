#include <deque>
#include <vector>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>
#include <random>
#include <exception>
#include <stdexcept>
#include <experimental/random>

#include <TROOT.h>

#include <ChargedAnalysis/Network/include/dnndataset.h>

class DataLoader {
    private:
        DNNDataSet sigSet;
        std::vector<DNNDataSet> bkgSets;
        std::size_t batchSize, nBatchesTrain, nBatchesVali;

        std::thread trainThread, valiThread;
        std::mutex mutex;
        std::condition_variable condition;
        std::exception_ptr exception = nullptr;

        std::size_t sigSetMaxEventTrain;
        std::vector<std::size_t> bkgSetMaxEventTrain;
        torch::Tensor clsWeights;

        std::deque<DNNTensor> trainBatches, valiBatches;

        void InitSets(DNNDataSet& sigSet, std::vector<DNNDataSet>& bkgSets);
        void ValidationBatcher(DNNDataSet sigSet, std::vector<DNNDataSet> bkgSets);
        void TrainBatcher(DNNDataSet sigSet, std::vector<DNNDataSet> bkgSets);

    public:
        DataLoader(const DNNDataSet& sigSet, const std::vector<DNNDataSet>& bkgSets, const std::size_t& batchSize, const float& validation, const bool& optimize);
        ~DataLoader();

        std::size_t GetNTrainBatches(){return nBatchesTrain;}
        std::size_t GetNValiBatches(){return nBatchesVali;}
        torch::Tensor GetClsWeights(){return clsWeights;}

        void InitEpoch();
        DNNTensor GetBatch(const bool& isVali);
};
