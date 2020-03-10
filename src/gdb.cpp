#include "gdb.hpp"
#include "utils.hpp"

#include <sstream>
#include <ostream>
#include <iostream>
#include <string>
#include <regex>
static GDBResult *findResult(GDBOutput *o, const std::string &name)
{
    for(std::vector<GDBResult *>::iterator it = o->rs.begin(); it != o->rs.end(); it++)
    {
        if((*it)->var == name)
            return *it;
    }

    return NULL;
}

GDB::GDB():
    m_state(GDB_STATE_INIT),
    m_currentFile(""),
    m_currentSourceLine(-1)
{

}

void GDB::init()
{
    GDBResult * res = this->sendCommandAndWaitForResult("-info-gdb-mi-command complete\n", "command");
    this->m_completionSupported = (res->mp["exists"]->cstr == "true");
}

std::ostringstream *GDB::getConsoleStream()
{
    return &m_consoleStream;
}

GDBState GDB::getState()
{
    return this->m_state;
}

std::string GDB::getCurrentFilePath()
{
    return this->m_currentFile;
}

int GDB::getCurrentSourceLine()
{
    return this->m_currentSourceLine;
}

int GDB::getCurrentFrameLevel()
{
    return this->m_currentFrameLevel;   
}

void GDB::setCurrentThread(const std::string &threadId)
{
    this->m_currentThread = threadId;
}

std::string GDB::getCurrentThread()
{
    return this->m_currentThread;
}

void GDB::setCurrentFrameLevel(int frameLevel)
{
    if(frameLevel < this->m_stackFrame.size())
    {
        GDBResult *variablesResult;
        //std::string frameSelectCmd = "-stack-select-frame " + std::to_string(frameLevel) + "\n";
        std::string cmd = "-stack-list-variables --thread " + this->m_currentThread + " --frame " + std::to_string(frameLevel) + " --simple-values\n";
        this->m_currentFrameLevel = frameLevel;
        this->m_currentSourceLine = this->m_stackFrame[this->m_currentThread][frameLevel].line;
        this->m_currentFile = this->m_stackFrame[this->m_currentThread][frameLevel].fullname;

        for(GDBVariableObject &object : this->m_variableObjects)
        {
            this->deleteVarObj(object.name);
        }

        this->m_variableObjects.clear();

        {
            /* send list variables command */
            variablesResult = this->sendCommandAndWaitForResult(cmd, "variables");

            for(GDBResult *v : variablesResult->vec)
            {
                this->createVarObj(v->mp["name"]->cstr, this->m_variableObjects);
            }    
        }

        
    }
}

bool GDB::setExeFile(const std::string &filename)
{
    std::ostringstream cmd;
    std::string response;
    GDBOutput *rsp;

    cmd << "-file-exec-and-symbols \"" << filename << "\"\n";
    this->send(cmd.str());

    rsp = this->getResponse();

    delete rsp;

    return true;
}

bool GDB::sendCLI(const std::string &command)
{
    std::ostringstream cmd;
    GDBOutput *rsp;
    cmd << "-interpreter-exec console \"" << command << "\"\n";

    this->send(cmd.str());

    return this->checkResultDone();
}

GDBResult *GDB::sendCommandAndWaitForResult(std::string command, std::string resultName)
{
    bool found = false;
    GDBResult *result;
    this->send(command);
    do
    {
        GDBOutput * output = this->getResponseBlk();
        GDBOutput * foundResponse = this->getNextResultRecordWithData(output);

        while(foundResponse && !found)
        {
            if(foundResponse->rs[0]->var == resultName)
            {
                found = true;
                result = new GDBResult(*(foundResponse->rs[0]));
            }
            else
            {
                foundResponse = this->getNextResultRecordWithData(foundResponse->next);
            }
            
        }
        freeOutput(output);
    } while (!found);

    return result;
}

void GDB::computeFrameStack()
{
    GDBResult *threadsResult = this->sendCommandAndWaitForResult("-thread-info\n", "threads");

    this->m_stackFrame.clear();
    for(GDBResult *thread : threadsResult->vec)
    {
        std::string threadId = thread->mp["id"]->cstr;
        GDBResult *stackResult = this->sendCommandAndWaitForResult("-stack-list-frames --thread " + threadId + "\n", "stack");
        GDBResult *argsResult = this->sendCommandAndWaitForResult("-stack-list-arguments --thread " + threadId + " 2\n", "stack-args");;
    
        { 
            int index = 0;
            m_logger.logInfo("==== Stack Frame ====");
            for(GDBResult *frame : stackResult->vec)
            {
                GDBFrame frameStruct;
                std::string argsString = "";
                frameStruct.func = frame->mp["func"]->cstr;
                if(frame->mp.find("fullname") != frame->mp.end())
                {
                    frameStruct.fullname = frame->mp["fullname"]->cstr;
                }
                if(frame->mp.find("line") != frame->mp.end())
                {
                    frameStruct.line = stoi(frame->mp["line"]->cstr);
                }
                else
                {
                    frameStruct.line = -1;
                }
                
                frameStruct.level = stoi(frame->mp["level"]->cstr);
                for(GDBResult *arg : argsResult->vec[index]->mp["args"]->vec)
                {
                    argsString = arg->mp["type"]->cstr;
                    
                    if(argsString[argsString.size()-1] != '*')
                        argsString += " ";
                        
                    argsString += arg->mp["name"]->cstr;
                    frameStruct.args.push_back(argsString);
                }
                m_logger.logInfo(frame->mp["level"]->cstr + " : " + frame->mp["func"]->cstr);
                this->m_stackFrame[threadId].push_back(frameStruct);
                index++;
            }
            this->freeResult(stackResult);
            this->freeResult(argsResult);  
        }
    }
    this->freeResult(threadsResult);
}

void GDB::stopped(GDBStopResult *s)
{
    if(s)
    {
        if(s->reason == GDB_STOP_REASON_EXITED_NORMALLY
            || s->reason == GDB_STOP_REASON_EXITED
            || s->reason == GDB_STOP_REASON_EXITED_SIGNALLED)
        {
            this->m_state = GDB_STATE_INIT; 
        }
        else if(s->reason != GDB_STOP_REASON_UNKNOWN)
        {
            this->m_state = GDB_STATE_STOPPED;
            this->m_currentThread = s->threadId;  
        }
    }
}

void GDB::poll(void)
{
    GDBOutput *rsp = this->getResponse();
    if(rsp)
    {
        GDBStopResult *s = this->getStopResult(rsp);

        this->stopped(s);
        if(this->m_state == GDB_STATE_STOPPED)
        {
            this->computeFrameStack();
            this->setCurrentFrameLevel(0);
        }
        this->freeOutput(rsp);
    }
    else if(!this->gdbProcessRunning())
    {
        this->m_state = GDB_STATE_EXITED;
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
            m_logger.logInfo(output);
            o = this->parseOutput(output);
            if(o)
            {
                this->stopped(this->getStopResult(o));
                if(o->t == GDB_TYPE_OUT_OF_BAND && o->st == GDB_SUBTYPE_STREAM)
                {
                    add = false;
                    switch(o->sst)
                    {
                        case GDB_SUBSUBTYPE_CONSOLE:
                            Utils::replaceAll(o->rs[0]->cstr, "(^|[^\\\\])\\\\n", "$1\n");
                            Utils::replaceAll(o->rs[0]->cstr, "(^|[^\\\\])\\\\t", "$1\t");
                            Utils::replaceAll(o->rs[0]->cstr, "\\\\\"", "\"");
                            this->m_consoleStream << o->rs[0]->cstr;
                            break;
                    }
                }
                else if(o->t == GDB_TYPE_OUT_OF_BAND && o->st == GDB_SUBTYPE_ASYNC && (o->cl == GDB_CLASS_BP_ADDED || o->cl == GDB_CLASS_BP_MODIFIED))
                {
                    add = false;
                    this->addOrUpdateBreakpoint(o);
                }
                else if(o->t == GDB_TYPE_OUT_OF_BAND && o->st == GDB_SUBTYPE_ASYNC && o->cl == GDB_CLASS_BP_DELETED)
                {
                    GDBResult *res = findResult(o, "id");
                    add = false;
                    this->deleteBreakpoint(res->cstr);
                }
                else if(o->cl == GDB_CLASS_RUNNING)
                {
                    this->m_state = GDB_STATE_RUNNING;
                }
            }
            else
            {
                add = false;
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
    } while(!o && this->gdbProcessRunning());

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
        else if(first == '=')
        {
            this->parseNotifyAsyncRecord(o, str);
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

void GDB::parseNotifyAsyncRecord(GDBOutput *o, std::string &str)
{
    o->sst = GDB_SUBSUBTYPE_NOTIFY;
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
    else if(!str.compare(0, 18, "breakpoint-created"))
    {
        o->cl = GDB_CLASS_BP_ADDED;
        str.erase(0, 18);
        this->parseResults(o, str);
    }
    else if(!str.compare(0, 19, "breakpoint-modified"))
    {
        o->cl = GDB_CLASS_BP_MODIFIED;
        str.erase(0, 19);
        this->parseResults(o, str);
    }
    else if(!str.compare(0, 18, "breakpoint-deleted"))
    {
        o->cl = GDB_CLASS_BP_DELETED;
        str.erase(0, 18);
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
    this->parseResults(o, str);
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

void GDB::parseString(GDBResult *res, std::string &str, size_t &index)
{
    if(str[index] != '\"')
        return;
    else
    {
         index++;   
        {
            res->vt = GDB_VALUE_TYPE_CONST;

            while(index < str.size() && (str[index] != '"' || str[index-1] == '\\'))
            {
                res->cstr += str[index];
                index++;
            }
            //Utils::replaceAll(res->cstr, "\\\\\"", "\"");
            //Utils::replaceAll(res->cstr, "(^|[^\\\\])\\\\n", "$1\n");
            //Utils::replaceAll(res->cstr, "(^|[^\\\\])\\\\t", "$1\t");
            //Utils::replaceAll(res->cstr, "\\\\\\\\", "\\");
            index++;
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
    GDB_STOP_REASON_EXITED_NORMALLY,
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

                /* Read thread id */
                tmp = findResult(o, "thread-id");
                if(tmp)
                {
                    res->threadId = tmp->cstr;
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

GDBOutput *GDB::getNextResultRecordWithData(GDBOutput *o)
{
    bool found = false;
    while(o && !found)
    {
        if(o->rs.size() > 0)
        {
            found = true;
        }
        else
        {
            o = o->next;
        }
        
    }
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

void GDB::parseTuple(GDBResult *res, std::string &str, size_t &index)
{
    res->vt = GDB_VALUE_TYPE_TUPLE;
    if(str[index] == '{')
    {
        index++;
        if(str[index] == '}')
        {
            index++;
            return;
        }

        do
        {
            GDBResult *next_res = this->parseResult(str, NULL, index);

            res->mp[next_res->var] = next_res;
        } while(str[index] != '}' && str[index] != '\0');
        if(str[index] == '}')
            index++;
    }
    else
    {
        /* TODO Handle error */
        return;
    }

}

void GDB::parseList(GDBResult *res, std::string &str, size_t &index)
{
    res->vt = GDB_VALUE_TYPE_LIST;
    if(str[index] == '[')
    {
        index++;
        if(str[index] == ']')
        {
            index++;
            return;
        }
        do
        {
            GDBResult *next_res = this->parseResult(str, NULL, index);

            res->vec.push_back(next_res);
        } while(str[index] != ']' && str[index] != '\0');
        if(str[index] == ']')
            index++;
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
    size_t index = 0;
    do
    {
        res = this->parseResult(str, NULL, index);
        if(res)
            o->rs.push_back(res);
    } while(res);

    //for(std::vector<GDBResult*>::iterator it = o->rs.begin(); it != o->rs.end(); it++)
    //    dumpRes(*it);

}

GDBResult *GDB::parseResult(std::string &str, GDBResult *pres, size_t &index)
{
    char first;
    GDBResult *res = NULL;
    if(str[index] != '\0')
    {
        size_t endPos;
        if(str[index] == ',')
            index++;
        {
            if(pres)
                res = pres;
            else
                res = new GDBResult();
            if(res)
            {
                if(str[index] == '\"')
                {
                    this->parseString(res, str, index);
                }
                else if(str[index])
                {
                    first = str[index];
                    if(first == '[')
                    {
                        this->parseList(res, str, index);
                    }
                    else if(first == '{')
                    {
                        this->parseTuple(res, str, index);
                    }
                    else if(first == '\"')
                    {
                        this->parseString(res, str, index);
                    }
                    else
                    {
                        while(index < str.size() && str[index] != '=')
                        {
                           res->var += str[index];
                           index++; 
                        }

                        if(index == str.size())
                            return NULL;

                        index++;
                        this->parseResult(str, res, index);
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

void GDB::getNearestExecutableLine(const std::string &filename, std::string & line)
{
    std::ostringstream cmd;
    GDBResult *execLinesResult;
    bool found = false;
    int index = 0;
    cmd << "-symbol-list-lines ";
    cmd << Utils::basename(filename);
    cmd << "\n";
    execLinesResult = this->sendCommandAndWaitForResult(cmd.str(), "lines");

    {
        GDBResult dummy;
        GDBResult dummy2;
        dummy.mp["line"] = &dummy2;
        dummy.mp["line"]->cstr = line;

        std::stable_sort(execLinesResult->vec.begin(), execLinesResult->vec.end(), [](GDBResult *a, GDBResult *b){
            if(a->mp["line"]->cstr < b->mp["line"]->cstr)
            {
                return true;
            }
            //else if(a->mp["line"]->cstr == b->mp["line"]->cstr)
            //{
            //    return a->mp["pc"]->cstr < b->mp["pc"]->cstr;
            //}
            else
            {
                return false;
            }
        });
        auto lineIt = std::lower_bound(execLinesResult->vec.begin(), execLinesResult->vec.end(), &dummy, 
            [](GDBResult *a, GDBResult *b) {
                return a->mp["line"]->cstr < b->mp["line"]->cstr;
            });
        line = (*lineIt)->mp["line"]->cstr;
    }

    this->freeResult(execLinesResult);
}

void GDB::addOrUpdateBreakpoint(GDBOutput *o)
{
    GDBBreakpoint *bp;
    GDBResult *res = o->rs[0];
    bool add = true;
    bp = this->findBreakpoint(res->mp["number"]->cstr);

    if(!bp)
    {
        bp = new GDBBreakpoint();
    }
    else
    {
        add = false;
    }

    if(bp)
    {
        if(res->mp["fullname"])
        {
            bp->fullname = res->mp["fullname"]->cstr;
        }

        bp->enabled = res->mp["enabled"]->cstr == "y" ? true : false;
        

        bp->times = stoi(res->mp["times"]->cstr);

        if(add)
        {
            this->m_breakpoints.push_back(bp);

            bp->number = res->mp["number"]->cstr;

            if(res->mp["file"])
            {
                bp->filename = res->mp["file"]->cstr;
            }

            if(res->mp["line"])
            {
                bp->line = stoi(res->mp["line"]->cstr);
            }
        }

    }
}

void GDB::deleteBreakpoint(const std::string &bp)
{

    std::vector<GDBBreakpoint *>::iterator it = std::find_if(this->m_breakpoints.begin(), this->m_breakpoints.end(), [bp](const auto &v){ return v->number == bp;});

    if(it != this->m_breakpoints.end())
    {
        delete(*it);
        this->m_breakpoints.erase(it);
    }
}

const std::vector<GDBBreakpoint *> &GDB::getBreakpoints(void)
{
    return this->m_breakpoints;
}

std::vector<GDBVariableObject> &GDB::getVariableObjects(void)
{
    return this->m_variableObjects;
}

void GDB::retrieveVariableObjectChildren(GDBVariableObject &var)
{
    if(this->m_state == GDB_STATE_STOPPED)
    {
        std::string cmd = "-var-list-children --simple-values \"" + var.name + "\"\n";

        this->send(cmd);
        GDBOutput *childrenOutput = this->getResponseBlk();

        if(childrenOutput->cl == GDB_CLASS_DONE && childrenOutput->t == GDB_TYPE_RESULT_RECORD)
        {
            for(GDBResult *child : childrenOutput->rs[1]->vec)
            {
                GDBVariableObject childVar;

                childVar.name = child->mp["name"]->cstr;
                childVar.expression = child->mp["exp"]->cstr;
                childVar.has_children = child->mp["numchild"]->cstr != "0";
                if(child->mp.find("type") != child->mp.end())
                {
                    childVar.type = child->mp["type"]->cstr;

                }

                if(child->mp.find("value") != child->mp.end())
                {
                    childVar.value = child->mp["value"]->cstr;
                    Utils::replaceAll(childVar.value, "\\\\\"", "\"");
                    Utils::replaceAll(childVar.value, "\\\\\\\\", "\\");
                }
                var.children.push_back(childVar);

            }
        }
    }
}

std::vector<GDBBreakpoint *> GDB::getBreakpoints(std::string filename)
{
    std::vector<GDBBreakpoint *> fileBreakpoints;

    for(auto it = this->m_breakpoints.begin(); it != this->m_breakpoints.end(); it++)
    {
        if((*it)->fullname == filename)
        {
            fileBreakpoints.push_back(*it);
        }
    }

    return fileBreakpoints;
}

GDBBreakpoint *GDB::findBreakpoint(std::string bp)
{
    std::vector<GDBBreakpoint *>::iterator it = std::find_if(this->m_breakpoints.begin(), this->m_breakpoints.end(), [bp](const auto &v){ return v->number == bp;});

    if(it != this->m_breakpoints.end())
    {
        return *it;
    }
    else
    {
        return NULL;
    }

}

GDBBreakpoint *GDB::findBreakpoint(std::string file, int line)
{
    std::vector<GDBBreakpoint *>::iterator it = std::find_if(this->m_breakpoints.begin(), this->m_breakpoints.end(),
        [file, line](const auto &v){
            return (v->line == line && v->fullname == file);
        });

    if(it != this->m_breakpoints.end())
    {
        return *it;
    }
    else
    {
        return NULL;
    }

}

void GDB::setBreakpointState(std::string bp, bool state)
{
    std::ostringstream cmd;
    if(state)
    {
        cmd << "-break-enable " << bp;
    }
    else
    {
        cmd << "-break-disable " << bp;
    }

    cmd << "\n";

    this->send(cmd.str());

    if(this->checkResultDone())
    {
        GDBBreakpoint * b = this->findBreakpoint(bp);
        if(b)
        {
            b->enabled = state;
        }
    }
}

void GDB::breakFileLine(const std::string &filename, int line)
{
    if(this->m_state == GDB_STATE_STOPPED)
    {

        GDBBreakpoint *bp;
        std::string sline = std::to_string(line);
        this->getNearestExecutableLine(filename, sline);

        line = stoi(sline);

        if(bp = this->findBreakpoint(filename, line))
        {
            this->breakDelete(bp->number);
        }
        else
        {    
            GDBOutput *o;
            std::ostringstream cmd;

            cmd << "-break-insert " << Utils::basename(filename) << ":" << line << "\n";
            this->send(cmd.str());
            o = this->getResponseBlk();

            if(o && o->t == GDB_TYPE_RESULT_RECORD && o->cl == GDB_CLASS_DONE)
            {
                this->addOrUpdateBreakpoint(o);
                this->freeOutput(o);
            }
        }
    }
}

void GDB::breakDelete(const std::string &number)
{
    std::ostringstream cmd;

    cmd << "-break-delete " << number << "\n";

    this->send(cmd.str());

    if(this->checkResultDone())
    {
        this->deleteBreakpoint(number);
    }
}



std::map<int, bool> *GDB::getExecutableLines(const std::string &filename)
{
    std::map<int, bool> *mp = NULL;
    if(filename.length() > 0)
    {
        mp = new std::map<int, bool>;

        if(mp)
        {
            std::ostringstream cmd;
            GDBOutput *o;
            cmd << "-symbol-list-lines ";
            cmd << Utils::basename(filename);
            cmd << "\n";
            this->send(cmd.str());

            o = this->getResponseBlk();
            if(o && o->t == GDB_TYPE_RESULT_RECORD && o->cl == GDB_CLASS_DONE)
            {
                GDBResult *res = o->rs[0];
                for(auto it = res->vec.begin(); it != res->vec.end(); it++)
                {
                    if((*it)->mp["line"])
                    {
                        (*mp)[stoi((*it)->mp["line"]->cstr)] = true;
                    }
                }
            }
            this->freeOutput(o);
        }
    }

    return mp;
}

void GDB::deleteVarObj(const std::string &name)
{
    std::ostringstream cmdstream;
    cmdstream << "-var-delete " << name << "\n";

    this->send(cmdstream.str());
    this->checkResultDone();
}

void GDB::createVarObj(const std::string &expression, std::vector<GDBVariableObject> &objects)
{
    std::ostringstream cmdstream;
    GDBOutput *output;
    GDBVariableObject object;
    cmdstream << "-var-create - * \"" << expression << "\"\n";

    this->send(cmdstream.str());

    output = this->getResponseBlk();

    if(output->cl == GDB_CLASS_DONE && output->t == GDB_TYPE_RESULT_RECORD)
    {
        for(GDBResult *res : output->rs)
        {
            if(res->var == "name")
                object.name = res->cstr;
            else if(res->var == "type")
                object.type = res->cstr;
            else if(res->var == "value")
            {
                object.value = res->cstr;
                Utils::replaceAll(object.value, "\\\\\"", "\"");
                Utils::replaceAll(object.value, "\\\\\\\\", "\\");
                
            }
            else if(res->var == "numchild")
                object.has_children = (res->cstr != "0");
        }
        object.expression = expression;
        objects.push_back(object);
    }
    this->freeOutput(output);
}

void GDB::resume()
{
    if(m_state == GDB_STATE_STOPPED)
    {
        GDBOutput *rsp;

        this->send("-exec-continue\n");

        rsp = this->getResponseBlk();
        if(rsp->t == GDB_TYPE_RESULT_RECORD && rsp->cl == GDB_CLASS_ERROR)
        {
            this->m_state = GDB_STATE_INIT;
        }

        this->freeOutput(rsp);
    }
}

void GDB::next()
{
    if(m_state == GDB_STATE_STOPPED)
    {
        GDBOutput *rsp;

        this->send("-exec-next\n");

        rsp = this->getResponseBlk();
        if(rsp->t == GDB_TYPE_RESULT_RECORD && rsp->cl == GDB_CLASS_ERROR)
        {
            this->m_state = GDB_STATE_INIT;
        }

        this->freeOutput(rsp);
    }

}

void GDB::step()
{
    if(m_state == GDB_STATE_STOPPED)
    {
        GDBOutput *rsp;

        this->send("-exec-step\n");

           rsp = this->getResponseBlk();
        if(rsp->t == GDB_TYPE_RESULT_RECORD && rsp->cl == GDB_CLASS_ERROR)
        {
            this->m_state = GDB_STATE_INIT;
        }

        this->freeOutput(rsp);
    }
}

void GDB::finish()
{
    if(m_state == GDB_STATE_STOPPED)
    {
        GDBOutput *rsp;

        this->send("-exec-finish\n");

           rsp = this->getResponseBlk();
        if(rsp->t == GDB_TYPE_RESULT_RECORD && rsp->cl == GDB_CLASS_ERROR)
        {
            this->m_state = GDB_STATE_INIT;
        }

        this->freeOutput(rsp);
    }
}

void GDB::run()
{
    if(m_state == GDB_STATE_STOPPED)
    {
        GDBOutput *rsp;

        this->send("-exec-run\n");

        rsp = this->getResponseBlk();
        if(rsp->t == GDB_TYPE_RESULT_RECORD && rsp->cl == GDB_CLASS_ERROR)
        {
            this->m_state = GDB_STATE_INIT;
        }

        this->freeOutput(rsp);
    }
}

const std::map<std::string, std::vector<GDBFrame>> &GDB::getFrameStack()
{
    return this->m_stackFrame;
}

void GDB::interrupt()
{
    this->send("-exec-interrupt\n");
}

std::vector<std::string> GDB::complete(std::string command)
{
    std::vector<std::string> completionList;
    if(this-m_completionSupported)
    {
        bool found = false;
        int index = 0;
        std::string cmd = "-complete \"" + command + "\"\n";
        GDBOutput *o;
        GDBOutput *res;
        this->send(cmd);
        o = this->getResponseBlk();
        res = this->getNextResultRecordWithData(o);

    
        if(res->rs[0]->var == "completion")
        {
            completionList.push_back(res->rs[0]->cstr);
        }
        else
        {
            completionList.push_back(command);
        }
        
        while(!found)
        {
            if(res->rs[index]->var == "matches")
            {
                found = true;
                for(GDBResult *str : res->rs[index]->vec)
                {
                    completionList.push_back(str->cstr);
                }
            }
            else
            {
                index++;
            }   
        }

        this->freeOutput(o);
    }
    return completionList;
}

void GDB::setVerbose()
{
    m_logger.setLogLevel(Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);
}