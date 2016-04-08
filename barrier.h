#ifndef __BARRIER_H__
#define __BARRIER_H__
#include <atomic>
#include <cstdlib>

struct CoreCacheLine{
	//char padding1[64-2*sizeof(std::atomic<int>)];
	std::atomic<int> signalLeft;
	//char padding2[64-2*sizeof(std::atomic<int>)];
	std::atomic<int> signalRight;
	//char padding3[64-2*sizeof(std::atomic<int>)];
};

class IBarrier{
public:
	virtual void barrier(size_t threadId) = 0;
};

class Barrier:public IBarrier{
private:
	CoreCacheLine *messages;
	size_t nThreads;

	inline void sendRight(size_t threadId){
		while(messages[threadId].signalRight.load(std::memory_order_relaxed) != 0);
		messages[threadId].signalRight.store(1, std::memory_order_relaxed);

		while(messages[threadId].signalRight.load(std::memory_order_relaxed) != 2);
		messages[threadId].signalRight.store(0, std::memory_order_acq_rel);
	}

	inline void receiveLeft(size_t threadId){
		while(messages[threadId-1].signalRight.load(std::memory_order_relaxed) != 1);
		messages[threadId-1].signalRight.store(2, std::memory_order_acq_rel);
	}

	inline void sendLeft(size_t threadId){
		int ex;
		do{
			ex = 0;
		}
		while(!messages[threadId].signalLeft.compare_exchange_weak(ex, 1, std::memory_order_relaxed));

		do{
			ex = 2;
		}
		while(!messages[threadId].signalLeft.compare_exchange_weak(ex, 0, std::memory_order_acq_rel));
	}

	inline void receiveRight(size_t threadId){
		int ex;
		do{
			ex = 1;
		}
		while(!messages[threadId+1].signalLeft.compare_exchange_weak(ex, 2, memory_order_acq_rel));
	}
public:
	Barrier(size_t n = 0){
		nThreads = n;
		messages = new CoreCacheLine[nThreads];
		for(size_t i=0; i<nThreads; ++i){
			messages[i].signalLeft.store(0);
			messages[i].signalRight.store(0);
		}
	}

	void barrier(size_t threadId){
		if (threadId == 0){
			sendRight(threadId);
		}
		else{
			receiveLeft(threadId);
			if (threadId != nThreads - 1){
				sendRight(threadId);
			}
		}

		if (threadId == nThreads-1){
			sendLeft(threadId);
		}
		else{
			receiveRight(threadId);
			if (threadId != 0){
				sendLeft(threadId);
			}
		}
	}
};

class BarrierCounter:public IBarrier{
private:
	std::atomic<int> entered;
	std::atomic<int> leaved;
	size_t nThreads;
public:
	BarrierCounter(size_t n){
		nThreads = n;
		entered.store(0);
		leaved.store(0);
	}
	
	inline void barrier(size_t threadId){
		while(leaved.load(std::memory_order_relaxed));
		
		entered.fetch_add(1, std::memory_order_relaxed);
		while(entered.load(std::memory_order_relaxed) < nThreads);

		leaved.fetch_add(1, std::memory_order_relaxed);
		while(leaved.load(std::memory_order_relaxed) < nThreads);

		int old = entered.fetch_add(-1, std::memory_order_acq_rel);
		if (old == 1){
			leaved.store(0, std::memory_order_relaxed);
		}
	}
};

class BarrierXOR:public IBarrier{
private:
	std::atomic<unsigned int> entered;
	std::atomic<unsigned int> leaved;
	unsigned int ones;
public:
	BarrierXOR(size_t n){
		ones = 1;
		for(size_t i=1; i<n; ++i){
			ones = (ones << 1) + 1;
		}

		leaved.store(0);
		entered.store(ones);
	}

	inline void barrier(size_t threadId){
		unsigned int mask = 1 << threadId;

		while(leaved.load(std::memory_order_relaxed)){
			//nop
		}

		entered.fetch_xor(mask, std::memory_order_acquire);
		while(entered.load(std::memory_order_relaxed)){
			//nop
		}

		leaved.fetch_xor(mask, std::memory_order_relaxed);
		while(leaved.load(std::memory_order_relaxed) != ones){
			//nop
		}

		unsigned int old = entered.fetch_xor(mask, std::memory_order_release);
		if (old == (ones ^ mask)){
			leaved.store(0, std::memory_order_relaxed);
		}
	}
};


class BarrierCond:public IBarrier{
private:
	unsigned int nThreads;

	// Your code here
public:
	BarrierCond(size_t n):nThreads(n){
		// Your code here
	}

	inline void barrier(int threadId){
		// Your code here
	}
};
#endif