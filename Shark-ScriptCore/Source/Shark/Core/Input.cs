
namespace Shark
{

	internal enum KeyState : ushort
	{
		None = 0,
		Pressed,
		Down,
		Released
	}

	internal enum MouseState : ushort
	{
		None = 0,
		Pressed,
		Down,
		Released,
		DoubleClicked
	}

	public static class Input
	{
		public static bool IsKeyPressed(KeyCode key, bool allowRepeate = false)
		{
			unsafe { return InternalCalls.Input_IsKeyStateSet(key, KeyState.Pressed, allowRepeate); }
		}

		public static bool IsKeyDown(KeyCode key)
		{
			unsafe { return InternalCalls.Input_IsKeyStateSet(key, KeyState.Down, false); }
		}

		public static bool IsKeyReleased(KeyCode key)
		{
			unsafe { return InternalCalls.Input_IsKeyStateSet(key, KeyState.Released, false); }
		}

		public static bool IsMousePressed(MouseButton button)
		{
			unsafe { return InternalCalls.Input_IsMouseStateSet(button, MouseState.Pressed); }
		}

		public static bool IsMouseDown(MouseButton button)
		{
			unsafe { return InternalCalls.Input_IsMouseStateSet(button, MouseState.Down); }
		}

		public static bool IsMouseReleased(MouseButton button)
		{
			unsafe { return InternalCalls.Input_IsMouseStateSet(button, MouseState.Released); }
		}

		public static bool IsMouseDoubleClicked(MouseButton button)
		{
			unsafe { return InternalCalls.Input_IsMouseStateSet(button, MouseState.DoubleClicked); }
		}

		public static float MouseScroll
		{
			get
			{
				unsafe { return InternalCalls.Input_GetMouseScroll(); }
			}
		}

		public static Vector2i MousePos
		{
			get
			{
				Vector2i position;
				unsafe { InternalCalls.Input_GetMousePos(&position); }
				return position;
			}
		}
	}

}
