#define CORECON_SOURCE
#define NO_LOGGING
#define KEYS_DEFINED
#include <stdhdrs.h>
#include <keybrd.h>
// included to have pending struct available to pass to
// command line update....
//#include "WinLogic.h"

#define CHAT_CONTROL_SOURCE
#include "chat_control.h"
#include "chat_control_internal.h"


extern int myTypeID;

enum{
   KS_DELETE
};

static void CPROC _Key_KeystrokePaste( PCONSOLE_INFO pmdp )
{
	if( pmdp->KeystrokePaste )
      pmdp->KeystrokePaste( pmdp );
}

//int KeyLeft( void * list, PUSER_INPUT_BUFFER pci );
//int KeyRight( void * list, PUSER_INPUT_BUFFER pci );
//int KeyInsert( void * list, PUSER_INPUT_BUFFER pci );
//int CommandKeyUp( void * list, PUSER_INPUT_BUFFER pci );
//int HandleKeyDown(  void * list, PUSER_INPUT_BUFFER pci );
//int KeyHome( void * list, PUSER_INPUT_BUFFER pci );
//int KeyEndCmd( uintptr_t list, PUSER_INPUT_BUFFER pci );

DECLTEXT( KeyStroke, "\x7f" ); // DECLTEXT implies 'static'

//----------------------------------------------------------------------------

int CPROC KeyGetGatheredLine( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	PTEXT tmp_input = GetUserInputLine( pci );
	PTEXT line = BuildLine( tmp_input );
	// input is in segments of 256 characters... collapse into a single line.
	if( line && GetTextSize( line ) )
	{
		list->input.phb_Input->pBlock->pLines[0].nLineLength = LineLengthExEx( list->input.CommandInfo->CollectionBuffer, FALSE, 8, NULL );
		list->input.phb_Input->pBlock->pLines[0].pLine = list->input.CommandInfo->CollectionBuffer;
		BuildDisplayInfoLines( list->input.phb_Input, list->input_font );
		if( line )
			list->InputData( list->psvInputData, line );
	}
	else
		LineRelease( line );

	return UPDATE_COMMAND; 
}


#if defined( GCC ) || defined( __LINUX__ )
CORECON_EXPORT( PSIKEYDEFINE, ConsoleKeyDefs[256] ) =
#if defined( __cplusplus ) || 1
{{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
, {"esc","escape",0|0|0|0|0|0,{{KEYDATA}}}
, {"1",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"2",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"3",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"4",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"5",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"6",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"7",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"8",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"9",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"0",0,0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"-","dash",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"=","equal",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"back","backspace",0|0|0|0|0|0,{{KEYDATA}}}
, {"tab",0,0|0|0|0|0|0,{{KEYDATA}}}
, {"q",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"w",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"e",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"r",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"t",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"y",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"u",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"i",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"o",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"p",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"[","lbracket",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"]","rbracket",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
,{0}
,{0}
, {"a",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"s",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"d",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"f",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"g",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"h",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"j",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"k",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"l",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {";","semicolon",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"'","quote",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"`","accent",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
,{0}
, {"\\","backslash",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"z",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"x",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"c",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"v",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA},{}}}
, {"b",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"n",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {"m",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA}}}
, {",","comma",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {".","period",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
, {"/","slash",0|0|0|0|0|0,{{KEYDATA},{KEYDATA}}}
,{0}
, {"mult","mulitply",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
,{0}
, {"space","blank",0|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA}}}
,{0}
, {"f1",0,0|0|0|0|0|0,{}}
, {"f2",0,0|0|0|0|0|0,{}}
, {"f3",0,0|0|0|0|0|0,{}}
, {"f4",0,0|0|0|0|0|0,{}}
, {"f5",0,0|0|0|0|0|0,{}}
, {"f6",0,0|0|0|0|0|0,{}}
, {"f7",0,0|0|0|0|0|0,{}}
, {"f8",0,0|0|0|0|0|0,{}}
, {"f9",0,0|0|0|0|0|0,{}}
, {"f10",0,0|0|0|0|0|0,{}}
,{0}
,{0}
, {"num7","pad7",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"num8","pad8",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"num9","pad9",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"subtract",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"num4","pad4",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"num5","pad5",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"num6","pad6",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"add",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"separator",0,KDF_NODEFINE|0|0|0|0|0,{}}
, {"num2","pad2",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"num3","pad3",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"num0","pad0",KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
, {"decimal",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
,{0}
,{0}
,{0}
, {"f11",0,0|0|0|0|0|0,{}}
, {"f12",0,0|0|0|0|0|0,{}}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
, {"insert",0,0|0|0|0|0|0,{{}}}
, {"delete",0,0|0|0|0|0|0,{{}}}
, {"help",0,0|0|0|0|0|0,{}}
,{0}
,{0}
,{0}
, {"divide",0,KDF_NODEFINE|0|0|0|0|0,{{KEYDATA}}}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
, {"return","enter",0|0|0|0|0|0,{{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA},{KEYDATA}}}
,{0}
,{0}
,{0}
,{0}
,{0}
,{0}
, {"next","pgdn",0|0|0|0|0|0,{{}}}
, {"down",0,0|0|0|0|0|0,{{},{}}}
, {"end",0,0|0|0|0|0|0,{{}}}
, {"right",0,0|0|0|0|0|0,{{}}}
,{0}
, {"left",0,0|0|0|0|0|0,{{}}}
, {"prior","pgup",0|0|0|0|0|0,{{}}}
, {"up",0,0|0|0|0|0|0,{{},{}}}
, {"home",0,0|0|0|0|0|0,{{}}}
,{0}
,{0}
,{0}};
#else
{ [KEY_BACKSPACE]={"back","backspace",0,{{KEYDATA}} }
                      , [KEY_TAB]={"tab",0,0,{{KEYDATA}} }
                      , [KEY_ENTER]={"return", "enter",0,{{KEYDATA}
                                             ,{COMMANDKEY, (PTEXT)KeyGetGatheredLine}
                                             ,{COMMANDKEY, (PTEXT)KeyGetGatheredLine)
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}} }
                      //, [KEY_PAUSE]={"pause",0,KDF_NODEFINE }
                      , [KEY_ESCAPE]={"esc", "escape", 0, {{KEYDATA}}} // 0x1b
                      , [KEY_SPACE]={"space", "blank", 0, {{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}} } //0x20
                      , [KEY_PGUP]={"prior", "pgup", 0, {{HISTORYKEY,(PTEXT)HistoryPageUp}} }
                      , [KEY_PGDN]={"next", "pgdn", 0, {{HISTORYKEY,(PTEXT)HistoryPageDown}} }
                      , [KEY_END]={"end", 0, 0, {{COMMANDKEY, (PTEXT)KeyEndCmd}}}
                      , [KEY_HOME]={"home", 0, 0, {{COMMANDKEY, (PTEXT)KeyHome}}}
                      , [KEY_LEFT]={"left", 0, 0, {{COMMANDKEY, (PTEXT)KeyLeft}}}

                      , [KEY_UP]={"up" , 0, 0, { {COMMANDKEY, (PTEXT)CommandKeyUp}
                                       , {HISTORYKEY, (PTEXT)HistoryLineUp}}}
                      , [KEY_RIGHT]={"right", 0, 0, {{COMMANDKEY, (PTEXT)KeyRight}}}
                      , [KEY_DOWN]={"down", 0, 0, {{COMMANDKEY, (PTEXT)HandleKeyDown}
                                       , {HISTORYKEY, (PTEXT)HistoryLineDown}}}
                      //, {"select"}
                      //, [KEY_PRINT]={"print"}
                      //, {"execute"}
                      //, {"snapshot"}
                      , [KEY_GREY_INSERT]={"insert", 0, 0, {{COMMANDKEY, (PTEXT)KeyInsert}}}
                      , [KEY_GREY_DELETE]={"delete", 0, 0, {{KEYDATA_DEFINED, (PTEXT)&KeyStroke}}}
                      , {"help"}
                      , [KEY_0]={"0", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}      //0x30
                      , [KEY_1]={"1", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_2]={"2", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_3]={"3", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_4]={"4", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_5]={"5", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_6]={"6", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_7]={"7", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_8]={"8", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , [KEY_9]={"9", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}

/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
                      //..... to hmm 39
                      , [KEY_A] = {"a",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}} // 0x41 'A'
                      , [KEY_B] = {"b",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_C] = {"c",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_D] = {"d",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_E] = {"e",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_F] = {"f",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_G] = {"g",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_H] = {"h",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_I] = {"i",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_J] = {"j",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_K] = {"k",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_L] = {"l",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_M] = {"m",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_N] = {"n",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_O] = {"o",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_P] = {"p",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_Q] = {"q",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_R] = {"r",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_S] = {"s",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_T] = {"t",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_U] = {"u",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_V] = {"v",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}
                                           ,{0}
                                           ,{SPECIALKEY,(PTEXT)_Key_KeystrokePaste}}}
                      , [KEY_W] = {"w",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_X] = {"x",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_Y] = {"y",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_Z] = {"z",0,KDF_NODEFINE,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , [KEY_PAD_0] = {"num0", "pad0",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_1] = {"num1", "pad1",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_2] = {"num2", "pad2",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_3] = {"num3", "pad3",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_4] = {"num4", "pad4",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_5] = {"num5", "pad5",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_6] = {"num6", "pad6",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_7] = {"num7", "pad7",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_8] = {"num8", "pad8",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_9] = {"num9", "pad9",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_MULT] = {"mult", "mulitply",KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_PLUS] = {"add",0,KDF_NODEFINE,{{KEYDATA} }}
                      , {"separator", 0, KDF_NODEFINE }
                      , [KEY_PAD_MINUS] = {"subtract",0,KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_DOT] = {"decimal",0,KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_PAD_DIV] = {"divide",0,KDF_NODEFINE,{{KEYDATA}} }
                      , [KEY_F1]={"f1" }   // 0x70
                      , [KEY_F2]={"f2" }
                      , [KEY_F3]={"f3" }
                      , [KEY_F4]={"f4" }
                      , [KEY_F5]={"f5" }
                      , [KEY_F6]={"f6" }
                      , [KEY_F7]={"f7" }
                      , [KEY_F8]={"f8" }
                      , [KEY_F9]={"f9" }
                      , [KEY_F10]={"f10" }
                      , [KEY_F11]={"f11" }
                      , [KEY_F12]={"f12" }
                      //, {"numlock",0,KDF_NODEFINE} // 0x90
                      //, {"scroll",0,KDF_NODEFINE}

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
                      //, NONAMES // {"lshift",0,KDF_NODEFINE}  // 0xa0
                      //, NONAMES // {"rshift",0,KDF_NODEFINE}
                      //, NONAMES // {"lctrl", "lcontrol",0,KDF_NODEFINE}
                      //, NONAMES // {"rctrl", "rcontrol",0,KDF_NODEFINE}
                      //, NONAMES // {"lmenu", "lalt",0,KDF_NODEFINE }
                      //, NONAMES // {"rmenu", "ralt",0,KDF_NODEFINE }
                      , [KEY_SEMICOLON]={";", "semicolon",0,{{KEYDATA}
                                            ,{KEYDATA}} }
                      , [KEY_EQUAL]={"=", "equal",0    ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , [KEY_COMMA]={",", "comma",0    ,{{KEYDATA}
                                            ,{KEYDATA}} }
                      , [KEY_DASH]={"-", "dash",0     ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , [KEY_STOP]={".", "period",0   ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , [KEY_SLASH]={"/","slash",0     ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , [KEY_ACCENT]={"`", "accent",0   ,{{KEYDATA}
                                            ,{KEYDATA}} } // 0xc0
                      , [KEY_LEFT_BRACKET]={ "[", "lbracket",0  ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , [KEY_BACKSLASH]={ "\\", "backslash",0,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , [KEY_RIGHT_BRACKET]={ "]", "rbracket",0  ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , [KEY_QUOTE]={ "'", "quote",0     ,{{KEYDATA}
                                              ,{KEYDATA}} }
};
#endif
#ifndef __cplusplus
#  if 0
PRELOAD( WriteSymbols)
{
	FILE *junk = fopen( "out.keysyms", "wt" );
	if( junk )
	{
		int n;
		for( n = 0; n <256; n++ )
		{
         int m;
			if( ConsoleKeyDefs[n].name1 || ConsoleKeyDefs[n].name2 )
			{
				TEXTCHAR tmp[4];
				TEXTCHAR tmp2[4];
				TEXTSTR n1 = ConsoleKeyDefs[n].name1;
				TEXTSTR n2 = ConsoleKeyDefs[n].name2;
				if( StrCmp( ConsoleKeyDefs[n].name1, "\"" ) == 0 )
				{
               n1 = tmp;
               tmp[0] = '\\';
               tmp[1] = '\"';
					tmp[2] = 0;
				}
				if( StrCmp( ConsoleKeyDefs[n].name2, "\"" ) == 0 )
				{
               n2 = tmp2;
               tmp2[0] = '\\';
               tmp2[1] = '\"';
					tmp2[2] = 0;
				}
				if( StrCmp( ConsoleKeyDefs[n].name1, "\\" ) == 0 )
				{
               n1 = tmp;
               tmp[0] = '\\';
               tmp[1] = '\\';
					tmp[2] = 0;
				}
				if( StrCmp( ConsoleKeyDefs[n].name2, "\\" ) == 0 )
				{
               n2 = tmp2;
               tmp2[0] = '\\';
               tmp2[1] = '\\';
					tmp2[2] = 0;
				}
				fprintf( junk, ", {%s%s%s,%s%s%s,%s|%s|%s|%s|%s|%s,{"
						 , ConsoleKeyDefs[n].name1?"\"":""
						 , ConsoleKeyDefs[n].name1?n1:"NULL"
						 , ConsoleKeyDefs[n].name1?"\")":""
						 , ConsoleKeyDefs[n].name2?"\"":""
						 , ConsoleKeyDefs[n].name2?n2:"NULL"
						 , ConsoleKeyDefs[n].name2?"\")":""
						 , ConsoleKeyDefs[n].flags&KDF_NODEFINE?"KDF_NODEFINE":"0"
						 , ConsoleKeyDefs[n].flags&KDF_NOREDEF?"KDF_NOREDEF":"0"
						 , ConsoleKeyDefs[n].flags&KDF_CAPSKEY?"KDF_CAPSKEY":"0"
						 , ConsoleKeyDefs[n].flags&KDF_NUMKEY?"KDF_NUMKEY":"0"
						 , ConsoleKeyDefs[n].flags&KDF_UPACTION?"KDF_UPACTION":"0"
						 , ConsoleKeyDefs[n].flags&KDF_NOKEY?"KDF_NOKEY":"0"
						 );
				for( m = 0; m < 8; m++ )
				{
					if( ConsoleKeyDefs[n].op[m].bFunction )
					{
						fprintf( junk, "%s", m?",":"" );
						fprintf( junk, "{%s}"
								 , (ConsoleKeyDefs[n].op[m].bFunction==KEYDATA)?"KEYDATA":""
								 );
					}

				}
				fprintf( junk, "}}\n" );
			}
			else
			{
				fprintf( junk, ",{0}\n");
			}
		}
		fclose( junk );
	}
}
#  endif

#endif
#else
#define NONAMES {NULL,NULL,0}
PSIKEYDEFINE ConsoleKeyDefs[] = { NONAMES
                      , {"lbutton",0,0 }
                      , {"rbutton",0,0 }
                      , {"cancel",0,0}
                      , {"mbutton",0,0} // 0x04
                      , NONAMES, NONAMES, NONAMES
                      , {"back","backspace",0,{{KEYDATA}} }
                      , {"tab",0,0,{{KEYDATA}} }
                      , NONAMES, NONAMES
                      , {"clear",0,0 }   // 0x0c
                      , {"return", "enter",0,{{KEYDATA}
                                             ,{COMMANDKEY, (PTEXT)KeyGetGatheredLine}
                                             ,{COMMANDKEY, (PTEXT)KeyGetGatheredLine}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}
                                             ,{KEYDATA}} }
                      , NONAMES, NONAMES
                      , {"shift",0,0, {{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}
                                                 ,{CONTROLKEY,(PTEXT)KeyShift}} } // 0x10
                      , {"control",0,0, {{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}
                                                 ,{CONTROLKEY,(PTEXT)KeyControl}} }
                      , {"menu",0,0, {{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}
                                                 ,{CONTROLKEY,(PTEXT)KeyAlt}} }
                      , {"pause",0,0 }
                      , {"captial",0,0 }
                      , NONAMES, NONAMES, NONAMES, NONAMES, NONAMES, NONAMES
                      , {"esc", "escape", 0, {{KEYDATA}}} // 0x1b
                      , NONAMES, NONAMES
                      , NONAMES, NONAMES
                      , {"space", "blank", 0, {{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}
                                              ,{KEYDATA}} } //0x20
                      , {"prior", "pgup", 0, {{HISTORYKEY,(PTEXT)HistoryPageUp}} }
                      , {"next", "pgdn", 0, {{HISTORYKEY,(PTEXT)HistoryPageDown}} }
                      , {"end", 0, 0, {{COMMANDKEY, (PTEXT)KeyEndCmd}}}
                      , {"home", 0, 0, {{COMMANDKEY, (PTEXT)KeyHome}}}
                      , {"left", 0, 0, {{COMMANDKEY, (PTEXT)KeyLeft}}}

                      , {"up" , 0, 0, { {COMMANDKEY, (PTEXT)CommandKeyUp}
                                       , {HISTORYKEY, (PTEXT)HistoryLineUp}}}
                      , {"right", 0, 0, {{COMMANDKEY, (PTEXT)KeyRight}}}
                      , {"down", 0, 0, {{COMMANDKEY, (PTEXT)HandleKeyDown}
                                       , {HISTORYKEY, (PTEXT)HistoryLineDown}}}
                      , {"select"}
                      , {"print"}
                      , {"execute"}
                      , {"snapshot"}
                      , {"insert", 0, 0, {{COMMANDKEY, (PTEXT)KeyInsert}}}
                      , {"delete", 0, 0, {{KEYDATA_DEFINED, (PTEXT)&KeyStroke}}}
                      , {"help"}
                      , {"0", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}      //0x30
                      , {"1", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {"2", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}
                                    ,{KEYDATA}}}
                      , {"3", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {"4", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {"5", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {"6", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {"7", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {"8", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , {"9", 0, 0, {{KEYDATA}
                                    ,{KEYDATA}}}
                      , NONAMES, NONAMES
                      , NONAMES, NONAMES
                      , NONAMES, NONAMES
                      , NONAMES
/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
                      //..... to hmm 39
                      , {"a",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}} // 0x41 'A'
                      , {"b",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"c",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"d",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"e",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"f",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"g",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"h",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"i",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"j",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"k",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"l",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"m",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"n",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"o",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"p",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"q",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"r",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"s",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"t",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"u",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"v",0,0,{{KEYDATA}
											,{KEYDATA}
											,{SPECIALKEY,(PTEXT)_Key_KeystrokePaste}
											,{SPECIALKEY,(PTEXT)_Key_KeystrokePaste}
											//,{KEYDATA}
							 }}
                      , {"w",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"x",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"y",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"z",0,0,{{KEYDATA}
                                           ,{KEYDATA}
                                           ,{KEYDATA}}}
                      , {"lwin" }
                      , {"rwin" }
                      , {"apps" }
                      , NONAMES, NONAMES
                      , {"num0", "pad0",0,{{KEYDATA}} }
                      , {"num1", "pad1",0,{{KEYDATA}} }
                      , {"num2", "pad2",0,{{KEYDATA}} }
                      , {"num3", "pad3",0,{{KEYDATA}} }
                      , {"num4", "pad4",0,{{KEYDATA}} }
                      , {"num5", "pad5",0,{{KEYDATA}} }
                      , {"num6", "pad6",0,{{KEYDATA}} }
                      , {"num7", "pad7",0,{{KEYDATA}} }
                      , {"num8", "pad8",0,{{KEYDATA}} }
                      , {"num9", "pad9",0,{{KEYDATA}} }
                      , {"mult", "mulitply",0,{{KEYDATA}} }
                      , {"add",0,0,{{KEYDATA} }}
                      , {"separator", 0, 0 }
                      , {"subtract",0,0,{{KEYDATA}} }
                      , {"decimal",0,0,{{KEYDATA}} }
                      , {"divide",0,0,{{KEYDATA}} }
                      , {"f1" }   // 0x70
                      , {"f2" }
                      , {"f3" }
                      , {"f4" }
                      , {"f5" }
                      , {"f6" }
                      , {"f7" }
                      , {"f8" }
                      , {"f9" }
                      , {"f10" }
                      , {"f11" }
                      , {"f12" }
                      , {"f13" }
                      , {"f14" }
                      , {"f15" }
                      , {"f16" }
                      , {"f17" }
                      , {"f18" }
                      , {"f19" }
                      , {"f20" }
                      , {"f21" }
                      , {"f22" }
                      , {"f23" }
                      , {"f24" } // 0x87
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , {"numlock",0,0} // 0x90
                      , {"scroll",0,0}

                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
                      , NONAMES // {"lshift",0,0}  // 0xa0
                      , NONAMES // {"rshift",0,0}
                      , NONAMES // {"lctrl", "lcontrol",0,0}
                      , NONAMES // {"rctrl", "rcontrol",0,0}
                      , NONAMES // {"lmenu", "lalt",0,0 }
                      , NONAMES // {"rmenu", "ralt",0,0 }
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES // 0xb0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , {";", "semicolon",0,{{KEYDATA}
                                            ,{KEYDATA}} }
                      , {"=", "equal",0    ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , {",", "comma",0    ,{{KEYDATA}
                                            ,{KEYDATA}} }
                      , {"-", "dash",0     ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , {".", "period",0   ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , {"/","slash",0     ,{{KEYDATA}
                                            ,{KEYDATA}}}
                      , {"`", "accent",0   ,{{KEYDATA}
                                            ,{KEYDATA}} } // 0xc0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES // 0xd0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , { "[", "lbracket",0  ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , { "\\", "backslash",0,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , { "]", "rbracket",0  ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , { "'", "quote",0     ,{{KEYDATA}
                                              ,{KEYDATA}} }
                      , NONAMES
                      , NONAMES // 0xe0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES // 0xf0
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES
                      , NONAMES  // 0xff
                       };
#endif
/*
#if(WINVER >= 0x0400)
#define VK_PROCESSKEY     0xE5
#endif

#define VK_ATTN           0xF6
#define VK_CRSEL          0xF7
#define VK_EXSEL          0xF8
#define VK_EREOF          0xF9
#define VK_PLAY           0xFA
#define VK_ZOOM           0xFB
#define VK_NONAME         0xFC
#define VK_PA1            0xFD
#define VK_OEM_CLEAR      0xFE
*/
//----------------------------------------------------------------------------

int CommandKeyUp( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
   RecallUserInput( pci, TRUE );
   return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int HandleKeyDown( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
   RecallUserInput( pci, FALSE );
   return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyHome( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	SetUserInputPosition( pci, 0, COMMAND_POS_SET );
	return UPDATE_COMMAND; 
}


int KeyEndCmd( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	SetUserInputPosition( pci, -1, COMMAND_POS_SET );
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyInsert( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	SetUserInputInsert( pci, -1 );
   return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyRight( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	SetUserInputPosition( pci, 1, COMMAND_POS_CUR );
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyLeft( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	SetUserInputPosition( pci, -1, COMMAND_POS_CUR );
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int KeyShift( uint32_t* pKeyState, LOGICAL bDown )
{
   if( bDown )
   {
      *pKeyState |= KEY_MOD_SHIFT;
   }
   else
   {
      *pKeyState &= ~KEY_MOD_SHIFT;
   }
   return FALSE;
}

//----------------------------------------------------------------------------

int KeyControl( uint32_t* pKeyState, LOGICAL bDown )
{
   if( bDown )
   {
      *pKeyState |= KEY_MOD_CTRL;
   }
   else
   {
      *pKeyState &= ~KEY_MOD_CTRL;
   }
   return FALSE;
}

//----------------------------------------------------------------------------

int KeyAlt( uint32_t* pKeyState, LOGICAL bDown )
{
   if( bDown )
   {
      *pKeyState |= KEY_MOD_ALT;
   }
   else
   {
      *pKeyState &= ~KEY_MOD_ALT;
   }
   return FALSE;
}

//----------------------------------------------------------------------------
// Extensions and usage of keybinding data
// -- so far seperated so that perhaps it could be a seperate module...
//----------------------------------------------------------------------------

#define NUM_MODS ( sizeof( ModNames ) / sizeof( char * ) )

CTEXTSTR ModNames[] = { "shift", "ctrl", "alt"
                   , NULL, "control", NULL
                   , "$", "^", "@" };

int FindMod( PTEXT pMod )
{
   int i;
   for( i = 0; i < NUM_MODS; i++ )
   {
      if( ModNames[i] )
         if( TextLike( pMod, ModNames[ i ] ) )
            break;
   }
   if( i < NUM_MODS )
      return ( 1 << ( i % 3 ) );
   return 0;
}

int FindKey( PTEXT pKey )
{
   int i;
   for( i = 0; i < NUM_KEYS; i++ )
   {
		//if( ( !ConsoleKeyDefs[i].flags ) ||
		//    ( ConsoleKeyDefs[i].flags & (KDF_NODEFINE) ) )
      //   continue;
      if( ConsoleKeyDefs[i].name1 && TextLike( pKey, ConsoleKeyDefs[i].name1 ) )
      {
         return i;
      }
      else if( ConsoleKeyDefs[i].name2 && TextLike( pKey, ConsoleKeyDefs[i].name2 ) )
      {
         return i;
      }
   }
   return 0;
}

//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
static void DestroyKeyMacro( PMACRO pm )
{
   PTEXT temp;
   INDEX idx;
   if( pm->flags.un.macro.bUsed )
   {
      pm->flags.un.macro.bDelete = TRUE;
      return;
   }
   LineRelease( pm->pArgs );
   LineRelease( pm->pName );
   LineRelease( pm->pDescription );
   LIST_FORALL( pm->pCommands, idx, PTEXT, temp )
   {
      LineRelease( temp );
   }
   DeleteList( &pm->pCommands );
   Release( pm );
}
#endif

// Usage: /KeyBind shift-F1
//        ... #commands
//        /endmac
// Usage: /KeyBind shift-F1 kill
// Usage: /KeyBind $F1 ... ^F1 $^F1
//  if parameters follow the keybind key-def, those params
//  are taken as keystrokes to type...
//  if no parameters follow, the definition is assumed to
//  be a macro definition, and the macro is invoked by
//  the processing entity...
#ifdef __DEKWARE_PLUGIN__

CORECON_NPROC( int, KeyBind )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
   PTEXT pKey;
   int ModVal;
   int Mod = 0;
   int KeyVal;
	PVARTEXT vt;
   PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;

   do{
      pKey = GetParam( ps, &parameters );
      if( pKey )
      {
         ModVal = FindMod( pKey );
         Mod |= ModVal;
      }
      else
         break;
   }while( ModVal );
   if( pKey )
      KeyVal = FindKey( pKey );
   else
   {
      DECLTEXT( msg, "Not enough parameters to KeyBind..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }

   if( !KeyVal )
   {
      DECLTEXT( msg, "First parameters to KeyBind were not a known key..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   else
   {
      //DECLTEXTSZ( msg, 256 );
      PMACRO pm;
      PTEXT pStroke;
      if( pmdp->Keyboard[KeyVal][Mod].flags.bMacro )
      {
         DECLTEXT( msg, "Destroyed prior key macro" );
         EnqueLink( &ps->Command->Output, &msg );
         DestroyKeyMacro( pmdp->Keyboard[KeyVal][Mod].data.macro );
      }
      if( pmdp->Keyboard[KeyVal][Mod].flags.bStroke )
      {
         DECLTEXT( msg, "Destroyed prior key stroke" );
         EnqueLink( &ps->Command->Output, &msg );
         LineRelease( pmdp->Keyboard[KeyVal][Mod].data.stroke );
      }
      {
         PTEXT pSubst;
         pSubst = MacroDuplicateEx( ps, parameters, FALSE, TRUE );
         if( pSubst )
         {
            pStroke = BuildLine( pSubst );
            LineRelease( pSubst );
         }
         else
            pStroke = NULL;
      }
      if( pStroke )
      {
         pmdp->Keyboard[KeyVal][Mod].flags.bStroke = TRUE;
         pmdp->Keyboard[KeyVal][Mod].flags.bMacro = FALSE;
         pmdp->Keyboard[KeyVal][Mod].data.stroke = pStroke;
         return 0;
      }
      vt = VarTextCreate();
      if( Mod & KEYMOD_SHIFT )
      {
         if( Mod & KEYMOD_CTRL )
            if( Mod & KEYMOD_ALT )
               vtprintf( vt, "shift-ctrl-alt-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
            else
               vtprintf( vt, "shift-ctrl-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
         else
            if( Mod & KEYMOD_ALT )
               vtprintf( vt, "shift-alt-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
            else
               vtprintf( vt, "shift-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
      }
      else
      {
         if( Mod & KEYMOD_CTRL )
            if( Mod & KEYMOD_ALT )
               vtprintf( vt, "ctrl-alt-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
            else
               vtprintf( vt, "ctrl-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
         else
            if( Mod & KEYMOD_ALT )
               vtprintf( vt, "alt-%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
            else
               vtprintf( vt, "%s"
                           ,ConsoleKeyDefs[KeyVal].name1 );
      }
      pm = (PMACRO)Allocate( sizeof( MACRO ) );
      MemSet( pm, 0, sizeof( MACRO ) );
      pm->flags.bMacro = TRUE;
      pm->pName = VarTextGet( vt );
      pm->pDescription = NULL;
      pm->nArgs = 0;
      pm->pArgs = NULL;
      ps->pRecord = pm;
      pmdp->Keyboard[KeyVal][Mod].flags.bMacro = TRUE;
      pmdp->Keyboard[KeyVal][Mod].flags.bStroke = FALSE;
      pmdp->Keyboard[KeyVal][Mod].data.macro = pm;
		VarTextDestroy( &vt );
   }
   return 0;
}
#endif
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
CORECON_NPROC( int, KeyUnBind )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
   PTEXT pKey;
   int ModVal;
   int Mod = 0;
   int KeyVal;
   PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;

   do{
      pKey = GetParam( ps, &parameters );
      if( pKey )
      {
         ModVal = FindMod( pKey );
         Mod |= ModVal;
      }
      else
         break;
   }while( ModVal );
   if( pKey )
      KeyVal = FindKey( pKey );
   else
   {
      DECLTEXT( msg, "Not enough parameters to KeyUnBind..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }

   if( !KeyVal )
   {
      DECLTEXT( msg, "First parameters to KeyUnBind were not a known key..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   else
   {
      if( pmdp->Keyboard[KeyVal][Mod].flags.bMacro )
      {
         DECLTEXT( msg, "Destroyed prior key macro" );
         EnqueLink( &ps->Command->Output, &msg );
         DestroyKeyMacro( pmdp->Keyboard[KeyVal][Mod].data.macro );
         pmdp->Keyboard[KeyVal][Mod].data.macro = NULL;
         pmdp->Keyboard[KeyVal][Mod].flags.bMacro = FALSE;

      }
      else if( pmdp->Keyboard[KeyVal][Mod].flags.bStroke )
      {
         DECLTEXT( msg, "Destroyed prior key stroke" );
         EnqueLink( &ps->Command->Output, &msg );
         LineRelease( pmdp->Keyboard[KeyVal][Mod].data.stroke );
         pmdp->Keyboard[KeyVal][Mod].data.stroke = NULL;
         pmdp->Keyboard[KeyVal][Mod].flags.bStroke = FALSE;
      }
      else
      {
         DECLTEXT( msg, "Key was not bound..." );
         EnqueLink( &ps->Command->Output, &msg );
      }
   }

   return 0;
}
#endif
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
int KeyList( PSENTIENT ps, PTEXT parameters )
{
	// somewhere I should have macro dump 
	// this would be useful for this thing also..
	// header's a little different though
	return 0;
}
#endif
//----------------------------------------------------------------------------

int Widget_DoStroke( PCHAT_LIST list, PTEXT stroke )
{
   INDEX i;
   int bOutput = FALSE;
   DECLTEXT( key, " " );
   //Log1( "Do Stroke with %c", stroke->data.data[0] );
   while( stroke )
   {
      for( i = 0; i < stroke->data.size; i++ )
      {
         switch( key.data.data[0] = stroke->data.data[i] )
         {
		 case '\n':
			 continue;
         case '\r':
            // output is always prefix linefed...
            key.data.data[0] = '\n'; // carriage return = linefeed
            goto normal_process;  // do not output return... extra lines otherwise
         case 9:
            key.data.data[0] = ' ';
         case 27:
         case '\b':
            //pdp->bUpdateToEnd = FALSE; // need update ALL not just to end
         default:
         normal_process:
            {
				PTEXT pLine;
				{
					if( ( pLine =
							GatherUserInput( list->input.CommandInfo
											, (PTEXT)&key ) ) )
					{
                     // I dunno - maybe we shouldn't enque the
                     // command... maybe we should count on whatever
                     // the destination is to echo it
                     // or - perhaps this should be another
                     // option only effective while bDirect...
					if( list->InputData )
						  list->InputData( list->psvInputData, pLine );
                  }
                  else // didn't get a linefeed... so.. safe echo
                  {
                     //Log( "Failed to Gather Line..." );
                  }
               }
            }
            bOutput = TRUE;
            break;
         }
      }
      stroke = NEXTLINE( stroke );
   }
   return bOutput;
}

//----------------------------------------------------------------------------
void Widget_WinLogicDoStroke( PCHAT_LIST list, PTEXT stroke )
{
	//EnterCriticalSec( &pdp->Lock );
	if( Widget_DoStroke( list, stroke ) )
	{
		if( !list->input.phb_Input->pBlock )
		{
			list->input.phb_Input->pBlock = list->input.phb_Input->region->pHistory.root.next;
			list->input.phb_Input->pBlock->nLinesUsed = 1;
			list->input.phb_Input->pBlock->pLines[0].flags.deleted = 0;
			list->input.phb_Input->nLine = 1;
		}
		list->input.phb_Input->pBlock->pLines[0].pLine = list->input.CommandInfo->CollectionBuffer;
		SetStart( list->input.phb_Input->pBlock->pLines[0].pLine );
		list->input.phb_Input->pBlock->pLines[0].nLineLength = LineLengthExEx( list->input.phb_Input->pBlock->pLines[0].pLine, FALSE, 8, NULL );
		//if( !pdp->flags.bDirect && pdp->flags.bWrapCommand )
		BuildDisplayInfoLines( list->input.phb_Input, list->input_font );
	}

}

//----------------------------------------------------------------------------
typedef struct penging_rectangle_tag
{
	struct {
		uint32_t bHasContent : 1;
		uint32_t bTmpRect : 1;
	} flags;
   CRITICALSECTION cs;
	int32_t x, y;
   uint32_t width, height;
} PENDING_RECT, *PPENDING_RECT;

void Widget_KeyPressHandler( PCHAT_LIST list
						  , uint8_t key_index
						  , uint8_t mod
						  , PTEXT characters
						  )
{
//cpg26dec2006 console\keydefs.c(1409): Warning! W202: Symbol 'result' has been defined, but not referenced
//cpg26dec2006    int result;
	int bOutput = 0;
	// check current keyboard override...
#ifdef programmable_keys
	if( pdp->Keyboard[key_index][mod].flags.bStroke ||
		pdp->Keyboard[key_index][mod].flags.bMacro )
	{
		if( pdp->Keyboard[key_index][mod].flags.bStroke )
		{
			extern void CPROC PSI_WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke );
			PSI_WinLogicDoStroke(pdp, pdp->Keyboard[key_index][mod].data.stroke);
			SmudgeCommon( pdp->psicon.frame );
		}
#ifdef __DEKWARE_PLUGIN__
		else if( pdp->Keyboard[key_index][mod].flags.bMacro )
		{
			if( pdp->common.Owner->pRecord != pdp->Keyboard[key_index][mod].data.macro )
				InvokeMacro( pdp->common.Owner
							  , pdp->Keyboard[key_index][mod].data.macro
							  , NULL );
		}
#endif
	}
	else // key was not overridden
#endif
	{
		int result = 0;
		//Log1( "Keyfunc = %d", KeyDefs[key_index].op[mod].bFunction );
		switch( ConsoleKeyDefs[key_index].op[mod].bFunction )
		{
		case KEYDATA_DEFINED:
			//Log( "Key data_defined" );
				Widget_WinLogicDoStroke( list, (PTEXT)ConsoleKeyDefs[key_index].op[mod].data.pStroke );
			{
				//extern void CPROC PSI_WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke );
				//PSI_WinLogicDoStroke( pp, (PTEXT)&ConsoleKeyDefs[key_index].op[mod].data.pStroke );
				//SmudgeCommon( pdp->psicon.frame );
			}
			result = UPDATE_NOTHING; // unsure about this - recently added.
			// well it would appear that the stroke results in whether to update
			// the command prompt or not.
			break;
		case KEYDATA:
			if( GetTextSize( characters ) )
			{
				//extern void CPROC PSI_WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke );
				Widget_WinLogicDoStroke( list, characters );
				//SmudgeCommon( list->psicon.frame );
			}
			result = UPDATE_NOTHING; // already taken care of?!
			break;
		case COMMANDKEY:
			result = ConsoleKeyDefs[key_index].op[mod].data.CommandKey( list, list->input.CommandInfo );
			//SmudgeCommon( pdp->psicon.frame );
			break;
		case HISTORYKEY:
			//result = ConsoleKeyDefs[key_index].op[mod].data.HistoryKey( list->pHistoryDisplay );
			break;
		case CONTROLKEY:
			//ConsoleKeyDefs[key_index].op[mod].data.ControlKey( &pdp->dwControlKeyState, TRUE );
			result = UPDATE_NOTHING;
			break;
		case SPECIALKEY:
		    list->InputPaste( list->psvInputPaste );
			//result = ConsoleKeyDefs[key_index].op[mod].data.SpecialKey( (PCONSOLE_INFO)list );
			break;
		}
		switch( result )
		{
		case UPDATE_COMMAND:
			{
				PENDING_RECT upd;
				//extern void RenderCommandLine( PCONSOLE_INFO pdp, POINTER region );
				upd.flags.bHasContent = 0;
				upd.flags.bTmpRect = 1;
				//PSI_RenderCommandLine( pdp, &upd );
				/*
				{
					RECT r;
					r.left = upd.x;
					r.right = upd.x + upd.width;
					r.top = upd.y;
					r.bottom = upd.y + upd.height;
					//pdp->Update( pdp, &r );
				}
				*/
			}

			bOutput = TRUE;
			break;
		case UPDATE_HISTORY:
			{
				/*
				extern int CPROC PSI_UpdateHistory( PCONSOLE_INFO pdp );
				if( PSI_UpdateHistory( pdp ) )
				{
					extern void CPROC PSI_RenderConsole( PCONSOLE_INFO pdp );
					PSI_RenderConsole( pdp );
				}
				*/
			}
			break;
		case UPDATE_DISPLAY:
			{
				/*
				extern void CPROC PSI_ConsoleCalculate( PCONSOLE_INFO pdp );
				PSI_ConsoleCalculate( pdp );
				*/
			}
			break;
		}
	}
}

//----------------------------------------------------------------------------

