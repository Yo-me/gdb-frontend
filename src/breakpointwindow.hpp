#ifndef __BREAKPOINTWINDOW_HPP__
#define __BREAKPOINTWINDOW_HPP__

#include "gdb.hpp"

class BreakpointWindow
{
    private:
        GDB *m_gdb;
    public:
        BreakpointWindow(GDB *gdb);
        void draw(void);
};

#endif
