#ifndef __GDB_HPP__
#define __GDB_HPP__

#include <string>
#include <ostream>
#include <vector>
#include <map>

typedef enum
{
    GDB_TYPE_OUT_OF_BAND = 0,
    GDB_TYPE_RESULT_RECORD,
} GDB_MESSAGE_TYPE;

typedef enum
{
    GDB_SUBTYPE_ASYNC = 0,
    GDB_SUBTYPE_STREAM
} GDB_MESSAGE_SUBTYPE;

typedef enum
{
    GDB_SUBSUBTYPE_EXEC = 0,
    GDB_SUBSUBTYPE_STATUS,
    GDB_SUBSUBTYPE_NOTIFY,
    GDB_SUBSUBTYPE_CONSOLE,
    GDB_SUBSUBTYPE_TARGET,
    GDB_SUBSUBTYPE_LOG
} GDB_MESSAGE_SUBSUBTYPE;

typedef enum
{
    GDB_CLASS_UNKNOWN = 0,
    GDB_CLASS_STOPPED,
    GDB_CLASS_DOWNLOAD,
    GDB_CLASS_DONE,
    GDB_CLASS_RUNNING,
    GDB_CLASS_CONNECTED,
    GDB_CLASS_ERROR,
    GDB_CLASS_EXIT
} GDB_MESSAGE_CLASS;

typedef enum
{
    GDB_VALUE_TYPE_CONST = 0,
    GDB_VALUE_TYPE_TUPLE,
    GDB_VALUE_TYPE_LIST
} GDB_VALUE_TYPE;

typedef struct GDBResult
{
    std::string var;
    GDB_VALUE_TYPE vt;

    std::string cstr;
    std::vector<struct GDBResult *> vec;
    std::map<std::string, struct GDBResult *> mp;
    struct GDBResult *next;

    bool operator==(const std::string &str) const
    {
        return str == (this->var);
    }

} GDBResult;

typedef struct GDBOutput
{
    GDB_MESSAGE_TYPE t;
    GDB_MESSAGE_SUBTYPE st;
    GDB_MESSAGE_SUBSUBTYPE sst;
    GDB_MESSAGE_CLASS cl;

    std::vector<GDBResult *> rs;

    struct GDBOutput *next;
} GDBOutput;

typedef enum
{
    GDB_STOP_REASON_UNKNOWN = 0,
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
} GDB_STOP_REASON;

typedef struct
{
    int level;
    void *addr;
    std::string func;
    std::string file;
    std::string fullname;
    int line;
    GDBResult *args;

} GDBFrame;

typedef struct GDBStopResult
{
    GDB_STOP_REASON reason;
    bool hasThreadId;
    bool hasBkptNo;
    bool hasExitCode;
    bool hasWpNo;

    int threadId;
    GDBFrame *frame;

    int bkptNo;

    /* TODO handle rest of stop reasons */

} GDBStopResult;


typedef enum
{
    GDB_STATE_RUNNING = 0,
    GDB_STATE_STOPPED
} GDBState;

class GDB
{
    private:
        std::ostream &m_consoleStream;
        GDBState m_state;
        std::string m_currentFile;
        int m_currentSourceLine;
    public:
        GDB(std::ostream &consoleStream);

        bool setExeFile(const std::string &filename);
        GDBOutput *getResponse();
        GDBOutput *getResponseBlk();
        bool sendCLI(const std::string &command);
        void poll(void);
        std::string getCurrentFilePath();
        int getCurrentSourceLine();

    protected:
        virtual bool send(const std::string &message) = 0;
        virtual bool readline(std::string &message) = 0;
    private:
        GDBOutput *parseOutput(std::string &str);
        void parseResultRecord(GDBOutput *o, std::string &str);
        void parseExecAsyncRecord(GDBOutput *o, std::string &str);
        void parseAsyncRecord(GDBOutput *o, std::string &str);
        void parseConsoleStreamOutput(GDBOutput *o, std::string &str);
        void parseConsoleOutput(GDBOutput *o, std::string &str);
        void parseString(GDBResult *o, std::string &str);
        void parseTuple(GDBResult *o, std::string &str);
        void parseList(GDBResult *o, std::string &str);
        GDBOutput *getResultRecord(GDBOutput *o);
        GDBStopResult *getStopResult(GDBOutput *o);
        bool checkResult(GDB_MESSAGE_CLASS c);
        void freeOutput(GDBOutput *o);
        void freeResult(GDBResult *res);
        bool checkResultDone();
        void parseResults(GDBOutput *o, std::string &str);
        GDBResult *parseResult(std::string &str, GDBResult *pres = NULL);
};

#endif
