/*
 * Copyright 2007-2018 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */



/** @file log.cpp
 *
 *  @brief This file contains function for GMSEC API C logging framework
 */


#include <gmsec4/c/util/log.h>

#undef LOG_LEVEL_DEFINE
#undef GMSEC_ERROR
#undef GMSEC_SECURE
#undef GMSEC_WARNING
#undef GMSEC_INFO
#undef GMSEC_VERBOSE
#undef GMSEC_DEBUG

#include <gmsec4/util/Log.h>

#include <gmsec4/internal/StringUtil.h>

#ifdef __SUNPRO_CC
#include <sys/varargs.h>
#endif

using namespace gmsec::api::util;


void CALL_TYPE logSetReportingLevel(GMSEC_LogLevelGMSEC level)
{
	Log::setReportingLevel(level);
}


GMSEC_LogLevelGMSEC CALL_TYPE logGetReportingLevel()
{
	return Log::getReportingLevel();
}


GMSEC_BOOL CALL_TYPE logIsLoggingEnabled(GMSEC_LogLevelGMSEC level)
{
	return (level <= logGetReportingLevel() ? GMSEC_TRUE : GMSEC_FALSE);
}


void CALL_TYPE logRegisterHandler(GMSEC_LogHandler* handler)
{
	Log::registerHandler(handler);
}


void CALL_TYPE logRegisterHandlerByLevel(GMSEC_LogLevelGMSEC level, GMSEC_LogHandler* handler)
{
	Log::registerHandler(level, handler);
}


const char* CALL_TYPE LogLevelGMSECToString(GMSEC_LogLevelGMSEC level)
{
	return Log::toString(level);
}


GMSEC_LogLevelGMSEC CALL_TYPE LogLevelGMSECFromString(const char* level)
{
	return Log::fromString(level);
}


void CALL_TYPE logPrint(const char* file, int line, GMSEC_LogLevelGMSEC level, const char* fmt, ...)
{
	char buffer[4092];

	va_list args;
	va_start(args, fmt);
	StringUtil::stringFormatV(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	LoggerStream(file, line).get(level) << buffer;
}
