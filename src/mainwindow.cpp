#include "imgui/imgui.h"
#include "mainwindow.hpp"


MainWindow::MainWindow(GDB *gdb) :
    m_gdb(gdb),
    m_sourceWindow(gdb),
    m_breakpointWindow(gdb),
    m_consoleWindow(gdb)
{

}

void MainWindow::draw(void)
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar
        | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar 
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoNavFocus;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoDockingInCentralNode;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("MainWindow", NULL, window_flags);
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    ImGui::End();
    
    ImGui::SetNextWindowDockID(dockspace_id); 
    m_sourceWindow.draw();
    m_breakpointWindow.draw();
    m_consoleWindow.draw();
    ImGui::PopStyleVar(3);
}
