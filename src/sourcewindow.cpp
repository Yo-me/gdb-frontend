#include "sourcewindow.hpp"
#include "imgui.h"

SourceWindow::SourceWindow(GDB *gdb)
    :m_gdb(gdb)
{}

void SourceWindow::draw()
{
    static bool once = true;
    int currentSourceLine = -1;

    if(ImGui::Begin("Source File"))
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 textSize;
        int nbLines;
        float lineNumbersColumnWidth;
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
        nbLines = textSize.y / ImGui::GetTextLineHeight();
        ImGui::Columns(2, "#cols", false);

        /* Draw line numbers */
        /* Move Clipping Region down to be in front of first line */
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemInnerSpacing.y);
        /* Create Clipping Region (for scrolling) */
        ImGui::BeginChild("ScrollNumbers", ImVec2(0.0, 0.0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        /* Disable item spacing */
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0, 0.0));
        for(int i = 1; i <= nbLines; i++)
        {
            ImGui::Selectable(std::to_string(i).c_str());
        }
        ImGui::PopStyleVar();

        /* Compute column width */
        ImGui::EndChild();
        lineNumbersColumnWidth = ImGui::CalcTextSize(std::to_string(nbLines).c_str()).x + (ImGui::GetStyle().WindowPadding.x * 2.0) + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetColumnWidth(0, lineNumbersColumnWidth);
        ImGui::NextColumn();

        ImGui::InputTextMultiline("##Source", (char *)this->m_currentFileContent.str().c_str(), this->m_currentFileContent.str().length(), ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
        {
            float scrollY = 0;
            float size;
            currentSourceLine = this->m_gdb->getCurrentSourceLine() - 1;
            if(currentSourceLine != -1)
            {
                ImGui::BeginChild(ImGui::GetID("##Source"), ImVec2(-1.0, -1.0));
                {
                    scrollY = ImGui::GetScrollY();
                    float spacing = ImGui::GetTextLineHeight();

                    float lineStartY = currentSourceLine * spacing + ImGui::GetStyle().ItemSpacing.y;

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
