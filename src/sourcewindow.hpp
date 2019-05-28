#ifndef __SOURCEWINDOW_HPP__
#define __SOURCEWINDOW_HPP__

#include "gdb.hpp"

#include <fstream>
#include <sstream>
#include <map>
#include "TextEditor.h"


class SourceWindow
{
    private:
        GDB *m_gdb;
        std::string m_currentFileName;
        std::stringstream m_currentFileContent;
        int m_currentSourceLine;
        std::map<int, bool> *m_executableLines;
        
        TextEditor m_sourceView;
        bool m_isFocused;

    public:
        SourceWindow(GDB *gdb);
        void draw();
        bool isFocused();
};

#endif
