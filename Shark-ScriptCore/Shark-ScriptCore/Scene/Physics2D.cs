﻿
namespace Shark
{
	public static class Physics2D
	{
		public static Vector2 Gravity
		{
			get
			{
				InternalCalls.Physics2D_GetGravity(out Vector2 gravity);
				return gravity;
			}
			set
			{
				InternalCalls.Physics2D_SetGravity(ref value);
			}
		}

		public static bool AllowSleep
		{
			get => InternalCalls.Physics2D_GetAllowSleep();
			set => InternalCalls.Physics2D_SetAllowSleep(value);
		}
	}
}
