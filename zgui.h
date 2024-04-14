#ifndef ZGUI_H
#define ZGUI_H


#ifdef __cplusplus
extern "C" {
#endif


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/chooser.h>
//#include <proto/layout.h>

#include <workbench/icon.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/chooser.h>
//#include <images/label.h>
#include <images/bitmap.h>
//#include <gadgets/space.h>

/*
#define VERSION  0
#define REVISION 2

#define DATE "("__AMIGADATE__")"
#define VERS "ZGUI 0.2"
#define VSTRING VERS" "DATE"\r\n"
#define VERSTAG "\0$VER: "VERS" "DATE
*/


enum {
 OID_MAIN = 0,
 OID_ZGLOOM_CHOOSER,
 OID_ZGLOOM,
 OID_ZGLOOM_BTN,
 OID_ZGLOOM_IMG,
 LAST_NUM
};


struct ZGloomGUI {
	struct Window *win;
	uint32 game; // 0:Gloom, 1:G_Deluxe; 2:Zombie_Ed; 3:Z_Massacre; 4:none/null/quit
};


BOOL OpenLibs(void);
void CloseLibs(void);
int32 launch_gui(void);
struct Screen *FrontMostScr(void);


#ifdef __cplusplus
}
#endif


#endif
