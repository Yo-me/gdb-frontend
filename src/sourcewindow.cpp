#include "sourcewindow.hpp"
#include "imgui.h"

#include <algorithm>

SourceWindow::SourceWindow(GDB *gdb)
    :m_gdb(gdb), m_currentSourceLine(-1)
{}

void SourceWindow::draw()
{
    if(ImGui::Begin("Source File"))
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 textSize;
        int nbLines;
        float lineNumbersColumnWidth;
        float numbersScrollMax;
        if(this->m_currentFileName != this->m_gdb->getCurrentFilePath())
        {
            this->m_currentFileContent.str("");
            this->m_currentFileName = this->m_gdb->getCurrentFilePath();

            std::ifstream f(this->m_currentFileName);
            if(!f.fail())
            {
                this->m_currentFileContent << f.rdbuf();
            }
            else
            {
                this->m_currentFileContent << "Unable to open file " << this->m_currentFileName;
            }
        }

        textSize = ImGui::CalcTextSize((char *)this->m_currentFileContent.str().c_str(), NULL);
        if(this->m_currentFileContent.str().length() > 0)
        {
            std::string content = this->m_currentFileContent.str();
            nbLines = std::count(content.begin(), content.end(), '\n') + 2;
        }
        else
        {
            nbLines = 1;
        }
        ImGui::Columns(2, "#cols", false);

        /* Draw line numbers */
        /* Move Clipping Region down to be in front of first line */
        /* Create Clipping Region (for scrolling) */
        ImGui::BeginChild("ScrollNumbers", ImVec2(0.0, 0.0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);

        /* Disable item spacing */
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0, 0.0));
        for(int i = 1; i <= nbLines; i++)
        {
            ImGui::Selectable(std::to_string(i).c_str());
        }
        ImGui::PopStyleVar();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
        /* Compute column width */
        numbersScrollMax = ImGui::GetScrollMaxY();
        ImGui::EndChild();
        lineNumbersColumnWidth = ImGui::CalcTextSize(std::to_string(nbLines).c_str()).x + (ImGui::GetStyle().WindowPadding.x)  + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetColumnWidth(0, lineNumbersColumnWidth);
        ImGui::NextColumn();

        ImGui::InputTextMultiline("##Source", (char *)this->m_currentFileContent.str().c_str(), this->m_currentFileContent.str().length(), ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
        {
            float scrollY = 0;
            float size;
            bool scroll = false;
            if(this->m_gdb->getCurrentSourceLine() != -1 && this->m_currentSourceLine != this->m_gdb->getCurrentSourceLine()-1)
            {
                this->m_currentSourceLine = this->m_gdb->getCurrentSourceLine() - 1;
                scroll=true;
            }
            if(this->m_currentSourceLine != -1)
            {
                ImGui::BeginChild(ImGui::GetID("##Source"), ImVec2(-1.0, -1.0));
                {
                    if(scroll)
                    {
                        scrollY = this->m_currentSourceLine * ImGui::GetTextLineHeight() - ImGui::GetWindowHeight()/2;
                        ImGui::SetScrollY(scrollY);
                    }
                    else
                    {
                        scrollY = ImGui::GetScrollY();
                    }
                    if(scrollY > numbersScrollMax)
                        scrollY = numbersScrollMax;
                    float spacing = ImGui::GetTextLineHeight();

                    float lineStartY = this->m_currentSourceLine * spacing + ImGui::GetStyle().ItemSpacing.y;

                    float lineEndY = ImGui::GetTextLineHeight();
                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pos.x, pos.y + lineStartY - scrollY), ImVec2(pos.x + ImGui::GetWindowContentRegionWidth() + ImGui::GetStyle().ItemSpacing.x, pos.y + lineStartY + lineEndY - scrollY), IM_COL32(255, 255, 255, 64));
                }
                size = ImGui::GetWindowContentRegionWidth();
                ImGui::EndChild();
                ImGui::BeginChild("ScrollNumbers", ImVec2(0.0, size));
                ImGui::SetScrollY(scrollY);
                ImGui::EndChild();
            }
        }
    }
    ImGui::End();
}
