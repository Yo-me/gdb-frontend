#include "stackwindow.hpp"
#include "imgui.h"
#include <vector>
#include "gdb.hpp"


StackWindow::StackWindow(GDB *gdb)
    :m_gdb(gdb)
{
}


void StackWindow::draw(void)
{
    const std::vector<GDBFrame> &callStack = m_gdb->getFrameStack();
    ImGui::Begin("Call Stack", NULL, ImGuiWindowFlags_NoCollapse);
    
    for(const GDBFrame &frame : callStack)
    {
        ImGui::Selectable(frame.func.c_str());
    }

    ImGui::End();

}
