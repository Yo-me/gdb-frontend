#ifndef __MAINWINDOW_HPP__
#define __MAINWINDOW_HPP__

#include "gdbconsole.hpp"
#include "sourcewindow.hpp"
#include "breakpointwindow.hpp"
#include "stackwindow.hpp"

class GDB;
class GLFWwindow;

class MainWindow
{
    private:
        GLFWwindow *m_window;
        GDB *m_gdb;
        GDBConsole m_consoleWindow;
        SourceWindow m_sourceWindow;
        BreakpointWindow m_breakpointWindow;
        StackWindow m_stackWindow;
        bool m_isFocused;

        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void characterCallback(GLFWwindow* window, unsigned int codepoint);
    public:
        MainWindow(GDB *gdb);
        void draw(void);
        bool shouldClose();
};


#endif
