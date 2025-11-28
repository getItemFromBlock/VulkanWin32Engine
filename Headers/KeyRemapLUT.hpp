#pragma once

#include <WinUser.h>

#include "Types.hpp"

namespace KeyRemapLUT
{
	const u8 ScanToVirtual[] =
	{
		0,
		VK_ESCAPE,
		'1',
		'2',
		'3',
		'4',
		'5',
		'6',
		'7',
		'8',
		'9',
		'0',
		VK_OEM_MINUS,
		VK_OEM_PLUS,
		VK_BACK,
		VK_TAB,
		'Q',
		'W',
		'E',
		'R',
		'T',
		'Y',
		'U',
		'I',
		'O',
		'P',
		VK_OEM_4,
		VK_OEM_6,
		VK_RETURN,
		VK_LCONTROL,
		'A',
		'S',
		'D',
		'F',
		'G',
		'H',
		'J',
		'K',
		'L',
		VK_OEM_1,
		VK_OEM_7,
		VK_OEM_3,
		VK_LSHIFT,
		VK_OEM_5,
		'Z',
		'X',
		'C',
		'V',
		'B',
		'N',
		'M',
		VK_OEM_COMMA,
		VK_OEM_PERIOD,
		VK_OEM_2,
		VK_RSHIFT,
		VK_MULTIPLY,
		VK_LMENU,
		VK_SPACE,
		VK_CAPITAL,
		VK_F1,
		VK_F2,
		VK_F3,
		VK_F4,
		VK_F5,
		VK_F6,
		VK_F7,
		VK_F8,
		VK_F9,
		VK_F10,
		VK_PAUSE,
		VK_SCROLL,
		VK_HOME,
		VK_UP,
		VK_PRIOR,
		VK_SUBTRACT,
		VK_LEFT,
		VK_CLEAR,
		VK_RIGHT,
		VK_ADD,
		VK_END,
		VK_DOWN,
		VK_NEXT,
		VK_INSERT,
		VK_DELETE,
		VK_SNAPSHOT,
		0,
		VK_OEM_102,
		VK_F11,
		VK_F12,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0, // JIS ???
		0,
		0,
		0, // ABNT_C1 ???
		0,
		0,
		0,
		0,
		0,
		0, // JIS ???
		0,
		0, // JIS ???
		0,
		0, // JIS ???
		0, // ABNT_C2 ???
		0
	};

	const u8 VirtualToScan[] =
	{
		0,
		0,		// Mouse buttons
		0,
		0,		// VK_CANCEL - TODO remap to whatever will be 0xe046 (CTRL + BREAK)
		0,		// More mouse stuff
		0,
		0,
		0,		// Reserved
		0x0E,	// VK_BACK
		0x0F,	// VK_TAB
		0,		// Reserved
		0,
		0x4C,	// VK_CLEAR
		0x1C,	// VK_RETURN
		0,		// Unused
		0,
		0x2A,	// VK_SHIFT, will map to VK_LSHIFT
		0x1D,	// VK_CONTROL, will map to VK_LCONTROL
		0x38,	// VK_MENU, will ap to VK_LMENU
		0x45,	// VK_PAUSE
		0x3A,	// VK_CAPITAL
		0,		// VK_KANA/VK_HANGUL - IEM Not implemeted for now
		0,		// VK_IME_ON - Idem for this one and the next ones
		0,		// VK_JUNJA
		0,		// VK_FINAL
		0,		// VK_HANJA/VK_KANJI
		0,		// VK_IME_OFF
		0x01,	// VK_ESCAPE
		0,		// VK_CONVERT - IME
		0,		// VK_NONCONVERT
		0,		// VK_ACCEPT
		0,		// VK_MODECHANGE
		0x39,	// VK_SPACE
		0x49,	// VK_PRIOR
		0x51,	// VK_NEXT
		0x4F,	// VK_END
		0x47,	// VK_HOME
		0x4B,	// VK_LEFT
		0x48,	// VK_UP
		0x4D,	// VK_RIGHT
		0x50,	// VK_DOWN
		0,		// VK_SELECT - Idk what that one's scan code is is
		0,		// VK_PRINT - Idem for this one and the one after
		0,		// VK_EXECUTE
		0x54,	// VK_SNAPSHOT
		0x52,	// VK_INSERT
		0x53,	// VK_DELETE
		0,		// VK_HELP - Same as select
		0x0B,	// '0' .. '9'
		0x02,
		0x03,
		0x04,
		0x05,
		0x06,
		0x07,
		0x08,
		0x09,
		0x0A,
		0,		// Unused
		0,
		0,
		0,
		0,
		0,
		0,
		0x1E,	// 'A' .. 'Z'
		0x30,
		0x2E,
		0x20,
		0x12,
		0x21,
		0x22,
		0x23,
		0x17,
		0x24,
		0x25,
		0x26,
		0x32,
		0x31,
		0x18,
		0x19,
		0x10,
		0x13,
		0x1F,
		0x14,
		0x16,
		0x2F,
		0x11,
		0x2D,
		0x15,
		0x2C,
		0x5B,	// VK_LWIN, incorrect because it should be 0xE05B but not implemented yet
		0x5C,	// VK_RWIN, same as above
		0,		// VK_APPS, idk what that is
		0,		// Reserved
		0,		// VK_SLEEP
		0,		// VK_NUMPAD0 - VK_NUMPAD9 - TODO implement that
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0x37,	// VK_MULTIPLY
		0x4E,	// VK_ADD
		0,		// VK_SEPARATOR
		0x4A,	// VK_SUBTRACT
		0,		// VK_DECIMAL - TODO same as mumpad numbers
		0,		// VK_DIVIDE, same as above
		0x3B,	// VK_F1 .. VK_F12
		0x3C,
		0x3D,
		0x3E,
		0x3F,
		0x40,
		0x41,
		0x42,
		0x43,
		0x44,
		0x57,
		0x58,
		0,		// VK_F13++ not implemented
		0,
		0,
		0
	};
}