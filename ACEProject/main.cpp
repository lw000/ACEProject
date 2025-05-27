#include <string>
#include <iostream>

#include "x_ace_queue.h"
#include "x_ace_thread_pool.h"
#include "x_ace_reactor.h"
#include "x_ace_server.h"

#include <ace/config.h>
#include <ace/OS.h>
#include <ace/Log_Msg.h>
#include <ace/INET_Addr.h>
#include <ace/Reactor.h>
#include <ace/Shared_Memory.h>

#include <ace/UUID.h>

#include <ace/Signal.h>
#include <ace/Sig_Handler.h>

char buffer[30] = { 0 };

static char reset(char& c)
{
	return c = '\0';
}

void print_ip_addr(const char* doc, const ACE_INET_Addr& addr)
{
	addr.addr_to_string(buffer, sizeof(buffer));
	ACE_DEBUG((LM_DEBUG, "%s addr_to_string buffer:[%s]\n", doc, buffer));

	ACE_DEBUG((LM_DEBUG, "%s get_host_addr:[%s]\n", doc, addr.get_host_addr()));

	ACE_DEBUG((LM_DEBUG, "%s get_port_number:[%d]\n\n", doc, addr.get_port_number()));

	std::transform(buffer, buffer + sizeof(buffer), buffer, reset);
}

class ShutdownHandler : public ACE_Event_Handler
{
	int signum_;

public:
	ShutdownHandler(int signum) : signum_{signum}
	{

	}

	~ShutdownHandler() override
	{

	}

public:
	int handle_signal(int signum, siginfo_t* = 0, ucontext_t* = 0) override
	{
		ACE_TRACE("MySignalHandler::handle_signal");

		ACE_ASSERT(signum == signum_);

		ACE_DEBUG((LM_DEBUG, ACE_TEXT("%S occurred\n"), signum));

		ACE_Reactor::instance()->end_reactor_event_loop();

		return 0;
	}
};

int ACE_TMAIN(int argc, char** args)
{
	// 初始化日志，指定程序名
	ACE_LOG_MSG->open("MyApp");

#if 0

	{
		char* env = nullptr;
		env = ACE_OS::getenv("Path");
		std::cout << "env: " << env << std::endl;
	}

	{
		ACE_INET_Addr addr("127.0.0.1:3305");
		print_ip_addr("local address", addr);
	}

	{
		ACE_Utils::UUID uuid;
		auto ss = uuid.to_string();
		std::cout << "uuid:" << ss << std::endl;
	}
#endif // 0

#if 1
	//run_ace_queue(argc, args);

	//run_thread_pool(argc, args);

	run_reactor(argc, args);

	//run_server(argc, args);
#endif // 0

#if 0

	ShutdownHandler h1(SIGINT);
	ACE_Sig_Handler handler;
	handler.register_handler(SIGINT, &h1);

	ACE_Reactor::instance()->run_reactor_event_loop();

	//ACE_OS::kill(ACE_OS::getpid(), SIGINT);

	//int time = 10;
	//while ((time = ACE_OS::sleep(time)) == -1)
	//{
	//	if (errno == EINTR)
	//		continue;
	//	else
	//	{
	//		ACE_ERROR_RETURN((LM_ERROR,
	//			ACE_TEXT("%p\n"),
	//			ACE_TEXT("sleep")), -1);
	//	}
	//}

#endif // 0


	return 0;
}
