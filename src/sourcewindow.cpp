#include "sourcewindow.hpp"
#include "imgui.h"

#include <algorithm>

SourceWindow::SourceWindow(GDB *gdb)
    :m_gdb(gdb), m_currentSourceLine(-1), m_executableLines(NULL), m_isFocused(false)
{

    m_sourceView.SetPalette(TextEditor::GetDarkPalette());
    m_sourceView.SetReadOnly(true);
}

void SourceWindow::draw()
{
    if(ImGui::Begin("Source File", NULL, ImGuiWindowFlags_NoCollapse))
    {
        TextEditor::Breakpoints breakpoints;
        ImVec2 textSize;
        int nbLines;
        float lineNumbersColumnWidth;
        float numbersScrollMax;
        float bpWidth = ImGui::CalcTextSize("-").x + ImGui::GetStyle().WindowPadding.x;
        std::map<int, GDBBreakpoint *> *bpMap;
        m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        if(this->m_currentFileName != this->m_gdb->getCurrentFilePath())
        {
            this->m_currentFileContent.str("");
            this->m_currentFileName = this->m_gdb->getCurrentFilePath();
            //if(this->m_executableLines)
            //    delete(this->m_executableLines);
            //this->m_executableLines = this->m_gdb->getExecutableLines(this->m_currentFileName);
            std::ifstream f(this->m_currentFileName);
            if(!f.fail())
            {
                this->m_currentFileContent << f.rdbuf();
            }
            else
            {
                this->m_currentFileContent << "Unable to open file " << this->m_currentFileName;
            }
            this->m_sourceView.SetText(this->m_currentFileContent.str());
        }
        bpMap = this->m_gdb->getBreakpoints(this->m_currentFileName);

        for(auto bp : *bpMap)
        {
            if(bp.second->enabled)
                breakpoints.insert(bp.first);
        }

        m_sourceView.SetBreakpoints(breakpoints);

        if(this->m_gdb->getCurrentSourceLine() != -1 && this->m_currentSourceLine != this->m_gdb->getCurrentSourceLine()-1)
        {
            this->m_currentSourceLine = this->m_gdb->getCurrentSourceLine() - 1;
            m_sourceView.SetCursorPosition( TextEditor::Coordinates(m_currentSourceLine, 0));
        }
        m_sourceView.Render("SourceView");
#if 0
        if(this->m_currentFileContent.str().length() > 0)
        {
            std::string content = this->m_currentFileContent.str();
            nbLines = std::count(content.begin(), content.end(), '\n') + 2;
        }
        else
        {
            nbLines = 1;
        }
        ImGui::Columns(3, "#cols", false);

        /* Draw line numbers */
        /* Move Clipping Region down to be in front of first line */
        /* Create Clipping Region (for scrolling) */
        /* Disable item spacing */
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0, 0.0));
        ImGui::BeginChild("ScrollBps", ImVec2(0.0, 0.0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);

        //ImGui::SetCursorPosX(ImGui::GetStyle().ItemInnerSpacing.x);
        {
            for(int i = 1; i <= nbLines; i++)
            {
                if(this->m_executableLines && (*this->m_executableLines)[i])
                    ImGui::Text("-", false, 0, ImVec2(bpWidth, 0));
                else
                    ImGui::Text(" ");
            }
        }
        ImGui::EndChild();
        ImGui::SetColumnWidth(0, bpWidth);

        ImGui::NextColumn();

        ImGui::BeginChild("ScrollNumbers", ImVec2(0.0, 0.0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
        for(int i = 1; i <= nbLines; i++)
        {
                if(this->m_executableLines && (*this->m_executableLines)[i])
                {
                    bool selected = (*bpMap)[i];
                    bool enabled = selected && ((*bpMap)[i]->enabled == true);

                    if(selected && enabled)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(255, 0, 0, 255));
                    }
                    else if(selected)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(128, 0, 0, 255));
                    }
                    if(ImGui::Selectable(std::to_string(i).c_str(), selected))
                    {
                        if(selected)
                        {
                            this->m_gdb->breakDelete((*bpMap)[i]->number);
                        }
                        else
                        {
                            this->m_gdb->breakFileLine(this->m_currentFileName, i);
                        }
                    }
                    if(selected || enabled)
                        ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::Text(std::to_string(i).c_str());
                }
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
        /* Compute column width */
        numbersScrollMax = ImGui::GetScrollMaxY();
        ImGui::EndChild();
        lineNumbersColumnWidth = ImGui::CalcTextSize(std::to_string(nbLines).c_str()).x + (ImGui::GetStyle().WindowPadding.x)  + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetColumnWidth(1, lineNumbersColumnWidth);
        ImGui::NextColumn();

        ImVec2 pos = ImGui::GetCursorScreenPos();
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

                    float lineStartY = this->m_currentSourceLine * spacing + ImGui::GetStyle().ItemInnerSpacing.y;

                    float lineEndY = ImGui::GetTextLineHeight();
                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pos.x, pos.y + lineStartY - scrollY), ImVec2(pos.x + ImGui::GetWindowContentRegionWidth() + ImGui::GetStyle().ItemSpacing.x, pos.y + lineStartY + lineEndY - scrollY), IM_COL32(255, 255, 255, 64));
                }
                size = ImGui::GetWindowContentRegionWidth();
                ImGui::EndChild();
                ImGui::BeginChild("ScrollNumbers", ImVec2(0.0, size));
                ImGui::SetScrollY(scrollY);
                ImGui::EndChild();
                ImGui::BeginChild("ScrollBps", ImVec2(0.0, size));
                ImGui::SetScrollY(scrollY);
                ImGui::EndChild();
            }
        }
        ImGui::PopStyleVar();
#endif
    }
    ImGui::End();
}

bool SourceWindow::isFocused()
{
    return m_isFocused;
}
