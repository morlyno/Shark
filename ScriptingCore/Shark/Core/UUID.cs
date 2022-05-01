
using System;
using System.Runtime.InteropServices;

namespace Shark
{
	[StructLayout(LayoutKind.Sequential)]
	public struct UUID : IFormattable
	{
		private ulong m_UUID;

		public static UUID Generate => InternalCalls.UUID_Generate();
		public static UUID Null => new UUID(0);

		public UUID(ulong uuid)
		{
			m_UUID = uuid;
		}

		public bool IsValid()
		{
			return m_UUID != 0;
		}

		public string ToString(string format, IFormatProvider formatProvider)
		{
			return string.Format("0x{0:x}", m_UUID);
		}

	}

}
