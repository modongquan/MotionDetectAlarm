#ifndef CSVPARSE_H
#define CSVPARSE_H

#include "csv.h"
#include "sinks/basic_file_sink.h"
#include <vector>
#include <map>

class CsvParse
{
public:
    CsvParse();
    ~CsvParse();

    void SetPtrLogger(std::shared_ptr<spdlog::logger> logger);
    bool ParseCsvData(const char * ptrCsvPath);

    void GetDbIp(std::string &DbIp);
    void GetDbPort(std::string &DbPort);
    void GetDbUserName(std::string &DbUserName);
    void GetDbPasswd(std::string &DbPasswd);
    void GetDbName(std::string &DbName);
    void GetDbTable(std::string &DbTable);

private:

    std::string strDbIp, strDbPort, strDbUserName, strDbPasswd, strDbName, strDbTable;
    std::shared_ptr<spdlog::logger> ptrLogger;
};

#endif // CSVPARSE_H
