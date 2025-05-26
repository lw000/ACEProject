#include "x_ace_reactor.h"

#include <thread>

#include <ace/Reactor.h>
#include <ace/SOCK_Acceptor.h>

#include <ace/SOCK_Connector.h>

#include <ace/OS.h>
#include <ace/Log_Msg.h>

/**
 * The timeout value for connections. (30 seconds)
 */
static const ACE_Time_Value connTimeout(30);

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

	// 定时器事件
	int handle_timeout(const ACE_Time_Value& current_time, const void* act = 0) override
	{

		return 0;
	}

	ACE_HANDLE get_handle() const override { return handle_; }
};

class Acceptor : public ACE_Event_Handler
{
	ACE_SOCK_Acceptor acceptor_;

public:
	Acceptor(u_short port) : acceptor_{ ACE_INET_Addr("0.0.0.0", port) }
	{

	}

public:

	/// Get the I/O handle.
	ACE_HANDLE get_handle() const override
	{
		return acceptor_.get_handle();
	}

	int handle_input(ACE_HANDLE handle) override {
		ACE_SOCK_Stream stream;
		if (acceptor_.accept(stream))
		{
			new EchoHandler(stream.get_handle());
		}

		return 0;
	}
};

void client_worker()
{
	ACE_OS::sleep(ACE_Time_Value(2, 0));

	ACE_INET_Addr serverAddr(8080, "127.0.0.1");

	ACE_SOCK_Stream stream;
	ACE_SOCK_Connector connector;

    // connect to the server and get the stream
    if (connector.connect(stream, serverAddr) == -1) {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("%N:%l: Failed to connect to ")
            ACE_TEXT("server. (errno = %i: %m)\n"), ACE_ERRNO_GET));
        return;
    }

	while (1)
	{
		try {

			char buff[512] = { "11111111111111111" };

			ssize_t send_bytes = stream.send_n(buff, std::strlen(buff), &connTimeout);
			if (send_bytes <= 0) {
				if (send_bytes == 0)
				{

				}
				else
				{
					ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: Failed to  send ")
						ACE_TEXT("request. (errno = %i: %m)\n"), ACE_ERRNO_GET));
				}

				throw 1;
			}

			char answer[512] = {};
			// receive the answer
			if (stream.recv_n(answer, std::strlen(buff), &connTimeout) != 1) {
				ACE_ERROR((LM_ERROR, ACE_TEXT("%N: %l: Failed to receive ")
					ACE_TEXT("1st response. (errno = %i: %m)\n"), ACE_ERRNO_GET));
				throw 1;
			}
		}
		catch (...) {
			// ok we know an error occurred, we need to close the socket.
			// The we'll try again.
		}
	};
    

    // close the current stream
    if (stream.close() == -1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: Failed to close ")
            ACE_TEXT("socket. (errno = %i: %m)\n"), ACE_ERRNO_GET));
        return;
    }
}

int run_reactor(int argc, char** args)
{
	//std::thread t(client_worker);
	//t.detach();

	Acceptor acceptor(8080);
	ACE_Reactor::instance()->register_handler(&acceptor, ACE_Event_Handler::ACCEPT_MASK);
	ACE_Reactor::instance()->run_reactor_event_loop();

	return 0;
}