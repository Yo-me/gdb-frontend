#include <iostream>
#include <sstream>

#include "gdbwindows.hpp"

GDBWindows::GDBWindows(std::ostream &consoleStream, std::string path)
: GDB(consoleStream), m_path(path)
{

}


bool GDBWindows::connect()
{
    HANDLE tmpGDBRead;
    HANDLE tmpGDBWrite;
    SECURITY_ATTRIBUTES saAttr;
    SECURITY_DESCRIPTOR saDesc;
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;
    DWORD pipeMode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    std::ostringstream commandLine;
    std::string consolePipeName = "\\\\.\\pipe\\gdbfrontend";
    InitializeSecurityDescriptor(&saDesc, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&saDesc, FALSE, NULL, FALSE);

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = &saDesc;

    if(!CreatePipe(&this->m_fromGDB, &tmpGDBWrite, &saAttr, 0))
    {
        std::cerr << "Error creating pipe from GDB" << std::endl;
        return false;
    }

    if(!SetHandleInformation(this->m_fromGDB, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "Error setting handle information for pipe from GDB" << std::endl;
        return false;
    }

    if(!CreatePipe(&tmpGDBRead, &this->m_toGDB, &saAttr, 0))
    {
        std::cerr << "Error creating pipe to GDB" << std::endl;
        return false;
    }

    if(!SetHandleInformation(this->m_toGDB, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "Error setting handle information for pipe to GDB" << std::endl;
        return false;
    }

    if(!CreateNamedPipe((LPCSTR)consolePipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES, 8*1024, 8*1024, 0, NULL))
    {
        std::cerr << "Error creating console named pipe" << std::endl;
        return false;
    }
    SetNamedPipeHandleState(this->m_fromGDB, &pipeMode, NULL, NULL);
    //SetNamedPipeHandleState(this->m_toGDB, &pipeMode, NULL, NULL);

    // Set up members of the PROCESS_INFORMATION structure.

    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

    // Set up members of the STARTUPINFO structure.
    // This structure specifies the STDIN and STDOUT handles for redirection.

    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = tmpGDBWrite;
    siStartInfo.hStdOutput = tmpGDBWrite;
    siStartInfo.hStdInput = tmpGDBRead;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    commandLine << this->m_path << " --interpreter=mi --quiet";

    // Create the child process.

    bSuccess = CreateProcess(NULL,
            (LPSTR)commandLine.str().c_str(),     // command line
            NULL,          // process security attributes
            NULL,          // primary thread security attributes
            TRUE,          // handles are inherited
            0,             // creation flags
            NULL,          // use parent's environment
            NULL,          // use parent's current directory
            &siStartInfo,  // STARTUPINFO pointer
            &piProcInfo);  // receives PROCESS_INFORMATION

    // If an error occurs, exit the application.
    if ( ! bSuccess )
    {
        std::cerr << "Error creating gdb process" << std::endl;
        return false;
    }
    else
    {
        /* TODO : Find a better solution */
        Sleep(1000);
    }

    return true;
}

bool GDBWindows::readline(std::string &message)
{
    char c;
    DWORD length = 0;
    BOOL success;

    do
    {
        success = ReadFile(this->m_fromGDB, &c, 1, &length, NULL);

        if(c == '\n')
        {
            return true;
        }
        else if(c != '\r')
        {
            message.append(&c, 1);
        }
    } while(success && length > 0);

    if(message.length() > 0)
        std::cout << "Received from gdb : " << message << std::endl;

    return false;
}

bool GDBWindows::send(const std::string &message)
{
    char *buffer[8*1024];
    DWORD length;
    BOOL success;
    std::cout << "Sending data to gdb : " << message;

    success = WriteFile(this->m_toGDB, message.c_str(), message.length(), &length, NULL);

    std::cout << ((success && length > 0) ? "Success " : "Failed ") << GetLastError() << std::endl;
    return (success && length > 0);
}
