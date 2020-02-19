#ifndef __VARIABLESWINDOW_HPP__
#define __VARIABLESWINDOW_HPP__

#include "gdb.hpp"
#include <vector>

class VariablesWindow
{
    private:
        GDB *m_gdb;

        void drawVariableRecursive(std::vector<GDBVariableObject> &variables);
    public:
        VariablesWindow(GDB *gdb);
        void draw(void);

};

#endif