
namespace Shark
{
	public static class Physics2D
	{
		public static Vector2 Gravity
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.Physics2D_GetGravity(&result); return result; }
			}
			set
			{
				unsafe { InternalCalls.Physics2D_SetGravity(&value); }
			}
		}

		public static bool AllowSleep
		{
			get { unsafe { return InternalCalls.Physics2D_GetAllowSleep(); } }
			set { unsafe { InternalCalls.Physics2D_SetAllowSleep(value); } }
		}
	}
}
