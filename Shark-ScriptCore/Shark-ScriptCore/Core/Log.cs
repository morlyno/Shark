
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
		{
			InternalCalls.Log_LogMessage(level, msg);
		}

		public static void Trace(string msg)
		{
			LogLevel(Level.Trace, msg);
		}
		public static void Trace(object obj)
		{
			Trace(string.Format("{0}", obj));
		}
		public static void Trace(string fmt, params object[] args)
		{
			Trace(string.Format(fmt, args));
		}


		public static void Info(string msg)
		{
			LogLevel(Level.Info, msg);
		}
		public static void Info(object obj)
		{
			Info(string.Format("{0}", obj));
		}
		public static void Info(string fmt, params object[] args)
		{
			Info(string.Format(fmt, args));
		}


		public static void Warn(string msg)
		{
			LogLevel(Level.Warn, msg);
		}
		public static void Warn(object obj)
		{
			Warn(string.Format("{0}", obj));
		}
		public static void Warn(string fmt, params object[] args)
		{
			Warn(string.Format(fmt, args));
		}


		public static void Error(string msg)
		{
			LogLevel(Level.Error, msg);
		}
		public static void Error(object obj)
		{
			Error(string.Format("{0}", obj));
		}
		public static void Error(string fmt, params object[] args)
		{
			Error(string.Format(fmt, args));
		}


		public static void Critical(string msg)
		{
			LogLevel(Level.Critical, msg);
		}
		public static void Critical(object obj)
		{
			Critical(string.Format("{0}", obj));
		}
		public static void Critical(string fmt, params object[] args)
		{
			Critical(string.Format(fmt, args));
		}


		public static void Debug(string msg)
		{
			LogLevel(Level.Debug, msg);
		}
		public static void Debug(object obj)
		{
			Debug(string.Format("{0}", obj));
		}
		public static void Debug(string fmt, params object[] args)
		{
			Debug(string.Format(fmt, args));
		}


		//public static void Info(string format, object arg0)
		//{
		//	Info(string.Format(format, arg0));
		//}
		//public static void Info(string format, object arg0, object arg1)
		//{
		//	Info(string.Format(format, arg0, arg1));
		//}
		//public static void Info(string format, object arg0, object arg1, object arg2)
		//{
		//	Info(string.Format(format, arg0, arg1, arg2));
		//}
		//public static void Info(string format, params object[] args)
		//{
		//	Info(string.Format(format, args));
		//}
		//
		//public static void Warn(string format, object arg0)
		//{
		//	Warn(string.Format(format, arg0));
		//}
		//public static void Warn(string format, object arg0, object arg1)
		//{
		//	Warn(string.Format(format, arg0, arg1));
		//}
		//public static void Warn(string format, object arg0, object arg1, object arg2)
		//{
		//	Warn(string.Format(format, arg0, arg1, arg2));
		//}
		//public static void Warn(string format, params object[] args)
		//{
		//	Warn(string.Format(format, args));
		//}
		//
		//public static void Error(string format, object arg0)
		//{
		//	Error(string.Format(format, arg0));
		//}
		//public static void Error(string format, object arg0, object arg1)
		//{
		//	Error(string.Format(format, arg0, arg1));
		//}
		//public static void Error(string format, object arg0, object arg1, object arg2)
		//{
		//	Error(string.Format(format, arg0, arg1, arg2));
		//}
		//public static void Error(string format, params object[] args)
		//{
		//	Error(string.Format(format, args));
		//}
	}

}
