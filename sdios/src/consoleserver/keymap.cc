// 
// File: src/consoleserver/keymap.cc
//
// Description: Scancode-to-Char mapping
//
union modifiers_t {
    L4_Word_t raw;
    struct {
        L4_Word_t capsLock   : 1;
        L4_Word_t numLock    : 1;
        L4_Word_t scrollLock : 1;
        L4_Word_t ctrlDown   : 1;
        L4_Word_t altDown    : 1;
        L4_Word_t shiftDown  : 1;
    };
};

modifiers_t modifiers = { raw: 0UL };

#define MAX_CHAR_LEN 7
#define HIGH_CODE 0x59
struct keymap {
	char none        [HIGH_CODE][MAX_CHAR_LEN];
	char ctrl        [HIGH_CODE][MAX_CHAR_LEN];
	char alt         [HIGH_CODE][MAX_CHAR_LEN];
	char ctrlalt     [HIGH_CODE][MAX_CHAR_LEN];
	char shift       [HIGH_CODE][MAX_CHAR_LEN];
	char ctrlshift   [HIGH_CODE][MAX_CHAR_LEN];
	char altshift    [HIGH_CODE][MAX_CHAR_LEN];
	char ctrlaltshift[HIGH_CODE][MAX_CHAR_LEN];
};


keymap stdmap = {
	/* none */ { /*err*/"", /*Esc*/"\033",                                              //  2
	    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "ß", "'", /*BS*/"\010",       // 13
	    "\t", "q", "w", "e", "r", "t", "z", "u", "i", "o", "p", "ü", "+", "\n",         // 14
	    /*LCtrl*/"", "a", "s", "d", "f", "g", "h", "j", "k", "l", "ö", "ä", "#",/* "^",*/   // 14
	    /*LShift*/"", "<", "y", "x", "c", "v", "b", "n", "m", ",", ".", "-",            // 12
	    /*RShift*/"", /*KP**/"*", /*LAlt*/"", " ", /*CapsLk*/"",                        //  5
	    /*F1*/"\033[OP", /*F2*/"\033[OQ", /*F3*/"\033[OR", /*F4*/"\033[OS",             //  4
	    /*F5*/"\033[[15~", /*F6*/"\033[[17~", /*F7*/"\033[[18~",                        //  3
	    /*F8*/"\033[[19~", /*F9*/"\033[[20~", /*F10*/"\033[[21~",                       //  3
	    /*NumLk*/"", /*ScrLk*/"",                                                       //  2
	    /*KP7*/"\033[1~", /*KP8*/"\033[A", /*KP9*/"\033[5~", /*KP-*/"-",                //  4
	    /*KP4*/"\033[D", /*KP5*/"\033[E", /*KP6*/"\033[C", /*KP+*/"+",                  //  4
	    /*KP1*/"\033[4~", /*KP2*/"\033[B", /*KP3*/"\033[6~",                            //  3
	    /*KP0*/"\033[2~", /*KP,*/"\033[3~",                                             //  2
	    /*Alt-SysRq*/"", "", "", /*F11*/"\033[23~", /*F12*/"\033[24~"                   //  5
	},
	/* ctrl */ { /*err*/"", /*Esc*/"\033", 
	    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "ß", "'", /*BS*/"\010",
	    "\t", "\021", "\027", "\005", "\022", "\024", "\032", "\025", "\011", "\017", "\020", "ü", "+", "\n",
	    /*LCtrl*/"", "\001", "\023", "\004", "\006", "\007", "\010", "\012", "\013", "\014", "ö", "ä", "#",/* "^",*/
	    /*LShift*/"", "<", "\031", "\030", "\003", "\026", "\002", "\016", "\015", ",", ".", "-", 
	    /*RShift*/"", /*KP**/"*", /*LAlt*/"", " ", /*CapsLk*/"", 
	    /*F1*/"\033[OP", /*F2*/"\033[OQ", /*F3*/"\033[OR", /*F4*/"\033[OS",
	    /*F5*/"\033[[15~", /*F6*/"\033[[17~", /*F7*/"\033[[18~", 
	    /*F8*/"\033[[19~", /*F9*/"\033[[20~", /*F10*/"\033[[21~",
	    /*NumLk*/"", /*ScrLk*/"",
	    /*KP7*/"\033[1~", /*KP8*/"\033[A", /*KP9*/"\033[5~", /*KP-*/"-", 
	    /*KP4*/"\033[D", /*KP5*/"\033[E", /*KP6*/"\033[C", /*KP+*/"+",
	    /*KP1*/"\033[4~", /*KP2*/"\033[B", /*KP3*/"\033[6~",
	    /*KP0*/"\033[2~", /*KP,*/"\033[3~", 
	    /*Alt-SysRq*/"", "", "", /*F11*/"\033[23~", /*F12*/"\033[24~"
    },
	/* alt */ { /*err*/"", /*Esc*/"\033", 
	    "\0331", "\0332", "\0333", "\0334", "\0335", "\0336", "\0337", "\0338", "\0339", "\0330", "\033ß", "\033'", /*BS*/"\033\010",
	    "\033\t", "\033q", "\033w", "\033e", "\033r", "\033t", "\033z", "\033u", "\033i", "\033o", "\033p", "\033ü", "\033+", "\033\n",
	    /*LCtrl*/"", "\033a", "\033s", "\033d", "\033f", "\033g", "\033h", "\033j", "\033k", "\033l", "\033ö", "\033ä", "\033#",/* "^",*/
	    /*LShift*/"", "\033<", "\033y", "\033x", "\033c", "\033v", "\033b", "\033n", "\033m", "\033,", "\033.", "\033-", 
	    /*RShift*/"", /*KP**/"*", /*LAlt*/"", "\033 ", /*CapsLk*/"", 
	    /*F1*/"\033[OP", /*F2*/"\033[OQ", /*F3*/"\033[OR", /*F4*/"\033[OS",
	    /*F5*/"\033[[15~", /*F6*/"\033[[17~", /*F7*/"\033[[18~", 
	    /*F8*/"\033[[19~", /*F9*/"\033[[20~", /*F10*/"\033[[21~",
	    /*NumLk*/"", /*ScrLk*/"",
	    /*KP7*/"\033[1~", /*KP8*/"\033[A", /*KP9*/"\033[5~", /*KP-*/"-", 
	    /*KP4*/"\033[D", /*KP5*/"\033[E", /*KP6*/"\033[C", /*KP+*/"+",
	    /*KP1*/"\033[4~", /*KP2*/"\033[B", /*KP3*/"\033[6~",
	    /*KP0*/"\033[2~", /*KP,*/"\033[3~", 
	    /*Alt-SysRq*/"", "", "", /*F11*/"\033[23~", /*F12*/"\033[24~" 
	},
	/* ctrlalt */ { /*err*/"", /*Esc*/"\033", 
	    "¹", "²", "³", "¼", "½", "¬", "{", "[", "]", "}", "\\", "¸", /*BS*/"\010",
	    "\t", "@", "ł", "€", "¶", "ŧ", "←", "↓", "→", "ø", "þ", "¨", "~", "\n",
	    /*LCtrl*/"", "æ", "ß", "ð", "đ", "ŋ", "ħ", "j", "ĸ", "ł", "˝", "^", "`",/* "¬",*/
	    /*LShift*/"", "|", "«", "»", "¢", "“", "”", "n", "µ", ",", "·", "-", 
	    /*RShift*/"", /*KP**/"*", /*LAlt*/"", " ", /*CapsLk*/"", 
	    /*F1*/"\033[OP", /*F2*/"\033[OQ", /*F3*/"\033[OR", /*F4*/"\033[OS",
	    /*F5*/"\033[[15~", /*F6*/"\033[[17~", /*F7*/"\033[[18~", 
	    /*F8*/"\033[[19~", /*F9*/"\033[[20~", /*F10*/"\033[[21~",
	    /*NumLk*/"", /*ScrLk*/"",
	    /*KP7*/"\033[1~", /*KP8*/"\033[A", /*KP9*/"\033[5~", /*KP-*/"-", 
	    /*KP4*/"\033[D", /*KP5*/"\033[E", /*KP6*/"\033[C", /*KP+*/"+",
	    /*KP1*/"\033[4~", /*KP2*/"\033[B", /*KP3*/"\033[6~",
	    /*KP0*/"\033[2~", /*KP,*/"\033[3~", 
	    /*Alt-SysRq*/"", "", "", /*F11*/"\033[23~", /*F12*/"\033[24~" 
	},
	/* shift */ { /*err*/"", /*Esc*/"\033", 
	    "!", "\"", "§", "$", "%", "&", "/", "(", ")", "=", "?", "`", /*BS*/"\010",
	    "\t", "Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P", "Ü", "*", "\n",
	    /*LCtrl*/"", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Ö", "Ä", "'",/* "°",*/
	    /*LShift*/"", ">", "Y", "X", "C", "V", "B", "N", "M", ";", ":", "_", 
	    /*RShift*/"", /*KP**/"*", /*LAlt*/"", " ", /*CapsLk*/"", 
	    /*F1*/"\033[OP", /*F2*/"\033[OQ", /*F3*/"\033[OR", /*F4*/"\033[OS",
	    /*F5*/"\033[[15~", /*F6*/"\033[[17~", /*F7*/"\033[[18~", 
	    /*F8*/"\033[[19~", /*F9*/"\033[[20~", /*F10*/"\033[[21~",
	    /*NumLk*/"", /*ScrLk*/"",
	    /*KP7*/"\033[1~", /*KP8*/"\033[A", /*KP9*/"\033[5~", /*KP-*/"-", 
	    /*KP4*/"\033[D", /*KP5*/"\033[E", /*KP6*/"\033[C", /*KP+*/"+",
	    /*KP1*/"\033[4~", /*KP2*/"\033[B", /*KP3*/"\033[6~",
	    /*KP0*/"\033[2~", /*KP,*/"\033[3~", 
	    /*Alt-SysRq*/"", "", "", /*F11*/"\033[23~", /*F12*/"\033[24~" 
	},
	/* ctrlshift */ { /*err*/"", /*Esc*/"\033", 
	    "!", "\"", "§", "$", "%", "&", "/", "(", ")", "=", "?", "`", /*BS*/"\010",
	    "\t", "Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P", "Ü", "*", "\n",
	    /*LCtrl*/"", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Ö", "Ä", "'",/* "°",*/
	    /*LShift*/"", ">", "Y", "X", "C", "V", "B", "N", "M", ";", ":", "_", 
	    /*RShift*/"", /*KP**/"*", /*LAlt*/"", " ", /*CapsLk*/"", 
	    /*F1*/"\033[OP", /*F2*/"\033[OQ", /*F3*/"\033[OR", /*F4*/"\033[OS",
	    /*F5*/"\033[[15~", /*F6*/"\033[[17~", /*F7*/"\033[[18~", 
	    /*F8*/"\033[[19~", /*F9*/"\033[[20~", /*F10*/"\033[[21~",
	    /*NumLk*/"", /*ScrLk*/"",
	    /*KP7*/"\033[1~", /*KP8*/"\033[A", /*KP9*/"\033[5~", /*KP-*/"-", 
	    /*KP4*/"\033[D", /*KP5*/"\033[E", /*KP6*/"\033[C", /*KP+*/"+",
	    /*KP1*/"\033[4~", /*KP2*/"\033[B", /*KP3*/"\033[6~",
	    /*KP0*/"\033[2~", /*KP,*/"\033[3~", 
	    /*Alt-SysRq*/"", "", "", /*F11*/"\033[23~", /*F12*/"\033[24~" 
	},
	/* altshift */ { /*err*/"", /*Esc*/"\033", 
	    "!", "\"", "§", "$", "%", "&", "/", "(", ")", "=", "?", "`", /*BS*/"\010",
	    "\t", "Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P", "Ü", "*", "\n",
	    /*LCtrl*/"", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Ö", "Ä", "'",/* "°",*/
	    /*LShift*/"", ">", "Y", "X", "C", "V", "B", "N", "M", ";", ":", "_", 
	    /*RShift*/"", /*KP**/"*", /*LAlt*/"", " ", /*CapsLk*/"", 
	    /*F1*/"\033[OP", /*F2*/"\033[OQ", /*F3*/"\033[OR", /*F4*/"\033[OS",
	    /*F5*/"\033[[15~", /*F6*/"\033[[17~", /*F7*/"\033[[18~", 
	    /*F8*/"\033[[19~", /*F9*/"\033[[20~", /*F10*/"\033[[21~",
	    /*NumLk*/"", /*ScrLk*/"",
	    /*KP7*/"\033[1~", /*KP8*/"\033[A", /*KP9*/"\033[5~", /*KP-*/"-", 
	    /*KP4*/"\033[D", /*KP5*/"\033[E", /*KP6*/"\033[C", /*KP+*/"+",
	    /*KP1*/"\033[4~", /*KP2*/"\033[B", /*KP3*/"\033[6~",
	    /*KP0*/"\033[2~", /*KP,*/"\033[3~", 
	    /*Alt-SysRq*/"", "", "", /*F11*/"\033[23~", /*F12*/"\033[24~" 
	},
	/* ctrlaltshift */ { /*err*/"", /*Esc*/"\033", 
	    "¡", "⅛", "£", "¤", "⅜", "⅝", "⅞", "™", "±", "°", "¿", "˛", /*BS*/"\010",
	    "\t", "Ω", "Ł", "€", "®", "Ŧ", "¥", "↑", "ı", "Ø", "Þ", "Ü", "*", "\n",
	    /*LCtrl*/"", "Æ", "§", "Ð", "ª", "Ŋ", "Ħ", "J", "&", "Ł", "˝", "ˇ", "˘",/* "¬",*/
	    /*LShift*/"", "¦", "<", ">", "©", "‘", "’", "N", "º", "×", "÷", "_", 
	    /*RShift*/"", /*KP**/"*", /*LAlt*/"", " ", /*CapsLk*/"", 
	    /*F1*/"\033[OP", /*F2*/"\033[OQ", /*F3*/"\033[OR", /*F4*/"\033[OS",
	    /*F5*/"\033[[15~", /*F6*/"\033[[17~", /*F7*/"\033[[18~", 
	    /*F8*/"\033[[19~", /*F9*/"\033[[20~", /*F10*/"\033[[21~",
	    /*NumLk*/"", /*ScrLk*/"",
	    /*KP7*/"\033[1~", /*KP8*/"\033[A", /*KP9*/"\033[5~", /*KP-*/"-", 
	    /*KP4*/"\033[D", /*KP5*/"\033[E", /*KP6*/"\033[C", /*KP+*/"+",
	    /*KP1*/"\033[4~", /*KP2*/"\033[B", /*KP3*/"\033[6~",
	    /*KP0*/"\033[2~", /*KP,*/"\033[3~", 
	    /*Alt-SysRq*/"", "", "", /*F11*/"\033[23~", /*F12*/"\033[24~" 
	}
};

void keycodeToChars(char scancode, char * buf) {
    char * current;
    char null[1] = "";
    switch (scancode) {
        case 0xe0: // Escape... just discard, something meaningful should happen anyway :)
            break;
        case 0x1d: // LCtrl make
            modifiers.ctrlDown = 1;
            break;
        case 0x9d: // LCtrl break
            modifiers.ctrlDown = 0;
            break;
        case 0x2a: // LShift make
        case 0x36: // RShift make
            modifiers.shiftDown = 1;
            break;
        case 0xaa: // LShift break
        case 0xb6: // RShift break
            modifiers.shiftDown = modifiers.capsLock = 0;
            break;
        case 0x38: // LAlt make
            modifiers.altDown = 1;
            break;
        case 0xb8: // LAlt break
            modifiers.altDown = 0;
            break;
        case 0x3a: // CapsLock make
            modifiers.capsLock = modifiers.shiftDown = 1;
            break;
        case 0x45: // NumLock make
            modifiers.numLock = 1 - modifiers.numLock;
            break;
        case 0x46: // ScrollLock make
            modifiers.scrollLock = 1 - modifiers.scrollLock;
            break;

        default: // not a special modifier key
            if (scancode < HIGH_CODE) {
                if (!modifiers.shiftDown) {
                    if (!modifiers.altDown) {
                        if (!modifiers.ctrlDown)
                            current = (char *)stdmap.none;
                        else
                            current = (char *)stdmap.ctrl;
                    } else {
                        if (!modifiers.ctrlDown)
                            current = (char *)stdmap.alt;
                        else
                            current = (char *)stdmap.ctrlalt;
                    }
                } else {
                    if (!modifiers.altDown) {
                        if (!modifiers.ctrlDown)
                            current = (char *)stdmap.shift;
                        else
                            current = (char *)stdmap.ctrlshift;
                    } else {
                        if (!modifiers.ctrlDown)
                            current = (char *)stdmap.altshift;
                        else
                            current = (char *)stdmap.ctrlaltshift;
                    }
                }
                strncpy(buf, current + (scancode * MAX_CHAR_LEN), MAX_CHAR_LEN);
                return;
            }
        }
    // no scancode handled by the array, so pad with null characters
    strncpy(buf, (char *)null, MAX_CHAR_LEN);
}
