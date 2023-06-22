#pragma once
#include <functional>

namespace CU
{
	class ThreadPool
	{
	public:
		/* Constructors & Destructor */
		// Default the thread pool size to hardware_concurrency - 2.
		ThreadPool();
		ThreadPool(size_t aSize);
		~ThreadPool();

		ThreadPool(const ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		/* Interface */
		enum class eAddJobResult
		{
			Success, ErrorQueueFull
		};
		eAddJobResult AddJob(std::function<void()> aJob);

	private:
		void Work();

		struct InternalPImpl;
		InternalPImpl* myInternal;
	};
}