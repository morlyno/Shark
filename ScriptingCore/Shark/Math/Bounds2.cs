
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Bounds2i
	{
		public Vector2i LowerBound;
		public Vector2i UpperBound;
	}

}
