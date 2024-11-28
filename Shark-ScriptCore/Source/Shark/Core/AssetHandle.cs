
using System;
using System.Runtime.InteropServices;

namespace Shark
{
	[StructLayout(LayoutKind.Sequential)]
	public readonly struct AssetHandle
	{
		public static readonly AssetHandle Invalid = new AssetHandle(0);
		private readonly ulong m_Handle;

		public AssetHandle(ulong assetHandle) { m_Handle = assetHandle; }

		public readonly bool IsValid()
		{
			unsafe { return InternalCalls.AssetHandle_IsValid(this); }
		}

		public static implicit operator bool(AssetHandle assetHandle)
		{
			unsafe { return InternalCalls.AssetHandle_IsValid(assetHandle); }
		}

		public override string ToString() => m_Handle.ToString();
		public override int GetHashCode() => m_Handle.GetHashCode();
	}

}
