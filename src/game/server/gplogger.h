#ifndef GAME_SERVER_GPLOGGER_H
#define GAME_SERVER_GPLOGGER_H

#include <zlib.h>
#include <stdint.h>

// *** meta.json format ***
// version: Version of log format
// score: Final player score
// start_time: Unix time at player join
// end_time: Unix time at end of round
// volume: Number of frames
// cid: Player ingame client id
// player: Player nickname
// ip: Player ip address
// domain: Domain running the game server
// serverid: Game server id
// map: Teeworlds map
// gametype: Teeworlds gametype
// sessionid: Unique session identifer
// roundid: Unique round identifier

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

class CGameplayLogger
{
public:
	struct CInputData
	{
		int32_t TargetX;
		int32_t TargetY;
		int8_t Direction;
		int8_t Weapon; // 0=Hammer 1=Gun 2=Shotgun 3=Grenade 4=Rifle 5=Ninja
		bool Jump;
		bool Fire;
		bool Hook;
	};

	~CGameplayLogger();

	void NewSession(int ClientID, const char *PlayerName, const char *PlayerAddr);
	void AddFrame(uint8_t *RenderData, CInputData *InputData);
	void SaveSession(int Score, const char* RoundID);
	void AbortSession();
	static void GenRoundID(char *buffer, int len);
	static void RandomStr(char *str, int len);

private:
	bool m_Active = false;
	int m_StartTime;
	int m_FrameCount;
	int m_ClientID;
	char m_PlayerName[MAX_NAME_LENGTH];
	char m_PlayerAddr[NETADDR_MAXSTRSIZE];
	char m_SessionID[64];
	char m_SessionTmpDir[320];
	char m_DataFilePath[340];
	char m_LabelsFilePath[340];
	gzFile m_DataFile;
	gzFile m_LabelsFile;
};

#endif
