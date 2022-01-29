#ifndef __RAYMON_SHAN_LOG_H
#define __RAYMON_SHAN_LOG_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>


#include "../include/raymoncommon.hpp"

#define __LOG(format, args...) fprintf(stdout, format, ##args)

// #define MAX_VALID_LOG_LENGTH   1024
#define MAX_TIME_BUFFER_LENGTH 32

#define LEVEL_LOG_NONE         0
#define LEVEL_LOG_CRIT         1
#define LEVEL_LOG_EROR         2
#define LEVEL_LOG_WARN         3
#define LEVEL_LOG_INFO         4
#define LEVEL_LOG_DBUG         5
#define LEVEL_LOG_TRAC         6

typedef class LogParser {
public:

  static thread_local char timebuffer[MAX_TIME_BUFFER_LENGTH];
  static thread_local unsigned int threadid;
  static int loghandle;
  static int loglevel;

public:
  static char* Initialize(int level, char *logname) {
    char logfilename[MAX_VALID_COMMAND_LENGTH];
    struct stat logstat;

    loglevel = level;
    if (logname) {
      if (logname[strlen(logname) - 1] == '/' && stat(logname, &logstat) == 0 && (logstat.st_mode & S_IFDIR)) {
	GetFormat();
	snprintf(logfilename, MAX_VALID_COMMAND_LENGTH, "%s%s%s", logname, "pdfservice", timebuffer);
	loghandle = open(logfilename, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
      } else {
	return (char*)"logpath invalid";	
      }
      if (loghandle == -1) {
	return (char*)"log file open fail";
      }
    } else {
      loghandle = 0;
    }
    threadid = syscall(SYS_gettid);
    return NULL;
  }
  static bool IsLog(int level) {
    return level <= loglevel;
  }
  static int GetLogHandle() {
    return loghandle;
  }
  static char* GetTimebuffer() {
    return timebuffer;
  }
  static unsigned int GetThreadId() {
    return syscall(SYS_gettid);
  }
  static void GetFormat() {
    time_t time_struct;
    time(&time_struct);
    struct tm *gmt = localtime(&time_struct);
    strftime(timebuffer, 32, "%Y-%m-%d-%T", gmt);
  }

} LogParser, *pLogParser;



#define filename(x) strrchr(x,'/')?strrchr(x,'/')+1:x

#define FORMAT "[%s][%8d][%s][%16s:%-3d] "
#define CREND "\n"
#define ARGVS(LEVEL) LogParser::GetTimebuffer(), LogParser::GetThreadId(), LEVEL, filename(__FILE__), __LINE__

#define _LOG_OUT(LEVEL, format, args...) LogParser::GetFormat(); \
  if (LogParser::GetLogHandle() > 0) dprintf(LogParser::GetLogHandle(), FORMAT format CREND, ARGVS(LEVEL), ##args); \
  else printf(FORMAT format CREND, ARGVS(LEVEL), ##args);

#define _LOG_BOTH(LEVEL, format, args...) LogParser::GetFormat(); \
  if (LogParser::GetLogHandle() > 0) dprintf(LogParser::GetLogHandle(), FORMAT format CREND, ARGVS(LEVEL), ##args); \
  printf(FORMAT format CREND, ARGVS(LEVEL), ##args);

#define _LOG_CRIT(format, args...) if (LogParser::IsLog(LEVEL_LOG_CRIT)) { _LOG_OUT("CRIT", format, ##args) };
#define _LOG_EROR(format, args...) if (LogParser::IsLog(LEVEL_LOG_EROR)) { _LOG_OUT("EROR", format, ##args) };
#define _LOG_WARN(format, args...) if (LogParser::IsLog(LEVEL_LOG_WARN)) { _LOG_OUT("WARN", format, ##args) };
#define _LOG_INFO(format, args...) if (LogParser::IsLog(LEVEL_LOG_INFO)) { _LOG_OUT("INFO", format, ##args) };
#define _LOG_DBUG(format, args...) if (LogParser::IsLog(LEVEL_LOG_DBUG)) { _LOG_OUT("DBUG", format, ##args) };
#define _LOG_TRAC(format, args...) if (LogParser::IsLog(LEVEL_LOG_TRAC)) { _LOG_OUT("TRAC", format, ##args) };
#define _LOG_ATTE(format, args...) { _LOG_BOTH("CRIT", format, ##args) };

char thread_local LogParser::timebuffer[MAX_TIME_BUFFER_LENGTH];
unsigned int thread_local LogParser::threadid;
int LogParser::loghandle;
int LogParser::loglevel;

#endif  // __RAYMON_SHAN_LOG_H

