#include "sourcewindow.hpp"
#include "imgui.h"

#include <algorithm>

SourceWindow::SourceWindow(GDB *gdb)
    :m_gdb(gdb), m_currentSourceLine(-1), m_executableLines(NULL), m_isFocused(false)
{

    m_sourceView.SetPalette(TextEditor::GetDarkPalette());
    m_sourceView.SetReadOnly(true);
    m_sourceView.SetAddBreakPointHandler(std::bind(SourceWindow::addBreakPoint, this, std::placeholders::_1));
}

void SourceWindow::addBreakPoint(int line)
{
    m_gdb->breakFileLine(this->m_currentFileName, line);
}

void SourceWindow::draw()
{
    if(ImGui::Begin("Source File", NULL, ImGuiWindowFlags_NoCollapse))
    {
        std::map<int, GDBBreakpoint *> *bpMap;
        m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
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
            this->m_sourceView.SetText(this->m_currentFileContent.str());
        }
        std::vector<GDBBreakpoint *> breakpoints = this->m_gdb->getBreakpoints(this->m_currentFileName);
        m_breakpoints.clear();
        for(auto bp : breakpoints)
        {
            m_breakpoints[bp->line] = bp->enabled;
        }

        m_sourceView.SetBreakpoints(m_breakpoints);

        if(this->m_gdb->getCurrentSourceLine() != -1 && this->m_currentSourceLine != this->m_gdb->getCurrentSourceLine()-1)
        {
            this->m_currentSourceLine = this->m_gdb->getCurrentSourceLine() - 1;
            m_sourceView.SetCursorPosition( TextEditor::Coordinates(m_currentSourceLine, 0));
        }
        m_sourceView.Render("SourceView");
    }
    ImGui::End();
}

bool SourceWindow::isFocused()
{
    return m_isFocused;
}
