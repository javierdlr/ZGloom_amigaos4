/*
To compile: execute zmain.c
*/

#ifdef __cplusplus
extern "C" {
#endif


#include "zgloom_rev.h"
#include "zgui.h"


struct Library *IconBase = NULL;
//struct Library *DOSBase;
struct Library *IntuitionBase = NULL;
struct Library *UtilityBase = NULL;

struct IconIFace *IIcon = NULL;
//struct DOSIFace *IDOS = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct UtilityIFace *IUtility = NULL;

struct Library *ChooserBase = NULL;
// the class library base
struct ClassLibrary *ButtonBase = NULL, *BitMapBase = NULL, *LayoutBase = NULL, *WindowBase = NULL;
// the class pointer
Class *ButtonClass, *BitMapClass, *ChooserClass, *LayoutClass, *WindowClass;
// some interfaces needed
//struct LayoutIFace *ILayout;
//struct ChooserIFace *IChooser;


#define OBJ(x) Objects[x]
#define GAD(x) (struct Gadget *)Objects[x]
Object *Objects[LAST_NUM];

CONST_STRPTR zgloom_game_drw[] = { "Gloom", "GloomDeluxe", "ZombieEdition", "ZombieMassacre", NULL };


BOOL OpenLibs(void)
{
	//DOSBase = IExec->OpenLibrary("dos.library", 52);
	//IDOS = (struct DOSIFace *)IExec->GetInterface(DOSBase, "main", 1, NULL);

	UtilityBase = IExec->OpenLibrary("utility.library", 52);
	IUtility = (struct UtilityIFace *)IExec->GetInterface(UtilityBase, "main", 1, NULL);

	IntuitionBase = IExec->OpenLibrary("intuition.library", 52);
	IIntuition = (struct IntuitionIFace *)IExec->GetInterface(IntuitionBase, "main", 1, NULL);

	IconBase = IExec->OpenLibrary("icon.library", 52);
	IIcon = (struct IconIFace *)IExec->GetInterface(IconBase, "main", 1, NULL);

	if(IIntuition) {
		LayoutBase = IIntuition->OpenClass("gadgets/layout.gadget", 52, &LayoutClass);
		BitMapBase = IIntuition->OpenClass("images/bitmap.image", 52, &BitMapClass);
		ButtonBase = IIntuition->OpenClass("gadgets/button.gadget", 52, &ButtonClass);
		WindowBase = IIntuition->OpenClass("window.class", 52, &WindowClass);

		//ILayout = (struct LayoutIFace *)IExec->GetInterface( (struct Library *)LayoutBase, "main", 1, NULL );
		ChooserBase = (struct Library *)IIntuition->OpenClass("gadgets/chooser.gadget", 52, &ChooserClass);
		//IChooser = (struct ChooserIFace *)IExec->GetInterface( (struct Library *)ChooserBase, "main", 1, NULL );
	}

	if(!UtilityBase || !IntuitionBase || !IconBase || !LayoutBase
	   || !BitMapBase || !ButtonBase || !WindowBase || !ChooserBase) { return FALSE; }

	return TRUE;
}

void CloseLibs(void)
{
	if(IIntuition) {
		/*if(BitMapBase)*/ IIntuition->CloseClass(BitMapBase);
		BitMapBase = NULL;
		/*if(LayoutBase)*/ IIntuition->CloseClass(LayoutBase);
		LayoutBase = NULL;
		/*if(ButtonBase)*/ IIntuition->CloseClass(ButtonBase);
		ButtonBase = NULL;
		/*if(WindowBase)*/ IIntuition->CloseClass(WindowBase);
		WindowBase = NULL;

		//IExec->DropInterface( (struct Interface *)IChooser );
		//IChooser = NULL;
		/*if(ChooserBase)*/ IIntuition->CloseClass( (struct ClassLibrary *)ChooserBase );
		ChooserBase = NULL;
	}

	//IExec->DropInterface( (struct Interface *)ILayout );
	IExec->DropInterface( (struct Interface *)IIcon );
	IIcon = NULL;
	IExec->CloseLibrary(IconBase);
	IconBase = NULL;

	IExec->DropInterface( (struct Interface *)IIntuition );
	IIntuition = NULL;
	IExec->CloseLibrary(IntuitionBase);
	IntuitionBase = NULL;

	IExec->DropInterface( (struct Interface *)IUtility );
	IUtility = NULL;
	IExec->CloseLibrary(UtilityBase);
	UtilityBase = NULL;

	//IExec->CloseLibrary(DOSBase);
	//DOSBase = NULL;
}


/*BOOL make_chooser_list(struct List *list, int32 str_num, int32 index)
{
	struct Node *node;
	int32 j;

	IExec->NewList(list);
	for(j=0; j<index; j++)
	{
		node = IChooser->AllocChooserNode(CNA_CopyText, TRUE,
		                                  CNA_Text, GetString(&li, str_num+j),
		                                 TAG_DONE);
		if(node) IExec->AddTail(list, node);
		else return(FALSE);
	}

	return(TRUE);
}*/

/*void free_chooser_list(struct List *list)
{
	struct Node *node, *nextnode;

	node = list->lh_Head;
	while( (nextnode=node->ln_Succ) )
	{
		IChooser->FreeChooserNode(node);
		node = nextnode;
	}

	IExec->NewList(list);
}*/

void UpdateImageButton(struct ZGloomGUI *zgg)
{
	//APTR context;
	BPTR res_lock;
	BOOL dir_flag = FALSE;
	char zgloom_game_img[32], zgloom_img_path[512] = "games_images";
	struct Screen *screen = FrontMostScr();

	// "Build" the image filename
	IUtility->Strlcpy( zgloom_game_img, zgloom_game_drw[zgg->game], sizeof(zgloom_game_img) );	
	IUtility->Strlcat( zgloom_game_img, ".png", sizeof(zgloom_game_img) );
#if 1
	dir_flag = TRUE;
	// Lazy check to know Gloom release exists/available
	IUtility->Strlcpy( zgloom_img_path, zgloom_game_drw[zgg->game], sizeof(zgloom_img_path) );
	IDOS->AddPart( zgloom_img_path, "txts", sizeof(zgloom_img_path) );
	//context = IDOS->ObtainDirContextTags(EX_StringNameInput,zgloom_img_path, TAG_END);
	if( (res_lock=IDOS->Lock(zgloom_img_path, SHARED_LOCK)) != ZERO )
	{
		IDOS->UnLock(res_lock);
		dir_flag = FALSE;
	}

		IUtility->Strlcpy( zgloom_img_path, zgloom_game_drw[zgg->game], sizeof(zgloom_img_path) );
#endif
	IDOS->AddPart( zgloom_img_path, zgloom_game_img, sizeof(zgloom_img_path) );
//IExec->DebugPrintF("new image: '%s'\n",zgloom_img_path);

	// Dispose "previous" image
	IIntuition->DisposeObject( OBJ(OID_ZGLOOM_IMG) );
	OBJ(OID_ZGLOOM_IMG) = NULL;

	IIntuition->IDoMethod( OBJ(OID_ZGLOOM), LM_REMOVECHILD, zgg->win, OBJ(OID_ZGLOOM_BTN) );

	OBJ(OID_ZGLOOM_BTN) = IIntuition->NewObject(ButtonClass, NULL, //"button.gadget",
	  GA_Text,      "START",
	  GA_ID,        OID_ZGLOOM_BTN,
	  GA_RelVerify, TRUE,
	  //GA_Disabled,  context? FALSE:TRUE,
	  GA_Disabled,  dir_flag,
	  BUTTON_RenderImage, OBJ(OID_ZGLOOM_IMG) = IIntuition->NewObject(BitMapClass, NULL, //"bitmap.image",
	   IA_Scalable, TRUE,
	   BITMAP_SourceFile, zgloom_img_path,
	   BITMAP_Screen,     screen,
	   BITMAP_Masking,    TRUE,
	  TAG_DONE),
	TAG_DONE);

	IIntuition->IDoMethod(OBJ(OID_ZGLOOM), LM_ADDCHILD, zgg->win, OBJ(OID_ZGLOOM_BTN), NULL);
	IIntuition->IDoMethod(OBJ(OID_MAIN), WM_RETHINK);

	//IDOS->ReleaseDirContext(context);
}


BOOL ProcessGUI(struct ZGloomGUI *zgg)
{
	BOOL done = TRUE;
	uint16 code = 0;
	uint32 result = WMHI_LASTMSG, siggot = 0, wsigmask = 0, res_value = 0;

	IIntuition->GetAttr(WINDOW_SigMask, OBJ(OID_MAIN), &wsigmask);
	siggot = IExec->Wait(wsigmask|SIGBREAKF_CTRL_C);

	if(siggot & SIGBREAKF_CTRL_C) return(FALSE);

	while( (result=IIntuition->IDoMethod(OBJ(OID_MAIN), WM_HANDLEINPUT, &code)) != WMHI_LASTMSG )
	{
//IExec->DebugPrintF("result=0x%lx\n",result);
	switch(result & WMHI_CLASSMASK)
	{
		case WMHI_CLOSEWINDOW:
			zgg->game = sizeof(zgloom_game_drw)/sizeof(STRPTR) - 1; // last array value -> NULL
//IExec->DebugPrintF("zgg->game = %ld\n",zgg->game);
			done = FALSE;
		break;
		case WMHI_ICONIFY:
//IExec->DebugPrintF("[zgui.c] WMHI_ICONIFY\n");
			IIntuition->IDoMethod(OBJ(OID_MAIN), WM_ICONIFY);
			zgg->win = NULL;
//IExec->DebugPrintF("\t0x%08lx\n",zgg->win);
		break;
		case WMHI_UNICONIFY:
//IExec->DebugPrintF("[zgui.c] WMHI_UNICONIFY\n");
			zgg->win = (struct Window *)IIntuition->IDoMethod(OBJ(OID_MAIN), WM_OPEN, NULL);
//IExec->DebugPrintF("\t0x%08lx\n",zgg->win);
		break;
		case WMHI_GADGETUP:
//IExec->DebugPrintF("[zgui.c] WMHI_GADGETUP\n");
			switch(result & WMHI_GADGETMASK)
			{
				case OID_ZGLOOM_CHOOSER:
//IExec->DebugPrintF("\tOID_ZGLOOM_CHOOSER\n");
					IIntuition->GetAttr(CHOOSER_Selected, OBJ(OID_ZGLOOM_CHOOSER), &res_value);

					zgg->game = res_value;

					UpdateImageButton(zgg);
				break;
				case OID_ZGLOOM_BTN:
					//IIntuition->GetAttr(CHOOSER_Selected, OBJ(OID_ZGLOOM_CHOOSER), &res_value);
					//gloom_game = res_value;
//IExec->DebugPrintF("\tOID_ZGLOOM_BTN (0x%08lx '%s')\n",zgg->game,zgloom_game_drw[zgg->game]);
					done = FALSE;
				break;
			} // END WMHI_GADGETUP switch
		} // END switch
	} // END while( (result..

	return(done);
}


int32 launch_gui(void)
{
	struct MsgPort *gAppPort = NULL;
	//struct List zgloom_chooser;
	struct DiskObject *iconify = NULL;
	CONST_STRPTR zgloom_chooser[] = { "Gloom", "Gloom Deluxe", "Gloom Zombie Edition", "Gloom Zombie Massacre", NULL };
	uint32 res = sizeof(zgloom_game_drw)/sizeof(STRPTR) - 1; // last array value -> NULL
	struct ZGloomGUI *ZGG = IExec->AllocVecTags(sizeof(struct ZGloomGUI), AVT_ClearWithValue,NULL, TAG_END);

	gAppPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_END);

	// Reset icon X/Y positions so it iconifies properly on Workbench
	iconify = IIcon->GetIconTags("PROGDIR:ZGloom", ICONGETA_FailIfUnavailable,FALSE, TAG_END);
	if(iconify)
	{
		iconify->do_CurrentX = NO_ICON_POSITION;
		iconify->do_CurrentY = NO_ICON_POSITION;
	}

	//make_chooser_list(&zgloom_chooser, CHS_ACTION_7, 5);

	OBJ(OID_MAIN) = IIntuition->NewObject(WindowClass, NULL, //"window.class",
		WA_ScreenTitle, VERS " " DATE,
		WA_Title,       "ZGloomStart GUI",
		WA_DragBar,     TRUE,
		WA_CloseGadget, TRUE,
		WA_SizeGadget,  TRUE,
		WA_DepthGadget, TRUE,
		WA_Activate,    TRUE,
		WINDOW_AppPort,       gAppPort,
		WINDOW_IconifyGadget, TRUE,
		WINDOW_Icon,          iconify,
		WINDOW_Position,      WPOS_CENTERSCREEN,
		WINDOW_PopupGadget,   TRUE,
		//WINDOW_JumpScreensMenu, TRUE,
		//WINDOW_LockHeight,    TRUE,
		WINDOW_GadgetHelp,    TRUE,
		WINDOW_Layout, IIntuition->NewObject(LayoutClass, NULL, //"layout.gadget",
         LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
         LAYOUT_SpaceOuter,  TRUE,
// GROUP
         LAYOUT_AddChild, IIntuition->NewObject(LayoutClass, NULL, //"layout.gadget",
           LAYOUT_Orientation,    LAYOUT_ORIENT_VERT,
           //LAYOUT_HorizAlignment, LALIGN_CENTER,
           //LAYOUT_SpaceOuter,     TRUE,
           //LAYOUT_BevelStyle, BVS_SBAR_VERT,//BVS_GROUP,
           //LAYOUT_Label,      "Select Gloom Game",
           LAYOUT_AddChild, OBJ(OID_ZGLOOM_CHOOSER) = IIntuition->NewObject(ChooserClass, NULL, //"chooser.gadget",
             GA_ID,         OID_ZGLOOM_CHOOSER,
             GA_RelVerify,  TRUE,
             GA_Underscore, 0,
             //GA_HintInfo,   "Select Gloom game",
             CHOOSER_LabelArray,  zgloom_chooser,
             CHOOSER_Selected, 0,
           TAG_DONE),
// BUTTON/IMAGE GROUP
           LAYOUT_AddChild, OBJ(OID_ZGLOOM) = IIntuition->NewObject(LayoutClass, NULL, //"layout.gadget",
           TAG_DONE), // END of BUTTON/IMAGE GROUP
           CHILD_MinWidth,  128,
           CHILD_MinHeight, 96,
         TAG_DONE), // END of GROUP
         //CHILD_WeightedHeight, 0,
        TAG_DONE),
	TAG_END);

	if( (ZGG->win=(struct Window *)IIntuition->IDoMethod(OBJ(OID_MAIN), WM_OPEN, NULL)) )
	{
		UpdateImageButton(ZGG);
		while(ProcessGUI(ZGG) == TRUE);
		res = ZGG->game;
	}
	//IDOS->Printf("Launching %ld: '%s'...\n",ZGG->game,zgloom_game_drw[res]);

	IIntuition->DisposeObject( OBJ(OID_ZGLOOM_IMG) );
	//OBJ(OID_ZGLOOM_IMG) = NULL;
	IIntuition->DisposeObject( OBJ(OID_MAIN) );
	//OBJ(OID_MAIN) = NULL;

	//free_chooser_list(&zgloom_chooser);

	IExec->FreeSysObject(ASOT_PORT, gAppPort);

	IExec->FreeVec(ZGG);
	return res;
}


/* Get screen at front */
struct Screen *FrontMostScr(void)
{
	ULONG intuition_lock;
	struct Screen *front_screen_address, *public_screen_address = NULL;
	struct List *public_screen_list;
	struct PubScreenNode *public_screen_node;

	intuition_lock = IIntuition->LockIBase(0L);

	front_screen_address = ((struct IntuitionBase *)IntuitionBase)->FirstScreen;
	if( (front_screen_address->Flags & PUBLICSCREEN) || (front_screen_address->Flags & WBENCHSCREEN) ) {
		 IIntuition->UnlockIBase(intuition_lock);

		public_screen_list = IIntuition->LockPubScreenList();
		public_screen_node = (struct PubScreenNode *)public_screen_list->lh_Head;
		while(public_screen_node) {
			if(public_screen_node->psn_Screen == front_screen_address) {
				public_screen_address = public_screen_node->psn_Screen;
				break;
			}

			public_screen_node = (struct PubScreenNode *)public_screen_node->psn_Node.ln_Succ;
		}

		IIntuition->UnlockPubScreenList();
	}
	else
		IIntuition->UnlockIBase(intuition_lock);

	if(!public_screen_address) {
	public_screen_address = IIntuition->LockPubScreen(NULL);
	IIntuition->UnlockPubScreen(NULL, public_screen_address);
	}

//IExec->DebugPrintF("0x%lx\n", (int)public_screen_address);
	return(public_screen_address);
}


#ifdef __cplusplus
}
#endif
