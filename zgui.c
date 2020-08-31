/*
To compile: execute zmain.c
*/

#ifdef __cplusplus
extern "C" {
#endif


#include "zgloom_rev.h"
#include "zgui.h"


BOOL OpenLibs(void)
{
	BOOL res = TRUE;

	DOSBase = IExec->OpenLibrary("dos.library", 52);

	UtilityBase = IExec->OpenLibrary("utility.library", 52);
	IUtility = (struct UtilityIFace *)IExec->GetInterface(UtilityBase, "main", 1, NULL);

	IntuitionBase = IExec->OpenLibrary("intuition.library", 52);
	IIntuition = (struct IntuitionIFace *)IExec->GetInterface(IntuitionBase, "main", 1, NULL);

	IconBase = IExec->OpenLibrary("icon.library", 52);
	IIcon = (struct IconIFace *)IExec->GetInterface(IconBase, "main", 1, NULL);

	LayoutBase = IIntuition->OpenClass("gadgets/layout.gadget", 52, &LayoutClass);
	BitMapBase = IIntuition->OpenClass("images/bitmap.image", 52, &BitMapClass);
	ButtonBase = IIntuition->OpenClass("gadgets/button.gadget", 52, &ButtonClass);
	WindowBase = IIntuition->OpenClass("window.class", 52, &WindowClass);

	//ILayout = (struct LayoutIFace *)IExec->GetInterface( (struct Library *)LayoutBase, "main", 1, NULL );
	ChooserBase = (struct Library *)IIntuition->OpenClass("gadgets/chooser.gadget", 52, &ChooserClass);
	//IChooser = (struct ChooserIFace *)IExec->GetInterface( (struct Library *)ChooserBase, "main", 1, NULL );

	if(!DOSBase || !UtilityBase || !IntuitionBase || !IconBase || !LayoutBase
	   || !BitMapBase || !ButtonBase || !WindowBase || !ChooserBase) res = FALSE;

	return(res);
}

void CloseLibs(void)
{
	IIntuition->CloseClass(BitMapBase);
	IIntuition->CloseClass(LayoutBase);
	//IIntuition->CloseClass(LabelBase);
	IIntuition->CloseClass(ButtonBase);
	IIntuition->CloseClass(WindowBase);

	//IExec->DropInterface( (struct Interface *)IChooser );
	IIntuition->CloseClass( (struct ClassLibrary *)ChooserBase );
	//IExec->DropInterface( (struct Interface *)ILayout );

	IExec->DropInterface( (struct Interface *)IIcon );
	IExec->CloseLibrary(IconBase);

	IExec->DropInterface( (struct Interface *)IIntuition );
	IExec->CloseLibrary(IntuitionBase);

	IExec->DropInterface( (struct Interface *)IUtility );
	IExec->CloseLibrary(UtilityBase);
	IExec->CloseLibrary(DOSBase);
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


BOOL ProcessGUI(struct Window *pwin)
{
	BOOL done = TRUE;
	uint16 code = 0;
	uint32 result = WMHI_LASTMSG, siggot = 0, wsigmask = 0, res_value = 0;
//	STRPTR zgloom_game_drw[] = { "Gloom", "GloomDeluxe", "ZombieEdition", "ZombieMassacre" },
	CONST_STRPTR zgloom_game_img[] = { "Gloom.png", "Gloom Deluxe.png", "ZombieEdition.png", "Zombie massacre.png" };
	char zgloom_img_path[512] = "games_images";

	IIntuition->GetAttr(WINDOW_SigMask, OBJ(OID_MAIN), &wsigmask);
	siggot = IExec->Wait(wsigmask|SIGBREAKF_CTRL_C);

	if(siggot & SIGBREAKF_CTRL_C) return(FALSE);

	while( (result=IIntuition->IDoMethod(OBJ(OID_MAIN), WM_HANDLEINPUT, &code)) != WMHI_LASTMSG )
	{
//IDOS->Printf("result=0x%lx\n",result);
	switch(result & WMHI_CLASSMASK)
	{
		case WMHI_CLOSEWINDOW:
			gloom_game = sizeof(zgloom_game_drw)/sizeof(STRPTR) - 1; // last array entry -> NULL
			done = FALSE;
		break;
		case WMHI_ICONIFY:
//IDOS->Printf("[zgui.c] WMHI_ICONIFY\n");
			if( IIntuition->IDoMethod(OBJ(OID_MAIN), WM_ICONIFY) ) {
				pwin = NULL;
			}
		break;
		case WMHI_UNICONIFY:
//IDOS->Printf("[zgui.c] WMHI_UNICONIFY\n");
			pwin = (struct Window *)IIntuition->IDoMethod(OBJ(OID_MAIN), WM_OPEN, NULL);
			/*if( (pwin=(struct Window *)IIntuition->IDoMethod(OBJ(OID_MAIN), WM_OPEN, NULL)) ) {
			}
			else {
				gloom_game = sizeof(zgloom_game_drw)/sizeof(STRPTR) - 1; // last array entry -> NULL
				done = FALSE;
			}*/
		break;
		case WMHI_GADGETUP:
//IDOS->Printf("[zgui.c] WMHI_GADGETUP\n");
			switch(result & WMHI_GADGETMASK)
			{
				case OID_ZGLOOM_CHOOSER:
//IDOS->Printf("\tOID_ZGLOOM_CHOOSER\n");
					IIntuition->GetAttr(CHOOSER_Selected, OBJ(OID_ZGLOOM_CHOOSER), &res_value);

					gloom_game = res_value;

					IUtility->Strlcpy( zgloom_img_path, zgloom_game_drw[res_value], sizeof(zgloom_img_path) );
					IDOS->AddPart( zgloom_img_path, zgloom_game_img[res_value], sizeof(zgloom_img_path) );
//IDOS->Printf("new image: '%s'\n",zgloom_img_path);
					Object *newobj = IIntuition->NewObject(BitMapClass, NULL, //"bitmap.image",
					                                       IA_Scalable,TRUE,
					                                       BITMAP_Masking,TRUE, BITMAP_Screen,pwin->WScreen,//screen,
					                                       BITMAP_SourceFile,zgloom_img_path, TAG_DONE);
//IDOS->Printf("new image: 0x%08lx\n",newobj);
					if(newobj)
					{
						IIntuition->SetGadgetAttrs(GAD(OID_ZGLOOM), pwin, NULL, BUTTON_RenderImage,newobj, TAG_DONE);
//IDOS->Printf("disposing: 0x%08lx\n",OBJ(OID_ZGLOOM_IMG));
						IIntuition->DisposeObject( OBJ(OID_ZGLOOM_IMG) );
						OBJ(OID_ZGLOOM_IMG) = newobj;
						IIntuition->RefreshGadgets(GAD(OID_ZGLOOM), pwin, NULL);
					}
				break;
				case OID_ZGLOOM:
					//IIntuition->GetAttr(CHOOSER_Selected, OBJ(OID_ZGLOOM_CHOOSER), &res_value);
					//gloom_game = res_value;
//IDOS->Printf("\tOID_ZGLOOM (0x%08lx '%s')\n",res_value,zgloom_game_drw[res_value]);
					done = FALSE;
				break;
			} // END WMHI_GADGETUP switch
		} // END switch
	} // END while( (result..

	return(done);
}


void launch_gui(void)
{
	struct MsgPort *gAppPort = NULL;
	struct Window *pwindow = NULL;
	//struct List zgloom_chooser;
	struct DiskObject *iconify = NULL;
	struct Screen *screen = FrontMostScr();
	CONST_STRPTR zgloom_chooser[] = { "Gloom", "Gloom Deluxe", "Gloom Zombie Ed.", "Gloom Zombie Massacre", NULL };

	gAppPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_END);

	// Reset icon X/Y positions so it iconifies properly on Workbench
	iconify = IIcon->GetIconTags("PROGDIR:ZGloom", ICONGETA_FailIfUnavailable,FALSE, TAG_END);
	if(iconify)
	{
		iconify->do_CurrentX = NO_ICON_POSITION;
		iconify->do_CurrentY = NO_ICON_POSITION;
	}

	// chooserlist: Action
	//make_chooser_list(&zgloom_chooser, CHS_ACTION_7, 5);
	// chooserlist: Scope

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
             //CHOOSER_Labels,   &action_chooser,
             CHOOSER_LabelArray,  zgloom_chooser,
             CHOOSER_Selected, 0,
            TAG_DONE),
            LAYOUT_AddChild, OBJ(OID_ZGLOOM) = IIntuition->NewObject(ButtonClass, NULL, //"button.gadget",
             GA_Text,      "START",
             GA_ID,        OID_ZGLOOM,
             GA_RelVerify, TRUE,
             //GA_HintInfo,  "Click to start",
             //BUTTON_BevelStyle,  BVS_THIN,
             //BUTTON_Transparent, TRUE,
             BUTTON_RenderImage, OBJ(OID_ZGLOOM_IMG) = IIntuition->NewObject(BitMapClass, NULL, //"bitmap.image",
              IA_Scalable, TRUE,
              //BITMAP_SourceFile, "games_images/Gloom.png",
              BITMAP_SourceFile, "Gloom/Gloom.png",
              BITMAP_Screen,     screen,
              BITMAP_Masking,    TRUE,
             TAG_DONE),
            TAG_DONE),
            CHILD_MinWidth,  128,
            CHILD_MinHeight, 96,
          TAG_DONE), // END of group
          //CHILD_WeightedHeight, 0,

         TAG_DONE),
	TAG_END);

	if( (pwindow=(struct Window *)IIntuition->IDoMethod(OBJ(OID_MAIN), WM_OPEN, NULL)) )
	{
		while(ProcessGUI(pwindow) == TRUE);
	}

	IIntuition->DisposeObject( OBJ(OID_MAIN) );
	OBJ(OID_MAIN) = NULL;

	//free_chooser_list(&scope_chooser);
	//free_chooser_list(&action_chooser);

	IExec->FreeSysObject(ASOT_PORT, gAppPort);
}


/*int main(int argc, char **argv)
{
	if( OpenLibs() ) {
		launch_gui();
IDOS->Printf("Launching %ld:'%s'...\n",gloom_game,zgloom_game_drw[gloom_game]);
	}

	CloseLibs();

	return(RETURN_OK);
}*/


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

//IDOS->Printf("%lx\n", (int)public_screen_address);
	return(public_screen_address);
}


#ifdef __cplusplus
}
#endif
