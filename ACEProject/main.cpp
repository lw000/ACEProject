#include <string>
#include <iostream>

#include "x_ace_queue.h"
#include "x_ace_thread_pool.h"
#include "x_ace_reactor.h"

#include <ace/config.h>
#include <ace/OS.h>
#include <ace/Log_Msg.h>
#include <ace/INET_Addr.h>

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


class MySignalHandler : public ACE_Event_Handler
{
	int signum_;

public:
	MySignalHandler(int signum) : signum_{signum}
	{

	}

	~MySignalHandler() override
	{

	}

public:
	int handle_signal(int signum, siginfo_t* = 0, ucontext_t* = 0) override
	{
		ACE_TRACE("MySignalHandler::handle_signal");

		ACE_ASSERT(signum == signum_);

		ACE_DEBUG((LM_DEBUG, ACE_TEXT("%S occurred\n"), signum));

		return 0;
	}
};

int ACE_TMAIN(int argc, char** args)
{
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

	//run_ace_queue(argc, args);

	//run_thread_pool(argc, args);

	run_reactor(argc, args);

#if 0

	MySignalHandler h1(SIGINT), h2(SIGINT), h3(SIGINT);
	ACE_Sig_Handler handler;
	handler.register_handler(SIGINT, &h1);
	handler.register_handler(SIGINT, &h2);
	handler.register_handler(SIGINT, &h3);

	//ACE_OS::kill(ACE_OS::getpid(), SIGINT);

	int time = 10;
	while ((time = ACE_OS::sleep(time)) == -1)
	{
		if (errno == EINTR)
			continue;
		else
		{
			ACE_ERROR_RETURN((LM_ERROR,
				ACE_TEXT("%p\n"),
				ACE_TEXT("sleep")), -1);
		}
	}

#endif // 0


	return 0;
}
