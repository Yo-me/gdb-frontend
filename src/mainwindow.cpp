#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "mainwindow.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void MainWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureKeyboard)
    {
    }
    else
    {
    }
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void MainWindow::characterCallback(GLFWwindow* window, unsigned int codepoint)
{
    MainWindow *mainWindow = static_cast<MainWindow *>(glfwGetWindowUserPointer(window));
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureKeyboard || mainWindow->m_sourceWindow.isFocused())
    {
        switch(codepoint)
        {
            case 'n':
                mainWindow->m_gdb->next();
                break;
            case 's':
                mainWindow->m_gdb->step();
                break;
            case 'c':
                mainWindow->m_gdb->resume();
                break;
            case 'f':
                mainWindow->m_gdb->finish();
                break;
            case 'r':
                mainWindow->m_gdb->run();
                break;
        }
    }
    ImGui_ImplGlfw_CharCallback(window, codepoint);

}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureMouse)
    {
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureMouse)
    {
#if 0
        if(button == GLFW_MOUSE_BUTTON_RIGHT)
            displayConfig = !displayConfig;
#endif
    }

    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureMouse)
    {
    }
    else
    {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    }
}


MainWindow::MainWindow(GDB *gdb) :
    m_gdb(gdb),
    m_sourceWindow(gdb),
    m_breakpointWindow(gdb),
    m_consoleWindow(gdb)
{
    if (!glfwInit())
        std::cout << "Error initializing glfw" << std::endl;

    m_window = glfwCreateWindow(800, 600, "GDB-frontend", NULL, NULL);
    if (!m_window)
    {
        glfwTerminate();
        std::cout << "Error creating window" << std::endl;
    }

    /* Make the m_window's context current */
    glfwMakeContextCurrent(m_window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    /* Setup imgui */
    ImGui::CreateContext();
    {
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable;
        //io.ConfigViewportsNoAutoMerge = true;
    }
    ImGui_ImplGlfw_InitForOpenGL(m_window, false);
    ImGui_ImplOpenGL3_Init();
    // Setup style
    {
        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::StyleColorsDark();

        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
    }
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
    glfwSetKeyCallback(m_window, MainWindow::keyCallback);
    glfwSetCharCallback(m_window, MainWindow::characterCallback);
    glfwSetCursorPosCallback(m_window, cursor_position_callback);
    glfwSetWindowUserPointer(m_window, this);
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
    ImGuiIO& io = ImGui::GetIO();

    int display_w, display_h;
    bool displayWindow = true;

    /* Poll for and process events */
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#if 0
    ImGui::ShowMetricsWindow();
    ImGui::ShowDemoWindow(&displayWindow);
#endif
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

    ImGui::Render();
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    /* Swap front and back buffers */
    glfwSwapBuffers(m_window);
}

bool MainWindow::shouldClose()
{
    if(m_window != NULL)
    {
        return glfwWindowShouldClose(m_window);
    }
    else
    {
        return true;
    }
}
