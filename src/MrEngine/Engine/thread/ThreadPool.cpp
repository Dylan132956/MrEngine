#include "ThreadPool.h"
#include "Object.h"
#include "Engine.h"

namespace moonriver
{
	void Thread::Sleep(int ms)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}

	Thread::Thread(Action init, Action done):
        m_init_action(init),
        m_done_action(done)
	{
		m_close = false;
        m_thread = std::make_shared<std::thread>(&Thread::Run, this);
	}

	Thread::~Thread()
	{
		if (m_thread->joinable())
		{
			this->Wait();

			m_mutex.lock();
			m_close = true;
			m_condition.notify_one();
			m_mutex.unlock();
			m_thread->join();
		}
	}

	void Thread::Wait()
	{
		std::unique_lock<Mutex> lock(m_mutex);

		// wait until all job done
		m_condition.wait(lock, [this]() {
			return m_job_queue.empty();
		});
	}

	int Thread::GetQueueLength()
	{
		std::lock_guard<Mutex> lock(m_mutex);
		return m_job_queue.size();
	}

    void Thread::AddTask(const Task& task)
    {
        std::lock_guard<Mutex> lock(m_mutex);
        m_job_queue.push_back(task);
        m_condition.notify_one();
    }

    void Thread::Run()
    {
        if (m_init_action)
        {
            m_init_action();
        }

        while (true)
        {
            Task task;

            {
                std::unique_lock<Mutex> lock(m_mutex);

                // wait until has a job or close
                m_condition.wait(lock, [this] {
                    return !m_job_queue.empty() || m_close;
                });

                if (m_close)
                {
                    break;
                }

                task = m_job_queue.front();
            }

            if (task.job)
            {
                void* result = task.job();

                if (task.complete)
                {
                    Engine::Instance()->PostAction([=]() {
                        task.complete(result);
                    });
                }
            }

            {
                std::lock_guard<Mutex> lock(m_mutex);
                m_job_queue.pop_front();
                m_condition.notify_one();
            }
        }

        if (m_done_action)
        {
            m_done_action();
        }
    }

	ThreadPool::ThreadPool(int thread_count, Action init, Action done)
	{
		m_threads.resize(thread_count);
		for (int i = 0; i < m_threads.size(); ++i)
		{
			m_threads[i] = std::make_shared<Thread>(init, done);
		}
	}

	void ThreadPool::WaitAll()
	{
		for (auto& i : m_threads)
		{
			i->Wait();
		}
	}

    void ThreadPool::AddTask(const Thread::Task& task, int thread_index)
    {
        if (thread_index >= 0 && thread_index < m_threads.size())
        {
            m_threads[thread_index]->AddTask(task);
        }
        else
        {
            int min_len = 0x7fffffff;
            int min_index = -1;

            for (int i = 0; i < m_threads.size(); ++i)
            {
                int len = m_threads[i]->GetQueueLength();
                if (min_len > len)
                {
                    min_len = len;
                    min_index = i;

                    if (min_len == 0)
                    {
                        break;
                    }
                }
            }

            m_threads[min_index]->AddTask(task);
        }
    }
}
