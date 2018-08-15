/*
 * Copyright 2007-2018 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */


/**
 * @file Log.java
 *
 * @brief Basic logging class.
 */

package gov.nasa.gsfc.gmsec.api.util;

import gov.nasa.gsfc.gmsec.api.jni.*;


/**
 * @class Log
 *
 * @brief Basic logging class. This class supports basic logging to any output
 *        stream. This class is thread-safe.
 */
public class Log
{
	/**
	 * @fn void setReportingLevel(LogLevelGMSEC level)
	 *
	 * @brief Sets maximum logging level.
	 *
	 * @param level - Possible values LogLevelGMSEC.NONE, LogLevelGMSEC.ERROR,
	 *                LogLevelGMSEC.SECURE, LogLevelGMSEC.WARNING, LogLevelGMSEC.INFO,
	 *                LogLevelGMSEC.VERBOSE, and LogLevelGMSEC.DEBUG.
	 */
	public static void setReportingLevel(LogLevelGMSEC level)
	{
		JNILog.setReportingLevel(level);
	}


	/**
	 * @fn LogLevelGMSEC getReportingLevel()
	 *
	 * @brief Returns the maximum logging level.
	 *
	 * @return Possible values LogLevelGMSEC.NONE, LogLevelGMSEC.ERROR,
	 *         LogLevelGMSEC.SECURE, LogLevelGMSEC.WARNING, LogLevelGMSEC.INFO,
	 *         LogLevelGMSEC.VERBOSE, and LogLevelGMSEC.DEBUG.
	 */
	public static LogLevelGMSEC getReportingLevel()
	{
		return JNILog.getReportingLevel();
	}


	/**
	 * @fn void registerHandler(LogHandler handler)
	 *
	 * @brief Registers a LogHandler that will be called for all logging levels.
	 *
	 * @param handler - An instance of a class which extends LogHandler, or null to
	 * restore the default log handler.
	 */
	public static void registerHandler(LogHandler handler)
	{
		JNILog.registerHandler(handler);
	}


	/**
	 * @fn registerHandler(LogLevelGMSEC level, LogHandler handler)
	 *
	 * @brief Registers a LogHandler that will be called for a specific
	 *        logging level.
	 *
	 * @param level - Possible values LogLevelGMSEC.NONE, LogLevelGMSEC.ERROR,
	 *                LogLevelGMSEC.SECURE, LogLevelGMSEC.WARNING, LogLevelGMSEC.INFO,
	 *                LogLevelGMSEC.VERBOSE, and LogLevelGMSEC.DEBUG.
	 *
	 * @param handler - An instance of a class which extends LogHandler, or null to
	 * restore the default log handler.
	 */
	public static void registerHandler(LogLevelGMSEC level, LogHandler handler)
	{
		JNILog.registerHandler(level, handler);
	}


	/**
	 * @fn String levelToString(LogLevelGMSEC level)
	 *
	 * @brief Converts the log level number to a string value.
	 *
	 * @param level - Possible values LogLevelGMSEC.NONE, LogLevelGMSEC.ERROR,
	 *                LogLevelGMSEC.SECURE, LogLevelGMSEC.WARNING, LogLevelGMSEC.INFO,
	 *                LogLevelGMSEC.VERBOSE, and LogLevelGMSEC.DEBUG
	 *
	 * @return String with possible contents of "NONE," "ERROR,"
	 *         "SECURE," "WARNING," "INFO," "VERBOSE," "DEBUG," or
	 *         "INVALID."
	 */
	public static String levelToString(LogLevelGMSEC level)
	{
		return JNILog.logLevelToString(level);
	}


	/**
	 * @fn LogLevelGMSEC levelFromString(String level)
	 *
	 * @brief Converts the string value to a log level number.
	 *
	 * @param level - String with possible contents of "NONE," "ERROR,"
	 *                "SECURE," "WARNING," "INFO," "VERBOSE," "DEBUG," or
	 *                "INVALID."
	 *
	 * @return Possible values LogLevelGMSEC.NONE, LogLevelGMSEC.ERROR,
	 *          LogLevelGMSEC.SECURE, LogLevelGMSEC.WARNING, LogLevelGMSEC.INFO,
	 *          LogLevelGMSEC.VERBOSE, and LogLevelGMSEC.DEBUG
	 */
	public static LogLevelGMSEC levelFromString(String level)
	{
		if (level == null)
		{
			return LogLevelGMSEC.NONE;
		}
		return JNILog.logLevelFromString(level);
	}


	/**
	 * @fn void error(String message)
	 *
	 * @brief Logs an error message.
	 *
	 * @param message - String to log.
	 */
	public static void error(String message)
	{
		if (message != null)
		{
			JNILog.logError(message);
		}
	}


	/**
	 * @fn void secure(String message)
	 *
	 * @brief Logs a secure message.
	 *
	 * @param message - String to log.
	 */
	public static void secure(String message)
	{
		if (message != null)
		{
			JNILog.logSecure(message);
		}
	}


	/**
	 * @fn void warning(String message)
	 *
	 * @brief Logs a warning message.
	 *
	 * @param message - String to log.
	 */
	public static void warning(String message)
	{
		if (message != null)
		{
			JNILog.logWarning(message);
		}
	}


	/**
	 * @fn void info(String message)
	 *
	 * @brief Logs an info message.
	 *
	 * @param message - String to log.
	 */
	public static void info(String message)
	{
		if (message != null)
		{
			JNILog.logInfo(message);
		}
	}


	/**
	 * @fn void verbose(String message)
	 *
	 * @brief Logs a verbose message.
	 *
	 * @param message - String to log.
	 */
	public static void verbose(String message)
	{
		if (message != null)
		{
			JNILog.logVerbose(message);
		}
	}


	/**
	 * @fn void debug(String message)
	 *
	 * @brief Logs a debug message.
	 *
	 * @param message - String to log.
	 */
	public static void debug(String message)
	{
		if (message != null)
		{
			JNILog.logDebug(message);
		}
	}
}
