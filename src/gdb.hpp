#ifndef __GDB_HPP__
#define __GDB_HPP__

#include <string>
#include <ostream>

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
    struct GDBResult *rs;
} GDBResult;

typedef struct GDBOutput
{
    GDB_MESSAGE_TYPE t;
    GDB_MESSAGE_SUBTYPE st;
    GDB_MESSAGE_SUBSUBTYPE sst;
    GDB_MESSAGE_CLASS cl;

    GDBResult rs;

    struct GDBOutput *next;
} GDBOutput;

class GDB
{
    private:
        std::ostream &m_consoleStream;
    public:
        GDB(std::ostream &consoleStream);

        bool setExeFile(const std::string &filename);
        GDBOutput *getResponse();
        GDBOutput *getResponseBlk();
        bool sendCLI(const std::string &command);
        void poll(void);

    protected:
        virtual bool send(const std::string &message) = 0;
        virtual bool readline(std::string &message) = 0;
    private:
        GDBOutput *parseOutput(std::string &str);
        void parseResultRecord(GDBOutput *o, std::string &str);
        void parseConsoleStreamOutput(GDBOutput *o, std::string &str);
        void parseConsoleOutput(GDBOutput *o, std::string &str);
        void parseString(GDBOutput *o, std::string &str);
        GDBOutput *getResultRecord(GDBOutput *o);
        bool checkResult(GDB_MESSAGE_CLASS c);
        void freeOutput(GDBOutput *o);
        bool checkResultDone();
};

#endif
