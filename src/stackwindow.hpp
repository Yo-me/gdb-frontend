#ifndef __STACKWINDOW_HPP__
#define __STACKWINDOW_HPP__

class GDB;

class StackWindow
{
    private:
        GDB *m_gdb;

    public:
        StackWindow(GDB *gdb);
        void draw(void);

};

#endif
