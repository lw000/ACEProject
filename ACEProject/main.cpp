#include <string>
#include <iostream>
#include "x_ace_queue.h"
#include "x_ace_thread_pool.h"

#include <ace/OS.h>
#include <ace/Log_Msg.h>
#include <ace/INET_Addr.h>

char buffer[30] = { 0 };

char reset(char& c)
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

	//run_ace_queue(argc, args);

	run_thread_pool(argc, args);

	return 0;
}
