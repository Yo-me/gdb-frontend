#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "gdbwindows.hpp"
#include "gdbconsole.hpp"
#include "sourcewindow.hpp"
#include "breakpointwindow.hpp"

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureKeyboard)
    {
    }
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

static void character_callback(GLFWwindow* window, unsigned int codepoint)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureKeyboard)
    {
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


int main(int argc, char **argv)
{
    GLFWwindow* window;
    std::string message;
    std::string command;
    std::ostringstream consoleStream;
    std::string gdbPath = "gdb.exe";
    int opt;

    while((opt = getopt(argc, argv, "g:")) != -1)
    {
        switch(opt)
        {
            case 'g':
                gdbPath = optarg;
                break;
            default:
                std::cerr << "Usage : " << std::endl << argv[0] << " [-g <gdb executable>]" << std::endl;
                exit(1);
                break;
        }
    }

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(800, 600, "GDB-frontend", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
	std::cout << "Error initializing glfw" << std::endl;
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    /* Setup imgui */
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    // Setup style
    {
        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::StyleColorsDark();

        style.WindowRounding = 6.0f;
        style.FrameRounding = 6.0f;
    }

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, character_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    GDBWindows gdb(dynamic_cast<std::ostream&>(consoleStream), gdbPath);

    if(!gdb.connect())
    {
        exit(1);
        std::cout << "Error connecting to gdb" << std::endl;
    }
    {
        GDBConsole console(&gdb, consoleStream);
        SourceWindow srcWindow(&gdb);
        BreakpointWindow bpWindow(&gdb);
        while (!glfwWindowShouldClose(window) && gdb.getState() != GDB_STATE_EXITED)
        {
            gdb.poll();
            /* Render ImgUI windows */
            {
                int display_w, display_h;
                bool displayWindow = true;

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();



                /* Breakpoints */
                bpWindow.draw();
                /* Console */
                console.draw();
                /* Source File */
                srcWindow.draw();


#if 1
                ImGui::ShowMetricsWindow();
                ImGui::ShowDemoWindow(&displayWindow);
#endif
                ImGui::Render();
                glfwGetFramebufferSize(window, &display_w, &display_h);
                glViewport(0, 0, display_w, display_h);
                glClearColor(0.0, 0.0, 0.0, 1.0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
        }
    }

    return 0;
}
