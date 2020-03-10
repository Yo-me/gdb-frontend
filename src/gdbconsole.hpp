#ifndef __GDBCONSOLE_HPP__
#define __GDBCONSOLE_HPP__

#include <sstream>
#include <deque>
#include <vector>
#include <string>
#include "imgui.h"  
#include "gdb.hpp"

class GDBConsole
{
    private:
        GDB *m_gdb;
        std::ostringstream &m_stream;
        std::deque<std::string> m_lastCommands;
        std::vector<std::string> m_completionList;
        bool m_openCompletionList;
        int m_currentCommandIndex;
        bool m_isActive;
        size_t m_consoleTextLength;

        void TextEditCallback(ImGuiInputTextCallbackData *data);
        static void TextEditCallbackStub(ImGuiInputTextCallbackData *data);
    public:
        GDBConsole(GDB *gdb);
        void draw(void);
        bool isActive();
};

#endif
