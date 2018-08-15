/*
 * Copyright 2007-2018 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */


package gov.nasa.gsfc.gmsec.api.jni;

import gov.nasa.gsfc.gmsec.api.util.Log;
import gov.nasa.gsfc.gmsec.api.util.LogEntry;
import gov.nasa.gsfc.gmsec.api.util.LogLevelGMSEC;
import gov.nasa.gsfc.gmsec.api.util.LogHandler;
import gov.nasa.gsfc.gmsec.api.util.TimeUtil;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.Writer;
import java.io.IOException;

import java.util.HashMap;
import java.util.Map;


public class JNILog
{
	private static Map<LogLevelGMSEC, LogHandler> logHandlers = new HashMap<LogLevelGMSEC, LogHandler>();


	public static class DefaultLogHandler extends LogHandler
	{
		private Writer writer = null;

		public DefaultLogHandler()
		{
			writer = new PrintWriter(System.err);
		}

		public void onMessage(LogEntry entry)
		{
			StringBuilder sb = new StringBuilder();

			sb.append(TimeUtil.formatTime(entry.time)).append(" [").append(entry.level.toString()).append("] ");
			sb.append("[").append(entry.fileName).append(":").append(entry.lineNumber).append("] ");
			sb.append(entry.message).append("\n");

			String output = sb.toString().replace("\n", "\n\t");

			try
			{
				writer.write(output + "\n");
				writer.flush();
			}
			catch (IOException e)
			{
				System.err.println("*** Error writing log ***\n");
				System.err.println(output);
			}
		}

		public void setStream(String stream)
		{
			if (stream.equalsIgnoreCase("stderr"))
			{
				if (writer instanceof FileWriter)
				{
					try{ writer.close(); } catch (IOException e) { System.err.println("ERROR: Unable to close file."); }
				}

				writer = new PrintWriter(System.err);
			}
			else if (stream.equalsIgnoreCase("stdout"))
			{
				if (writer instanceof FileWriter)
				{
					try{ writer.close(); } catch (IOException e) { System.err.println("ERROR: Unable to close file."); }
				}

				writer = new PrintWriter(System.out);
			}
			else
			{
				if (writer instanceof FileWriter)
				{
					try{ writer.close(); } catch (IOException e) { System.err.println("ERROR: Unable to close file."); }

					writer = new PrintWriter(System.err);
				}

				try
				{
					Writer tmp = new FileWriter(stream, true);

					writer = tmp;
				}
				catch (IOException e)
				{
					System.err.println("ERROR: Unable to open and/or append to log file\n");
				}
			}
		}
	}


	private static final DefaultLogHandler defaultLogHandler = new DefaultLogHandler();

	static
	{
		for (int i = 0; i < 7; ++i)
		{
			Log.registerHandler(LogLevelGMSEC.getUsingValue(i), defaultLogHandler);
		}
	}


	private static void storeLogHandler(LogLevelGMSEC level, LogHandler handler)
	{
		logHandlers.put(level, handler);
	}


	public static DefaultLogHandler getDefaultLogHandler()
	{
		return defaultLogHandler;
	}


	public static void setReportingLevel(LogLevelGMSEC level)
	{
		gmsecJNI.Log_SetReportingLevel(level.getValue());
	}


	public static LogLevelGMSEC getReportingLevel()
	{
		int level = gmsecJNI.Log_GetReportingLevel();

		return LogLevelGMSEC.values()[level];
	}


	public static void registerHandler(LogHandler handler)
	{
		if (handler == null)
		{
			handler = getDefaultLogHandler();
		}

		long handlerPtr = JNILogHandler.getCPtr(LogHandler.getInternal(handler));

		gmsecJNI.Log_RegisterHandler(handlerPtr);

		// We need to save a reference to the log handler (or null if such is the case)
		// for all log levels.  This will assist in preventing the JVM garbage collector
		// from disposing of anonymous log handler objects.
		for (int l = LogLevelGMSEC.ERROR.getValue() ; l <= LogLevelGMSEC.DEBUG.getValue(); ++l)
		{
			storeLogHandler(LogLevelGMSEC.getUsingValue(l), handler);
		}
	}


	public static void registerHandler(LogLevelGMSEC level, LogHandler handler)
	{
		if (handler == null)
		{
			handler = getDefaultLogHandler();
		}

		long handlerPtr = JNILogHandler.getCPtr(LogHandler.getInternal(handler));

		gmsecJNI.Log_RegisterHandler(level.getValue(), handlerPtr);

		// We need to save a reference to the log handler (or null if such is the case)
		// for the given log level.  This will assist in preventing the JVM garbage collector
		// from disposing of anonymous log handler objects.
		storeLogHandler(level, handler);
	}


	public static String logLevelToString(LogLevelGMSEC level)
	{
		return gmsecJNI.Log_LogLevelToString(level.getValue());
	}


	public static LogLevelGMSEC logLevelFromString(String level)
	{
		int nivel = gmsecJNI.Log_LogLevelFromString(level);

		return LogLevelGMSEC.values()[nivel];
	}


	public static void logError(String message)
	{
		doLog(LogLevelGMSEC.ERROR, message);
	}


	public static void logSecure(String message)
	{
		doLog(LogLevelGMSEC.SECURE, message);
	}


	public static void logWarning(String message)
	{
		doLog(LogLevelGMSEC.WARNING, message);
	}


	public static void logInfo(String message)
	{
		doLog(LogLevelGMSEC.INFO, message);
	}


	public static void logVerbose(String message)
	{
		doLog(LogLevelGMSEC.VERBOSE, message);
	}


	public static void logDebug(String message)
	{
		doLog(LogLevelGMSEC.DEBUG, message);
	}


	private static void doLog(LogLevelGMSEC level, String message)
	{
		if (Log.getReportingLevel().getValue() >= level.getValue())
		{
			LogHandler handler = logHandlers.get(level);

			if (handler != null)
			{
				handler.onMessage(generateEntry(level, message));
			}
		}
	}


	private static LogEntry generateEntry(LogLevelGMSEC level, String message)
	{
		LogEntry entry   = new LogEntry();
		entry.time       = TimeUtil.getCurrentTime();
		entry.level      = level;
		entry.fileName   = Thread.currentThread().getStackTrace()[5].getFileName();
		entry.lineNumber = Thread.currentThread().getStackTrace()[5].getLineNumber();
		entry.message    = message;
		return entry;
	}
}
