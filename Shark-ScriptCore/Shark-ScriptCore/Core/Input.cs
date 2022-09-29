
namespace Shark
{

	public static class Input
	{
		public static bool IsKeyPressed(KeyCode key)
			=> InternalCalls.Input_IsKeyStateSet(key, InternalCalls.KeyState.Pressed);
		public static bool IsKeyDown(KeyCode key)
			=> InternalCalls.Input_IsKeyStateSet(key, InternalCalls.KeyState.Down);
		public static bool IsKeyReleased(KeyCode key)
			=> InternalCalls.Input_IsKeyStateSet(key, InternalCalls.KeyState.Released);

		public static bool IsMousePressed(MouseButton button)
			=> InternalCalls.Input_IsMouseStateSet(button, InternalCalls.MouseState.Pressed);
		public static bool IsMouseDown(MouseButton button)
			=> InternalCalls.Input_IsMouseStateSet(button, InternalCalls.MouseState.Down);
		public static bool IsMouseReleased(MouseButton button)
			=> InternalCalls.Input_IsMouseStateSet(button, InternalCalls.MouseState.Released);
		public static bool IsMouseDoubleClicked(MouseButton button)
			=> InternalCalls.Input_IsMouseStateSet(button, InternalCalls.MouseState.DoubleClicked);

		public static float MouseScroll
			=> InternalCalls.Input_GetMouseScroll();

		public static Vector2i MousePos
		{
			get
			{
				InternalCalls.Input_GetMousePos(out var mousePos);
				return mousePos;
			}
		}
	}

}
