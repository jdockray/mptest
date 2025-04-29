export LD_LIBRARY_PATH=/usr/local/lib
export PMIX_MCA_pcompress_base_silence_warning=1
mpic++ -o mptest -fopenmp main.cpp && \
mpirun --host localhost:2 ./mptest 10 100
