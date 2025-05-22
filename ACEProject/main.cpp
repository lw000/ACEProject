#include <string>
#include <iostream>

#include <ace/Thread_Manager.h>
#include <ace/Message_Queue.h>

#include <ace/Message_Block.h>

#include <ace/Task.h>
#include <ace/OS.h>

#include "x_ace_queue.h"

class ThreadPoolTask : public ACE_Task<ACE_MT_SYNCH>
{
public:
	// ��ʼ���̳߳�
	int open(int num_threads)
	{
		return activate(THR_NEW_LWP | THR_JOINABLE, num_threads);
	}

	// �ر��̳߳�
	int close(u_long flags = 0)
	{
		// ֹͣ��Ϣ����
		msg_queue()->deactivate();

		return wait();
	}

	// �߳���ں���
	int svc() override
	{
		ACE_Message_Block* mb = nullptr;
		while (msg_queue()->dequeue_head(mb) != -1) {
			// ��������
			process_task(mb);
			mb->release(); // �ͷ���Ϣ��
		}
		return 0;
	}

	// �����߳��쳣
	int handle_exception(ACE_HANDLE handle) override {
		ACE_DEBUG((LM_ERROR, "Thread %t crashed!\n"));
		return -1;
	}

	int submit_task(ACE_Message_Block* task)
	{
		return msg_queue()->enqueue_tail(task);
	}

private:
	void process_task(ACE_Message_Block *mb)
	{
		ACE_DEBUG((LM_INFO, "Processing task: %s\n", mb->rd_ptr()));
	}
};


int ACE_TMAIN(int argc, char** args)
{
	//run_ace_queue(argc, args);

	ThreadPoolTask pool;

	if (pool.open(8) != 0)
	{
		ACE_ERROR_RETURN((LM_ERROR, "Failed to open thread pool\n"), 1);
	}

	for (auto i = 0; i < 100; i++)
	{
		ACE_Message_Block* mb = new ACE_Message_Block(1024);
		std::snprintf(mb->wr_ptr(), 1024, "Task-%d", i);
		mb->wr_ptr(std::strlen(mb->wr_ptr()) + 1);
		pool.submit_task(mb);
	}

	pool.close();

	ACE_Thread_Manager::instance()->wait();

	return 0;
}
