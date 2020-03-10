#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "gdbwindows.hpp"
#include "mainwindow.hpp"

void BindStdHandlesToConsole()
{
    //TODO: Add Error checking.
    
    // Redirect the CRT standard input, output, and error handles to the console
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stderr);
    freopen("CONOUT$", "w", stdout);
    
    // Note that there is no CONERR$ file
    HANDLE hStdout = CreateFile("CONOUT$",  GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hStdin = CreateFile("CONIN$",  GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    SetStdHandle(STD_OUTPUT_HANDLE,hStdout);
    SetStdHandle(STD_ERROR_HANDLE,hStdout);
    SetStdHandle(STD_INPUT_HANDLE,hStdin);

    //Clear the error state for each of the C++ standard stream objects. 
    std::wclog.clear();
    std::clog.clear();
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();
}


int main(int argc, char **argv)
{
    GLFWwindow* window;
    std::string message;
    std::string command;
    std::ostringstream consoleStream;
    std::string gdbPath = "gdb.exe";
    std::string gdbArgs = "";
    bool verbose = false;
    int opt;

    while((opt = getopt(argc, argv, "g:v")) != -1)
    {
        switch(opt)
        {
            case 'g':
                gdbPath = optarg;
                break;
            case 'v':
                verbose = true;
                AllocConsole();
                BindStdHandlesToConsole();
                break;
            default:
                std::cerr << "Usage : " << std::endl << argv[0] << " [-g <gdb executable>]" << std::endl;
                exit(1);
                break;
        }
    }

    for(int i = optind; i < argc; i++)
    {
        gdbArgs += argv[i] + std::string(" ");
    }

    GDBWindows gdb(gdbPath, gdbArgs);
    if(verbose)
        gdb.setVerbose();
        
    if(!gdb.connect())
    {
        exit(1);
        std::cout << "Error connecting to gdb" << std::endl;
    }
    {
        MainWindow mainWindow(&gdb);

        while (!mainWindow.shouldClose() && gdb.getState() != GDB_STATE_EXITED)
        {

            gdb.poll();
            /* Render ImgUI windows */
            {
                mainWindow.draw();
            }


        }
    }

    return 0;
}
