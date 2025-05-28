#include <string>
#include <iostream>

#include <ace/Reactor.h>
#include <ace/Event_Handler.h>
#include <ace/SOCK_Connector.h>
#include <ace/SOCK_Stream.h>
#include <ace/Log_Msg.h>
#include <ace/Thread_Manager.h>
#include <ace/OS.h>

#include <ace/streams.h>

#if 0

const u_short PORT = 9998;
const char* SERVER_IP = "127.0.0.1";

// 客户端连接处理器
class ClientHandler : public ACE_Event_Handler {
public:
    ClientHandler() : connected_(false) {
        // 异步连接服务器
        ACE_SOCK_Connector connector;
        ACE_INET_Addr server_addr(PORT, SERVER_IP);
        if (connector.connect(stream_, server_addr) == -1) {
            ACE_ERROR((LM_ERROR, "[Client] connect error: %m\n"));
            return;
        }
        connected_ = true;

        // 注册读事件
        ACE_Reactor::instance()->register_handler(this, ACE_Event_Handler::READ_MASK);
        // 触发写事件发送数据
        ACE_Reactor::instance()->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK);
    }

    int handle_input(ACE_HANDLE handle) override {
        char buf[4096]{};
        ssize_t bytes_read = stream_.recv(buf, sizeof(buf));

        if (bytes_read <= 0) {
            ACE_DEBUG((LM_INFO, "[Client] Server closed connection\n"));
            ACE_Reactor::instance()->end_reactor_event_loop();
            return -1;
        }

        // 打印服务器回显
        ACE_DEBUG((LM_INFO, "[Client] Echo: %s", buf));
        return 0;
    }

    int handle_output(ACE_HANDLE handle) override {
        // 发送用户输入
        std::string input;
        std::getline(std::cin, input);
        input += '\n';

        if (stream_.send(input.c_str(), input.size()) == -1) {
            ACE_ERROR((LM_ERROR, "[Client] send error: %m\n"));
            return -1;
        }

        // 取消写事件注册（等待下一次输入）
        ACE_Reactor::instance()->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK);
        return 0;
    }

    int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask) override {
        stream_.close();
        delete this;
        return 0;
    }

    ACE_HANDLE get_handle() const override { return connected_ ? stream_.get_handle() : ACE_INVALID_HANDLE; }

private:
    ACE_SOCK_Stream stream_;
    bool connected_;
};

void* run(void*)
{
    ACE_Reactor::instance()->run_reactor_event_loop();
    return nullptr;
}

int main(int argc, char** args) {
    ACE_Reactor reactor;
    ACE_Reactor::instance(&reactor);

    // 启动客户端连接
    ClientHandler* client = new ClientHandler();

    // 启动独立线程处理 Reactor 事件循环
    ACE_Thread_Manager::instance()->spawn(
        ACE_THR_FUNC(run),
        nullptr
    );

    // 主线程等待用户输入并触发写事件
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        if (input == "exit") break;
        ACE_Reactor::instance()->schedule_wakeup(
            client->get_handle(),
            ACE_Event_Handler::WRITE_MASK
        );
    }

    ACE_Reactor::instance()->end_reactor_event_loop();
    return 0;
}
#endif // 0

class StdinHandler : public ACE_Event_Handler
{
public:
    StdinHandler()
    {
        this->reactor(ACE_Reactor::instance());
        this->reactor()->register_handler(this, ACE_Event_Handler::READ_MASK);
    }

public:
    int handle_input(ACE_HANDLE fd) override {
        char buf[1024];
        ssize_t n = ACE_OS::read(fd, buf, sizeof(buf));
        if (n > 0) {
            ACE_DEBUG((LM_INFO, "Input: %s", buf));
        }
        else if (n == 0) { // EOF（如Ctrl+D）
            ACE_Reactor::instance()->end_reactor_event_loop();
        }
        else {
            ACE_ERROR((LM_ERROR, "Read error: %m\n"));
        }
        return 0;
    }

    ACE_HANDLE get_handle() const override {
        return ACE_STDIN; // 标准输入句柄
    }
};

int main(int argc, char** args) {
    
    {
        // Create a persistent store.
        const char* filename = "output.log";
        ofstream outfile(filename, ios::out | ios::trunc);

        // Check for errors.
        if (outfile.bad())
            return 1;

        // Set the ostream.
        ACE_LOG_MSG->msg_ostream(&outfile);

        ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);

        // This message should show up in the ostream.
        ACE_DEBUG((LM_DEBUG,
            "fourth message %d, %d, %s\n", 1,2,"this is test message"));
    }

    StdinHandler handler;
    
    ACE_Reactor::instance()->run_reactor_event_loop();

    return 0;
}