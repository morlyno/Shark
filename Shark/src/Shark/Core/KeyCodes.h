#pragma once

namespace Shark {
	
	using KeyCode = uint16_t;

	namespace Key {

		enum : KeyCode
		{
			Invalid                    = 0,

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
			LeftAlt                   = 0xA4,
			RightAlt                  = 0xA5,

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
		};


		inline std::string ToString(KeyCode key)
		{
			switch (key)
			{
				case BackSpace:             return SK_STRINGIFY(BackSpace);
				case Tab:			        return SK_STRINGIFY(Tab);
				case Clear:			        return SK_STRINGIFY(Clear);
				case Return:		        return SK_STRINGIFY(Return);
				case Shift:			        return SK_STRINGIFY(Shift);
				case Control:		        return SK_STRINGIFY(Control);
				case Alt:			        return SK_STRINGIFY(Alt);
				case Pause:			        return SK_STRINGIFY(Pause);
				case Capital:		        return SK_STRINGIFY(Capital);
				case Escape:		        return SK_STRINGIFY(Escape);
				case Convert:		        return SK_STRINGIFY(Convert);
				case Nonconvert:	        return SK_STRINGIFY(Nonconvert);
				case Accept:		        return SK_STRINGIFY(Accept);
				case Modechange:	        return SK_STRINGIFY(Modechange);
				case Space:			        return SK_STRINGIFY(Space);
				case PageUp:		        return SK_STRINGIFY(PageUp);
				case PageDown:		        return SK_STRINGIFY(PageDown);
				case End:			        return SK_STRINGIFY(End);
				case Home:			        return SK_STRINGIFY(Home);
				case LeftArrow:		        return SK_STRINGIFY(LeftArrow);
				case UpArrow:		        return SK_STRINGIFY(UpArrow);
				case RightArrow:	        return SK_STRINGIFY(RightArrow);
				case DownArrow:		        return SK_STRINGIFY(DownArrow);
				case Select:		        return SK_STRINGIFY(Select);
				case Print:			        return SK_STRINGIFY(Print);
				case Execute:		        return SK_STRINGIFY(Execute);
				case Snapshot:		        return SK_STRINGIFY(Snapshot);
				case Insert:		        return SK_STRINGIFY(Insert);
				case Delete:		        return SK_STRINGIFY(Delete);
				case Help:			        return SK_STRINGIFY(Help);
				case D0:			        return SK_STRINGIFY(D0);
				case D1:			        return SK_STRINGIFY(D1);
				case D2:			        return SK_STRINGIFY(D2);
				case D3:			        return SK_STRINGIFY(D3);
				case D4:			        return SK_STRINGIFY(D4);
				case D5:			        return SK_STRINGIFY(D5);
				case D6:			        return SK_STRINGIFY(D6);
				case D7:			        return SK_STRINGIFY(D7);
				case D8:			        return SK_STRINGIFY(D8);
				case D9:			        return SK_STRINGIFY(D9);
				case A:				        return SK_STRINGIFY(A);
				case B:				        return SK_STRINGIFY(B);
				case C:				        return SK_STRINGIFY(C);
				case D:				        return SK_STRINGIFY(D);
				case E:				        return SK_STRINGIFY(E);
				case F:				        return SK_STRINGIFY(F);
				case G:				        return SK_STRINGIFY(G);
				case H:				        return SK_STRINGIFY(H);
				case I:				        return SK_STRINGIFY(I);
				case J:				        return SK_STRINGIFY(J);
				case K:				        return SK_STRINGIFY(K);
				case L:				        return SK_STRINGIFY(L);
				case M:				        return SK_STRINGIFY(M);
				case N:				        return SK_STRINGIFY(N);
				case O:				        return SK_STRINGIFY(O);
				case P:				        return SK_STRINGIFY(P);
				case Q:				        return SK_STRINGIFY(Q);
				case R:				        return SK_STRINGIFY(R);
				case S:				        return SK_STRINGIFY(S);
				case T:				        return SK_STRINGIFY(T);
				case U:				        return SK_STRINGIFY(U);
				case V:				        return SK_STRINGIFY(V);
				case W:				        return SK_STRINGIFY(W);
				case X:				        return SK_STRINGIFY(X);
				case Y:				        return SK_STRINGIFY(Y);
				case Z:				        return SK_STRINGIFY(Z);
				case LeftWindows:	        return SK_STRINGIFY(LeftWindows);
				case RightWindows:	        return SK_STRINGIFY(RightWindows);
				case Applications:	        return SK_STRINGIFY(Applications);
				case Sleep:			        return SK_STRINGIFY(Sleep);
				case Numpad0:		        return SK_STRINGIFY(Numpad0);
				case Numpad1:		        return SK_STRINGIFY(Numpad1);
				case Numpad2:		        return SK_STRINGIFY(Numpad2);
				case Numpad3:		        return SK_STRINGIFY(Numpad3);
				case Numpad4:		        return SK_STRINGIFY(Numpad4);
				case Numpad5:		        return SK_STRINGIFY(Numpad5);
				case Numpad6:		        return SK_STRINGIFY(Numpad6);
				case Numpad7:		        return SK_STRINGIFY(Numpad7);
				case Numpad8:		        return SK_STRINGIFY(Numpad8);
				case Numpad9:		        return SK_STRINGIFY(Numpad9);
				case Multiply:		        return SK_STRINGIFY(Multiply);
				case Add:			        return SK_STRINGIFY(Add);
				case Separator:		        return SK_STRINGIFY(Separator);
				case Subtract:		        return SK_STRINGIFY(Subtract);
				case Decimal:		        return SK_STRINGIFY(Decimal);
				case Divide:		        return SK_STRINGIFY(Divide);
				case F1:			        return SK_STRINGIFY(F1);
				case F2:			        return SK_STRINGIFY(F2);
				case F3:			        return SK_STRINGIFY(F3);
				case F4:			        return SK_STRINGIFY(F4);
				case F5:			        return SK_STRINGIFY(F5);
				case F6:			        return SK_STRINGIFY(F6);
				case F7:			        return SK_STRINGIFY(F7);
				case F8:			        return SK_STRINGIFY(F8);
				case F9:			        return SK_STRINGIFY(F9);
				case F10:			        return SK_STRINGIFY(F10);
				case F11:			        return SK_STRINGIFY(F11);
				case F12:			        return SK_STRINGIFY(F12);
				case F13:			        return SK_STRINGIFY(F13);
				case F14:			        return SK_STRINGIFY(F14);
				case F15:			        return SK_STRINGIFY(F15);
				case F16:			        return SK_STRINGIFY(F16);
				case F17:			        return SK_STRINGIFY(F17);
				case F18:			        return SK_STRINGIFY(F18);
				case F19:			        return SK_STRINGIFY(F19);
				case F20:			        return SK_STRINGIFY(F20);
				case F21:			        return SK_STRINGIFY(F21);
				case F22:			        return SK_STRINGIFY(F22);
				case F23:			        return SK_STRINGIFY(F23);
				case F24:			        return SK_STRINGIFY(F24);
				case LeftShift:		        return SK_STRINGIFY(LeftShift);
				case RightShift:	        return SK_STRINGIFY(RightShift);
				case LeftControl:	        return SK_STRINGIFY(LeftControl);
				case RightControl:	        return SK_STRINGIFY(RightControl);
				case LeftAlt:		        return SK_STRINGIFY(LeftAlt);
				case RightAlt:		        return SK_STRINGIFY(RightAlt);
				case BrowserBack:	        return SK_STRINGIFY(BrowserBack);
				case BrowserForward:        return SK_STRINGIFY(BrowserForward);
				case BrowserRefresh:        return SK_STRINGIFY(BrowserRefresh);
				case BrowserStop:	        return SK_STRINGIFY(BrowserStop);
				case BrowserSearch:	        return SK_STRINGIFY(BrowserSearch);
				case BrowserFavorites:      return SK_STRINGIFY(BrowserFavorites);
				case BrowserHome:	        return SK_STRINGIFY(BrowserHome);
				case VolumeMute:	        return SK_STRINGIFY(VolumeMute);
				case VolumeDown:	        return SK_STRINGIFY(VolumeDown);
				case VolumeUp:		        return SK_STRINGIFY(VolumeUp);
				case MediaNext:		        return SK_STRINGIFY(MediaNext);
				case MediaPrev:		        return SK_STRINGIFY(MediaPrev);
				case MediaStop:		        return SK_STRINGIFY(MediaStop);
				case MediaPlay:		        return SK_STRINGIFY(MediaPlay);
				case LaunchMail:	        return SK_STRINGIFY(LaunchMail);
				case LaunchMediaSelect:     return SK_STRINGIFY(LaunchMediaSelect);
				case LaunchApp1:	        return SK_STRINGIFY(LaunchApp1);
				case LaunchApp2:	        return SK_STRINGIFY(LaunchApp2);
				case Oem1:			        return SK_STRINGIFY(Oem1);
				case Oemplus:		        return SK_STRINGIFY(Oemplus);
				case OemComma:		        return SK_STRINGIFY(OemComma);
				case OemMinus:		        return SK_STRINGIFY(OemMinus);
				case OemPeriod:		        return SK_STRINGIFY(OemPeriod);
				case Oem2:			        return SK_STRINGIFY(Oem2);
				case Oem3:			        return SK_STRINGIFY(Oem3);
				case Oem4:			        return SK_STRINGIFY(Oem4);
				case Oem5:			        return SK_STRINGIFY(Oem5);
				case Oem6:			        return SK_STRINGIFY(Oem6);
				case Oem7:			        return SK_STRINGIFY(Oem7);
				case Oem8:			        return SK_STRINGIFY(Oem8);
				case OemAX:			        return SK_STRINGIFY(OemAX);
				case Oem102:		        return SK_STRINGIFY(Oem102);
				case IcoHelp:		        return SK_STRINGIFY(IcoHelp);
				case Ico00:			        return SK_STRINGIFY(Ico00);
				case ProcessKey:	        return SK_STRINGIFY(ProcessKey);
				case Packet:		        return SK_STRINGIFY(Packet);
				case Attnention:	        return SK_STRINGIFY(Attnention);
				case CrSel:			        return SK_STRINGIFY(CrSel);
				case ExSel:			        return SK_STRINGIFY(ExSel);
				case EraseEndOfFile:        return SK_STRINGIFY(EraseEndOfFile);
				case Play:			        return SK_STRINGIFY(Play);
				case Zoom:			        return SK_STRINGIFY(Zoom);
				case NoName:		        return SK_STRINGIFY(NoName);
				case Pa1:			        return SK_STRINGIFY(Pa1);
				case OemClear:		        return SK_STRINGIFY(OemClear);
			}

			return "Unkown";
		}
	}



}

namespace Shark {

}
