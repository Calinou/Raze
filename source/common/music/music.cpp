//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2016 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------

/* For code that originates from ZDoom the following applies:
**
**---------------------------------------------------------------------------
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "zmusic/zmusic.h"
#include "z_music.h"
#include "zstring.h"
#include "name.h"
#include "s_music.h"
#include "printf.h"
#include "files.h"
#include "filesystem.h"
#include "cmdlib.h"
#include "gamecvars.h"
#include "filereadermusicinterface.h"

MusPlayingInfo mus_playing;
MusicAliasMap MusicAliases;
MidiDeviceMap MidiDevices;


//==========================================================================
//
// starts playing this song
//
//==========================================================================

static void S_StartMusicPlaying(MusInfo* song, bool loop, float rel_vol, int subsong)
{
	if (rel_vol > 0.f)
	{
		float factor = relative_volume / saved_relative_volume;
		saved_relative_volume = rel_vol;
		I_SetRelativeVolume(saved_relative_volume * factor);
	}
	ZMusic_Stop(song);
	ZMusic_Start(song, subsong, loop);

	// Notify the sound system of the changed relative volume
	snd_musicvolume.Callback();
}


//==========================================================================
//
// S_PauseSound
//
// Stop music and sound effects, during game PAUSE.
//==========================================================================

void S_PauseMusic ()
{
	if (mus_playing.handle && !MusicPaused)
	{
		ZMusic_Pause(mus_playing.handle);
		S_PauseStream(true);
		MusicPaused = true;
	}
}

//==========================================================================
//
// S_ResumeSound
//
// Resume music and sound effects, after game PAUSE.
//==========================================================================

void S_ResumeMusic ()
{
	if (mus_playing.handle && MusicPaused)
	{
		ZMusic_Resume(mus_playing.handle);
		S_PauseStream(false);
		MusicPaused = false;
	}
}

//==========================================================================
//
// S_UpdateSound
//
//==========================================================================

void S_UpdateMusic ()
{
	if (mus_playing.handle != nullptr)
	{
		ZMusic_Update(mus_playing.handle);
		
		// [RH] Update music and/or playlist. IsPlaying() must be called
		// to attempt to reconnect to broken net streams and to advance the
		// playlist when the current song finishes.
		if (!ZMusic_IsPlaying(mus_playing.handle))
		{
			if (PlayList.GetNumSongs())
			{
				PlayList.Advance();
				S_ActivatePlayList(false);
			}
			else
			{
				S_StopMusic(true);
			}
		}
	}
}

//==========================================================================
//
// S_Start
//
// Per level startup code. Kills playing sounds at start of level
// and starts new music.
//==========================================================================

void S_StartMusic ()
{
	// stop the old music if it has been paused.
	// This ensures that the new music is started from the beginning
	// if it's the same as the last one and it has been paused.
	if (MusicPaused) S_StopMusic(true);

	// start new music for the level
	MusicPaused = false;

	// Don't start the music if loading a savegame, because the music is stored there.
	// Don't start the music if revisiting a level in a hub for the same reason.
	if (!primaryLevel->IsReentering())
	{
		primaryLevel->SetMusic();
	}
}



//==========================================================================
//
// S_ChangeCDMusic
//
// Starts a CD track as music.
//==========================================================================

bool S_ChangeCDMusic (int track, unsigned int id, bool looping)
{
	char temp[32];

	if (id != 0)
	{
		mysnprintf (temp, countof(temp), ",CD,%d,%x", track, id);
	}
	else
	{
		mysnprintf (temp, countof(temp), ",CD,%d", track);
	}
	return S_ChangeMusic (temp, 0, looping);
}

//==========================================================================
//
// S_StartMusic
//
// Starts some music with the given name.
//==========================================================================

bool S_StartMusic (const char *m_id)
{
	return S_ChangeMusic (m_id, 0, false);
}

//==========================================================================
//
// S_ChangeMusic
//
// Starts playing a music, possibly looping.
//
// [RH] If music is a MOD, starts it at position order. If name is of the
// format ",CD,<track>,[cd id]" song is a CD track, and if [cd id] is
// specified, it will only be played if the specified CD is in a drive.
//==========================================================================

bool S_ChangeMusic(const char* musicname, int order, bool looping, bool force)
{
	if (musicname == nullptr || musicname[0] == 0)
	{
		// Don't choke if the map doesn't have a song attached
		S_StopMusic (true);
		mus_playing.name = "";
		mus_playing.LastSong = "";
		return true;
	}

	FString DEH_Music;

	FName* aliasp = MusicAliases.CheckKey(musicname);
	if (aliasp != nullptr)
	{
		if (*aliasp == NAME_None)
		{
			return true;	// flagged to be ignored
		}
		musicname = aliasp->GetChars();
	}

	if (!mus_playing.name.IsEmpty() &&
		mus_playing.handle != nullptr &&
		stricmp(mus_playing.name, musicname) == 0 &&
		ZMusic_IsLooping(mus_playing.handle) == looping)
	{
		if (order != mus_playing.baseorder)
		{
			if (ZMusic_SetSubsong(mus_playing.handle, order))
			{
				mus_playing.baseorder = order;
			}
		}
		else if (!ZMusic_IsPlaying(mus_playing.handle))
		{
			try
			{
				ZMusic_Start(mus_playing.handle, looping, order);
				S_CreateStream();
			}
			catch (const std::runtime_error & err)
			{
				Printf("Unable to start %s: %s\n", mus_playing.name.GetChars(), err.what());
			}

		}
		return true;
	}

	if (strnicmp(musicname, ",CD,", 4) == 0)
	{
		int track = strtoul(musicname + 4, nullptr, 0);
		const char* more = strchr(musicname + 4, ',');
		unsigned int id = 0;

		if (more != nullptr)
		{
			id = strtoul(more + 1, nullptr, 16);
		}
		S_StopMusic (true);
		mus_playing.handle = ZMusic_OpenCDSong (track, id);
	}
	else
	{
		int lumpnum = -1;
		int length = 0;
		MusInfo* handle = nullptr;
		MidiDeviceSetting* devp = MidiDevices.CheckKey(musicname);

		// Strip off any leading file:// component.
		if (strncmp(musicname, "file://", 7) == 0)
		{
			musicname += 7;
		}

		FileReader reader;
		if (!FileExists(musicname))
		{
			if ((lumpnum = fileSystem.FindFile(musicname)) == -1)
			{
				Printf("Music \"%s\" not found\n", musicname);
				return false;
			}
			if (handle == nullptr)
			{
				if (fileSystem.FileLength(lumpnum) == 0)
				{
					return false;
				}
				reader = fileSystem.ReopenFileReader(lumpnum);
			}
		}
		else
		{
			// Load an external file.
			if (!reader.OpenFile(musicname))
			{
				return false;
			}
		}

		// shutdown old music
		S_StopMusic (true);

		// Just record it if volume is 0
		if (mus_volume <= 0)
		{
			mus_playing.loop = looping;
			mus_playing.name = musicname;
			mus_playing.baseorder = order;
			mus_playing.LastSong = musicname;
			return true;
		}

		// load & register it
		if (handle != nullptr)
		{
			mus_playing.handle = handle;
		}
		else
		{
			try
			{
				auto mreader = new FileReaderMusicInterface(reader);
				mus_playing.handle = ZMusic_OpenSong(mreader, devp ? (EMidiDevice)devp->device : MDEV_DEFAULT, devp ? devp->args.GetChars() : "");
			}
			catch (const std::runtime_error & err)
			{
				Printf("Unable to load %s: %s\n", mus_playing.name.GetChars(), err.what());
			}
		}
	}

	mus_playing.loop = looping;
	mus_playing.name = musicname;
	mus_playing.baseorder = 0;
	mus_playing.LastSong = "";

	if (mus_playing.handle != 0)
	{ // play it
		try
		{
			S_StartMusicPlaying(mus_playing.handle, looping, S_GetMusicVolume(musicname), order);
			S_CreateStream();
			mus_playing.baseorder = order;
		}
		catch (const std::runtime_error & err)
		{
			Printf("Unable to start %s: %s\n", mus_playing.name.GetChars(), err.what());
		}
		return true;
	}
	return false;
}

//==========================================================================
//
// S_RestartMusic
//
// Must only be called from snd_reset in i_sound.cpp!
//==========================================================================

void S_RestartMusic ()
{
	if (!mus_playing.LastSong.IsEmpty())
	{
		FString song = mus_playing.LastSong;
		mus_playing.LastSong = "";
		S_ChangeMusic (song, mus_playing.baseorder, mus_playing.loop, true);
	}
}

//==========================================================================
//
// S_MIDIDeviceChanged
//
//==========================================================================


void S_MIDIDeviceChanged(int newdev)
{
	MusInfo* song = mus_playing.handle;
	if (song != nullptr && ZMusic_IsMIDI(song) && ZMusic_IsPlaying(song))
	{
		// Reload the song to change the device
		auto mi = mus_playing;
		S_StopMusic(true);
		S_ChangeMusic(mi.name, mi.baseorder, mi.loop);
	}
}

//==========================================================================
//
// S_GetMusic
//
//==========================================================================

int S_GetMusic (const char **name)
{
	int order;

	if (mus_playing.name.IsNotEmpty())
	{
		*name = mus_playing.name;
		order = mus_playing.baseorder;
	}
	else
	{
		*name = nullptr;
		order = 0;
	}
	return order;
}

//==========================================================================
//
// S_StopMusic
//
//==========================================================================

void S_StopMusic (bool force)
{
	try
	{
		// [RH] Don't stop if a playlist is active.
		if ((force || PlayList.GetNumSongs() == 0) && !mus_playing.name.IsEmpty())
		{
			if (mus_playing.handle != nullptr)
			{
				S_ResumeMusic();
				S_StopStream();
				ZMusic_Stop(mus_playing.handle);
				auto h = mus_playing.handle;
				mus_playing.handle = nullptr;
				ZMusic_Close(h);
			}
			mus_playing.LastSong = std::move(mus_playing.name);
		}
	}
	catch (const std::runtime_error& )
	{
		//Printf("Unable to stop %s: %s\n", mus_playing.name.GetChars(), err.what());
		if (mus_playing.handle != nullptr)
		{
			auto h = mus_playing.handle;
			mus_playing.handle = nullptr;
			ZMusic_Close(h);
		}
		mus_playing.name = "";
	}
}

}

//==========================================================================
//
// CCMD changemus
//
//==========================================================================

CCMD (changemus)
{
	if (!nomusic)
	{
		if (argv.argc() > 1)
		{
			PlayList.Clear();
			S_ChangeMusic (argv[1], argv.argc() > 2 ? atoi (argv[2]) : 0);
		}
		else
		{
			const char *currentmus = mus_playing.name.GetChars();
			if(currentmus != nullptr && *currentmus != 0)
			{
				Printf ("currently playing %s\n", currentmus);
			}
			else
			{
				Printf ("no music playing\n");
			}
		}
	}
	else
	{
		Printf("Music is disabled\n");
	}
}

//==========================================================================
//
// CCMD stopmus
//
//==========================================================================

CCMD (stopmus)
{
	PlayList.Clear();
	S_StopMusic (false);
	mus_playing.LastSong = "";	// forget the last played song so that it won't get restarted if some volume changes occur
}

{
}

void Mus_SetVolume(float vol)
{
}

void Mus_SetPaused(bool on)
{
}
