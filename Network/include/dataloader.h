#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <experimental/random>

#include <TROOT.h>

#include <ChargedAnalysis/Network/include/dnndataset.h>

class DataLoader {
    private:
        std::vector<std::thread> threads;
        std::mutex mutex;
        std::condition_variable condition;

        std::vector<int> readOrder, readVali;
        int nProcessed = 0;

        std::size_t nBatches, nBatchesTrain;
        torch::Tensor classWeights;

        std::deque<std::tuple<std::vector<DNNTensor>, std::vector<int>, std::vector<int>>> batchQueue;

        void ReadBatch(std::vector<DNNDataSet> sigSets, std::vector<DNNDataSet> bkgSets, const std::vector<std::vector<std::pair<int, int>>> sigBatches, std::vector<std::vector<std::pair<int, int>>> bkgBatches);

    public:
        DataLoader(std::vector<DNNDataSet>& sigSets, std::vector<DNNDataSet>& bkgSets, const int& nClasses, const int& batchSize, const std::size_t& nThreads);

        ~DataLoader(){
            for(std::thread& t : threads) t.join();
        }

        std::size_t GetNBatches(){return nBatches;}
        std::size_t GetNTrainBatches(){return nBatchesTrain;}
        torch::Tensor GetClassWeights(){return classWeights;}

        void InitEpoch(const float& vali);
        std::tuple<std::vector<DNNTensor>, std::vector<int>, std::vector<int>> GetBatch();
};
