// BILLY FRONTIER ENTRY POINT
// (C) 2025 Iliyas Jorio
// This file is part of Billy Frontier. https://github.com/jorio/billyfrontier

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"

#ifdef __ANDROID__
#include <stdlib.h>
#endif

extern "C"
{
	#include "game.h"

	SDL_Window* gSDLWindow = nullptr;
	FSSpec gDataSpec;
	int gCurrentAntialiasingLevel;
}

#ifdef __ANDROID__
// Complete list of all game data files to extract from APK assets
static const char* kAllDataFiles[] = {
    "Audio/Duel.mp3",
    "Audio/Lose.mp3",
    "Audio/Shootout.mp3",
    "Audio/SoundBank/Alarm.aiff",
    "Audio/SoundBank/BulletHit.aiff",
    "Audio/SoundBank/BulletHitMetal.aiff",
    "Audio/SoundBank/CoinSmash.aiff",
    "Audio/SoundBank/CrateExplode.aiff",
    "Audio/SoundBank/DeathSkull.aiff",
    "Audio/SoundBank/DuelFail.aiff",
    "Audio/SoundBank/DuelKey.aiff",
    "Audio/SoundBank/DuelKeysDone.aiff",
    "Audio/SoundBank/Empty.aiff",
    "Audio/SoundBank/Explosion.aiff",
    "Audio/SoundBank/GetCoin.aiff",
    "Audio/SoundBank/GhostVaporize.aiff",
    "Audio/SoundBank/GlassBreak.aiff",
    "Audio/SoundBank/GunShot1.aiff",
    "Audio/SoundBank/GunShot2.aiff",
    "Audio/SoundBank/GunShot3.aiff",
    "Audio/SoundBank/Hoof.aiff",
    "Audio/SoundBank/LaunchMissile.aiff",
    "Audio/SoundBank/Moo1.aiff",
    "Audio/SoundBank/Moo2.aiff",
    "Audio/SoundBank/Reload.aiff",
    "Audio/SoundBank/Ricochet.aiff",
    "Audio/SoundBank/ShieldHit.aiff",
    "Audio/SoundBank/Spurs1.aiff",
    "Audio/SoundBank/Spurs2.aiff",
    "Audio/SoundBank/Swish.aiff",
    "Audio/SoundBank/TimerChime.aiff",
    "Audio/SoundBank/Trampled.aiff",
    "Audio/SoundBank/WalkerAmbient.aiff",
    "Audio/SoundBank/WalkerCrash.aiff",
    "Audio/SoundBank/WalkerFootStep.aiff",
    "Audio/SoundBank/Wind.aiff",
    "Audio/SoundBank/Yelp.aiff",
    "Audio/Stampede.mp3",
    "Audio/Theme.mp3",
    "Images/BigBoard.png",
    "Images/Credits.jpg",
    "Images/HighScores.jpg",
    "Images/Logo.png",
    "Images/LoseScreen.jpg",
    "Images/MainMenu.png",
    "Images/MainMenuArcade.png",
    "Images/WinScreen.jpg",
    "Models/buildings.bg3d",
    "Models/global.bg3d",
    "Models/swamp.bg3d",
    "Models/targetpractice.bg3d",
    "Models/town.bg3d",
    "Skeletons/Bandito.bg3d",
    "Skeletons/Bandito.skeleton.rsrc",
    "Skeletons/Billy.bg3d",
    "Skeletons/Billy.skeleton.rsrc",
    "Skeletons/FrogMan.bg3d",
    "Skeletons/FrogMan.skeleton.rsrc",
    "Skeletons/KangaCow.bg3d",
    "Skeletons/KangaCow.skeleton.rsrc",
    "Skeletons/KangaRex.bg3d",
    "Skeletons/KangaRex.skeleton.rsrc",
    "Skeletons/Rygar.bg3d",
    "Skeletons/Rygar.skeleton.rsrc",
    "Skeletons/Shorty.bg3d",
    "Skeletons/Shorty.skeleton.rsrc",
    "Skeletons/TremorAlien.bg3d",
    "Skeletons/TremorAlien.skeleton.rsrc",
    "Skeletons/TremorGhost.bg3d",
    "Skeletons/TremorGhost.skeleton.rsrc",
    "Skeletons/Walker.bg3d",
    "Skeletons/Walker.skeleton.rsrc",
    "Sprites/bigboard/bigboard000.png",
    "Sprites/bigboard/bigboard001.png",
    "Sprites/bigboard/bigboard002.png",
    "Sprites/bigboard/bigboard003.png",
    "Sprites/bigboard/bigboard004.png",
    "Sprites/bigboard/bigboard005.png",
    "Sprites/bigboard/bigboard006.png",
    "Sprites/bigboard/bigboard007.png",
    "Sprites/bigboard/bigboard008.png",
    "Sprites/bigboard/bigboard009.png",
    "Sprites/bigboard/bigboard010.png",
    "Sprites/bigboard/bigboard011.png",
    "Sprites/bigboard/bigboard012.png",
    "Sprites/bigboard/bigboard013.png",
    "Sprites/bigboard/bigboard014.png",
    "Sprites/bigboard/bigboard015.png",
    "Sprites/cursor/cursor000.png",
    "Sprites/cursor/cursor001.png",
    "Sprites/duel/duel000.png",
    "Sprites/duel/duel001.png",
    "Sprites/duel/duel002.png",
    "Sprites/duel/duel003.png",
    "Sprites/duel/duel004.png",
    "Sprites/duel/duel005.png",
    "Sprites/duel/duel006.png",
    "Sprites/duel/duel007.png",
    "Sprites/duel/duel008.png",
    "Sprites/duel/duel009.png",
    "Sprites/duel/duel010.png",
    "Sprites/duel/duel011.png",
    "Sprites/font/font000.png",
    "Sprites/font/font001.png",
    "Sprites/font/font002.png",
    "Sprites/font/font003.png",
    "Sprites/font/font004.png",
    "Sprites/font/font005.png",
    "Sprites/font/font006.png",
    "Sprites/font/font007.png",
    "Sprites/font/font008.png",
    "Sprites/font/font009.png",
    "Sprites/font/font010.png",
    "Sprites/font/font011.png",
    "Sprites/font/font012.png",
    "Sprites/font/font013.png",
    "Sprites/font/font014.png",
    "Sprites/font/font015.png",
    "Sprites/font/font016.png",
    "Sprites/font/font017.png",
    "Sprites/font/font018.png",
    "Sprites/font/font019.png",
    "Sprites/font/font020.png",
    "Sprites/font/font021.png",
    "Sprites/font/font022.png",
    "Sprites/font/font023.png",
    "Sprites/font/font024.png",
    "Sprites/font/font025.png",
    "Sprites/font/font026.png",
    "Sprites/font/font027.png",
    "Sprites/font/font028.png",
    "Sprites/font/font029.png",
    "Sprites/font/font030.png",
    "Sprites/font/font031.png",
    "Sprites/font/font032.png",
    "Sprites/font/font033.png",
    "Sprites/font/font034.png",
    "Sprites/font/font035.png",
    "Sprites/font/font036.png",
    "Sprites/font/font037.png",
    "Sprites/font/font038.png",
    "Sprites/font/font039.png",
    "Sprites/font/font040.png",
    "Sprites/font/font041.png",
    "Sprites/font/font042.png",
    "Sprites/font/font043.png",
    "Sprites/font/font044.png",
    "Sprites/font/font045.png",
    "Sprites/font/font046.png",
    "Sprites/font/font047.png",
    "Sprites/font/font048.png",
    "Sprites/font/font049.png",
    "Sprites/font/font050.png",
    "Sprites/font/font051.png",
    "Sprites/font/font052.png",
    "Sprites/font/font053.png",
    "Sprites/font/font054.png",
    "Sprites/font/font055.png",
    "Sprites/font/font056.png",
    "Sprites/font/font057.png",
    "Sprites/font/font058.png",
    "Sprites/font/font059.png",
    "Sprites/font/font060.png",
    "Sprites/font/font061.png",
    "Sprites/font/font062.png",
    "Sprites/font/font063.png",
    "Sprites/font/font064.png",
    "Sprites/font/font065.png",
    "Sprites/font/font066.png",
    "Sprites/font/font067.png",
    "Sprites/font/font068.png",
    "Sprites/font/font069.png",
    "Sprites/font/font070.png",
    "Sprites/font/font071.png",
    "Sprites/font/font072.png",
    "Sprites/font/font073.png",
    "Sprites/font/font074.png",
    "Sprites/font/font075.png",
    "Sprites/font/font076.png",
    "Sprites/font/font077.png",
    "Sprites/font/font078.png",
    "Sprites/font/font079.png",
    "Sprites/font/font080.png",
    "Sprites/font/font081.png",
    "Sprites/font/font082.png",
    "Sprites/font/font083.png",
    "Sprites/font/font084.png",
    "Sprites/font/font085.png",
    "Sprites/font/font086.png",
    "Sprites/font/font087.png",
    "Sprites/font/font088.png",
    "Sprites/font/font089.png",
    "Sprites/font/font090.png",
    "Sprites/font/font091.png",
    "Sprites/font/font092.png",
    "Sprites/font/font093.png",
    "Sprites/font/font094.png",
    "Sprites/global/global000.png",
    "Sprites/global/global001.png",
    "Sprites/global/global002.png",
    "Sprites/global/global003.png",
    "Sprites/global/global004.png",
    "Sprites/global/global005.png",
    "Sprites/global/global006.png",
    "Sprites/global/global007.png",
    "Sprites/infobar/infobar000.png",
    "Sprites/infobar/infobar001.png",
    "Sprites/infobar/infobar002.png",
    "Sprites/infobar/infobar003.png",
    "Sprites/infobar/infobar004.png",
    "Sprites/infobar/infobar005.png",
    "Sprites/infobar/infobar006.png",
    "Sprites/infobar/infobar007.png",
    "Sprites/infobar/infobar008.png",
    "Sprites/infobar/infobar009.png",
    "Sprites/infobar/infobar010.png",
    "Sprites/infobar/infobar011.png",
    "Sprites/particle/particle000.png",
    "Sprites/particle/particle001.png",
    "Sprites/particle/particle002.png",
    "Sprites/particle/particle003.png",
    "Sprites/particle/particle004.png",
    "Sprites/particle/particle005.png",
    "Sprites/particle/particle006.png",
    "Sprites/particle/particle007.png",
    "Sprites/particle/particle008.png",
    "Sprites/particle/particle009.png",
    "Sprites/particle/particle010.png",
    "Sprites/particle/particle011.png",
    "Sprites/particle/particle012.png",
    "Sprites/particle/particle013.png",
    "Sprites/particle/particle014.png",
    "Sprites/particle/particle015.png",
    "Sprites/particle/particle016.png",
    "Sprites/particle/particle017.png",
    "Sprites/particle/particle018.png",
    "Sprites/particle/particle019.png",
    "Sprites/particle/particle020.png",
    "Sprites/particle/particle021.png",
    "Sprites/particle/particle022.png",
    "Sprites/particle/particle023.png",
    "Sprites/particle/particle024.png",
    "Sprites/particle/particle025.png",
    "Sprites/particle/particle026.png",
    "Sprites/particle/particle027.png",
    "Sprites/particle/particle028.png",
    "Sprites/particle/particle029.png",
    "Sprites/particle/particle030.png",
    "Sprites/particle/particle031.png",
    "Sprites/particle/particle032.png",
    "Sprites/particle/particle033.png",
    "Sprites/particle/particle034.png",
    "Sprites/particle/particle035.png",
    "Sprites/spheremap/spheremap000.png",
    "Sprites/spheremap/spheremap001.png",
    "Sprites/stampede/stampede000.png",
    "System/gamecontrollerdb.txt",
    "Terrain/swamp_duel.ter",
    "Terrain/swamp_duel.ter.rsrc",
    "Terrain/swamp_shootout.ter",
    "Terrain/swamp_shootout.ter.rsrc",
    "Terrain/swamp_stampede.ter",
    "Terrain/swamp_stampede.ter.rsrc",
    "Terrain/town_duel.ter",
    "Terrain/town_duel.ter.rsrc",
    "Terrain/town_shootout.ter",
    "Terrain/town_shootout.ter.rsrc",
    "Terrain/town_stampede.ter",
    "Terrain/town_stampede.ter.rsrc",
    nullptr
};

static bool ExtractAssets(const char* destDir) {
    for (int i = 0; kAllDataFiles[i]; i++) {
        // Build dest path
        char destPath[1024];
        snprintf(destPath, sizeof(destPath), "%s/%s", destDir, kAllDataFiles[i]);

        // Create directory
        char dirPath[1024];
        strncpy(dirPath, destPath, sizeof(dirPath)-1);
        dirPath[sizeof(dirPath)-1] = '\0';
        char* slash = strrchr(dirPath, '/');
        if (slash) {
            *slash = '\0';
            SDL_CreateDirectory(dirPath);
        }

        // Open from APK assets (relative path)
        SDL_IOStream* src = SDL_IOFromFile(kAllDataFiles[i], "rb");
        if (!src) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ExtractAssets: couldn't open %s", kAllDataFiles[i]);
            continue;
        }

        // Get size
        Sint64 size = SDL_SeekIO(src, 0, SDL_IO_SEEK_END);
        SDL_SeekIO(src, 0, SDL_IO_SEEK_SET);
        if (size <= 0) {
            SDL_CloseIO(src);
            continue;
        }

        // Read all data
        void* buf = malloc((size_t)size);
        if (!buf) { SDL_CloseIO(src); return false; }
        SDL_ReadIO(src, buf, (size_t)size);
        SDL_CloseIO(src);

        // Write to filesystem
        SDL_IOStream* dst = SDL_IOFromFile(destPath, "wb");
        if (!dst) {
            free(buf);
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ExtractAssets: couldn't write %s", destPath);
            continue;
        }
        SDL_WriteIO(dst, buf, (size_t)size);
        SDL_CloseIO(dst);
        free(buf);
    }
    return true;
}
#endif // __ANDROID__

static fs::path FindGameData(const char* executablePath)
{
	fs::path dataPath;

	int attemptNum = 0;

#if !(__APPLE__)
	attemptNum++;		// skip macOS special case #0
#endif

#ifdef __ANDROID__
	attemptNum = 4;	// jump to Android-specific case
#endif

	if (!executablePath)
		attemptNum = 2;

tryAgain:
	switch (attemptNum)
	{
		case 0:			// special case for macOS app bundles
			dataPath = executablePath;
			dataPath = dataPath.parent_path().parent_path() / "Resources";
			break;

		case 1:
			dataPath = executablePath;
			dataPath = dataPath.parent_path() / "Data";
			break;

		case 2:
			dataPath = "Data";
			break;

		case 4:		// Android: internal storage
			{
				const char* iPath = SDL_GetAndroidInternalStoragePath();
				dataPath = iPath ? fs::path(iPath) / "Data" : fs::path("Data");
			}
			break;

		default:
			throw std::runtime_error("Couldn't find the Data folder.");
	}

	attemptNum++;

	dataPath = dataPath.lexically_normal();

	// Set data spec -- Lets the game know where to find its asset files
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "System");

	FSSpec someDataFileSpec;
	OSErr iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":System:gamecontrollerdb.txt", &someDataFileSpec);
	if (iErr)
	{
		goto tryAgain;
	}

	return dataPath;
}

static void Boot(int argc, char** argv)
{
	SDL_SetAppMetadata(GAME_FULL_NAME, GAME_VERSION, GAME_IDENTIFIER);
#if _DEBUG
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#else
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
#endif

	// Start our "machine"
#ifdef __ANDROID__
	// Set HOME to internal storage path so Pomme can find preferences
	if (!SDL_getenv("HOME")) {
		const char* internalPath = SDL_GetAndroidInternalStoragePath();
		if (internalPath)
			SDL_setenv_unsafe("HOME", internalPath, 1);
	}
	// Create ~/.config directory
	{
		const char* home = SDL_getenv("HOME");
		if (home) {
			char configDir[512];
			snprintf(configDir, sizeof(configDir), "%s/.config", home);
			SDL_CreateDirectory(configDir);
		}
	}
#endif
	Pomme::Init();

#ifdef __ANDROID__
	{
		const char* internalPath = SDL_GetAndroidInternalStoragePath();
		if (internalPath) {
			char dataDir[512];
			snprintf(dataDir, sizeof(dataDir), "%s/Data", internalPath);
			SDL_CreateDirectory(dataDir);

			// Check if already extracted
			char stampPath[512];
			snprintf(stampPath, sizeof(stampPath), "%s/.extracted", dataDir);
			SDL_IOStream* stamp = SDL_IOFromFile(stampPath, "r");
			bool needsExtraction = (stamp == nullptr);
			if (stamp) SDL_CloseIO(stamp);

			if (needsExtraction) {
				SDL_Log("Extracting game assets...");
				if (ExtractAssets(dataDir)) {
					// Write extraction stamp only after success
					SDL_IOStream* s = SDL_IOFromFile(stampPath, "w");
					if (s) { SDL_WriteIO(s, "1", 1); SDL_CloseIO(s); }
				}
			}
		}
	}
#endif

	// Find path to game data folder
	const char* executablePath = argc > 0 ? argv[0] : NULL;
	fs::path dataPath = FindGameData(executablePath);

	// Load game prefs before starting
	LoadPrefs();

retryVideo:
	// Initialize SDL video subsystem
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

	// Create window
#ifdef __ANDROID__
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	gCurrentAntialiasingLevel = gGamePrefs.antialiasingLevel;
	if (gCurrentAntialiasingLevel != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1 << gCurrentAntialiasingLevel);
	}

	gSDLWindow = SDL_CreateWindow(
			GAME_FULL_NAME " " GAME_VERSION,
			640, 480,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

	if (!gSDLWindow)
	{
		if (gCurrentAntialiasingLevel != 0)
		{
			SDL_Log("Couldn't create SDL window with the requested MSAA level. Retrying without MSAA...");

			// retry without MSAA
			gGamePrefs.antialiasingLevel = 0;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			goto retryVideo;
		}
		else
		{
			throw std::runtime_error("Couldn't create SDL window.");
		}
	}

#ifdef __ANDROID__
	// Hide the mouse cursor on Android (finger taps are the input method)
	SDL_HideCursor();
#endif

	// Init gamepad subsystem
	SDL_Init(SDL_INIT_GAMEPAD);
	auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
	if (-1 == SDL_AddGamepadMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, GAME_FULL_NAME, "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
	}
}

static void Shutdown()
{
	// Always restore the user's mouse acceleration before exiting.
	// SetMacLinearMouse(false);

	Pomme::Shutdown();

	if (gSDLWindow)
	{
		SDL_DestroyWindow(gSDLWindow);
		gSDLWindow = NULL;
	}

	SDL_Quit();
}

int main(int argc, char** argv)
{
	bool success = true;
	std::string uncaught = "";

	try
	{
		Boot(argc, argv);
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}
#if !(_DEBUG) || defined(__ANDROID__)
	// In release builds, catch anything that might be thrown by GameMain
	// so we can show an error dialog to the user.
	catch (std::exception& ex)		// Last-resort catch
	{
		success = false;
		uncaught = ex.what();
	}
	catch (...)						// Last-resort catch
	{
		success = false;
		uncaught = "unknown";
	}
#endif

	Shutdown();

	if (!success)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Uncaught exception: %s", uncaught.c_str());
		SDL_ShowSimpleMessageBox(0, GAME_FULL_NAME, uncaught.c_str(), nullptr);
	}

	return success ? 0 : 1;
}
