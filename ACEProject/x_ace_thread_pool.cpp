#include "x_ace_thread_pool.h"

#include <string>
#include <iostream>

#include <ace/Thread_Manager.h>
#include <ace/Message_Queue.h>
#include <ace/Task.h>
#include <ace/Message_Block.h>

#include <ace/OS.h>
#include <ace/Refcounted_Auto_Ptr.h>

class ThreadPoolTask : public ACE_Task<ACE_MT_SYNCH>
{
public:
	// 初始化线程池
	int open(int num_threads)
	{
		return activate(THR_NEW_LWP | THR_JOINABLE, num_threads);
	}

	// 关闭线程池
	int close(u_long flags = 0)
	{
		// 停止消息队列
		msg_queue()->deactivate();

		//return wait();

		return 0;
	}

	// 线程入口函数
	int svc() override
	{
		ACE_Message_Block* mb = nullptr;
		while (msg_queue()->dequeue_head(mb) != -1) {
			// 处理任务
			process_task(mb);
			mb->release(); // 释放消息块
		}
		return 0;
	}

	// 捕获线程异常
	int handle_exception(ACE_HANDLE handle) override {
		ACE_DEBUG((LM_ERROR, "Thread %t crashed!\n"));
		return -1;
	}

	int submit_task(ACE_Message_Block* task)
	{
		return msg_queue()->enqueue_tail(task);
	}

private:
	void process_task(ACE_Message_Block* mb)
	{
		ACE_DEBUG((LM_INFO, "Processing task: %s\n", mb->rd_ptr()));
	}
};


class WorkerTask : public ACE_Task<ACE_MT_SYNCH>
{
public:
	int svc() override
	{
		ACE_Message_Block* mb{};
		while (getq(mb) != -1)
		{
			if (mb->msg_type() == ACE_Message_Block::MB_STOP) {
				ACE_DEBUG((LM_INFO, "Received STOP signal\n"));
				mb->release();  // 必须释放内存
				break;
			}

			ACE_DEBUG((LM_INFO, "Processing task: %s\n", mb->rd_ptr()));

			// 处理其他消息...
			mb->release();
		}

		return 0;
	}
};

constexpr auto THREAD_COUNT = 8;

class MessageData
{
public:
	int age;
	std::string name;

public:
	~MessageData()
	{

	}

};

int run_thread_pool(int argc, char** args)
{
	//run_ace_queue(argc, args);

	{
		ACE_Refcounted_Auto_Ptr<MessageData, ACE_Null_Mutex> q(new MessageData{30, "levi"});
	}

	WorkerTask task;
	task.activate(THR_NEW_LWP | THR_JOINABLE, THREAD_COUNT);

	for (auto i = 0; i < 1000; i++)
	{
		ACE_Message_Block* mb = new ACE_Message_Block(256);
		std::snprintf(mb->wr_ptr(), 256, "Task-%d", i);
		mb->wr_ptr(std::strlen(mb->wr_ptr()) + 1);
		task.putq(mb);
	}

	for (auto i = 0; i < THREAD_COUNT; i++)
	{
		ACE_Message_Block* stop_mb = new ACE_Message_Block(0, ACE_Message_Block::MB_STOP);
		task.putq(stop_mb);
	}
	
	task.wait();

#if 0

	ACE_Message_Queue<ACE_MT_SYNCH>* record_queue = new ACE_Message_Queue<ACE_MT_SYNCH>;

	ThreadPoolTask pool;
	pool.msg_queue(record_queue);
	if (pool.open(8) != 0)
	{
		ACE_ERROR_RETURN((LM_ERROR, "Failed to open thread pool\n"), 1);
	}

	for (auto i = 0; i < 100; i++)
	{
		ACE_Message_Block* mb = new ACE_Message_Block(256);
		std::snprintf(mb->wr_ptr(), 256, "Task-%d", i);
		mb->wr_ptr(std::strlen(mb->wr_ptr()) + 1);
		pool.submit_task(mb);
	}

	//ACE_OS::sleep(ACE_Time_Value(1, 0));

	pool.close();

	ACE_Thread_Manager::instance()->wait();

	delete record_queue;

#endif // 0

	return 0;
}
