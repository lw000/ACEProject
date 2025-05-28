// ace_nt_server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <ace/ACE.h>
#include <ace/Reactor.h>
#include <ace/NT_Service.h>
#include <ace/Log_Msg.h>
#include <ace/OS.h>
#include <ace/streams.h>
#include <ace/Mutex.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#define ACE_MAX_PATH 256

class Service : public ACE_NT_Service
{
public:
    Service();

    ~Service();

    /// We override <handle_control> because it handles stop requests
    /// privately.
    virtual void handle_control(DWORD control_code);

    /// We override <handle_exception> so a 'stop' control code can pop
    /// the reactor off of its wait.
    virtual int  handle_exception(ACE_HANDLE h);

    /// This is a virtual method inherited from ACE_NT_Service.
    virtual int svc();

    /// Where the real work is done:
    virtual int handle_timeout(const ACE_Time_Value& tv,
        const void* arg = 0);

private:
    typedef ACE_NT_Service inherited;

private:
    int stop_;
    ofstream outfile_;
};


Service::Service()
{
    // Remember the Reactor instance.
    reactor(ACE_Reactor::instance());

    ACE_TCHAR exe_path[MAX_PATH]{};
    ACE_OS::getcwd(exe_path, MAX_PATH);

    //if (ACE_OS::get_executable_name(exe_path, MAX_PATH) == -1) {
    //    ACE_ERROR((LM_ERROR, ACE_TEXT("无法获取路径: %m\n")));
    //    return 1;
    //}

    ACE_DEBUG((LM_INFO, ACE_TEXT("程序路径: %s\n"), exe_path));

    // 获取目录
    const ACE_TCHAR* dir = ACE::dirname(exe_path);
    ACE_DEBUG((LM_INFO, ACE_TEXT("程序目录: %s\n"), dir));

    // Create a persistent store.
    std::string filename = std::string(exe_path) + "\\output.log";
    outfile_.open(filename, ios::out | ios::trunc);

    // Set the ostream.
    ACE_LOG_MSG->msg_ostream(&outfile_);

    ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
}

Service::~Service()
{
    if (ACE_Reactor::instance()->cancel_timer(this) == -1)
        ACE_ERROR((LM_ERROR,
            "Service::~Service failed to cancel_timer.\n"));
}

// This method is called when the service gets a control request.  It
// handles requests for stop and shutdown by calling terminate ().
// All others get handled by calling up to inherited::handle_control.

void
Service::handle_control(DWORD control_code)
{
    if (control_code == SERVICE_CONTROL_SHUTDOWN
        || control_code == SERVICE_CONTROL_STOP)
    {
        report_status(SERVICE_STOP_PENDING);

        ACE_DEBUG((LM_INFO,
            ACE_TEXT("Service control stop requested\n")));
        stop_ = 1;
        reactor()->notify(this,
            ACE_Event_Handler::EXCEPT_MASK);
    }
    else
        inherited::handle_control(control_code);
}

// This is just here to be the target of the notify from above... it
// doesn't do anything except aid on popping the reactor off its wait
// and causing a drop out of handle_events.

int
Service::handle_exception(ACE_HANDLE)
{
    return 0;
}

// Beep every two seconds.  This is what this NT service does...

int
Service::handle_timeout(const ACE_Time_Value& tv,
    const void*)
{
    //ACE_UNUSED_ARG(tv);
    //MessageBeep(MB_OK);
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%t): Beep...\n")));

    // This message should show up in the ostream.
    ACE_DEBUG((LM_DEBUG,
        "fourth message %d, %d, %s\n", 1, 2, "this is test message"));

    return 0;
}

// This is the main processing function for the Service.  It sets up
// the initial configuration and runs the event loop until a shutdown
// request is received.

int
Service::svc()
{
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("Service::svc\n")));

    // As an NT service, we come in here in a different thread than the
    // one which created the reactor.  So in order to do anything, we
    // need to own the reactor. If we are not a service, report_status
    // will return -1.
    if (report_status(SERVICE_RUNNING) == 0)
        reactor()->owner(ACE_Thread::self());

    this->stop_ = 0;

    // Schedule a timer every two seconds.
    ACE_Time_Value tv(2, 0);
    ACE_Reactor::instance()->schedule_timer(this, 0, tv, tv);

    while (!this->stop_)
        reactor()->handle_events();

    // Cleanly terminate connections, terminate threads.
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("Shutting down\n")));
    reactor()->cancel_timer(this);
    return 0;
}

// Define a singleton class as a way to insure that there's only one
// Service instance in the program, and to protect against access from
// multiple threads.  The first reference to it at runtime creates it,
// and the ACE_Object_Manager deletes it at run-down.

typedef ACE_Singleton<Service, ACE_Mutex> SERVICE;

static BOOL WINAPI
ConsoleHandler(DWORD /*ctrlType*/)
{
    SERVICE::instance()->handle_control(SERVICE_CONTROL_STOP);
    return TRUE;
}

ACE_NT_SERVICE_DEFINE(MyAceNtservice, Service, ACE_TEXT("MyAceNtservice"));

int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {
    // 解析命令行参数（安装/卸载）
    if (argc < 2) {
        return 0;
    }

    if (ACE_OS::strcmp(argv[1], ACE_TEXT("debug")) == 0)
    {
        SetConsoleCtrlHandler(&ConsoleHandler, 1);
        SERVICE::instance()->svc();
        return 0;
    }

    SERVICE::instance()->name(ACE_TEXT("MyAceNtservice"), ACE_TEXT("this is my ace nt service"));

    if (ACE_OS::strcmp(argv[1], ACE_TEXT("install")) == 0) {
        if (SERVICE::instance()->insert() == -1) {
            ACE_ERROR((LM_ERROR, "Install failed\n"));
            return 1;
        }
        ACE_DEBUG((LM_INFO, "Service installed\n"));
        return 0;
    }

    if (ACE_OS::strcmp(argv[1], ACE_TEXT("start")) == 0) {
        if (SERVICE::instance()->start_svc() == -1) {
            ACE_ERROR((LM_ERROR, "Service failed\n"));
            return 1;
        }
        ACE_DEBUG((LM_INFO, "Service start\n"));
        return 0;
    }

    if (ACE_OS::strcmp(argv[1], ACE_TEXT("stop")) == 0) {
        if (SERVICE::instance()->stop_svc() == -1) {
            ACE_ERROR((LM_ERROR, "Remove failed\n"));
            return 1;
        }
        ACE_DEBUG((LM_INFO, "Service stoped\n"));
        return 0;
    }

    if (ACE_OS::strcmp(argv[1], ACE_TEXT("remove")) == 0) {
        if (SERVICE::instance()->remove() == -1) {
            ACE_ERROR((LM_ERROR, "Remove failed\n"));
            return 1;
        }
        ACE_DEBUG((LM_INFO, "Service removed\n"));
        return 0;
    }

    ofstream* output_file = new ofstream("ntsvc.log", ios::out);
    if (output_file && output_file->rdstate() == ios::goodbit)
        ACE_LOG_MSG->msg_ostream(output_file, 1);
    ACE_LOG_MSG->open(argv[0],
        ACE_Log_Msg::STDERR | ACE_Log_Msg::OSTREAM,
        0);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%t): Starting service.\n")));

    ACE_NT_SERVICE_RUN(MyAceNtservice, SERVICE::instance(), ret);
    if (ret == 0)
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("%p\n"),
            ACE_TEXT("Couldn't start service")));
    else
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%t): Service stopped.\n")));

    return 0;
}