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
    const std::map<std::string, std::vector<GDBFrame>> &callStack = m_gdb->getFrameStack();
    ImGui::Begin("Call Stack", NULL, ImGuiWindowFlags_NoCollapse);
    for(const std::pair<const std::string, std::vector<GDBFrame>> &threadFrame : callStack)
    {
        std::string title = "Thread " + threadFrame.first;
        if(ImGui::CollapsingHeader(title.c_str(), this->m_gdb->getCurrentThread() == threadFrame.first ? ImGuiTreeNodeFlags_DefaultOpen : 0x0))
        {
            int currentFrameLevel = this->m_gdb->getCurrentFrameLevel();
            for(const GDBFrame &frame : threadFrame.second)
            {
                std::string func = frame.func + "(";
                for(int index = 0; index < frame.args.size(); ++index)
                {
                    func += frame.args[index];

                    if(index < frame.args.size() - 1)
                        func += ", ";
                }
                func += ")";
                if(ImGui::Selectable(func.c_str(), frame.level == currentFrameLevel && this->m_gdb->getCurrentThread() == threadFrame.first))
                {
                    this->m_gdb->setCurrentFrameLevel(frame.level);
                }
            }
        }
    }
    ImGui::End();


}
