
namespace Shark.Editor
{
	public static class UI
	{
		public static bool BeginWindow(string windowTitle)
			=> InternalCalls.EditorUI_BeginWindow(windowTitle);
		public static void EndWindow()
			=> InternalCalls.EditorUI_EndWindow();

		public static void Text(string text)
			=> InternalCalls.EditorUI_Text(text);

		public static void Text(string fmt, params object[] args)
			=> Text(string.Format(fmt, args));

		public static void NewLine()
			=> InternalCalls.EditorUI_NewLine();

		public static void Separator()
			=> InternalCalls.EditorUI_Separator();
	}

}
