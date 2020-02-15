#ifndef __GDBWINDOWS_HPP__
#define __GDBWINDOWS_HPP__

#include "gdb.hpp"

#include <windows.h>
#include <string>

class GDBWindows : public GDB
{
    private:
        std::string m_path;
        std::string m_args;
        HANDLE m_toGDB;
        HANDLE m_fromGDB;
        HANDLE m_appPipe;
        HANDLE m_processHandle;

    public:
        GDBWindows(std::string path = "gdb.exe", std::string gdbArgs = "");
        bool connect();
        bool readline(std::string &message);
        bool send(const std::string &message);
        bool gdbProcessRunning();
};

#endif
