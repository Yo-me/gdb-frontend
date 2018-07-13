#ifndef __SOURCEWINDOW_HPP__
#define __SOURCEWINDOW_HPP__

#include "gdb.hpp"

#include <fstream>
#include <sstream>
#include <map>


class SourceWindow
{
    private:
        GDB *m_gdb;
        std::string m_currentFileName;
        std::stringstream m_currentFileContent;
        int m_currentSourceLine;
        std::map<int, bool> *m_executableLines;

    public:
        SourceWindow(GDB *gdb);
        void draw();
};

#endif
