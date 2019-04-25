#ifndef __GDBCONSOLE_HPP__
#define __GDBCONSOLE_HPP__

#include <sstream>

#include "gdb.hpp"

class GDBConsole
{
    private:
        GDB *m_gdb;
        std::ostringstream &m_stream;
        int m_scrollToBottom;
    public:
        GDBConsole(GDB *gdb);
        void draw(void);
};

#endif
