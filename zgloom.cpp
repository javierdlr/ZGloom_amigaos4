// zgloom.cpp : Defines the entry point for the console application.
//

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "xmp/include/xmp.h"

#include "config.h"
#include "gloommap.h"
#include "script.h"
#include "crmfile.h"
#include "iffhandler.h"
#include "renderer.h"
#include "objectgraphics.h"
#include <iostream>
#include "gamelogic.h"
#include "soundhandler.h"
#include "font.h"
#include "titlescreen.h"
#include "menuscreen.h"
#include "hud.h"

#ifdef __amigaos4__
	//#include <SDL2/SDL_image.h> // to save as PNG
	#include "amigaos4.h"

	static const char version[] = VERSTAG;
	static const char stack[] = "$STACK:200000";

// zgui.c
extern "C" {
	extern BOOL OpenLibs(void);
	extern void CloseLibs(void);
	extern void launch_gui(void);

	extern STRPTR zgloom_game_drw[];
	extern uint32 gloom_game; // 0:Gloom, 1:G_Deluxe; 2:Zombie_Ed; 3:Z_Massacre; 4:none/quit
}
#endif

Uint32 my_callbackfunc(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;

	/* In this example, our callback pushes an SDL_USEREVENT event
	into the queue, and causes our callback to be called again at the
	same interval: */

	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = NULL;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	SDL_PushEvent(&event);
	return(interval);
}

#ifndef _NOMUSIC_
static void fill_audio(void *udata, Uint8 *stream, int len)
{
	auto res = xmp_play_buffer((xmp_context)udata, stream, len, 0);
}
#endif

void LoadPic(std::string name, SDL_Surface* render8)
{
	std::vector<uint8_t> pic;
	CrmFile picfile;
	CrmFile palfile;

	picfile.Load(name.c_str());
	palfile.Load((name+".pal").c_str());

	SDL_FillRect(render8, nullptr, 0);

	// is this some sort of weird AGA/ECS backwards compatible palette encoding? 4 MSBs, then LSBs?
	// Update: Yes, yes it is. 
	for (uint32_t c = 0; c < palfile.size / 4; c++)
	{
		SDL_Color col;
		col.a = 0xFF;
		col.r = palfile.data[c * 4 + 0] & 0xf;
		col.g = palfile.data[c * 4 + 1] >> 4;
		col.b = palfile.data[c * 4 + 1] & 0xF;

		col.r <<= 4;
		col.g <<= 4;
		col.b <<= 4;

		col.r |= palfile.data[c * 4 + 2] & 0xf;
		col.g |= palfile.data[c * 4 + 3] >> 4;
		col.b |= palfile.data[c * 4 + 3] & 0xF;

		SDL_SetPaletteColors(render8->format->palette, &col, c, 1);
	}

	uint32_t width = 0;

	IffHandler::DecodeIff(picfile.data, pic, width);

	if (width == render8->w)
	{
		if (pic.size() > (size_t)(render8->w * render8->h))
		{
			pic.resize(render8->w * render8->h);
		}
		std::copy(pic.begin(), pic.begin() + pic.size(), (uint8_t*)(render8->pixels));
	}
	else
	{
		// gloom 3 has some odd-sized intermission pictures. Do a line-by-line copy.

		uint32_t p = 0;
		uint32_t y = 0;

		if (pic.size() > (width * render8->h))
		{
			pic.resize(width * render8->h);
		}

		while (p < pic.size())
		{
			std::copy(pic.begin() + p, pic.begin() + p + render8->w, (uint8_t*)(render8->pixels) + y*render8->pitch);

			p += width;
			y++;
		}
	}
}

enum GameState
{
	STATE_PLAYING,
	STATE_PARSING,
	STATE_SPOOLING,
	STATE_WAITING,
	STATE_MENU,
	STATE_TITLE
};


int main(void)//int argc, char* argv[])
{
#ifdef __amigaos4__
	BPTR newdir = ZERO, olddir = ZERO;

	olddir = IDOS->GetCurrentDir();

	if( OpenLibs() == FALSE)
	{
		IDOS->PutErrStr("Missing classes/libs to show ZGloomStart GUI!");
		CloseLibs();
		return 1;
	}

	launch_gui();
//IDOS->Printf("Launching %ld:'%s'...\n",gloom_game,zgloom_game_drw[gloom_game]);
	if(zgloom_game_drw[gloom_game] == NULL)
	{
		CloseLibs();
		return 1;
	}

	newdir = IDOS->Lock(zgloom_game_drw[gloom_game], SHARED_LOCK);
	//if(newdir != ZERO) IDOS->SetCurrentDir(newdir);
	IDOS->SetCurrentDir(newdir);
#endif

	/* AUTODETECT ZM FIRST!*/
	if (FILE* file = fopen("stuf/stages", "r"))
	{
		fclose(file);
		Config::SetZM(true);
	}

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
	{
#ifdef __amigaos4__
		IDOS->SetCurrentDir(olddir);
		IDOS->UnLock(newdir);
#endif
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

#ifdef __amigaos4__
	SDL_GameControllerAddMappingsFromFile("PROGDIR:gamecontrollerdb.txt");
#endif

	// SDL needs to be initiated before this to pick up gamepad
	Config::Init();

	GloomMap gmap;
	Script script;
	TitleScreen titlescreen;
	MenuScreen menuscreen;
	GameState state = STATE_TITLE;

#ifndef _NOMUSIC_
	xmp_context ctx;

	ctx = xmp_create_context();
	Config::RegisterMusContext(ctx);
#endif

	int renderwidth, renderheight, windowwidth, windowheight;

	Config::GetRenderSizes(renderwidth, renderheight, windowwidth, windowheight);

	CrmFile titlemusic;
	CrmFile intermissionmusic;
	CrmFile ingamemusic;
	CrmFile titlepic;

	titlemusic.Load(Config::GetMusicFilename(0).c_str());
	intermissionmusic.Load(Config::GetMusicFilename(1).c_str());

	SoundHandler::Init();

#ifdef __amigaos4__
	SDL_Window* win = SDL_CreateWindow(VERS " " DATE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowwidth, windowheight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | (Config::GetFullscreen()?SDL_WINDOW_FULLSCREEN:0) );
#else
	SDL_Window* win = SDL_CreateWindow("ZGloom", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowwidth, windowheight, SDL_WINDOW_SHOWN | (Config::GetFullscreen()?SDL_WINDOW_FULLSCREEN:0) );
#endif
	if (win == nullptr)
	{
#ifdef __amigaos4__
		SDL_Quit();
		IDOS->SetCurrentDir(olddir);
		IDOS->UnLock(newdir);
#endif
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		CloseLibs();
		return 1;
	}

	Config::RegisterWin(win);

	SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | (Config::GetVSync()?SDL_RENDERER_PRESENTVSYNC:0));
	if (ren == nullptr)
	{
#ifdef __amigaos4__
		SDL_DestroyWindow(win);
		SDL_Quit();
		IDOS->SetCurrentDir(olddir);
		IDOS->UnLock(newdir);
#endif
		std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		CloseLibs();
		return 1;
	}

//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");//"nearest", "linear", "best") // 0,1,2

	SDL_Texture* rendertex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, renderwidth, renderheight);
	if (rendertex == nullptr)
	{
#ifdef __amigaos4__
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		SDL_Quit();
		IDOS->SetCurrentDir(olddir);
		IDOS->UnLock(newdir);
#endif
		std::cout << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
		CloseLibs();
		return 1;
	}

	SDL_ShowCursor(SDL_DISABLE);

	SDL_Surface* render8 = SDL_CreateRGBSurface(0, 320, 256, 8, 0, 0, 0, 0);
	SDL_Surface* intermissionscreen = SDL_CreateRGBSurface(0, 320, 256, 8, 0, 0, 0, 0);
	SDL_Surface* titlebitmap = SDL_CreateRGBSurface(0, 320, 256, 8, 0, 0, 0, 0);
	SDL_Surface* render32 = SDL_CreateRGBSurface(0, renderwidth, renderheight, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	SDL_Surface* screen32 = SDL_CreateRGBSurface(0, 320, 256, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	ObjectGraphics objgraphics;
	Renderer renderer;
	GameLogic logic;
	Camera cam;
	Hud hud;

	logic.Init(&objgraphics);
	SDL_AddTimer(1000 / 25, my_callbackfunc, NULL);

	SDL_Event sEvent;

	bool notdone = true;

#if 1
	Font smallfont, bigfont;
	CrmFile fontfile;
	fontfile.Load((Config::GetMiscDir() + "bigfont2.bin").c_str());
	if (fontfile.data)
	{
		bigfont.Load2(fontfile);
		smallfont.Load2(fontfile);
	}
	else
	{
		fontfile.Load((Config::GetMiscDir() + "smallfont.bin").c_str());
		if (fontfile.data)smallfont.Load(fontfile);
		fontfile.Load((Config::GetMiscDir() + "bigfont.bin").c_str());
		if (fontfile.data)bigfont.Load(fontfile);
	}
#endif

	titlepic.Load((Config::GetPicsDir() + "title").c_str());

	if (titlepic.data)
	{
		LoadPic(Config::GetPicsDir() + "title", titlebitmap);
	}
	else
	{
		LoadPic(Config::GetPicsDir() + "blackmagic", titlebitmap);
	}

#ifndef _NOMUSIC_
	if (titlemusic.data)
	{
		if (xmp_load_module_from_memory(ctx, titlemusic.data, titlemusic.size))
		{
			std::cout << "music error";
		}

		if (xmp_start_player(ctx, 22050, 0))
		{
			std::cout << "music error";
		}
		Mix_HookMusic(fill_audio, ctx);
		Config::SetMusicVol(Config::GetMusicVol());
	}
#endif

	std::string intermissiontext;

	bool intermissionmusplaying = false;
	bool haveingamemusic = false;
	bool printscreen = false;
	int screennum = 0;
	uint32_t fps = 0;
	uint32_t fpscounter = 0;

	Mix_Volume(-1, Config::GetSFXVol()*12);
	Mix_VolumeMusic(Config::GetMusicVol() * 12);

	//try and blit title etc into the middle of the screen
	SDL_Rect blitrect;

	int screenscale = renderheight / 256;
	blitrect.w = 320 * screenscale;
	blitrect.h = 256 * screenscale;
	blitrect.x = (renderwidth - 320 * screenscale) / 2;
	blitrect.y = (renderheight - 256 * screenscale) / 2;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	//set up the level select

	std::vector<std::string> levelnames;
	script.GetLevelNames(levelnames);
	titlescreen.SetLevels(levelnames);
	int levelselect = 0;

	while (notdone)
	{
		if ((state == STATE_PARSING) || (state == STATE_SPOOLING))
		{
			std::string scriptstring;
			Script::ScriptOp sop;

			sop = script.NextLine(scriptstring);

			switch (sop)
			{
				case Script::SOP_SETPICT:
				{
					scriptstring.insert(0, Config::GetPicsDir());
					LoadPic(scriptstring, intermissionscreen);
					SDL_SetPaletteColors(render8->format->palette, intermissionscreen->format->palette->colors, 0, 256);
					break;
				}
				case Script::SOP_SONG:
				{
					scriptstring.insert(0, Config::GetMusicDir());
					ingamemusic.Load(scriptstring.c_str());
					haveingamemusic = (ingamemusic.data != nullptr);
					break;
				}
				case Script::SOP_LOADFLAT:
				{
					//improve this, only supports 9 flats
					gmap.SetFlat(scriptstring[0] - '0');
					break;
				}
				case Script::SOP_TEXT:
				{
					 intermissiontext = scriptstring;

					 if (state == STATE_SPOOLING)
					 {
						 if (intermissiontext == levelnames[levelselect])
						 {
							 // level selector
#ifndef _NOMUSIC_
							 if (intermissionmusic.data)
							 {
								 if (xmp_load_module_from_memory(ctx, intermissionmusic.data, intermissionmusic.size))
								 {
									 std::cout << "music error";
								 }

								 if (xmp_start_player(ctx, 22050, 0))
								 {
									 std::cout << "music error";
								 }
								 Mix_HookMusic(fill_audio, ctx);
								 Config::SetMusicVol(Config::GetMusicVol());
								 intermissionmusplaying = true;
							 }
#endif

							 state = STATE_PARSING;
						 }
					 }
					 break;
				}
				case Script::SOP_DRAW:
				{
					if (state == STATE_PARSING)
					{
#ifndef _NOMUSIC_
						if (intermissionmusic.data)
						{
							if (xmp_load_module_from_memory(ctx, intermissionmusic.data, intermissionmusic.size))
							{
								std::cout << "music error";
							}

							if (xmp_start_player(ctx, 22050, 0))
							{
								std::cout << "music error";
							}
							Mix_HookMusic(fill_audio, ctx);
							Config::SetMusicVol(Config::GetMusicVol());
							intermissionmusplaying = true;
						}
#endif
					}
					break;
				}
				case Script::SOP_WAIT:
				{
					if (state == STATE_PARSING)
					{
						state = STATE_WAITING;

						SDL_SetPaletteColors(render8->format->palette, smallfont.GetPalette()->colors, 0, 16);
						SDL_BlitSurface(intermissionscreen, NULL, render8, NULL);
						smallfont.PrintMultiLineMessage(intermissiontext, 220, render8);
					}
					break;
				}
				case Script::SOP_PLAY:
				{
					if (state == STATE_PARSING)
					{

						cam.x.SetInt(0);
						cam.y = 120;
						cam.z.SetInt(0);
						cam.rotquick.SetInt(0);
						scriptstring.insert(0, Config::GetLevelDir());
						gmap.Load(scriptstring.c_str(), &objgraphics);
						//gmap.Load("maps/map1_4", &objgraphics);
						renderer.Init(render32, &gmap, &objgraphics);
						logic.InitLevel(&gmap, &cam, &objgraphics);
						state = STATE_PLAYING;

#ifndef _NOMUSIC_
						if (haveingamemusic)
						{
							if (xmp_load_module_from_memory(ctx, ingamemusic.data, ingamemusic.size))
							{
								std::cout << "music error";
							}

							if (xmp_start_player(ctx, 22050, 0))
							{
								std::cout << "music error";
							}
							Mix_HookMusic(fill_audio, ctx);
							Config::SetMusicVol(Config::GetMusicVol());
						}
#endif
					}
					break;
				}
				case Script::SOP_END:
				{
					state = STATE_TITLE;
#ifndef _NOMUSIC_
					if (intermissionmusic.data && intermissionmusplaying)
					{
						Mix_HookMusic(nullptr, nullptr);
						xmp_end_player(ctx);
						xmp_release_module(ctx);
						intermissionmusplaying = false;
					}
					if (titlemusic.data)
					{
						if (xmp_load_module_from_memory(ctx, titlemusic.data, titlemusic.size))
						{
							std::cout << "music error";
						}

						if (xmp_start_player(ctx, 22050, 0))
						{
							std::cout << "music error";
						}
						Mix_HookMusic(fill_audio, ctx);
						Config::SetMusicVol(Config::GetMusicVol());
					}
#endif
					break;
				}
			}
		}

		if (state == STATE_TITLE)
		{
			SDL_SetPaletteColors(render8->format->palette, titlebitmap->format->palette->colors, 0, 256);
			titlescreen.Render(titlebitmap, render8, smallfont);
		}

		while ((state!= STATE_SPOOLING) && SDL_PollEvent(&sEvent))
		{
			if (sEvent.type == SDL_WINDOWEVENT)
			{
				if (sEvent.window.event == SDL_WINDOWEVENT_CLOSE)
				{
					notdone = false;
				}
#ifdef __amigaos4__
				if (sEvent.window.event == SDL_WINDOWEVENT_MINIMIZED)
				{
//printf("Iconified\n");
					if(state == STATE_PLAYING) {
						sEvent.type = SDL_KEYDOWN;
						state = STATE_MENU;
						//state = STATE_WAITING;
					}
				}
				/*if (sEvent.window.event == SDL_WINDOWEVENT_RESTORED)
				{
printf("Uniconified\n");
				}*/
#endif
			}
			if (Config::HaveController() && (sEvent.type == SDL_CONTROLLERBUTTONDOWN))
			{
//printf("sEvent.type=0x%08x\n",sEvent.type);
				//fake up a key event
				if ((state == STATE_TITLE) || (state == STATE_MENU) || (state == STATE_WAITING))
				{
					if (Config::GetControllerFire())
					{
						sEvent.type = SDL_KEYDOWN;
						sEvent.key.keysym.sym = SDLK_SPACE;
					}
					if (Config::GetControllerUp())
					{
						sEvent.type = SDL_KEYDOWN;
						sEvent.key.keysym.sym = SDLK_UP;
					}
					if (Config::GetControllerDown())
					{
						sEvent.type = SDL_KEYDOWN;
						sEvent.key.keysym.sym = SDLK_DOWN;
					}
				}

				if (state == STATE_PLAYING)
				{
					// call up menu
					if (Config::GetControllerStart())
					{
						sEvent.type = SDL_KEYDOWN;
						sEvent.key.keysym.sym = SDLK_ESCAPE;
					}
					if (Config::GetControllerBack())
					{
						sEvent.type = SDL_KEYDOWN;
						sEvent.key.keysym.sym = SDLK_TAB;
					}
				}

			}

			if ((sEvent.type == SDL_KEYDOWN) && (sEvent.key.keysym.sym == SDLK_SPACE ||
				sEvent.key.keysym.sym == SDLK_RETURN ||
				sEvent.key.keysym.sym == SDLK_LCTRL))
			{
				if (state == STATE_WAITING)
				{
					state = STATE_PARSING;
#ifndef _NOMUSIC_
					if (intermissionmusic.data)
					{
						Mix_HookMusic(nullptr, nullptr);
						xmp_end_player(ctx);
						xmp_release_module(ctx);
						intermissionmusplaying = false;
					}
#endif
				}
			}

			if (sEvent.type == SDL_KEYDOWN)
			{
				if (state == STATE_TITLE)
				{
					switch (titlescreen.Update(sEvent, levelselect))
					{
						case TitleScreen::TITLERET_PLAY:
							state = STATE_PARSING;
							logic.Init(&objgraphics);
#ifndef _NOMUSIC_
							if (titlemusic.data)
							{
								Mix_HookMusic(nullptr, nullptr);
								xmp_end_player(ctx);
								xmp_release_module(ctx);
							}
#endif
							break;
						case TitleScreen::TITLERET_SELECT:
							state = STATE_SPOOLING;
							logic.Init(&objgraphics);
#ifndef _NOMUSIC_
							if (titlemusic.data)
							{
								Mix_HookMusic(nullptr, nullptr);
								xmp_end_player(ctx);
								xmp_release_module(ctx);
							}
#endif
							break;
						case TitleScreen::TITLERET_QUIT:
							notdone = false;
							break;
						default:
							break;
					}
				}
				if (state == STATE_MENU)
				{
					switch (menuscreen.Update(sEvent))
					{
						case MenuScreen::MENURET_PLAY:
							state = STATE_PLAYING;
							break;
						case MenuScreen::MENURET_QUIT:
							script.Reset();
							state = STATE_TITLE;
#ifndef _NOMUSIC_
							if (titlemusic.data)
							{
								if (xmp_load_module_from_memory(ctx, titlemusic.data, titlemusic.size))
								{
									std::cout << "music error";
								}

								if (xmp_start_player(ctx, 22050, 0))
								{
									std::cout << "music error";
								}
								Mix_HookMusic(fill_audio, ctx);
								Config::SetMusicVol(Config::GetMusicVol());
							}
#endif
							break;
						default:
							break;
					}
				}
				if ((state == STATE_PLAYING) && (sEvent.key.keysym.sym == SDLK_ESCAPE))
				{
					state = STATE_MENU;
				}
			}

			if ((sEvent.type == SDL_KEYDOWN) && sEvent.key.keysym.sym == SDLK_F12)
			{
				Config::SetFullscreen(!Config::GetFullscreen());
			}

			if ((sEvent.type == SDL_KEYDOWN) && sEvent.key.keysym.sym == SDLK_TAB)
			{
				Config::SetDebug(!Config::GetDebug());
			}

#ifdef __amigaos4__
			if ((sEvent.type == SDL_KEYDOWN) && sEvent.key.keysym.sym == SDLK_SYSREQ)
#else
			if ((sEvent.type == SDL_KEYDOWN) && sEvent.key.keysym.sym == SDLK_PRINTSCREEN)
#endif
			{
				printscreen = true;
			}

			if (sEvent.type == SDL_USEREVENT)
			{
				if (state == STATE_PLAYING)
				{
					if (logic.Update(&cam))
					{
#ifndef _NOMUSIC_
						if (haveingamemusic)
						{
							Mix_HookMusic(nullptr, nullptr);
							xmp_end_player(ctx);
							xmp_release_module(ctx);
							intermissionmusplaying = false;
						}
#endif
						state = STATE_PARSING;
					}
				}
				if (state == STATE_TITLE)
				{
					titlescreen.Clock();
				}
				if (state == STATE_MENU)
				{
					menuscreen.Clock();
				}

				fpscounter++;

				if (fpscounter >= 25)
				{
					Config::SetFPS(fps);
					fpscounter = 0;
					fps = 0;
				}
			}
		}

		SDL_FillRect(render32, NULL, 0);

		if (state == STATE_PLAYING)
		{
			renderer.SetTeleEffect(logic.GetTeleEffect());
			renderer.SetPlayerHit(logic.GetPlayerHit());
			renderer.SetThermo(logic.GetThermo());

			//cam.x.SetInt(3969);
			//cam.z.SetInt(5359);
			//cam.rotquick.SetInt(254);
			renderer.Render(&cam);
			MapObject pobj = logic.GetPlayerObj();
			hud.Render(render32, pobj, smallfont);
			fps++;
		}
		if (state == STATE_MENU)
		{
			renderer.Render(&cam);
			menuscreen.Render(render32, render32, smallfont);
		}

		if ((state == STATE_WAITING) || (state == STATE_TITLE))
		{
			// SDL does not seem to like scaled 8->32 copy?
			SDL_BlitSurface(render8, NULL, screen32, NULL);
			SDL_BlitScaled(screen32, NULL, render32, &blitrect);
		}

		if (printscreen)
		{
			std::string filename("img");

			filename += std::to_string(screennum);
//#ifdef __amigaos4__
//			filename += ".png";
//#else
			filename += ".bmp";
//#endif
			screennum++;

#ifdef __amigaos4__
			SDL_SetHint(SDL_HINT_BMP_SAVE_LEGACY_FORMAT, "1"); // to save in an "amiga"ish format
#endif

//#ifdef __amigaos4__
//			IMG_SavePNG(render32, filename.c_str());
//#else
			SDL_SaveBMP(render32, filename.c_str());
//#endif
			printscreen = false;
		}

		if (state != STATE_SPOOLING)
		{
			SDL_UpdateTexture(rendertex, NULL, render32->pixels, render32->pitch);
			SDL_RenderClear(ren);
			SDL_RenderCopy(ren, rendertex, NULL, NULL);
			SDL_RenderPresent(ren);
		}
	}

#ifndef _NOMUSIC_
	xmp_free_context(ctx);
#endif

	Config::Save();

	SoundHandler::Quit();

	SDL_FreeSurface(render8);
	SDL_FreeSurface(render32);
	SDL_FreeSurface(screen32);
	SDL_FreeSurface(intermissionscreen);
	SDL_FreeSurface(titlebitmap);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();

#ifdef __amigaos4__
	IDOS->SetCurrentDir(olddir);
	IDOS->UnLock(newdir);

	CloseLibs();
#endif

	return 0;
}
