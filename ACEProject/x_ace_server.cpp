#include "x_ace_server.h"

#include <string>
#include <iostream>
#include <atomic>

#include <ace/Reactor.h>
#include <ace/OS.h>
#include <ace/Log_Msg.h>
#include <ace/Signal.h>
#include <ace/Sig_Handler.h>

#include <ace/Thread_Manager.h>
#include <ace/Message_Queue.h>
#include <ace/Message_Block.h>

static std::atomic_bool serviceClosed;
ACE_Message_Queue<ACE_MT_SYNCH>* record_queue_ = new ACE_Message_Queue<ACE_MT_SYNCH>;

class Mock
{
	//ACE_Message_Queue<ACE_MT_SYNCH>* record_queue_;

public:
	Mock()
	{
		//record_queue_ = new ACE_Message_Queue<ACE_MT_SYNCH>;
	}

	~Mock()
	{
		//delete record_queue_;
	}

public:
	int run()
	{
		ACE_Thread_Manager::instance()->spawn(
			ACE_THR_FUNC(Producer),
			record_queue_,
			THR_NEW_LWP | THR_JOINABLE);

		ACE_Thread_Manager::instance()->spawn(
			ACE_THR_FUNC(Consumer),
			record_queue_,
			THR_NEW_LWP | THR_JOINABLE);

		return 0;
	}

	int close()
	{
		//record_queue_->close();
	}

private:
	static void* Producer(void* ace_message_queue)
	{
		ACE_Message_Queue<ACE_MT_SYNCH>* p_queue = (ACE_Message_Queue<ACE_MT_SYNCH>*)ace_message_queue;
		int i = 0;
		while (!serviceClosed)
		{
			ACE_Message_Block* mbl = new ACE_Message_Block(10);//在这里创建消息
			std::string temp = std::to_string(++i);
			mbl->copy(temp.c_str());
			p_queue->enqueue_tail(mbl);//消息被放到队列中（用指针引用消息实体）

			ACE_OS::sleep(ACE_Time_Value(0, 500));
		}
		return nullptr;
	}

	static void* Consumer(void* ace_message_queue)
	{
		ACE_Message_Queue<ACE_MT_SYNCH>* p_queue = (ACE_Message_Queue<ACE_MT_SYNCH>*)ace_message_queue;
		while (!serviceClosed)
		{
			ACE_Time_Value timeout(1);  // 1秒超时
			ACE_Message_Block* mbl = nullptr;

			//消息出队，出队的消息应该在用完之后被释放
			if (p_queue->dequeue_head(mbl, &timeout) == -1)
			{
				if (ACE_OS::last_error() == EWOULDBLOCK)
				{
					continue;
				}

				break;
			}

			if (mbl != nullptr)
			{
				if (mbl->msg_type() == ACE_Message_Block::MB_STOP)
				{
					ACE_DEBUG((LM_INFO, "Received stop signal\n"));
					mbl->release();
					break;
				}

				std::cout << mbl->rd_ptr() << std::endl;

				mbl->release();//消息已经用完，释放消息
			}
		}
		return nullptr;
	}
};

class GracefulShutdown : public ACE_Event_Handler
{
public:
	int handle_signal(int signum, siginfo_t* = 0, ucontext_t* = 0) override
	{
		switch (signum)
		{
		case SIGINT:
		{
			ACE_DEBUG((LM_INFO, "\n[SERVER] Shutting down...\n"));
		} break;
		case SIGBREAK:
		{
			ACE_DEBUG((LM_INFO, "SIGTERM received\n"));
		} break;
		default:
			break;
		}

		serviceClosed = true;

		record_queue_->close();

		//ACE_Thread_Manager::instance()->close();

		ACE_Thread_Manager::instance()->wait();

		ACE_Reactor::instance()->end_reactor_event_loop();

		return 0;
	}
};

int run_server(int argc, char** args)
{
	GracefulShutdown shutdown;

	ACE_Sig_Handler handler;
	if (handler.register_handler(SIGINT, &shutdown) == -1)
	{
		ACE_ERROR((LM_ERROR, "Failed to register SIGINT handler\n"));
		return 1;
	}

	Mock mock;
	mock.run();

	ACE_Reactor::instance()->run_reactor_event_loop();

	return 0;
}
