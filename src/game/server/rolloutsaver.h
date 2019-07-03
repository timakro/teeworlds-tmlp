#ifndef GAME_SERVER_ROLLOUTSAVER_H
#define GAME_SERVER_ROLLOUTSAVER_H

#include <zlib.h>
#include <stdint.h>
#include "model.h"

// *** Data indexes ***
//  0 - Void
//  1 - Solid tile
//  2 - Death tile
//  3 - Nohook tile
//  4 - Health pickup
//  5 - Armor pickup
//  6 - Shotgun pickup
//  7 - Grenade pickup
//  8 - Rifle pickup
//  9 - Ninja pickup
// 10 - Gun projectile
// 11 - Shotgun projectile
// 12 - Grenade projectile
// 13 - Laser beam
// 14 - Player with hammer
// 15 - Player with gun
// 16 - Player with shotgun
// 17 - Player with grenade
// 18 - Player with rifle
// 19 - Player with ninja
// 20 - Player hook
// 21 - HUD

// XXXX-XX-state.gz has 90*50=4500 bytes per frame
// XXXX-XX-action.gz has 14+4+4=22 bytes per frame

class CRolloutSaver
{
public:
	void NewRollout(int BotID);
	void WriteState(uint8_t *RenderData);
	void WriteActionAndValue(CModel::Action *Action, float Value);
	void WriteReward(float Reward);
	void CloseRollout();

private:
	gzFile m_StateFile;
	gzFile m_ActionFile;
};

#endif
