#include "imgui.h"
#include "breakpointwindow.hpp"


BreakpointWindow::BreakpointWindow(GDB *gdb)
    :m_gdb(gdb)
{
}

void BreakpointWindow::draw(void)
{
    ImGui::Begin("Breakpoints");
    {
        const std::vector<GDBBreakpoint *> &bps = this->m_gdb->getBreakpoints();
        ImGui::Columns(4, "#breapointlist", true);
        ImGui::Separator();
        ImGui::Text("Enabled"); ImGui::NextColumn();
        ImGui::Text("Number"); ImGui::NextColumn();
        ImGui::Text("Location"); ImGui::NextColumn();
        ImGui::Text("Count"); ImGui::NextColumn();
        ImGui::Separator();
        for(std::vector<GDBBreakpoint *>::const_iterator it = bps.begin(); it != bps.end(); it++)
        {
            bool checked = (*it)->enabled;
            ImGui::PushID(*it);
            if(ImGui::Checkbox("##enabled", &checked))
            {
                this->m_gdb->setBreakpointState((*it)->number, checked);
            }
            ImGui::NextColumn();
            ImGui::Text((char *)(*it)->number.c_str());
            ImGui::NextColumn();
            ImGui::Text("%s : %d", (char *)(*it)->fullname.c_str(), (*it)->line);
            ImGui::NextColumn();
            ImGui::Text((char *)(std::to_string((*it)->times).c_str()));
            ImGui::PopID();
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::Separator();

    }
    ImGui::End();
}
