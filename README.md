# mptest
A program demonstrating use of [Open MPI](https://www.open-mpi.org/) and [OpenMP](https://www.openmp.org/)

## Usage

Open MPI is an implementation of the [Message Passing Interface](https://www.mpi-forum.org/docs/) system for interprocess communication. To use Open MPI, it must be downloaded and built, programs must be compiled with a special wrapping compiler and then they should be started through an executable which launches the different processes.

OpenMP is a library which simplifies multithreaded execution and intraprocess communication. Many compilers already have the OpenMP library but you will likely need to enable it with a compiler flag.

[The program](main.cpp) should be started as an MPI job with a specified number of processes (a), number of threads per process (b) and number of values per thread (c). It generates a sequence of random numbers of length abc. It finds the sum of these numbers (modulo 2^64) by distributing the random numbers between the threads of each process. It combines the sums from each thread in each process and then combines the sums from each process in one of the processes, which then prints the sum (modulo 2^64). The expected sum is also calculated for verification by simply adding up all the values in the main thread of the root process.

## Usage

Compiling:
```
mpic++ -o mptest -fopenmp main.cpp
```
Example of running:
```
mpirun --host localhost:2 ./mptest 20 200
```
This starts the program using 2 processes, 20 threads per process and 200 values per thread. 8000 values will be summed.

Output from example:
```
Process 1 started
Process 0 started
2 processes, 20 threads per process, 200 values per thread
Expected result: 17320675716776
Actual result:   17320675716776
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file.
