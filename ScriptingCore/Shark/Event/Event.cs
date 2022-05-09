
using System;
using System.Runtime.InteropServices;
using Shark.KeyEvents;
using Shark.MouseEvents;

namespace Shark
{

	namespace KeyEvents
	{
		public class KeyPressedEvent
		{
			public Key Key { get; private set; }
			public bool IsRepeat { get; private set; }

			internal KeyPressedEvent(Key key, bool isRepeat)
			{
				Key = key;
				IsRepeat = isRepeat;
			}
		}

		public class KeyReleasedEvent
		{
			public Key Key { get; private set; }

			internal KeyReleasedEvent(Key key)
			{
				Key = key;
			}
		}

	}

	namespace MouseEvents
	{

		public class MouseMovedEvent
		{
			public Vector2i Position { get; protected set; }

			internal MouseMovedEvent(Vector2i position)
			{
				Position = position;
			}
		}

		public class MouseButtonPressedEvent
		{
			public MouseButton Button { get; protected set; }
			public Vector2i Position { get; protected set; }

			internal MouseButtonPressedEvent(MouseButton button, Vector2i position)
			{
				Button = button;
				Position = position;
			}
		}

		public class MouseButtonReleasedEvent
		{
			public MouseButton Button { get; protected set; }
			public Vector2i Position { get; protected set; }

			internal MouseButtonReleasedEvent(MouseButton button, Vector2i position)
			{
				Button = button;
				Position = position;
			}
		}

		public class MouseButtonDoubleClickedEvent
		{
			public Vector2i Position { get; protected set; }
			public MouseButton Button { get; protected set; }

			internal MouseButtonDoubleClickedEvent(MouseButton button, Vector2i position)
			{
				Button = button;
				Position = position;
			}
		}

		public class MouseScrolledEvent
		{
			public float Delta { get; protected set; }
			public Vector2i Position { get; protected set; }

			internal MouseScrolledEvent(float delta, Vector2i position)
			{
				Delta = delta;
				Position = position;
			}
		}

	}

	public enum EventType
	{
		None = 0,
		OnKeyPressed = 1,
		OnKryReleased = 2
	}

	public static class EventHandler
	{
		public static event Action<KeyPressedEvent> OnKeyPressed;
		public static event Action<KeyReleasedEvent> OnKeyReleased;

		public static event Action<MouseMovedEvent> OnMouseMoved;
		public static event Action<MouseButtonPressedEvent> OnMouseButtonPressed;
		public static event Action<MouseButtonReleasedEvent> OnMouseButtonReleased;
		public static event Action<MouseButtonDoubleClickedEvent> OnMouseButtonDoubleClicked;
		public static event Action<MouseScrolledEvent> OnMouseScrolled;

		internal static void RaiseOnKeyPressed(Key key, bool isRepeat)
		{
			if (OnKeyPressed != null)
			{
				var e = new KeyPressedEvent(key, isRepeat);
				OnKeyPressed(e);
			}
		}

		internal static void RaiseOnKeyReleased(Key key)
		{
			if (OnKeyReleased != null)
			{
				var e = new KeyReleasedEvent(key);
				OnKeyReleased(e);
			}
		}

		internal static void RaiseOnMouseMoved(Vector2i position)
		{
			if (OnMouseMoved != null)
			{
				var e = new MouseMovedEvent(position);
				OnMouseMoved(e);
			}
		}

		internal static void RaiseOnMouseButtonPressed(MouseButton button, Vector2i position)
		{
			if (OnMouseButtonPressed != null)
			{
				var e = new MouseButtonPressedEvent(button, position);
				OnMouseButtonPressed(e);
			}
		}
		
		internal static void RaiseOnMouseButtonReleased(MouseButton button, Vector2i position)
		{
			if (OnMouseButtonReleased != null)
			{
				var e = new MouseButtonReleasedEvent(button, position);
				OnMouseButtonReleased(e);
			}
		}

		internal static void RaiseOnMouseButtonDoubleClicked(MouseButton button, Vector2i position)
		{
			if (OnMouseButtonDoubleClicked != null)
			{
				var e = new MouseButtonDoubleClickedEvent(button, position);
				OnMouseButtonDoubleClicked(e);
			}
		}

		internal static void RaiseOnMouseScrolled(float delta, Vector2i position)
		{
			if (OnMouseScrolled != null)
			{
				var e = new MouseScrolledEvent(delta, position);
				OnMouseScrolled(e);
			}
		}

	}

}
