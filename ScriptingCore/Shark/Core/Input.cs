﻿
namespace Shark
{
	public enum Key : ushort
	{
		BackSpace                  = 0x08,
		Tab                        = 0x09,

		Clear                      = 0x0C,
		Return                     = 0x0D,

		Shift                      = 0x10,
		Control                    = 0x11,
		Alt                        = 0x12,
		Pause                      = 0x13,
		Capital                    = 0x14,

		// Kana                       = 0x15,
		// Hangeul                    = 0x15,  /* old name - should be here for compatibility */
		// Hangul                     = 0x15,
		// Ime_on                     = 0x16,
		// Junja                      = 0x17,
		// Final                      = 0x18,
		// Hanja                      = 0x19,
		// Kanji                      = 0x19,
		// ImeOff                     = 0x1A,

		Escape                     = 0x1B,

		Convert                    = 0x1C,
		Nonconvert                 = 0x1D,
		Accept                     = 0x1E,
		Modechange                 = 0x1F,
								       
		Space                      = 0x20,
		PageUp                     = 0x21,
		PageDown                   = 0x22,
		End                        = 0x23,
		Home                       = 0x24,
		LeftArrow                  = 0x25,
		UpArrow                    = 0x26,
		RightArrow                 = 0x27,
		DownArrow                  = 0x28,
		Select                     = 0x29,
		Print                      = 0x2A,
		Execute                    = 0x2B,
		Snapshot                   = 0x2C,
		Insert                     = 0x2D,
		Delete                     = 0x2E,
		Help                       = 0x2F,

		D0                         = 0x30,
		D1                         = 0x31,
		D2                         = 0x32,
		D3                         = 0x33,
		D4                         = 0x34,
		D5                         = 0x35,
		D6                         = 0x36,
		D7                         = 0x37,
		D8                         = 0x38,
		D9                         = 0x39,

		A                          = 0x41,
		B                          = 0x42,
		C                          = 0x43,
		D                          = 0x44,
		E                          = 0x45,
		F                          = 0x46,
		G                          = 0x47,
		H                          = 0x48,
		I                          = 0x49,
		J                          = 0x4A,
		K                          = 0x4B,
		L                          = 0x4C,
		M                          = 0x4D,
		N                          = 0x4E,
		O                          = 0x4F,
		P                          = 0x50,
		Q                          = 0x51,
		R                          = 0x52,
		S                          = 0x53,
		T                          = 0x54,
		U                          = 0x55,
		V                          = 0x56,
		W                          = 0x57,
		X                          = 0x58,
		Y                          = 0x59,
		Z                          = 0x5A,

		LeftWindows                = 0x5B,
		RightWindows               = 0x5C,
		Applications               = 0x5D,

		Sleep                      = 0x5F,
								       
		Numpad0                    = 0x60,
		Numpad1                    = 0x61,
		Numpad2                    = 0x62,
		Numpad3                    = 0x63,
		Numpad4                    = 0x64,
		Numpad5                    = 0x65,
		Numpad6                    = 0x66,
		Numpad7                    = 0x67,
		Numpad8                    = 0x68,
		Numpad9                    = 0x69,
		Multiply                   = 0x6A,
		Add                        = 0x6B,
		Separator                  = 0x6C,
		Subtract                   = 0x6D,
		Decimal                    = 0x6E,
		Divide                     = 0x6F,
		F1                         = 0x70,
		F2                         = 0x71,
		F3                         = 0x72,
		F4                         = 0x73,
		F5                         = 0x74,
		F6                         = 0x75,
		F7                         = 0x76,
		F8                         = 0x77,
		F9                         = 0x78,
		F10                        = 0x79,
		F11                        = 0x7A,
		F12                        = 0x7B,
		F13                        = 0x7C,
		F14                        = 0x7D,
		F15                        = 0x7E,
		F16                        = 0x7F,
		F17                        = 0x80,
		F18                        = 0x81,
		F19                        = 0x82,
		F20                        = 0x83,
		F21                        = 0x84,
		F22                        = 0x85,
		F23                        = 0x86,
		F24                        = 0x87,

		LeftShift                  = 0xA0,
		RightShift                 = 0xA1,
		LeftControl                = 0xA2,
		RightControl               = 0xA3,
		LeftMenu                   = 0xA4,
		RightMenu                  = 0xA5,

		BrowserBack                = 0xA6,
		BrowserForward             = 0xA7,
		BrowserRefresh             = 0xA8,
		BrowserStop                = 0xA9,
		BrowserSearch              = 0xAA,
		BrowserFavorites           = 0xAB,
		BrowserHome                = 0xAC,
							           
		VolumeMute                 = 0xAD,
		VolumeDown                 = 0xAE,
		VolumeUp                   = 0xAF,
		MediaNext                  = 0xB0,
		MediaPrev                  = 0xB1,
		MediaStop                  = 0xB2,
		MediaPlay                  = 0xB3,
		LaunchMail                 = 0xB4,
		LaunchMediaSelect          = 0xB5,
		LaunchApp1                 = 0xB6,
		LaunchApp2                 = 0xB7,

		Oem1                       = 0xBA,   // ';:' for US
		Oemplus                    = 0xBB,   // '+' any country
		OemComma                   = 0xBC,   // ',' any country
		OemMinus                   = 0xBD,   // '-' any country
		OemPeriod                  = 0xBE,   // '.' any country
		Oem2                       = 0xBF,   // '/?' for US
		Oem3                       = 0xC0,   // '`~' for US
							           
		Oem4                       = 0xDB,  //  '[{' for US
		Oem5                       = 0xDC,  //  '\|' for US
		Oem6                       = 0xDD,  //  ']}' for US
		Oem7                       = 0xDE,  //  ''"' for US
		Oem8                       = 0xDF,
							           
		OemAX                      = 0xE1,  //  'AX' key on Japanese AX kbd
		Oem102                     = 0xE2,  //  "<>" or "\|" on RT 102-key kbd.
		IcoHelp                    = 0xE3,  //  Help key on ICO
		Ico00                      = 0xE4,  //  00 key on ICO

		ProcessKey                 = 0xE5,
		Packet                     = 0xE7,

		Attnention                 = 0xF6,
		CrSel                      = 0xF7,
		ExSel                      = 0xF8,
		EraseEndOfFile             = 0xF9,
		Play                       = 0xFA,
		Zoom                       = 0xFB,
		NoName                     = 0xFC,
		Pa1                        = 0xFD,
		OemClear                   = 0xFE
	}

	public enum MouseButton : ushort
	{
		Left,
		Right,
		Middle,
		Thumb01,
		Thumb02,

		Invalid
	}

	public static class Input
	{
		public static bool KeyPressed(Key key)
			=> InternalCalls.Input_KeyPressed(key);

		public static bool MouseButtonPressed(MouseButton button)
			=> InternalCalls.Input_MouseButtonPressed(button);

		public static Vector2i GetMousePos()
			=> InternalCalls.Input_GetMousePos();

	}

}
