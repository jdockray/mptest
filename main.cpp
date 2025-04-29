
#include <iostream>
#include <random>
#include <inttypes.h>
#include <omp.h>
#include <mpi.h>

class MPISession { // To ensure MPI_Finalize() is called using RAII
    private:
        int providedThreadSupportLevelVal = MPI_THREAD_SINGLE;
        int clusterSizeVal = 0;
        int rankVal = 0;
    
    public:
        MPISession(int& argc, char**& argv, int requestedThreadSupportLevel);  
        int providedThreadSupportLevel() const;
        int clusterSize() const;
        int rank() const;
        ~MPISession();
};

void doProcessWork(unsigned int rank, uint32_t* dataStart,
        unsigned int valuesPerThread, uint64_t& globalTotal) {
    uint64_t processSpecificTotal = 0;
    #pragma omp parallel reduction(+:processSpecificTotal)
    {
        unsigned int thread = omp_get_thread_num();
        for (uint32_t* ptr = dataStart + thread * valuesPerThread;
            ptr < dataStart + (thread + 1) * valuesPerThread;
            ++ptr) {
                processSpecificTotal += *ptr;
            }
    }
    MPI_Reduce(&processSpecificTotal, &globalTotal, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
}

void doRootProcessWork(unsigned int clusterSize, unsigned int threadsPerProcess, unsigned int valuesPerThread) {
    std::printf("%u processes, %u threads per process, %u values per thread\n",
        clusterSize, threadsPerProcess, valuesPerThread);
    unsigned int totalNumberOfValues = clusterSize * threadsPerProcess * valuesPerThread;
    std::vector<uint32_t> values;
    values.reserve(totalNumberOfValues);
    std::mt19937 mersenneTwister; // Without seeding the random values will always be the same
    uint64_t expectedTotal = 0; // Will harmlessly overflow returning total modulo 2^64
    for (unsigned int i = 0; i < totalNumberOfValues; ++i) {
        uint32_t value = mersenneTwister(); 
        values.push_back(value);
        expectedTotal += value;
    }
    uint32_t* dataStart = values.data();
    unsigned int valuesPerProcess = threadsPerProcess * valuesPerThread;
    for (unsigned int targetRank = 1; targetRank < clusterSize; ++targetRank) {
        MPI_Send(values.data() + valuesPerProcess * targetRank,
            valuesPerProcess, MPI_UINT32_T, targetRank, 0, MPI_COMM_WORLD);
    }
    uint64_t globalTotal = 0;
    doProcessWork(0, values.data(), valuesPerThread, globalTotal);
    std::printf("Expected result: %" PRIu64 "\n", expectedTotal);
    std::printf("Actual result:   %" PRIu64 "\n", globalTotal);    
}

void doWorkerProcessWork(unsigned int threadsPerProcess, unsigned int valuesPerThread, unsigned int processRank) {
    unsigned int valuesPerProcess = threadsPerProcess * valuesPerThread;
    std::vector<uint32_t> values(valuesPerProcess);
    MPI_Status status = {0};
    MPI_Recv(values.data(), valuesPerProcess, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD, &status);
    uint64_t globalTotal = 0;
    doProcessWork(processRank, values.data(), valuesPerThread, globalTotal);
}

int main(int argc, char *argv[])
{
    MPISession mpiSession(argc, argv, MPI_THREAD_FUNNELED);
    std::printf("Process %u started\n", mpiSession.rank());

    if (argc != 3) {
        std::cerr << "Usage: ./mptest threads_per_process values_per_thread" << std::endl;
        return -1;
    }
    unsigned int threadsPerProcess = std::max(std::atoi(argv[1]), 0);
    omp_set_num_threads(threadsPerProcess);
    unsigned int valuesPerThread = std::max(std::atoi(argv[2]), 0);
    if (threadsPerProcess > 1
            && mpiSession.providedThreadSupportLevel() < MPI_THREAD_FUNNELED) {
        std::cerr << "MPI used does not support multiple threads" << std::endl;
        return -1;
    }
    
    if (mpiSession.rank() == 0) {
        doRootProcessWork(mpiSession.clusterSize(), threadsPerProcess, valuesPerThread);
    } else {
        doWorkerProcessWork(threadsPerProcess, valuesPerThread, mpiSession.rank());
    }

    return 0;
}

MPISession::MPISession(int& argc, char**& argv, int requestedThreadSupportLevel) {
    MPI_Init_thread(&argc, &argv, requestedThreadSupportLevel, &providedThreadSupportLevelVal);
    MPI_Comm_size(MPI_COMM_WORLD, &clusterSizeVal);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankVal);
}

int MPISession::providedThreadSupportLevel() const {
    return providedThreadSupportLevelVal;
}

int MPISession::clusterSize() const {
    return clusterSizeVal;
}

int MPISession::rank() const {
    return rankVal;
}    

MPISession::~MPISession() {
    MPI_Finalize();
}
