//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!


#include "baselayer.h"
#include "common_game.h"
#include "build.h"
#include "cache1d.h"
#include "sndcards.h"
#include "hash.h"
#include "scriplib.h"
#include "renderlayer.h"
#include "gamecontrol.h"
#include "blood.h"
#include "config.h"
#include "gamedefs.h"
#include "globals.h"
#include "screen.h"
#include "sound.h"
#include "tile.h"
#include "view.h"

#if defined RENDERTYPESDL && defined SDL_TARGET && SDL_TARGET > 1
# include "sdl_inc.h"
#endif

// we load this in to get default button and key assignments
// as well as setting up function mappings

#define __SETUP__   // JBF 20031211
#include "_functio.h"

BEGIN_BLD_NS


int32_t scripthandle;
int32_t setupread;
int32_t mus_restartonload;
char szPlayerName[MAXPLAYERNAME];
int32_t gTurnSpeed;
int32_t gDetail;
int32_t cl_weaponswitch;
int32_t gAutoRun;
int32_t gFollowMap;
int32_t gOverlayMap;
int32_t gRotateMap;
int32_t gMessageCount;
int32_t gMessageTime;
int32_t gMessageFont;
int32_t gbAdultContent;
char gzAdultPassword[9];
int32_t gMouseSensitivity;
bool gNoClip;
bool gInfiniteAmmo;
bool gFullMap;
int32_t gUpscaleFactor;
int32_t gDeliriumBlur;

//////////
int gWeaponsV10x;
/////////






void CONFIG_SetDefaults(void)
{
    scripthandle = -1;

#ifdef __ANDROID__
    droidinput.forward_sens = 5.f;
    droidinput.gameControlsAlpha = 0.5;
    droidinput.hideStick = 0;
    droidinput.pitch_sens = 5.f;
    droidinput.quickSelectWeapon = 1;
    droidinput.strafe_sens = 5.f;
    droidinput.toggleCrouch = 1;
    droidinput.yaw_sens = 5.f;

    gSetup.xdim = droidinfo.screen_width;
    gSetup.ydim = droidinfo.screen_height;
#else
# if defined RENDERTYPESDL && SDL_MAJOR_VERSION > 1
    uint32_t inited = SDL_WasInit(SDL_INIT_VIDEO);
    if (inited == 0)
        SDL_Init(SDL_INIT_VIDEO);
    else if (!(inited & SDL_INIT_VIDEO))
        SDL_InitSubSystem(SDL_INIT_VIDEO);

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
    {
        gSetup.xdim = dm.w;
        gSetup.ydim = dm.h;
    }
    else
# endif
    {
        gSetup.xdim = 1024;
        gSetup.ydim = 768;
    }
#endif

#ifdef USE_OPENGL
    gSetup.bpp = 32;
#else
    gSetup.bpp = 8;
#endif
	
    gSetup.fullscreen       = 1;

    //snd_ambience  = 1;
    //ud.config.AutoAim         = 1;
    gBrightness = 8;
    //ud.config.ShowWeapons     = 0;

    //ud.crosshair              = 1;
    //ud.default_skill          = 1;
    gUpscaleFactor = 0;
    //ud.display_bonus_screen   = 1;
    //ud.lockout                = 0;
    //ud.m_marker               = 1;
    //ud.maxautosaves           = 5;
    //ud.menu_scrollbartilenum  = -1;
    //ud.menu_scrollbarz        = 65536;
    //ud.menu_scrollcursorz     = 65536;
    //ud.menu_slidebarmargin    = 65536;
    //ud.menu_slidebarz         = 65536;
    //ud.menu_slidecursorz      = 65536;
    //ud.pwlockout[0]           = '\0';
    //ud.screen_size            = 4;
    //ud.screen_tilting         = 1;
    //ud.screenfade             = 1;
    //ud.shadow_pal             = 4;
    //ud.show_level_text        = 1;
    //ud.slidebar_paldisabled   = 1;
    //ud.statusbarflags         = STATUSBAR_NOSHRINK;
    //ud.statusbarmode          = 1;
    //ud.statusbarscale         = 100;
    //ud.team                   = 0;
    //cl_weaponswitch           = 3;  // new+empty
    gDeliriumBlur = 1;
    gViewSize = 2;
    gTurnSpeed = 92;
    gDetail = 4;
    gAutoRun = 0;
    gFollowMap = 1;
    gOverlayMap = 0;
    gRotateMap = 0;

    gMessageCount = 4;
    gMessageTime = 5;
    gMessageFont = 0;
    gbAdultContent = 0;
    gzAdultPassword[0] = 0;

    cl_weaponswitch = 1;

    Bstrcpy(szPlayerName, "Player");
}






int CONFIG_ReadSetup(void)
{
    char tempbuf[1024];

    CONFIG_SetDefaults();

    setupread = 1;
    pathsearchmode = 1;

    if (scripthandle < 0)
    {
        if (buildvfs_exists(SetupFilename))  // JBF 20031211
            scripthandle = SCRIPT_Load(SetupFilename);
#if !defined(EDUKE32_TOUCH_DEVICES) && !defined(EDUKE32_STANDALONE)
        else if (buildvfs_exists(SETUPFILENAME))
        {
            int const i = wm_ynbox("Import Configuration Settings",
                                   "The configuration file \"%s\" was not found. "
                                   "Import configuration data from \"%s\"?",
                                   SetupFilename, SETUPFILENAME);
            if (i)
                scripthandle = SCRIPT_Load(SETUPFILENAME);
        }
#endif
    }

    pathsearchmode = 0;

    if (scripthandle < 0)
        return -1;

    // Nuke: make cvar
    ///////
    SCRIPT_GetNumber(scripthandle, "Game Options", "WeaponsV10x", &gWeaponsV10x);
    ///////

    Bmemset(tempbuf, 0, sizeof(tempbuf));
    SCRIPT_GetString(scripthandle, "Comm Setup","PlayerName",&tempbuf[0]);

    char nameBuf[64];

    while (Bstrlen(OSD_StripColors(nameBuf, tempbuf)) > 10)
        tempbuf[Bstrlen(tempbuf) - 1] = '\0';

    Bstrncpyz(szPlayerName, tempbuf, sizeof(szPlayerName));

    if (gNoSetup == 0 && g_modDir[0] == '/')
    {
        struct Bstat st;
        SCRIPT_GetString(scripthandle, "Setup","ModDir",&g_modDir[0]);

        if (Bstat(g_modDir, &st))
        {
            if ((st.st_mode & S_IFDIR) != S_IFDIR)
            {
                initprintf("Invalid mod dir in cfg!\n");
                Bsprintf(g_modDir,"/");
            }
        }
    }

    //if (g_grpNamePtr == NULL && g_addonNum == 0)
    //{
    //    SCRIPT_GetStringPtr(scripthandle, "Setup", "SelectedGRP", &g_grpNamePtr);
    //    if (g_grpNamePtr && !Bstrlen(g_grpNamePtr))
    //        g_grpNamePtr = dup_filename(G_DefaultGrpFile());
    //}
    //
    //if (!NAM_WW2GI)
    //{
    //    SCRIPT_GetNumber(scripthandle, "Screen Setup", "Out", &ud.lockout);
    //    SCRIPT_GetString(scripthandle, "Screen Setup", "Password", &ud.pwlockout[0]);
    //}

    windowx = -1;
    windowy = -1;

    SCRIPT_GetNumber(scripthandle, "Screen Setup", "MaxRefreshFreq", (int32_t *)&maxrefreshfreq);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenBPP", &gSetup.bpp);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenHeight", &gSetup.ydim);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenMode", &gSetup.fullscreen);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "ScreenWidth", &gSetup.xdim);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "WindowPosX", (int32_t *)&windowx);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "WindowPosY", (int32_t *)&windowy);
    SCRIPT_GetNumber(scripthandle, "Screen Setup", "WindowPositioning", (int32_t *)&windowpos);

    if (gSetup.bpp < 8) gSetup.bpp = 32;

    setupread = 1;
    return 0;
}


void CONFIG_WriteSettings(void) // save binds and aliases to <cfgname>_settings.cfg
{
}



END_BLD_NS
