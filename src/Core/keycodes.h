#pragma once
#include <map>
#include <string>
std::map<std::string, short> keycode_mapper{
	// Function keys
	{"F1" , 112},
	{"F2" , 113},
	{"F3" , 114},
	{"F4" , 115},
	{"F5" , 116},
	{"F6" , 117},
	{"F7" , 118},
	{"F8" , 119},
	{"F9" , 120},
	{"F10" , 121},
	{"F11" , 122},
	{"F12" , 123},

	// Alphanumeric keys
	{"A" , 65},
	{"B" , 66},
	{"C" , 67},
	{"D" , 68},
	{"E" , 69},
	{"F" , 70},
	{"G" , 71},
	{"H" , 72},
	{"I" , 73},
	{"J" , 74},
	{"K" , 75},
	{"L" , 76},
	{"M" , 77},
	{"N" , 78},
	{"O" , 79},
	{"P" , 80},
	{"Q" , 81},
	{"R" , 82},
	{"S" , 83},
	{"T" , 84},
	{"U" , 85},
	{"V" , 86},
	{"W" , 87},
	{"X" , 88},
	{"Y" , 89},
	{"Z" , 90},

	// Numeric keys (top row)
	{"0" , 48},
	{"1" , 49},
	{"2" , 50},
	{"3" , 51},
	{"4" , 52},
	{"5" , 53},
	{"6" , 54},
	{"7" , 55},
	{"8" , 56},
	{"9" , 57},

	// Special characters
	{"TILDE" , 192},
	{"MINUS" , 189},
	{"EQUAL" , 187},
	{"BACKSPACE" , 8},

	// Control keys
	{"TAB" , 9},
	{"ENTER" , 13},
	{"SHIFT" , 16},
	{"CTRL" , 17},
	{"ALT" , 18},
	{"SPACE" , 32},

	// Arrow keys
	{"ARROW_LEFT" , 37},
	{"ARROW_UP" , 38},
	{"ARROW_RIGHT" , 39},
	{"ARROW_DOWN" , 40},

	// Punctuation and other symbols
	{"OPEN_BRACKET" , 219},
	{"CLOSE_BRACKET" , 221},
	{"BACKSLASH" , 220},
	{"SEMICOLON" , 186},
	{"QUOTE" , 222},
	{"COMMA" , 188},
	{"PERIOD" , 190},
	{"SLASH" , 191},

	// Other
	{"ESC" , 27},
	{"CAPSLOCK" , 20},
	{"INSERT" , 45},
	{"DELETE" , 46},
	{"HOME" , 36},
	{"END" , 35},
	{"PAGE_UP" , 33},
	{"PAGE_DOWN" , 34}
};