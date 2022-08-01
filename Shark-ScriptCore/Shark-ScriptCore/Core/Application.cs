
namespace Shark
{
	public static class Application
	{
		public static uint Width => InternalCalls.Application_GetWidth();
		public static uint Height => InternalCalls.Application_GetHeight();

		public static Vector2u Size => new Vector2u(Width, Height);
	}
}
