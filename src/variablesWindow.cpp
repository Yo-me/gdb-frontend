#include "variablesWindow.hpp"
#include "gdb.hpp"
#include "imgui.h"

VariablesWindow::VariablesWindow(GDB *gdb)
    :m_gdb(gdb)
{

}

void VariablesWindow::drawVariableRecursive(std::vector<GDBVariableObject> &variables)
{
    for(GDBVariableObject &v : variables)
    {
        std::string label = v.expression + ": " + v.value;
        ImGui::PushID(v.expression.c_str());
        if(v.has_children)
        {
            if(ImGui::TreeNode(label.c_str()))
            {
                if(v.children.size() == 0)
                {
                    this->m_gdb->retrieveVariableObjectChildren(v);
                }
                this->drawVariableRecursive(v.children);
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::Indent();
            ImGui::Text(label.c_str());
            ImGui::Unindent();
        }
        ImGui::PopID(); 
    }
}

void VariablesWindow::draw()
{
    ImGui::Begin("Variables", NULL, ImGuiWindowFlags_NoCollapse);
    if(ImGui::CollapsingHeader("Local Variables", ImGuiTreeNodeFlags_DefaultOpen))
    {
        this->drawVariableRecursive(this->m_gdb->getVariableObjects());
    }
    
    ImGui::End();
}