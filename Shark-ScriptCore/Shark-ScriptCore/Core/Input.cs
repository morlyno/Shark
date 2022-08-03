
namespace Shark
{

	public static class Input
	{
		public static bool IsKeyPressed(Key key)
			=> InternalCalls.Input_KeyPressed(key);

		public static bool IsMouseButtonPressed(MouseButton button)
			=> InternalCalls.Input_MouseButtonPressed(button);

		public static Vector2i GetMousePos()
		{
			InternalCalls.Input_GetMousePos(out var mousePos);
			return mousePos;
		}

	}

}
