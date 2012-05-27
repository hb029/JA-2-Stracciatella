#ifndef __INPUT_
#define __INPUT_

#include "Types.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"

#define KEY_DOWN									0x0001
#define KEY_UP										0x0002
#define KEY_REPEAT								0x0004
#define LEFT_BUTTON_DOWN					0x0008
#define LEFT_BUTTON_UP						0x0010
#define LEFT_BUTTON_DBL_CLK				0x0020
#define LEFT_BUTTON_REPEAT				0x0040
#define RIGHT_BUTTON_DOWN					0x0080
#define RIGHT_BUTTON_UP						0x0100
#define RIGHT_BUTTON_REPEAT				0x0200
#define MOUSE_POS									0x0400
#define MOUSE_WHEEL_UP      0x0800
#define MOUSE_WHEEL_DOWN    0x1000
#define MOUSE_EVENTS        0x1FF8

#define SHIFT_DOWN								0x01
#define CTRL_DOWN									0x02
#define ALT_DOWN									0x04

#define DBL_CLK_TIME							300     // Increased by Alex, Jun-10-97, 200 felt too short
#define BUTTON_REPEAT_TIMEOUT			250
#define BUTTON_REPEAT_TIME				50

struct InputAtom
{
  UINT16 usKeyState;
  UINT16 usEvent;
  UINT32 usParam;
	wchar_t Char;
};


extern BOOLEAN			DequeueEvent(InputAtom *Event);

void MouseButtonDown(const SDL_MouseButtonEvent*);
void MouseButtonUp(const SDL_MouseButtonEvent*);

void KeyDown(const SDL_keysym*);
void KeyUp(  const SDL_keysym*);

extern void					GetMousePos(SGPPoint *Point);

extern BOOLEAN DequeueSpecificEvent(InputAtom *Event, UINT32 uiMaskFlags );

extern void					RestrictMouseToXYXY(UINT16 usX1, UINT16 usY1, UINT16 usX2, UINT16 usY2);
void RestrictMouseCursor(const SGPRect* pRectangle);
extern void					FreeMouseCursor(void);
extern BOOLEAN			IsCursorRestricted( void );
extern void					GetRestrictedClipCursor( SGPRect *pRectangle );
extern void         RestoreCursorClipRect( void );


void SimulateMouseMovement( UINT32 uiNewXPos, UINT32 uiNewYPos );

void DequeueAllKeyBoardEvents(void);


extern BOOLEAN gfKeyState[SDLK_LAST]; // TRUE = Pressed, FALSE = Not Pressed

extern UINT16    gusMouseXPos;       // X position of the mouse on screen
extern UINT16    gusMouseYPos;       // y position of the mouse on screen
extern BOOLEAN   gfLeftButtonState;  // TRUE = Pressed, FALSE = Not Pressed
extern BOOLEAN   gfRightButtonState; // TRUE = Pressed, FALSE = Not Pressed


#define _KeyDown(a)        gfKeyState[(a)]
#define _LeftButtonDown    gfLeftButtonState
#define _RightButtonDown   gfRightButtonState

#endif
