#include "x_ace_reactor.h"

#include <ace/Reactor.h>
#include <ace/SOCK_Acceptor.h>

#include <ace/OS.h>

class EchoHandler : public ACE_Event_Handler
{
	ACE_HANDLE handle_;
	long time_handle_;
public:
	EchoHandler(ACE_HANDLE handle) : handle_{ handle }, time_handle_{} {
		this->reactor(ACE_Reactor::instance());
		time_handle_ = this->reactor()->schedule_timer(this, 0, ACE_Time_Value(1), ACE_Time_Value(3));
	}

	~EchoHandler() override
	{
		ACE_Reactor::instance()->cancel_timer(this->time_handle_);
	}

	int handle_input(ACE_HANDLE handle) override {
		char buf[1024];
		ssize_t cnt = ACE_OS::recv(handle_, buf, sizeof(buf), 0);
		if (cnt <= 0) {
			ACE_Reactor::instance()->remove_handler(this, ACE_Event_Handler::READ_MASK);
			return -1;
		}
		ACE_OS::send(handle_, buf, cnt);
		return 0;
	}

	// ��ʱ���¼�
	int handle_timeout(const ACE_Time_Value& current_time, const void* act = 0) override
	{

		return 0;
	}

	ACE_HANDLE get_handle() const override { return handle_; }
};

class Acceptor : public ACE_Event_Handler
{
public:
	int handle_input(ACE_HANDLE handle) override {
		ACE_SOCK_Stream stream;
		ACE_SOCK_Acceptor acceptor(ACE_INET_Addr(8080));
		if (acceptor.accept(stream))
		{
			new EchoHandler(stream.get_handle());
		}

		return 0;
	}
};

int run_reactor(int argc, char** args)
{
	Acceptor acceptor;
	ACE_Reactor::instance()->register_handler(&acceptor, ACE_Event_Handler::READ_MASK);
	ACE_Reactor::instance()->run_reactor_event_loop();

	return 0;
}