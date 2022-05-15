
using System;
using System.Runtime.InteropServices;

namespace Shark
{
	[StructLayout(LayoutKind.Sequential)]
	public struct AssetHandle : IFormattable
	{
		private ulong m_AssetHandle;

		public static AssetHandle Null => new AssetHandle(0);

		public AssetHandle(ulong assetHandle)
		{
			m_AssetHandle = assetHandle;
		}

		public bool IsValid()
		{
			return m_AssetHandle != 0;
		}

		public override string ToString()
		{
			return ToString(null, null);
		}
		public string ToString(string format)
		{
			return ToString(format, null);
		}
		public string ToString(string format, IFormatProvider formatProvider)
		{
			return string.Format("0x{0:x}", m_AssetHandle.ToString(format, formatProvider));
		}

	}

}
