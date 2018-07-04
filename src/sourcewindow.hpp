#ifndef __SOURCEWINDOW_HPP__
#define __SOURCEWINDOW_HPP__

#include "gdb.hpp"

#include <fstream>
#include <sstream>


class SourceWindow
{
    private:
        GDB *m_gdb;
        std::string m_currentFileName;
        std::stringstream m_currentFileContent;
        int m_currentSourceLine;

    public:
        SourceWindow(GDB *gdb);
        void draw();
};

#endif
