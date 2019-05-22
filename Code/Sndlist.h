/// \file sndlist.h
/// \brief Enumerated types for sounds.

#pragma once

/// \brief Game sound enumerated type. 
///
/// Sounds must be listed here in the same order that they
/// are in the sound settings XML file.

enum GameSoundType{ 
	GAMESTART_SOUND, GAMEOVER_SOUND, UNLEASH_SOUND, MENUSELECT_SOUND,
	HUBBUB_SOUND, MOOSEHEADHONK_SOUND, STEPPINOUT_SOUND, ORGANDONOR_SOUND,
	SHOOT_SOUND, EXPLOSION_SOUND, PLAYERHIT_SOUND, SHIELDHIT_SOUND,
	GETITEM_SOUND, MOVECURSOR_SOUND, SPECIALREADY_SOUND, ENDING_SOUND
}; //GameSoundType
