#include "gdb.hpp"

#include <sstream>
#include <ostream>
#include <iostream>
#include <string>
#include <regex>

static void replaceAll(std::string& str, const std::string& from, const std::string& to) {

    std::regex e(from);
    while(std::regex_search(str, e))
    {
        str = std::regex_replace(str, e, to);
    }
}

GDB::GDB(std::ostream &consoleStream):
    m_consoleStream(consoleStream),
    m_state(GDB_STATE_STOPPED),
    m_currentFile(""),
    m_currentSourceLine(-1)
{
}

std::string GDB::getCurrentFilePath()
{
    return this->m_currentFile;
}

int GDB::getCurrentSourceLine()
{
    return this->m_currentSourceLine;
}

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



void GDB::poll(void)
{
    GDBOutput *rsp = this->getResponse();
    if(rsp)
    {
        GDBStopResult *s = this->getStopResult(rsp);
        if(s)
        {
            if(s->reason != GDB_STOP_REASON_UNKNOWN)
            {
                this->m_state = GDB_STATE_STOPPED;
                if(s->frame)
                {
                    this->m_currentFile = s->frame->fullname;
                    this->m_currentSourceLine = s->frame->line;
                }
                else
                {
                    this->m_currentFile = "";
                    this->m_currentSourceLine = -1;
                }
            }
            delete(s->frame);
            delete(s);
        }
        this->freeOutput(rsp);
    }
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
            if(o)
                if(o->t == GDB_TYPE_OUT_OF_BAND && o->st == GDB_SUBTYPE_STREAM)
                {
                    add = false;
                    switch(o->sst)
                    {
                        case GDB_SUBSUBTYPE_CONSOLE:
                            this->m_consoleStream << o->rs[0]->cstr;
                            break;
                    }
                }
                else
                {

                }
            else
                add = false;

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
        if(first == '&' || first == '~')
        {
            this->parseConsoleStreamOutput(o, str);
        }
        else if(first == '^')
        {
            this->parseResultRecord(o, str);
        }
        else if(first == '*')
        {
            this->parseExecAsyncRecord(o, str);
        }
        else
        {
        }
    }
    return o;
}

void GDB::parseExecAsyncRecord(GDBOutput *o, std::string &str)
{
    o->sst = GDB_SUBSUBTYPE_EXEC;
    this->parseAsyncRecord(o, str);
}

void GDB::parseAsyncRecord(GDBOutput *o, std::string &str)
{
    o->t = GDB_TYPE_OUT_OF_BAND;
    o->st = GDB_SUBTYPE_ASYNC;
    if(!str.compare(0,7, "running"))
    {
        o->cl = GDB_CLASS_RUNNING;
        str.erase(0, 7);
    }
    else if(!str.compare(0, 7, "stopped"))
    {
        o->cl = GDB_CLASS_STOPPED;
        str.erase(0, 7);
        this->parseResults(o, str);
    }
    else
    {
        /* TODO Handle Error case */
    }
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
    this->parseResults(o, str);
}

void GDB::parseString(GDBResult *res, std::string &str)
{
    if(str[0] != '\"')
        return;
    else
    {
        std::regex r("[^\\\\]\"");
        std::smatch match;

        str.erase(0, 1);
        if(std::regex_search(str, match, r))
        {
            size_t endPos = match.position(0)+1;
            res->vt = GDB_VALUE_TYPE_CONST;
            res->cstr = str.substr(0, endPos);
            replaceAll(res->cstr, "\\\\\"", "\"");
            replaceAll(res->cstr, "(^|[^\\\\])\\\\n", "$1\n");
            replaceAll(res->cstr, "(^|[^\\\\])\\\\t", "$1\t");
            replaceAll(res->cstr, "\\\\\\\\", "\\");
            str.erase(0, endPos+1);
        }
    }
}

static
const char *reasonNames[]=
{
    "breakpoint-hit",
    "watchpoint-trigger",
    "read-watchpoint-trigger",
    "access-watchpoint-trigger",
    "watchpoint-scope",
    "function-finished",
    "location-reached",
    "end-stepping-range",
    "exited-signalled",
    "exited",
    "exited-normally",
    "signal-received"
};

static
GDB_STOP_REASON reasonValues[]=
{
    GDB_STOP_REASON_BP_HIT,
    GDB_STOP_REASON_WP_TRIGGER,
    GDB_STOP_REASON_READ_WP_TRIGGER,
    GDB_STOP_REASON_ACCESS_WP_TRIGGER,
    GDB_STOP_REASON_WP_SCOPE,
    GDB_STOP_REASON_FUNC_FINISHED,
    GDB_STOP_REASON_LOCATION_REACHED,
    GDB_STOP_REASON_END_STEPPING,
    GDB_STOP_REASON_EXITED_SIGNALLED,
    GDB_STOP_REASON_EXITED,
    GDB_STOP_REASON_SIGNAL_RECEIVED
};

static GDB_STOP_REASON getStopReasonFromString(const std::string &reason)
{
    for(int i = 0; i < sizeof(reasonNames)/sizeof(char*); i++)
    {
        if(reason == reasonNames[i])
            return reasonValues[i];
    }

    return GDB_STOP_REASON_UNKNOWN;
}

static GDBResult *findResult(GDBOutput *o, const std::string &name)
{
    for(std::vector<GDBResult *>::iterator it = o->rs.begin(); it != o->rs.end(); it++)
    {
        if((*it)->var == name)
            return *it;
    }

    return NULL;
}

GDBStopResult *GDB::getStopResult(GDBOutput *o)
{
    GDBStopResult *res = NULL;
    if(!o)
        return NULL;

    do
    {
        if(o->t == GDB_TYPE_OUT_OF_BAND && o->st == GDB_SUBTYPE_ASYNC
                && o->sst == GDB_SUBSUBTYPE_EXEC && o->cl == GDB_CLASS_STOPPED)
        {
            res = new GDBStopResult();
            if(res)
            {
                GDBResult *tmp;

                /* Reading Reason */
                tmp = findResult(o, "reason");
                res->reason = getStopReasonFromString(tmp->cstr);

                /* Reading bkptno */
                tmp = findResult(o, "bkptno");
                if(tmp)
                {
                    res->bkptNo = std::stoi(tmp->cstr);
                    res->hasBkptNo = true;
                }
                else
                {
                    res->hasBkptNo = false;
                }

                /* Reading Frame */
                tmp = findResult(o, "frame");
                if(tmp)
                {
                    res->frame = new GDBFrame();
                    res->frame->func = tmp->mp["func"]->cstr;
                    if(tmp->mp["file"])
                        res->frame->file = tmp->mp["file"]->cstr;
                    else
                        res->frame->file = "";
                    if(tmp->mp["fullname"])
                        res->frame->fullname = tmp->mp["fullname"]->cstr;
                    else
                        res->frame->fullname = "";
                    if(tmp->mp["line"])
                        res->frame->line = stoi(tmp->mp["line"]->cstr);
                    else
                        res->frame->line = -1;
                    res->frame->addr = (void *)stoul(tmp->mp["addr"]->cstr);
                }
                else
                {
                    res->frame = NULL;
                }
            }
            return res;
        }
        else
        {
            o = o->next;
        }
    } while(o);

    return NULL;
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

void GDB::freeResult(GDBResult *res)
{
    switch(res->vt)
    {
        case GDB_VALUE_TYPE_CONST:
            break;
        case GDB_VALUE_TYPE_LIST:
            for(std::vector<GDBResult*>::iterator it = res->vec.begin(); it != res->vec.end(); it++)
            {
                this->freeResult(*it);
            }
            break;
        case GDB_VALUE_TYPE_TUPLE:
            for(std::map<std::string, GDBResult*>::iterator it = res->mp.begin(); it != res->mp.end(); it++)
            {
                std::cout << "Freeing " << it->first << std::endl;
                if(it->second)
                    this->freeResult(it->second);
            }
            break;
    }

    delete(res);
}

void GDB::freeOutput(GDBOutput *o)
{
    if(o)
    {
        for(std::vector<GDBResult*>::iterator it = o->rs.begin(); it != o->rs.end(); ++it)
        {
            this->freeResult(*it);
        }
        this->freeOutput(o->next);
        delete o;
    }
}

void GDB::parseTuple(GDBResult *res, std::string &str)
{
    res->vt = GDB_VALUE_TYPE_TUPLE;
    if(str[0] == '{')
    {
        str.erase(0, 1);
        if(str[0] == '}')
        {
            str.erase(0, 1);
            return;
        }

        do
        {
            GDBResult *next_res = this->parseResult(str);

            res->mp[next_res->var] = next_res;
        } while(str[0] != '}' && str[0] != '\0');
        if(str[0] == '}')
            str.erase(0, 1);
    }
    else
    {
        /* TODO Handle error */
        return;
    }

}

void GDB::parseList(GDBResult *res, std::string &str)
{
    res->vt = GDB_VALUE_TYPE_LIST;
    if(str[0] == '[')
    {
        str.erase(0, 1);
        if(str[0] == ']')
        {
            str.erase(0, 1);
            return;
        }
        do
        {
            GDBResult *next_res = this->parseResult(str);

            res->vec.push_back(next_res);
        } while(str[0] != ']' && str[0] != '\0');
        if(str[0] == ']')
            str.erase(0, 1);
    }
    else
    {
        /* TODO Handle error */
        return;
    }

}

void indent(int level)
{
    for(int i = 0; i < level; i++)
    {
        std::cout << "    ";
    }
}
void dumpRes(GDBResult *res, int indentlevel = 0)
{
    switch(res->vt)
    {
        case GDB_VALUE_TYPE_CONST:
            indent(indentlevel);
            std::cout << res->var << " = " << res->cstr << std::endl;
            break;
        case GDB_VALUE_TYPE_LIST:
            indent(indentlevel);
            std::cout << res->var << " = [" << std::endl;
            indentlevel++;
            for(std::vector<GDBResult*>::iterator it = res->vec.begin(); it != res->vec.end(); it++)
            {
                dumpRes(*it, indentlevel);
            }
            indentlevel--;
            indent(indentlevel);
            std::cout << "]" << std::endl;
            break;
        case GDB_VALUE_TYPE_TUPLE:
            indent(indentlevel);
            std::cout << res->var << " = {" << std::endl;
            indentlevel++;
            for(std::map<std::string, GDBResult*>::iterator it = res->mp.begin(); it != res->mp.end(); it++)
            {
                dumpRes(it->second, indentlevel);
            }
            indentlevel--;
            indent(indentlevel);
            std::cout << "}" << std::endl;
            break;
    }
}

void GDB::parseResults(GDBOutput *o, std::string &str)
{
    GDBResult *res;
    do
    {
        res = this->parseResult(str);
        if(res)
            o->rs.push_back(res);
    } while(res);

    for(std::vector<GDBResult*>::iterator it = o->rs.begin(); it != o->rs.end(); it++)
        dumpRes(*it);

}

GDBResult *GDB::parseResult(std::string &str, GDBResult *pres)
{
    char first;
    GDBResult *res = NULL;
    if(str[0] != '\0')
    {
        size_t endPos;
        if(str[0] == ',')
            str.erase(0, 1);
        {
            if(pres)
                res = pres;
            else
                res = new GDBResult();
            if(res)
            {
                if(str[0] == '\"')
                {
                    this->parseString(res, str);
                }
                else if(str[0])
                {
                    first = str[0];
                    if(first == '[')
                    {
                        this->parseList(res, str);
                    }
                    else if(first == '{')
                    {
                        this->parseTuple(res, str);
                    }
                    else if(first == '\"')
                    {
                        this->parseString(res, str);
                    }
                    else
                    {
                        endPos = str.find('=');

                        if(endPos == std::string::npos)
                            return NULL;
                        res->var = str.substr(0, endPos);
                        str.erase(0, endPos+1 /* +1 to erase the = sign */ );
                        this->parseResult(str, res);
                    }
                }
            }
        }
    }
    else
    {
        return NULL;
    }
    return res;
}

