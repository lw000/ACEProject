#include "x_ace_queue.h"

#include <string>
#include <iostream>

#include <ace/Thread_Manager.h>
#include <ace/Message_Queue.h>

#include <ace/Message_Block.h>

#include <ace/Task.h>
#include <ace/OS.h>


static void* create_vairous_record(void* ace_message_queue);

static void* get_vairous_record(void* ace_message_queue);


int run_ace_queue(int argc, char** args)
{
	ACE_Message_Queue<ACE_MT_SYNCH>* various_record_queue = new ACE_Message_Queue<ACE_MT_SYNCH>;

	ACE_Thread_Manager::instance()->spawn(
		ACE_THR_FUNC(create_vairous_record),
		various_record_queue,
		THR_NEW_LWP | THR_DETACHED);

	ACE_Thread_Manager::instance()->spawn(
		ACE_THR_FUNC(get_vairous_record),
		various_record_queue,
		THR_NEW_LWP | THR_DETACHED);

	ACE_Thread_Manager::instance()->wait();

	return 0;
}

void* create_vairous_record(void* ace_message_queue)
{
	ACE_Message_Queue<ACE_MT_SYNCH>* p_queue = (ACE_Message_Queue<ACE_MT_SYNCH>*)ace_message_queue;
	int i = 0;
	while (i < 10000000)
	{
		ACE_Message_Block* mbl = new ACE_Message_Block(10);//�����ﴴ����Ϣ
		std::string temp = std::to_string(++i);
		mbl->copy(temp.c_str());
		p_queue->enqueue_tail(mbl);//��Ϣ���ŵ������У���ָ��������Ϣʵ�壩

		//ACE_OS::sleep(ACE_Time_Value(1, 0));
	}
	return nullptr;
}

void* get_vairous_record(void* ace_message_queue)
{
	ACE_Message_Queue<ACE_MT_SYNCH>* p_queue = (ACE_Message_Queue<ACE_MT_SYNCH>*)ace_message_queue;
	while (true)
	{
		ACE_Message_Block* mbl = nullptr;
		p_queue->dequeue_head(mbl);//��Ϣ���ӣ����ӵ���ϢӦ��������֮���ͷ�
		if (mbl)
		{
			std::cout << mbl->rd_ptr() << std::endl;
			mbl->release();//��Ϣ�Ѿ����꣬�ͷ���Ϣ
		}
	}
	return nullptr;

}