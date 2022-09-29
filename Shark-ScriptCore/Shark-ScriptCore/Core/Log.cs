
namespace Shark
{

	public static class Log
	{
		public enum Level : ushort
		{
			Trace,
			Info,
			Warn,
			Error,
			Critical,
			Debug
		}

		public static void LogLevel(Level level, string msg)
			=> InternalCalls.Log_LogMessage(level, msg);

		public static void Trace(string msg)
			=> LogLevel(Level.Trace, msg);

		public static void Trace(object obj)
			=> LogLevel(Level.Trace, string.Format("{0}", obj));

		public static void Trace(string fmt, params object[] args)
			=> LogLevel(Level.Trace, string.Format(fmt, args));


		public static void Info(string msg)
			=> LogLevel(Level.Info, msg);

		public static void Info(object obj)
			=> LogLevel(Level.Info, string.Format("{0}", obj));

		public static void Info(string fmt, params object[] args)
			=> LogLevel(Level.Info, string.Format(fmt, args));


		public static void Warn(string msg)
			=> LogLevel(Level.Warn, msg);

		public static void Warn(object obj)
			=> LogLevel(Level.Warn, string.Format("{0}", obj));

		public static void Warn(string fmt, params object[] args)
			=> LogLevel(Level.Warn, string.Format(fmt, args));


		public static void Error(string msg)
			=> LogLevel(Level.Error, msg);

		public static void Error(object obj)
			=> LogLevel(Level.Error, string.Format("{0}", obj));

		public static void Error(string fmt, params object[] args)
			=> LogLevel(Level.Error, string.Format(fmt, args));


		public static void Critical(string msg)
			=> LogLevel(Level.Critical, msg);

		public static void Critical(object obj)
			=> LogLevel(Level.Critical, string.Format("{0}", obj));

		public static void Critical(string fmt, params object[] args)
			=> LogLevel(Level.Critical, string.Format(fmt, args));


		public static void Debug(string msg)
			=> LogLevel(Level.Debug, msg);

		public static void Debug(object obj)
			=> LogLevel(Level.Debug, string.Format("{0}", obj));

		public static void Debug(string fmt, params object[] args)
			=> LogLevel(Level.Debug, string.Format(fmt, args));

	}

}
