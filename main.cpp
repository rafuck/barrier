#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <time.h>
#include <omp.h>
#include <atomic>
#include <unistd.h>
#include <thread>
#include <vector>
#include "barrier.h"

const size_t nAttempts = 10000;

double timer(){
	static clock_t start = clock();

	clock_t end = clock();
	double  ret = (double)(end - start)/CLOCKS_PER_SEC;
	
	start = end;
	
	return ret;
}

void threadFunction(IBarrier &b, size_t threadId, size_t nThreads){
	for(int i=0; i<nAttempts; ++i){
		if ((i%100) == 0) printf("%d", i);
		b.barrier(threadId);

		if(threadId == nThreads-1 && (i%100) == 0){
			printf("\n");
		}
		b.barrier(threadId);
	}
}

double testBarrier(IBarrier &b, size_t nThreads){
	std::vector<std::thread> worker;
	for(size_t i=0; i<nThreads-1; ++i){
		worker.push_back(std::thread(threadFunction, std::ref(b), i+1, nThreads));
	}
	timer();
	threadFunction(b, 0, nThreads);
	double ret = timer();

	for(size_t i=0; i<nThreads-1; ++i){
		worker[i].join();
	}

	return ret;
}

int main(void){
	const size_t nThreads = std::thread::hardware_concurrency();
	printf("Concurency: %zu\n", nThreads);
	BarrierCounter bc(nThreads);
	Barrier b(nThreads);
	BarrierXOR bxor(nThreads);
	timer();

	#pragma omp parallel num_threads(nThreads)
	{
		size_t threadId = omp_get_thread_num();
		for(int i=0; i<nAttempts; ++i){
			if ((i%100) == 0) printf("%d", i);
			#pragma omp barrier
			if(threadId == 0 && (i%100) == 0){
				printf("\n");
			}
			#pragma omp barrier
		}
	}
	printf("OpenMP = %e s\n", timer());

	printf("barrier counter = %e s\n", testBarrier(bc, nThreads));
	printf("barrier XOR = %e s\n", testBarrier(bxor, nThreads));
	printf("barrier messages = %e s\n", testBarrier(b, nThreads));

	return EXIT_SUCCESS;
}