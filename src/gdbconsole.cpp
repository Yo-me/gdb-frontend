#include "gdbconsole.hpp"

#include "imgui.h"

#include <iostream>


GDBConsole::GDBConsole(GDB *gdb, std::ostringstream &stream)
    :m_gdb(gdb),
    m_stream(stream),
    m_scrollToBottom(0)
{

}

void GDBConsole::draw()
{
    if(ImGui::Begin("Console"))
    {
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
        char command[1024] = "\0";
        bool reclaimFocus = false;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
        ImGui::TextUnformatted(this->m_stream.str().c_str());
        if(this->m_scrollToBottom)
        {
            ImGui::SetScrollHere(1.0);
            this->m_scrollToBottom--;
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::Separator();
        ImGui::PushItemWidth(ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::InputText("##ConsoleInput", command, IM_ARRAYSIZE(command), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            //command[strlen(command)-1] = '\0';
            this->m_stream << "(gdb) " << command << std::endl;
            this->m_gdb->sendCLI(command);
            std::cout << command << std::endl;
            command[0] = 0;
            reclaimFocus = true;
            this->m_scrollToBottom = 2;
        }
        ImGui::PopItemWidth();
        // Demonstrate keeping focus on the input box
        ImGui::SetItemDefaultFocus();
        if (reclaimFocus)
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
    }
    ImGui::End();
}
