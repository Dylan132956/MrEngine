#pragma once

#include "Action.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <vector>

namespace moonriver
{
	typedef std::mutex Mutex;

    class Object;

	class Thread
	{
	public:
		struct Task
		{
            typedef std::function<void*()> Job;
            typedef std::function<void(void*)> CompleteCallback;

			Job job;
            CompleteCallback complete;
		};

		static void Sleep(int ms);
        Thread(Action init, Action done);
		~Thread();
        void Wait();
        int GetQueueLength();
		void AddTask(const Task& task);

	private:
		void Run();

		std::shared_ptr<std::thread> m_thread;
        std::list<Task> m_job_queue;
        Mutex m_mutex;
		std::condition_variable m_condition;
		bool m_close;
        Action m_init_action;
        Action m_done_action;
	};

	class ThreadPool
	{
	public:
		ThreadPool(int thread_count, Action init = nullptr, Action done = nullptr);
		void WaitAll();
		int GetThreadCount() const { return m_threads.size(); }
        void AddTask(const Thread::Task& task, int thread_index = -1);

	private:
		std::vector<std::shared_ptr<Thread>> m_threads;
	};
}
