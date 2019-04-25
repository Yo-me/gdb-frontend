#ifndef __MAINWINDOW_HPP__
#define __MAINWINDOW_HPP__

#include "gdbconsole.hpp"
#include "sourcewindow.hpp"
#include "breakpointwindow.hpp"

class GDB;

class MainWindow
{
    private:
        GDB *m_gdb;
        GDBConsole m_consoleWindow;
        SourceWindow m_sourceWindow;
        BreakpointWindow m_breakpointWindow;

    public:
        MainWindow(GDB *gdb);
        void draw(void);
};


#endif
