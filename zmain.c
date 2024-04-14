;/*
ppc-amigaos-gcc zmain.c -c -Wall -gstabs -D__AMIGADATE__=\"`c:date LFORMAT %d.%m.%Y`\"
ppc-amigaos-gcc zgui.c -c -Wall -gstabs -D__AMIGADATE__=\"`c:date LFORMAT %d.%m.%Y`\"
gcc zgui.o zmain.o -o ZGUI
quit
*/

#include <proto/exec.h>
#include <proto/dos.h>

//#include "zgui.h"

extern BOOL OpenLibs(void);
extern void CloseLibs(void);
extern void launch_gui(void);


extern CONST_STRPTR zgloom_game_drw[];
//extern uint32 gloom_game; // 0:Gloom, 1:G_Deluxe; 2:Zombie_Ed; 3:Z_Massacre; 4:none/quit


/************************************************************/
/* This part of code is from Andy Broad's dockapps examples */
/*STRPTR DupStr(CONST_STRPTR str, int32 length)
{
	STRPTR dup;
	uint32 str_len = 0;

	if(!str) return NULL;

	str_len = IUtility->Strlen(str);
	if(length < 0)
	{
		length = str_len;
	}

	if( (dup=IExec->AllocVecTags(length+1, AVT_ClearWithValue,0, TAG_END)) )
	{
		uint32 copy_len = str_len;

		if(length < str_len)
		{
			copy_len = length;
		}
		IUtility->MoveMem(str, dup, copy_len);
		dup[length] = '\0';
	}

	return dup;
}

VOID FreeString(STRPTR *string)
{
	if(string)
	{
		if(*string)
		{
			IExec->FreeVec( (APTR)*string );
		}
		string = NULL;
	}
}*/
/* This part of code is from Andy Broad's dockapps examples */
/************************************************************/


int main(int argc, char **argv)
{
	if( OpenLibs() ) {
		launch_gui();
	}

	CloseLibs();

	return(RETURN_OK);
}
