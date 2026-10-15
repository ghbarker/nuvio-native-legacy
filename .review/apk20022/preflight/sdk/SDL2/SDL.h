/* Local syntax-only SDK type shim; never committed or used for renders. */
#ifndef NV_LOCAL_SDL_H
#define NV_LOCAL_SDL_H
#include <stdint.h>
#include <math.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
typedef uint8_t Uint8;typedef uint16_t Uint16;typedef uint32_t Uint32;typedef uint64_t Uint64;
typedef int32_t Sint32;typedef int64_t Sint64;typedef int32_t SDL_Keycode;typedef int SDL_Scancode;
typedef enum{SDL_FALSE,SDL_TRUE}SDL_bool;
typedef struct SDL_Window SDL_Window;typedef struct SDL_Thread SDL_Thread;typedef struct SDL_mutex SDL_mutex;typedef struct SDL_cond SDL_cond;
typedef struct{int w,h,pitch;void*pixels;}SDL_Surface;
typedef struct{SDL_Scancode scancode;SDL_Keycode sym;Uint16 mod;}SDL_Keysym;
typedef Sint64 SDL_TouchID; typedef Sint64 SDL_FingerID;
typedef struct{Uint32 type,timestamp;SDL_TouchID touchId;SDL_FingerID fingerId;float x,y,dx,dy,pressure;Uint32 windowID;}SDL_TouchFingerEvent;
typedef union SDL_Event{SDL_TouchFingerEvent tfinger;struct{Uint32 type,windowID,which,state;int x,y,xrel,yrel;}motion;struct{Uint32 type,windowID,which;Uint8 button,state,clicks;int x,y;}button;struct{Uint32 type,windowID,which;int x,y;Uint32 direction;float preciseX,preciseY;int mouseX,mouseY;}wheel;Uint32 type;struct{Uint32 type,timestamp;Uint8 state,repeat;SDL_Keysym keysym;}key;struct{Uint32 type;char text[32];}text;struct{Uint32 type;Uint8 event;int data1,data2;}window;}SDL_Event;
enum{SDL_KEYDOWN=0x300,SDL_KEYUP=0x301,SDL_TEXTINPUT=0x303,SDL_QUIT=0x100,SDL_WINDOWEVENT=0x200,SDL_INIT_TIMER=1};
Uint32 SDL_GetTicks(void);int SDL_Init(Uint32);void SDL_Quit(void);void SDL_Delay(Uint32);
#define SDL_zero(x) memset(&(x),0,sizeof(x))
#define SDL_TICKS_PASSED(A,B) ((Sint32)((B)-(A)) <= 0)
#define SDL_strcmp strcmp
#define SDL_snprintf snprintf
enum{SDLK_0=48,SDLK_9=57,SDLK_AC_BACK=1002,SDLK_AUDIOPLAY=1003,SDLK_BACKSPACE=1004,SDLK_DELETE=1005,SDLK_DOWN=1006,SDLK_END=1007,SDLK_ESCAPE=1008,SDLK_F10=1009,SDLK_F5=1010,SDLK_F6=1011,SDLK_F7=1012,SDLK_F8=1013,SDLK_F9=1014,SDLK_HOME=1015,SDLK_KP_0=1016,SDLK_KP_1=1017,SDLK_KP_9=1018,SDLK_KP_ENTER=1019,SDLK_LEFT=1020,SDLK_PAGEDOWN=1021,SDLK_PAGEUP=1022,SDLK_PAUSE=1023,SDLK_RETURN=1024,SDLK_RIGHT=1025,SDLK_SPACE=1026,SDLK_TAB=1027,SDLK_UP=1028,SDLK_i=105,SDLK_a=97,SDLK_d=100,SDLK_l=108,SDLK_q=113,SDLK_r=114,SDLK_s=115,SDLK_z=122};
enum{KMOD_CTRL=1,KMOD_ALT=2,KMOD_GUI=4};
enum{SDL_MOUSEMOTION=0x400,SDL_MOUSEBUTTONDOWN=0x401,SDL_MOUSEBUTTONUP=0x402,SDL_MOUSEWHEEL=0x403,SDL_FINGERDOWN=0x700,SDL_FINGERUP=0x701,SDL_FINGERMOTION=0x702,SDL_SYSWMEVENT=0x201,SDL_TEXTEDITING=0x302,SDL_APP_WILLENTERBACKGROUND=0x105,SDL_WINDOWEVENT_RESIZED=5,SDL_WINDOWEVENT_SIZE_CHANGED=6,SDL_WINDOWEVENT_FOCUS_LOST=13,SDL_BUTTON_LEFT=1,SDL_BUTTON_RIGHT=3,SDL_MOUSEWHEEL_FLIPPED=1,SDL_QUERY=-1,SDL_PRESSED=1,SDL_RELEASED=0};
#define SDL_MOUSE_TOUCHID ((SDL_TouchID)-1)
#define SDL_TOUCH_MOUSEID ((Uint32)-1)
int SDL_strcasecmp(const char*,const char*); Uint32 SDL_GetMouseState(int*,int*);SDL_Window*SDL_GetMouseFocus(void);SDL_Window*SDL_GetWindowFromID(Uint32);SDL_Window*SDL_GL_GetCurrentWindow(void);int SDL_GetNumTouchDevices(void);void SDL_GetWindowSize(SDL_Window*,int*,int*);void SDL_GL_GetDrawableSize(SDL_Window*,int*,int*);int SDL_ShowCursor(int);
typedef struct{Uint8 major,minor,patch;}SDL_version;void SDL_GetVersion(SDL_version*);SDL_bool SDL_HasScreenKeyboardSupport(void);SDL_bool SDL_IsTextInputActive(void);void SDL_StartTextInput(void);void SDL_StopTextInput(void);
float SDL_sinf(float);
typedef int (*SDL_ThreadFunction)(void*);SDL_Thread*SDL_CreateThread(SDL_ThreadFunction,const char*,void*);void SDL_DetachThread(SDL_Thread*);void SDL_WaitThread(SDL_Thread*,int*);SDL_mutex*SDL_CreateMutex(void);int SDL_LockMutex(SDL_mutex*);int SDL_UnlockMutex(SDL_mutex*);int SDL_PushEvent(SDL_Event*);typedef int(*SDL_EventFilter)(void*,SDL_Event*);void SDL_AddEventWatch(SDL_EventFilter,void*);
#endif
