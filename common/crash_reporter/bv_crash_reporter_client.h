#ifndef BISON_COMMON_CRASH_REPORTER_BV_CRASH_REPORTER_CLIENT_H_
#define BISON_COMMON_CRASH_REPORTER_BV_CRASH_REPORTER_CLIENT_H_



#include <string>

namespace bison {

void EnableCrashReporter(const std::string& process_type);

bool CrashReporterEnabled();

}  // namespace bison

#endif  // BISON_COMMON_CRASH_REPORTER_BV_CRASH_REPORTER_CLIENT_H_
