#include "CUpch.h"
#include "ThreadPool.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include "Containers/Queue.hpp"

#include <assert.h>

namespace CU
{
	constexpr uint32_t queueSize = 128U;

	struct ThreadPool::InternalPImpl
	{
		CU::Queue<std::function<void()>, queueSize> queue;
		std::mutex mutex;
		std::condition_variable cv;
		std::thread* workers = nullptr;
		size_t size = 0U;
		bool terminateWorkers = false;
	};

	ThreadPool::eAddJobResult ThreadPool::AddJob(std::function<void()> aJob)
	{
		if (myInternal->queue.Size() == queueSize)
		{
			return eAddJobResult::ErrorQueueFull;
		}

		{
			std::unique_lock<std::mutex> lock(myInternal->mutex);
			myInternal->queue.Enqueue(aJob);
		}
		myInternal->cv.notify_one();

		return eAddJobResult::Success;
	}

	void ThreadPool::Work()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(myInternal->mutex);

			myInternal->cv.wait(lock, [&] { return myInternal->queue.Size() != 0 || myInternal->terminateWorkers; });

			if (myInternal->terminateWorkers)
			{
				return;
			}

			myInternal->queue.Dequeue()();
		}
	}

	ThreadPool::ThreadPool()
		: ThreadPool(std::thread::hardware_concurrency() - 3U)
	{}

	ThreadPool::ThreadPool(size_t aSize)
	{
		assert(aSize > 0);

		myInternal = new InternalPImpl();
		myInternal->workers = new std::thread[aSize];
		myInternal->size = aSize;
		for (size_t index = 0U; index < aSize; ++index)
		{
			myInternal->workers[index] = std::thread([&]() { Work(); });
		}
	}

	ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(myInternal->mutex);
			myInternal->terminateWorkers = true;
		}
		myInternal->cv.notify_all();

		for (size_t index = 0U; index < myInternal->size; ++index)
		{
			if (myInternal->workers[index].joinable())
			{
				myInternal->workers[index].join();
			}
		}

		delete[] myInternal->workers;
		myInternal->workers = nullptr;
		delete myInternal;
		myInternal = nullptr;
	}
}