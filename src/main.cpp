// OpenLieroX

// Main entry point
// Created 28/6/02
// Jason Boettcher

// code under LGPL


#include <iostream>
#include <assert.h>

#include "LieroX.h"
#include "AuxLib.h"
#include "CClient.h"
#include "CServer.h"
#include "CBar.h"
#include "ConfigHandler.h"
#include "Graphics.h"
#include "Menu.h"
#include "console.h"
#include "GfxPrimitives.h"
#include "FindFile.h"
#include "InputEvents.h"
#include "StringUtils.h"
#include "Entity.h"
#include "Error.h"
#include "CMediaPlayer.h"
#include "DedicatedControl.h"
#include "Physics.h"
#include "Version.h"
#include "OLXModInterface.h"
#include "OLXG15.h"
#include "CrashHandler.h"


#ifndef WIN32
#include <sys/dir.h>
#include <sys/stat.h>
#endif

#include <libxml/parser.h>

// TODO: i hate globals ...
// we have to create a basic class Game or something

lierox_t	*tLX = NULL;
game_t		tGameInfo;
CInput		*cTakeScreenshot = NULL;
CInput		*cSwitchMode = NULL;
#ifdef WITH_MEDIAPLAYER
CInput		*cToggleMediaPlayer = NULL;
CMediaPlayer cMediaPlayer;
#endif

bool        bDisableSound = false;
#ifdef DEDICATED_ONLY
bool		bDedicated = true;
bool		bJoystickSupport = false;
#else //DEDICATED_ONLY
bool		bDedicated = false;
bool		bJoystickSupport = true;
#endif //DEDICATED_ONLY
bool		bRestartGameAfterQuit = false;
TStartFunction startFunction = NULL;
void*		startFunctionData = NULL;


keyboard_t	*kb = NULL;
IpToCountryDB *tIpToCountryDB = NULL;

CVec		vGravity = CVec(0,4);

//
// Loading screen info and functions
//
class LoadingScreen  {
public:
	LoadingScreen() {
		iBackgroundX = iBackgroundY = 0;
		bmpBackground = NULL;
		cBar = NULL;
	}

	int iBackgroundX;
	int iBackgroundY;
	int iLabelX;
	int iLabelY;
	Uint32 clLabel;
	SmartPointer<SDL_Surface> bmpBackground;
	CBar *cBar;
};

LoadingScreen cLoading;

void InitializeLoading();
void DrawLoading(byte percentage, const std::string &text);
void ShutdownLoading();

using namespace std;


// TODO: move this out here
void print_binary_string(const std::string& txt) {
	for(std::string::const_iterator it = txt.begin(); it != txt.end(); it++) {
		std::cout << std::hex << (ushort)(uchar)(*it) << " ";
	}
}

void test_Unicode_UTF8_Conversion() {
/*	std::string tmp;
	std::string::const_iterator tmpbegin;
	for(UnicodeChar c = 1; c != 0; c++) {
		//if(c % 0x100000 == 0) std::cout << std::hex << c << " ..." << std::endl;
//		std::cout << std::hex << c << " -> ";
		tmp = GetUtf8FromUnicode(c);
//		print_binary_string(tmp);
		tmpbegin = tmp.begin();
//		std::cout << " -> " << std::hex << GetNextUnicodeFromUtf8(tmpbegin, tmp) << std::endl;
		if(GetNextUnicodeFromUtf8(tmpbegin, tmp.end()) != c) {
			//std::cout << std::hex << c << " -> ";
			print_binary_string(tmp);
			//std::cout << " -> " << std::hex << GetNextUnicodeFromUtf8(tmpbegin, tmp.end()) << std::endl;
		}
	}*/

	/*char buf[5] = { 0xF1, 0xE1, 0xC3, 0x81, 0x00 };
	std::string tmp = buf;
	std::string::iterator i = tmp.begin();
	IncUtf8StringIterator(i, tmp.end());*/
}

char* apppath = NULL;

char* GetAppPath() { return apppath; }

///////////////////
// Main entry point
int main(int argc, char *argv[])
{
	printf(GAMENAME " " LX_VERSION " is starting ...\n");
#ifdef DEBUG
	printf("HINT: This is a DEBUG build.\n");
#endif
#ifdef DEDICATED_ONLY
	printf("HINT: This is a DEDICATED_ONLY build.\n");
#endif

#ifdef DEBUG
	setvbuf(stdout, NULL, _IOLBF, 1024 );
#endif

	// sadly, these sizeof are directly used in CGameScript.cpp/CMap.cpp
	// TODO: fix this issue
	assert(sizeof(char) == 1);
	assert(sizeof(short) == 2);
	assert(sizeof(int) == 4);
	assert(sizeof(float) == 4);

    bool startgame = false;

	apppath = argv[0];
	binary_dir = argv[0];
	size_t slashpos = findLastPathSep(binary_dir);
	if(slashpos != std::string::npos)  {
		binary_dir.erase(slashpos);
		binary_dir = SystemNativeToUtf8(binary_dir);
	} else
		binary_dir = "."; // TODO get exact path of binary

	CrashHandler::init();

startpoint:

	// Load options and other settings
	if(!GameOptions::Init())  {
		SystemError("Could not load options");
		return -1;
	}

	if(!NetworkTexts::Init())  {
		SystemError("Could not load network strings.");
		return -1;
	}

    // Parse the arguments
	// do it after the loading of the options as this can
	// overwrite the default options
	ParseArguments(argc, argv);

	// Start the G15 support, it's suitable that the display is showing while loading.
#ifdef WITH_G15
	OLXG15 = new OLXG15_t;
	// TODO: Why do we check for NULL all the time? new should not return NULL, it should throw std::bad_alloc!
	if (!OLXG15->init())
	{
		// Error when initialising, can't use it.
		delete OLXG15;
		OLXG15 = NULL;
	}
#endif //WITH_G15

	// Initialize LX
	if(!InitializeLieroX())  {
		SystemError("Could not initialize LieroX.");
		return -1;
	}

	kb = GetKeyboard();
	if (!bDedicated && !VideoPostProcessor::videoSurface()) {
		SystemError("Could not find screen.");
		return -1;
	}

	DrawLoading(60, "Initializing menu system");

	// Initialize menu
	if(!Menu_Initialize(&startgame)) {
        SystemError("Error: Could not initialize the menu system.\nError when loading graphics files");
		return -1;
	}


#ifdef WITH_MEDIAPLAYER
	DrawLoading(70, "Initializing Media Player");

	// Initialize the music system
	InitializeMusic();

	// Initialize the media player
	if (!cMediaPlayer.Initialize())  {
		printf("Warning: Media Player could not be initialized");
		// Don't quit, we can quite well play without music
	}

	// Load the playlist
	cMediaPlayer.LoadPlaylistFromFile("cfg/playlist.dat");
#endif

	// TODO: abstract the logging, make an uniform message system
	// Log the game start
	if (tLXOptions->bLogConvos)  {
		FILE *f;

		f = OpenGameFile("Conversations.log","a");
		if (f)  {
			std::string cTime = GetTime();
			fputs("<game starttime=\"",f);
			fputs(cTime.c_str(),f);
			fputs("\">\r\n",f);
			fclose(f);
		}
	}

	cTakeScreenshot = new CInput();
	cSwitchMode = new CInput();
#ifdef WITH_MEDIAPLAYER
	cToggleMediaPlayer = new CInput();
#endif

	// Setup the global keys
	cTakeScreenshot->Setup(tLXOptions->sGeneralControls[SIN_SCREENSHOTS]);
	cSwitchMode->Setup(tLXOptions->sGeneralControls[SIN_SWITCHMODE]);
#ifdef WITH_MEDIAPLAYER
	cToggleMediaPlayer->Setup(tLXOptions->sGeneralControls[SIN_MEDIAPLAYER]);
#endif

	// If the user wants to load the database on startup, do it
	// For dedicated server the database should be loaded on startup or it will crash
	if (tLXOptions->bLoadDbAtStartup || (bDedicated && tLXOptions->bUseIpToCountry) )  {
		DrawLoading(80, "Loading IP To Country Database");

		// Allocate & load
		tIpToCountryDB = new IpToCountryDB("ip_to_country.csv");
		if (!tIpToCountryDB)  {
			SystemError("Could not allocate the IP to Country database.");
			return -1;
		}

		// Wait while it fully loads
		while (!tIpToCountryDB->Loaded())
			SDL_Delay(50);
	}

	DrawLoading(99, "Loading Physics Engine");
	PhysicsEngine::Init();

	DrawLoading(100, "Done! Starting menu");

	// Everything loaded, this is not needed anymore
	ShutdownLoading();

	if(startFunction != NULL) {
		if(!startFunction(startFunctionData)) {
			SystemError("ERROR: startfunction not successfull");
			return -1;
		}
		// reset the data; the startfunction is intended to be called only once
		startFunction = NULL;
		startFunctionData = NULL;
	}

	tLX->bQuitGame = false;
	while(!tLX->bQuitGame) {

		startgame = false; // the menu has a reference to this variable

		Menu_Start();	// Start the menu

		if(startgame) {
			// Start the game
			// this means, start the local server and connect to it
			StartGame();

		} else {
			// Quit
			break;
		}

		// Pre-game initialization
		if(!bDedicated) FillSurface(VideoPostProcessor::videoSurface(), tLX->clBlack);
		float oldtime = GetMilliSeconds();

		ClearEntities();

		ProcessEvents();
		ResetQuitEngineFlag();
		printf("MaxFPS is %i\n", tLXOptions->nMaxFPS);

		//cCache.ClearExtraEntries(); // Do not clear anything before game started, it may be slow

		cout << "GameLoopStart" << endl;
		if( DedicatedControl::Get() )
			DedicatedControl::Get()->GameLoopStart_Signal();

		//
        // Main game loop
        //
		while(!tLX->bQuitEngine) {

            tLX->fCurTime = GetMilliSeconds();

			// Timing
			tLX->fDeltaTime = tLX->fCurTime - oldtime;
			tLX->fRealDeltaTime = tLX->fDeltaTime;
			oldtime = tLX->fCurTime;

			// cap the delta
			tLX->fDeltaTime = MIN(tLX->fDeltaTime, 0.5f);

			ProcessEvents();

			// Main frame
			GameLoopFrame();

			VideoPostProcessor::process();

			CapFPS();
		}

		PhysicsEngine::Get()->uninitGame();

		cout << "GameLoopEnd" << endl;
		if( DedicatedControl::Get() )
			DedicatedControl::Get()->GameLoopEnd_Signal();

		cCache.ClearExtraEntries(); // Game ended - clear cache

	}

	PhysicsEngine::UnInit();

	ShutdownLieroX();

	if(bRestartGameAfterQuit) {
		bRestartGameAfterQuit = false;
		printf("-- Restarting game --\n");
		goto startpoint;
	}

	printf("Good Bye and enjoy your day...\n");
	return 0;
}


///////////////////
// Parse the arguments
void ParseArguments(int argc, char *argv[])
{
    // Parameters passed to OpenLieroX overwrite the loaded options
    char *a;
    for(int i=1; i<argc; i++) {
        a = argv[i];

        // -opengl
        // Turns OpenGL on
        if( stricmp(a, "-opengl") == 0 ) {
            tLXOptions->bOpenGL = true;
        } else

        // -nojoystick
        // Turns joystick off
        if( stricmp(a, "-nojoystick") == 0 ) {
            bJoystickSupport = false;
        } else


        // -noopengl
        // Turns OpenGL off
        if( stricmp(a, "-noopengl") == 0 ) {
            tLXOptions->bOpenGL = false;
        } else

        // -nosound
        // Turns off the sound
        if( stricmp(a, "-nosound") == 0 ) {
            bDisableSound = true;
            tLXOptions->bSoundOn = false;
        } else

        // -dedicated
        // Turns on dedicated mode (no gfx, no sound)
        if( stricmp(a, "-dedicated") == 0 ) {
            bDedicated = true;
			bDisableSound = true;
            tLXOptions->bSoundOn = false;
        } else

        // -window
        // Turns fullscreen off
        if( stricmp(a, "-window") == 0 ) {
            tLXOptions->bFullscreen = false;
        } else

        // -fullscreen
        // Turns fullscreen on
        if( stricmp(a, "-fullscreen") == 0 ) {
            tLXOptions->bFullscreen = true;
        } else

#ifdef WIN32
		// -console
		// Attaches a console window to the main LX window
		if (stricmp(a, "-console") == 0)  {
			if (AllocConsole())  {
			  FILE *con = freopen("CONOUT$", "w", stdout);
			  if (con) {
				*stdout = *con;
				setvbuf(stdout, NULL, _IONBF, 0);
			  }
			  SetConsoleTitle("OpenLieroX Console");
			}
		} else
#endif

		// -help
		// Displays help and quits
        if( !stricmp(a, "-h") || !stricmp(a, "-help") || !stricmp(a, "--help") || !stricmp(a, "/?")) {
        	printf("available parameters:\n");
     		printf("   -opengl       OpenLieroX will use OpenGL for drawing\n");
     		printf("   -noopengl     Explicitly disable using OpenGL\n");
     		printf("   -dedicated    Dedicated mode\n");
     		printf("   -nojoystick   Disable Joystick support\n");
     		printf("   -nosound      Disable sound\n");
     		printf("   -window       Run in window mode\n");
     		printf("   -fullscreen   Run in fullscreen mode\n");
			#ifdef WIN32
			printf("   -console      Attach a console window to the main OpenLieroX window\n");
			#endif
			#ifdef DEBUG
     		printf("   -nettest      Test CChannel reliability\n");
			#endif

			// Shutdown and quit
			// ShutdownLieroX() works only correct when everything was inited because ProcessEvents() is used.
			// Therefore we just ignore a good clean up and just quit here.
			// This is not nice but still nicer than getting a segfault.
			// It is not worth to fix this in a nicer way as it is fixed anyway when we use the new engine system.
     		exit(0);
        }
		#ifdef DEBUG
		if( !stricmp(a, "-nettest") )
		{
			InitializeLieroX();
			TestCChannelRobustness();
			ShutdownLieroX();
     		exit(0);
		}
		#endif
    }
}


///////////////////
// Initialize the game
int InitializeLieroX(void)
{
	printf("Hello there, I am initializing me now...\n");

	LIBXML_TEST_VERSION;

	// Initialize the aux library
	if(!InitializeAuxLib("config.cfg",16,0)) {
        SystemError("strange problems with the aux library");
		return false;
	}

	// Setup the HTTP proxy
	AutoSetupHTTPProxy();

	// Initialize the LieroX structure
	tLX = new lierox_t;
    if(!tLX) {
        SystemError("Error: InitializeLieroX() Out of memory on creating lierox_t");
		return false;
    }
	tLX->bVideoModeChanged = false;
	tLX->bQuitGame = false;
	tLX->debug_string = "";
	tLX->fCurTime = 0;
	tLX->fDeltaTime = 0;

	// Initialize the game colors (must be called after GetVideoSurface is not NULL and tLX is not NULL)
	InitializeColors();

	// Load the fonts (must be after colors, because colors are used inside CFont::Load)
	if (!LoadFonts())  {
		SystemError("Could not load the fonts");
		return false;
	}

	// Initialize the loading screen
	InitializeLoading();

	DrawLoading(0, "Initializing network");

	// Initialize the network
	if(!InitNetworkSystem()) {
		SystemError("Error: Failed to initialize the network library");
		return false;
	}

	DrawLoading(5, "Initializing client and server");

	// Allocate the client & server
	cClient = new CClient;
	if(cClient == NULL) {
		SystemError("Error: InitializeLieroX() Out of memory");
		return false;
	}
	cClient->Clear();
	cClient->setLocalClient(true);



	cServer = new GameServer;
	if(cServer == NULL) {
		SystemError("Error: InitializeLieroX() Out of memory on creating GameServer");
		return false;
	}

	DrawLoading(10, "Initializing game entities");

	// Initialize the entities
	if(!InitializeEntities()) {
		SystemError("Error: InitializeEntities() Out of memory on initializing the entities");
		return false;
	}

	DrawLoading(15, "Loading graphics");


	// Load the graphics
	if(!LoadGraphics()) {
		SystemError("Error: Error loading graphics");
		return false;
	}

	DrawLoading(40, "Initializing console");

	// Initialize the console
	if(!Con_Initialize()) {
		SystemError("Error: Could not initialize the console");
		return false;
	}

	// TODO: move to console
    // Add some console commands
    Cmd_AddCommand("kick", Cmd_Kick);
	Cmd_AddCommand("ban", Cmd_Ban);
	Cmd_AddCommand("mute", Cmd_Mute);
	Cmd_AddCommand("unmute", Cmd_Unmute);
    Cmd_AddCommand("kickid", Cmd_KickId);
	Cmd_AddCommand("banid", Cmd_BanId);
	Cmd_AddCommand("muteid", Cmd_MuteId);
	Cmd_AddCommand("unmuteid", Cmd_UnmuteId);
	Cmd_AddCommand("crash", Cmd_Crash, true);
	Cmd_AddCommand("suicide", Cmd_Suicide);
	Cmd_AddCommand("unstuck", Cmd_Unstuck);
	Cmd_AddCommand("wantsjoin", Cmd_WantsJoin);
	Cmd_AddCommand("servername", Cmd_RenameServer);
	Cmd_AddCommand("help", Cmd_Help);
	Cmd_AddCommand("version", Cmd_About);
	Cmd_AddCommand("about", Cmd_About);
	Cmd_AddCommand("fuck", Cmd_BadWord, true);
	Cmd_AddCommand("ass", Cmd_BadWord, true);
	Cmd_AddCommand("bitch", Cmd_BadWord, true);
	Cmd_AddCommand("sex", Cmd_BadWord, true);
	Cmd_AddCommand("quit", Cmd_Quit);
	Cmd_AddCommand("exit", Cmd_Quit);
	Cmd_AddCommand("volume", Cmd_Volume);
	Cmd_AddCommand("sound", Cmd_Sound);
	Cmd_AddCommand("setname", Cmd_SetName);
	Cmd_AddCommand("setskin", Cmd_SetSkin);
	Cmd_AddCommand("setcolour", Cmd_SetColour, true);
	Cmd_AddCommand("setcolor", Cmd_SetColour);
	Cmd_AddCommand("ssh", Cmd_ServerSideHealth);

	DrawLoading(45, "Loading sounds");

	// Load the sounds
	LoadSounds();

	DrawLoading(55, "Loading player profiles");

	// Load the profiles
	LoadProfiles();

    // Initialize the game info structure
	tGameInfo.iNumPlayers = 0;
    tGameInfo.sMapRandom.psObjects = NULL;

	if(bDedicated)
		if(!DedicatedControl::Init()) {
			printf("ERROR: couldn't init dedicated control\n");
			return false;
		}

	printf("Initializing ready\n");

	return true;
}


///////////////////
// Start the game
void StartGame(void)
{
    // Clear the screen
	if(!bDedicated)
		FillSurface(VideoPostProcessor::videoSurface(), tLX->clBlack);

	// Local game
	if(tGameInfo.iGameType == GME_LOCAL) {

		// TODO: uniform message system

		// Start the server
		if(!cServer->StartServer( "local", tLXOptions->iNetworkPort, MAX_PLAYERS, false )) {
			// ERROR
			//MessageBox(NULL, "Error: Could not start server", "OpenLieroX Error", MB_OK);
			printf("Error: Could not start server\n");
			return;
		}

		// Setup the client
		if(!cClient->Initialize()) {
			// ERROR
			//MessageBox(NULL, "Error: Could not initialize client", "OpenLieroX Error", MB_OK);
			printf("Error: Could not initialize client\n");
			return;
		}

		// Tell the client to connect to the server
		cClient->Connect("127.0.0.1");
	}
}


///////////////////
// Game loop
void GameLoopFrame(void)
{
	if(bDedicated)
		DedicatedControl::Get()->GameLoop_Frame();

    if(tLX->bQuitEngine)
        return;

	// Switch between window and fullscreen mode
	if( cSwitchMode->isUp() )  {
		// Set to fullscreen
		tLXOptions->bFullscreen = !tLXOptions->bFullscreen;

		// Set the new video mode
		SetVideoMode();

		cSwitchMode->reset();
	}

#ifdef WITH_MEDIAPLAYER
	// Media player
	cMediaPlayer.Frame();
#endif

#ifdef WITH_G15
	if (OLXG15)
		OLXG15->gameFrame();
#endif //WITH_G15

	// Local
	switch (tGameInfo.iGameType)  {
	case GME_LOCAL:
		cClient->Frame();
		cServer->Frame();

		// If we are connected, just start the game straight away (bypass lobby in local)
		if(cClient->getStatus() == NET_CONNECTED) {
			if(cServer->getState() == SVS_LOBBY)
				cServer->StartGame();
		}

		cClient->Draw(VideoPostProcessor::videoSurface());
		break;


	// Hosting
	case GME_HOST:
		cClient->Frame();
		cServer->Frame();

		cClient->Draw(VideoPostProcessor::videoSurface());
		break;

	// Joined
	case GME_JOIN:
		cClient->Frame();
		cClient->Draw(VideoPostProcessor::videoSurface());
		break;

	} // SWITCH

	// We put it here, so the mouse never displays
    SDL_ShowCursor(SDL_DISABLE);
}


///////////////////
// Quit back to the menu
void QuittoMenu(void)
{
	SetQuitEngineFlag("QuittoMenu");
    Menu_SetSkipStart(false);
	cClient->Disconnect();
}

//////////////////
// Go to local menu
void GotoLocalMenu(void)
{
	SetQuitEngineFlag("GotoLocalMenu");
	cClient->Disconnect();
	Menu_SetSkipStart(true);
	Menu_LocalInitialize();
}

//////////////////
// Go to local menu
void GotoNetMenu(void)
{
	if(tGameInfo.iGameType == GME_HOST) {
		printf("Warning: called GotoLocalMenu as host, ignoring...\n");
		return;
	}

	std::cout << "GotoNetMenu" << std::endl;
	SetQuitEngineFlag("GotoNetMenu");
	cClient->Disconnect();
	Menu_SetSkipStart(true);
	Menu_NetInitialize();
}

////////////////////
// Initialize the loading screen
void InitializeLoading()  {
	if(bDedicated) return; // ignore this case

	FillSurface(VideoPostProcessor::videoSurface(), MakeColour(0,0,0));

	int bar_x, bar_y, bar_label_x, bar_label_y,bar_dir;
	bool bar_visible;
	std::string bar_direction;

	// Load the details
	ReadInteger("data/frontend/frontend.cfg", "Loading", "LoadingBarX", &bar_x, 210);
	ReadInteger("data/frontend/frontend.cfg", "Loading", "LoadingBarY", &bar_y, 230);
	ReadString ("data/frontend/frontend.cfg", "Loading", "LoadingBarDirection", bar_direction, "lefttoright");
	ReadInteger("data/frontend/frontend.cfg", "Loading", "PercentageLabelX", &bar_label_x, 0);
	ReadInteger("data/frontend/frontend.cfg", "Loading", "PercentageLabelY", &bar_label_y, 0);
	ReadKeyword("data/frontend/frontend.cfg", "Loading", "PercentageLabelVisible", &bar_visible, false);
	ReadInteger("data/frontend/frontend.cfg", "Loading", "BackgroundX", &cLoading.iBackgroundX, 190);
	ReadInteger("data/frontend/frontend.cfg", "Loading", "BackgroundY", &cLoading.iBackgroundY, 170);
	ReadInteger("data/frontend/frontend.cfg", "Loading", "LabelX", &cLoading.iLabelX, 235);
	ReadInteger("data/frontend/frontend.cfg", "Loading", "LabelY", &cLoading.iLabelY, 190);

	// Convert the loading direction
	if (!stringcasecmp(bar_direction,"lefttoright"))
		bar_dir = BAR_LEFTTORIGHT;
	else if (!stringcasecmp(bar_direction,"righttoleft"))
		bar_dir = BAR_RIGHTTOLEFT;
	else if (!stringcasecmp(bar_direction,"toptobottom"))
		bar_dir = BAR_TOPTOBOTTOM;
	else if (!stringcasecmp(bar_direction,"bottomtotop"))
		bar_dir = BAR_BOTTOMTOTOP;
	else
		bar_dir = BAR_LEFTTORIGHT;


	// Allocate bar
	cLoading.cBar = new CBar(LoadGameImage("./data/frontend/loading_bar.png",true), bar_x, bar_y, bar_label_x, bar_label_y, bar_dir);
	if (cLoading.cBar)
		cLoading.cBar->SetLabelVisible(bar_visible);

	// Load the background
	cLoading.bmpBackground = LoadGameImage("./data/frontend/background_loading.png", true);
}

/////////////////////
// Draw the loading
void DrawLoading(byte percentage, const std::string &text)  {
	if(bDedicated) {
		printf("Loading: "); printf(text); printf("\n");
		return;
	}

	if (cLoading.bmpBackground.get() == NULL)
		return;

	// Update the repainted area
	int x = MIN(cLoading.iBackgroundX, cLoading.cBar->GetX());
	int y = MIN(cLoading.cBar->GetY(), cLoading.iBackgroundY);
	int w = MAX(cLoading.bmpBackground.get()->w, cLoading.cBar->GetWidth());
	int h = MAX(cLoading.bmpBackground.get()->h, cLoading.cBar->GetHeight());
	DrawRectFill(VideoPostProcessor::videoSurface(), x, y, x+w, y+h, MakeColour(0,0,0));

	if (cLoading.bmpBackground.get() != NULL)
		DrawImage(VideoPostProcessor::videoSurface(), cLoading.bmpBackground, cLoading.iBackgroundX, cLoading.iBackgroundY);

	if (cLoading.cBar)  {
		cLoading.cBar->SetPosition(percentage);
		cLoading.cBar->Draw( VideoPostProcessor::videoSurface() );
	}

	tLX->cFont.Draw(VideoPostProcessor::videoSurface(), cLoading.iLabelX, cLoading.iLabelY, tLX->clLoadingLabel, text);

	VideoPostProcessor::process();
}

////////////////////
// Shutdown the loading screen
void ShutdownLoading()  {
	if (cLoading.cBar)
		delete cLoading.cBar;
	cLoading.cBar = NULL;
}

///////////////////
// Shutdown the game
void ShutdownLieroX(void)
{
	printf("Shutting me down...\n");

	if(!bDedicated) // only save if not in dedicated mode
		tLXOptions->SaveToDisc();

	if (tLXOptions->bLogConvos)  {
		FILE *f;

		f = OpenGameFile("Conversations.log","a");
		if (f)  {
			fputs("</game>\r\n",f);
			fclose(f);
		}
	}

	if(bDedicated)
		DedicatedControl::Uninit();

	ShutdownMusic();
#ifdef WITH_MEDIAPLAYER
	cMediaPlayer.SavePlaylistToFile("cfg/playlist.dat");
	cMediaPlayer.Shutdown();
#endif

    Con_Shutdown();

	ShutdownLoading();  // In case we're called when an error occured

	// Only do the deregistration for widgets if we are not restarting.
	// The problem is that we have registered most widgets globally (not by any init-function)
	// so we would not reinit them.
	if(!bRestartGameAfterQuit)
		CGuiSkin::DeInit();

	ShutdownGraphics();

	Menu_Shutdown();
	ShutdownProfiles();

	// Free the IP to Country DB
	if (tIpToCountryDB) {
		delete tIpToCountryDB;
		tIpToCountryDB = NULL;
	}

    // Free the game info structure
    if(tGameInfo.sMapRandom.psObjects)
        delete[] tGameInfo.sMapRandom.psObjects;
    tGameInfo.sMapRandom.psObjects = NULL;

	// Free the client & server
	if(cClient) {
		delete cClient;
		cClient = NULL;
	}

	if(cServer) {
		cServer->Shutdown();
		delete cServer;
		cServer = NULL;
	}

#ifdef WITH_G15
	if (OLXG15)
	{
		delete OLXG15;
		OLXG15 = NULL;
	}
#endif //WITH_G15
	// Entitites
	ShutdownEntities();

	// LieroX structure
	if(tLX) {
		delete tLX;
		tLX = NULL;
	}

	// Console commands
	Cmd_Free();

	// Network
	QuitNetworkSystem();

	// SDL, Cache and other small stuff
	ShutdownAuxLib();

	// Save and clear options
	ShutdownOptions();

	delete cTakeScreenshot; cTakeScreenshot = NULL;
	delete cSwitchMode; cSwitchMode = NULL;
#ifdef WITH_MEDIAPLAYER
	delete cToggleMediaPlayer; cToggleMediaPlayer = NULL;
#endif

	// Only do the deregistration for variables if we are not restarting.
	// The problem is that we have registered most vars globally (not by any init-function)
	// so we would not reinit them.
	// Multiple registration of a var is also not a problem because we are
	// using a map for all vars and with a new registration, we would just overwrite
	// the old registration.
	if(!bRestartGameAfterQuit)
	{
		CScriptableVars::DeInit();
		OlxMod::OlxMod_DeleteModList();
	};

	xmlCleanupParser();

#ifdef DEBUG
	ShutdownCacheDebug();
#endif

	printf("Everything was shut down\n");
}



std::string quitEngineFlagReason;

void ResetQuitEngineFlag() {
	tLX->bQuitEngine = false;
}

void SetQuitEngineFlag(const std::string& reason) {
	Warning_QuitEngineFlagSet("SetQuitEngineFlag(" + reason + "): ");
	quitEngineFlagReason = reason;
	tLX->bQuitEngine = true;
}

bool Warning_QuitEngineFlagSet(const std::string& preText) {
	if(tLX->bQuitEngine) {
		printf(preText);
		printf("WARNING: bQuitEngine is set because: " + quitEngineFlagReason + "\n");
		return true;
	}
	return false;
}
