#include "gdbconsole.hpp"

#include "imgui.h"

#include <iostream>


GDBConsole::GDBConsole(GDB *gdb)
    :m_gdb(gdb),
    m_stream(*gdb->getConsoleStream()),
    m_scrollToBottom(0),
    m_currentCommandIndex(-1),
    m_isActive(false)
{

}

void GDBConsole::TextEditCallbackStub(ImGuiInputTextCallbackData *data)
{
    GDBConsole *console = (GDBConsole*)data->UserData;
    console->TextEditCallback(data);
}

void GDBConsole::TextEditCallback(ImGuiInputTextCallbackData *data)
{
    switch(data->EventFlag)
    {
        case ImGuiInputTextFlags_CallbackCompletion:
        {
            this->m_completionList = this->m_gdb->complete(std::string(data->Buf));

            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, this->m_completionList[0].c_str());
            if(this->m_completionList.size() > 2)
            {
                /* Show completion popup */
                this->m_openCompletionList = true;
            }
            
        }
        break;
        case ImGuiInputTextFlags_CallbackHistory:
            {
                int prevHistoryPos = m_currentCommandIndex;
                if(data->EventKey == ImGuiKey_UpArrow)
                {
                    if(this->m_currentCommandIndex + 1 < this->m_lastCommands.size())
                    {
                        this->m_currentCommandIndex++;
                    }
                }
                else if(data->EventKey == ImGuiKey_DownArrow)
                {
                    if(this->m_currentCommandIndex >= 0)
                    {
                        this->m_currentCommandIndex--;
                    }
                }

                if(prevHistoryPos != m_currentCommandIndex)
                {
                    data->DeleteChars(0, data->BufTextLen);
                    if(m_currentCommandIndex != -1)
                    {
                        data->InsertChars(0, m_lastCommands.at(m_currentCommandIndex).c_str());
                    }
                    
                }
            }
            break;
        default:
            break;
    }
}

void GDBConsole::draw()
{
    if(ImGui::Begin("Console", NULL, ImGuiWindowFlags_NoCollapse))
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
        ImGui::PushItemWidth(-1);

        if (ImGui::InputText("##ConsoleInput", command, IM_ARRAYSIZE(command), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion, (ImGuiInputTextCallback)GDBConsole::TextEditCallbackStub, (void*)this))
        {
            std::string cmd(command);
            if(cmd.empty())
            {
                if(this->m_lastCommands.size() > 0)
                {
                    cmd = this->m_lastCommands.front();
                }
            }
            else
            {
                this->m_lastCommands.push_front(cmd);
            }
            
            //command[strlen(command)-1] = '\0';
            this->m_stream << "(gdb) " << cmd << std::endl;
            this->m_gdb->sendCLI(cmd.c_str());
            command[0] = 0;
            reclaimFocus = true;
            this->m_scrollToBottom = 2;
            this->m_currentCommandIndex = -1;
        }

        ImGui::PopItemWidth();
        // Demonstrate keeping focus on the input box
        ImGui::SetItemDefaultFocus();
        if (reclaimFocus)
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
        m_isActive = ImGui::IsItemActive();
    }
    ImGui::End();
}

bool GDBConsole::isActive()
{
    return this->m_isActive;
}
