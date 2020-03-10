#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "gdbwindows.hpp"
#include "mainwindow.hpp"


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
