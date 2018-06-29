#include "gdb.hpp"

#include <sstream>
#include <ostream>
#include <iostream>
#include <string>

static void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

GDB::GDB(std::ostream &consoleStream):m_consoleStream(consoleStream) {}

bool GDB::setExeFile(const std::string &filename)
{
    std::ostringstream cmd;
    std::string response;
    GDBOutput *rsp;

    cmd << "-file-exec-and-symbols \"" << filename << "\"\n";
    this->send(cmd.str());

    std::cout << response << std::endl;

    rsp = this->getResponse();

    delete rsp;
}

bool GDB::sendCLI(const std::string &command)
{
    std::ostringstream cmd;
    GDBOutput *rsp;
    cmd << "-interpreter-exec console \"" << command << "\"\n";
    this->send(cmd.str());

    return this->checkResultDone();
}

GDBOutput *GDB::getResponse()
{
    GDBOutput *ret = NULL;
    GDBOutput *current = NULL;

    GDBOutput *o;
    bool finished = false;
    do
    {
        std::string output;
        this->readline(output);
        bool add = true;

        if(output.length() > 0 && output.compare(0, 5, "(gdb)"))
        {
            std::cout << output << std::endl;
            o = this->parseOutput(output);
            if(o->t == GDB_TYPE_OUT_OF_BAND && o->st == GDB_SUBTYPE_STREAM)
            {
                add = false;
                switch(o->sst)
                {
                    case GDB_SUBSUBTYPE_CONSOLE:
                        this->m_consoleStream << o->rs.cstr;
                        break;
                }
            }
            else
            {

            }

            if(add)
            {
                if(current)
                {
                    current->next = o;
                    current = current->next;
                }
                else
                {
                    ret = o;
                    current = ret;
                }
            }
            else
            {
                delete o;
                o = NULL;
            }
        }
        else
        {
            finished = true;
        }
    } while(!finished);
    return ret;
}

GDBOutput *GDB::getResponseBlk()
{
    GDBOutput *o;
    do
    {
        o = this->getResponse();
    } while(!o);

    return o;
}


GDBOutput *GDB::parseOutput(std::string &str)
{
    GDBOutput *o = new GDBOutput;
    if(!o)
    {
        return NULL;
    }
    else
    {
        char first = str[0];
        o->next = NULL;
        str.erase(0, 1);
        if(first == '~' || first == '=' || first == '*')
        {
            this->parseConsoleStreamOutput(o, str);
        }
        else if(first == '^')
        {
            this->parseResultRecord(o, str);
        }
        else
        {
        }
    }
    return o;
}

void GDB::parseResultRecord(GDBOutput *o, std::string &str)
{
    if(!str.compare(0, 4, "done"))
    {
        o->cl = GDB_CLASS_DONE;
        str.erase(0, 4);
    }
    else if(!str.compare(0, 7, "running"))
    {
        o->cl = GDB_CLASS_RUNNING;
        str.erase(0, 7);
    }
    else if(!str.compare(0, 9, "connected"))
    {
        o->cl = GDB_CLASS_CONNECTED;
        str.erase(0, 9);
    }
    else if(!str.compare(0, 5, "error"))
    {
        o->cl = GDB_CLASS_ERROR;
        str.erase(0, 5);
    }
    else if(!str.compare(0, 4, "exit"))
    {
        o->cl = GDB_CLASS_EXIT;
        str.erase(0, 4);
    }
    else
    {
        /* TODO Handle error */
    }
    o->t = GDB_TYPE_RESULT_RECORD;
}

void GDB::parseConsoleStreamOutput(GDBOutput *o, std::string &str)
{
    o->sst = GDB_SUBSUBTYPE_CONSOLE;

    this->parseConsoleOutput(o, str);
}

void GDB::parseConsoleOutput(GDBOutput *o, std::string &str)
{
    o->t = GDB_TYPE_OUT_OF_BAND;
    o->st = GDB_SUBTYPE_STREAM;
    this->parseString(o, str);
}

void GDB::parseString(GDBOutput *o, std::string &str)
{
    if(str[0] != '\"')
        return;
    else
    {
        str.erase(0, 1);
        size_t endPos = str.rfind('\"');
        if(endPos == std::string::npos)
            return;
        else
        {
            o->rs.cstr = str.substr(0, endPos);
            replaceAll(o->rs.cstr, "\\\"", "\"");
            replaceAll(o->rs.cstr, "\\n", "\n");
            replaceAll(o->rs.cstr, "\\t", "\t");
            str.erase(0, endPos+1);
        }
    }
}

GDBOutput *GDB::getResultRecord(GDBOutput *o)
{
    if(!o)
        return NULL;

    do
    {
        if(o->t == GDB_TYPE_RESULT_RECORD)
        {
            return o;
        }
        else
        {
            o = o->next;
        }
    } while(o);

    return o;
}

bool GDB::checkResult(GDB_MESSAGE_CLASS c)
{
    bool success = false;
    GDBOutput *o;
    GDBOutput *res = this->getResponseBlk();
    o = this->getResultRecord(res);
    if(o)
        success = (o->cl == c);

    this->freeOutput(res);
    return success;
}

bool GDB::checkResultDone()
{
    return this->checkResult(GDB_CLASS_DONE);
}

void GDB::freeOutput(GDBOutput *o)
{
    if(o)
    {
        this->freeOutput(o->next);
        delete o;
    }
}

