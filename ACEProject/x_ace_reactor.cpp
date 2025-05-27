#include "x_ace_reactor.h"

#include <string>
#include <iostream>
#include <thread>

#include <ace/Reactor.h>
#include <ace/Event_Handler.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Connector.h>
#include <ace/WFMO_Reactor.h>
#include <ace/Thread_Manager.h>

#include <ace/OS.h>
#include <ace/Log_Msg.h>
#include <ace/Signal.h>
#include <ace/Sig_Handler.h>

const u_short PORT = 9998;

static const ACE_Time_Value connTimeout(3);

#if 0

class EchoHandler : public ACE_Event_Handler
{
	ACE_SOCK_Stream peer_stream_;
	//long time_handle_;
public:
	EchoHandler(ACE_SOCK_Stream& stream) : peer_stream_{ stream }/*, time_handle_{}*/ {
		this->reactor(ACE_Reactor::instance());
		reactor()->register_handler(this, ACE_Event_Handler::READ_MASK);

		//time_handle_ = this->reactor()->schedule_timer(this, 0, ACE_Time_Value(1), ACE_Time_Value(3));
	}

	~EchoHandler() override
	{
		//ACE_Reactor::instance()->cancel_timer(this->time_handle_);
	}

public:
	ACE_HANDLE get_handle() const override { return peer_stream_.get_handle(); }

	int handle_input(ACE_HANDLE handle) override {
		char buf[1024];
		ssize_t bytes_read = peer_stream_.recv(buf, sizeof(buf), 0);
		if (bytes_read <= 0) {
			if (bytes_read == 0)
				ACE_DEBUG((LM_INFO, "[SERVER] Client disconnected\n"));
			else
				ACE_ERROR((LM_ERROR, "[SERVER] recv error: %m\n"));
			reactor()->remove_handler(this, ACE_Event_Handler::ALL_EVENTS_MASK);
			peer_stream_.close();
			delete this;
			return -1;
		}

		ssize_t bytes_sent = peer_stream_.send(buf, bytes_read);
		if (bytes_sent != bytes_read)
			ACE_ERROR((LM_ERROR, "[SERVER] send error: %m\n"));

		return 0;
	}

	// 定时器事件
	int handle_timeout(const ACE_Time_Value& current_time, const void* act = 0) override
	{

		return 0;
	}
};

class ServerHandler : public ACE_Event_Handler
{
	ACE_SOCK_Acceptor acceptor_;

public:
	ServerHandler(u_short port) : acceptor_{ port }
	{
		this->reactor(ACE_Reactor::instance());
		reactor()->register_handler(this, ACE_Event_Handler::READ_MASK);
	}

public:

	/// Get the I/O handle.
	ACE_HANDLE get_handle() const override
	{
		return acceptor_.get_handle();
	}

	int handle_input(ACE_HANDLE handle) override {
		ACE_SOCK_Stream stream;
		ACE_INET_Addr client_addr;

		if (acceptor_.accept(stream, &client_addr) == -1) {
			ACE_ERROR((LM_ERROR, "[SERVER] accept error: %m\n"));
			return -1;
		}

		ACE_DEBUG((LM_INFO, "[SERVER] New connection from %s:%d\n",
			client_addr.get_host_name(), client_addr.get_port_number()));

		new EchoHandler(stream); // 创建处理器

		return 0;
	}
};

class ShutdownHandler : public ACE_Event_Handler
{
public:
	int handle_signal(int signum, siginfo_t* = 0, ucontext_t* = 0) override {

		if (signum == SIGINT)
		{
			ACE_DEBUG((LM_INFO, "\n[SERVER] Shutting down...\n"));
			ACE_Reactor::instance()->end_reactor_event_loop();
		}
		return 0;
	}
};

int run_reactor(int argc, char** args)
{
	//std::thread t(client_worker);
	//t.detach();

	ServerHandler acceptor(PORT);
	ShutdownHandler shutdown;

	// 注册信号处理器
	ACE_Reactor::instance()->register_handler(SIGINT, &shutdown);

	ACE_DEBUG((LM_INFO, "[SERVER] Listening on port %d\n", PORT));
	ACE_Reactor::instance()->run_reactor_event_loop();

	return 0;
}

#endif // 0

#if 0


void client_worker()
{
	ACE_OS::sleep(ACE_Time_Value(2, 0));

	ACE_SOCK_Stream stream;
	ACE_SOCK_Connector connector;
	ACE_INET_Addr serverAddr(PORT, "127.0.0.1");

	// connect to the server and get the stream
	if (connector.connect(stream, serverAddr) == -1) {
		ACE_ERROR((LM_ERROR,
			ACE_TEXT("%N:%l: Failed to connect to ")
			ACE_TEXT("server. (errno = %i: %m)\n"), ACE_ERRNO_GET));
		return;
	}

	ACE_DEBUG((LM_INFO, "[CLIENT] Connected to %s:%d\n",
		serverAddr.get_host_addr(), serverAddr.get_port_number()));

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
					ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: Failed to send ")
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

class EchoHandler : public ACE_Event_Handler {
public:
	EchoHandler(ACE_HANDLE handle) : peer_stream_(handle) {
		int rc = ACE_Reactor::instance()->register_handler(this, ACE_Event_Handler::READ_MASK);
		if (rc == -1)
		{
			ACE_ERROR((LM_ERROR, "%N:%l: Failed to register SIGINT handler\n"));
			//ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: Failed to register ")
			//	ACE_TEXT("read handler. (errno = %i: %m)\n"), ACE_ERRNO_GET), -1);
		}
	}

	int handle_input(ACE_HANDLE) override {
		char buf[4096];
		ssize_t bytes_read = peer_stream_.recv(buf, sizeof(buf));
		if (bytes_read <= 0) {
			ACE_Reactor::instance()->remove_handler(this, ACE_Event_Handler::ALL_EVENTS_MASK);
			delete this;
			return -1;
		}
		peer_stream_.send(buf, bytes_read);
		return 0;
	}

	int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask) override {
		peer_stream_.close();
		delete this;
		return 0;
	}

	ACE_HANDLE get_handle() const override { return peer_stream_.get_handle(); }

private:
	ACE_SOCK_Stream peer_stream_;
};

class Acceptor : public ACE_Event_Handler {
public:
	Acceptor(ACE_Reactor* reactor) : reactor_(reactor) {
		if (acceptor_.open(ACE_INET_Addr(PORT), 1) == -1) {
			ACE_ERROR((LM_ERROR, "Acceptor open failed\n"));
			return;
		}
		reactor_->register_handler(this, ACE_Event_Handler::ACCEPT_MASK);

		std::thread t(client_worker);
		t.detach();
	}

	int handle_input(ACE_HANDLE) override {
		ACE_SOCK_Stream stream;
		ACE_INET_Addr client_addr;
		if (acceptor_.accept(stream, &client_addr) == -1) {
			if (errno != EWOULDBLOCK)
				ACE_ERROR((LM_ERROR, "Accept error: %m\n"));
			return 0;
		}
		ACE_DEBUG((LM_INFO, "New connection from %s:%d\n",
			client_addr.get_host_addr(), client_addr.get_port_number()));

		new EchoHandler(stream.get_handle());
		return 0;
	}

	ACE_HANDLE get_handle() const override { return acceptor_.get_handle(); }

private:
	ACE_Reactor* reactor_;
	ACE_SOCK_Acceptor acceptor_;
};

int run_reactor(int argc, char** args)
{
	ACE_Reactor reactor(new ACE_WFMO_Reactor, 1);
	Acceptor acceptor(&reactor);
	reactor.run_reactor_event_loop();

	return 0;
}

#endif // 0

// 客户端连接处理器
class ClientHandler : public ACE_Event_Handler {
public:
	ClientHandler(ACE_SOCK_Stream& stream) : peer_stream_(stream) {
		// 注册读事件
		ACE_Reactor::instance()->register_handler(this, ACE_Event_Handler::READ_MASK);
	}

	int handle_input(ACE_HANDLE handle) override {
		char buf[4096]{};
		ssize_t bytes_read = peer_stream_.recv(buf, sizeof(buf));

		if (bytes_read <= 0) {
			if (bytes_read == 0)
				ACE_DEBUG((LM_INFO, "[Server] Client disconnected\n"));
			else
				ACE_ERROR((LM_ERROR, "[Server] recv error: %m\n"));

			// 清理资源
			ACE_Reactor::instance()->remove_handler(this, ACE_Event_Handler::ALL_EVENTS_MASK);
			delete this;
			return -1;
		}

		// 回显数据
		peer_stream_.send(buf, bytes_read);
		return 0;
	}

	ACE_HANDLE get_handle() const override { return peer_stream_.get_handle(); }

private:
	ACE_SOCK_Stream peer_stream_;
};

// 服务端连接接收器
class ServerAcceptor : public ACE_Event_Handler {
public:
	ServerAcceptor() {
		/*ACE_Reactor::instance()->register_handler(this, ACE_Event_Handler::READ_MASK);*/

		if (acceptor_.open(ACE_INET_Addr(PORT), 1) == -1) {
			ACE_ERROR((LM_ERROR, "Acceptor open failed\n"));
			return;
		}
		ACE_Reactor::instance()->register_handler(this, ACE_Event_Handler::ACCEPT_MASK);
	}

	int handle_input(ACE_HANDLE handle) override {
		ACE_SOCK_Stream stream;
		ACE_INET_Addr client_addr;

		if (acceptor_.accept(stream, &client_addr) == -1) {
			ACE_ERROR((LM_ERROR, "[Server] accept error: %m\n"));
			return -1;
		}

		ACE_DEBUG((LM_INFO, "[Server] New connection from %s:%d\n",
			client_addr.get_host_addr(), client_addr.get_port_number()));
		new ClientHandler(stream); // 创建新处理器
		return 0;
	}

	ACE_HANDLE get_handle() const override { return acceptor_.get_handle(); }

private:
	ACE_SOCK_Acceptor acceptor_;
};

int run_reactor(int argc, char** args)
{
	ACE_Reactor reactor(new ACE_WFMO_Reactor, 1);
	ACE_Reactor::instance(&reactor);

	ServerAcceptor acceptor;
	reactor.run_reactor_event_loop();

	return 0;
}
