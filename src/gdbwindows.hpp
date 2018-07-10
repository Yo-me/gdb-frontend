#ifndef __GDBWINDOWS_HPP__
#define __GDBWINDOWS_HPP__

#include "gdb.hpp"

#include <windows.h>
#include <string>

class GDBWindows : public GDB
{
    private:
        std::string m_path;
        HANDLE m_toGDB;
        HANDLE m_fromGDB;
        HANDLE m_consolePipe;
        HANDLE m_appPipe;
        HANDLE m_processHandle;

    public:
        GDBWindows(std::ostream &consoleOutputStream, std::string path = "gdb.exe");
        bool connect();
        bool readline(std::string &message);
        bool send(const std::string &message);
        bool gdbProcessRunning();
};

#endif
