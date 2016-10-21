/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H
#include <game/server/gamecontroller.h>
#include <engine/shared/protocol.h>

enum
{
	EMOTE_FLASHING=6,
};

enum
{
	TEAM_REAPER=1,
 	TEAM_MUTANT,
	TEAM_HUMAN, 
	TEAM_HERO,
};

enum
{
        POWERUP_HAMMER=5,
        POWERUP_GUN=6,	// currently unused
};


enum
{
	SCIENCE_LASER_TELE=1,		// 0
	SCIENCE_LASER_HEALING=2,	// 1
	SCIENCE_LASER_RIDDLE=4,		// 2
	SCIENCE_SHOTGUN_FORCEX2=8,	// 3
	SCIENCE_SHOTGUN_EXPL=16,	// 4
	SCIENCE_GRENADE_CLUSTER=32,	// 5
	SCIENCE_GUN_MK2=64,		// 6
	SCIENCE_SHOTGUN_FASTRELOAD=128,	// 7
	SCIENCE_BATTERY=256,		// 8
	SCIENCE_GAIN_WEAPONS=512,	// 9 
	SCIENCE_GAIN_HEALTH=1024,	// 10
	SCIENCE_AIRSTRIKE=2048,		// 11
	SCIENCE_GAIN_HERO=4096,		// 12
	MUTATOR_START_SHIELDS=1,	// 0
	MUTATOR_NOBIOHAZARD=2,		// 1
	MUTATOR_JUMP=4,			// 2
	MUTATOR_ENDLESSHOOK=8,		// 3
	MUTATOR_SELFHEAL=16,		// 4
	MUTATOR_GAIN_REAPER=32,		// 5
};

struct ScoreData
{
	int m_HighScoreHuman;
	char m_HighScoreNameHuman[MAX_NAME_LENGTH];
	char m_HighScoreClanHuman[MAX_CLAN_LENGTH];
	int m_HighScoreMutant;
	char m_HighScoreNameMutant[MAX_NAME_LENGTH];
	char m_HighScoreClanMutant[MAX_CLAN_LENGTH];
};
	
class CGameControllerMOD : public IGameController
{
public:
	CGameControllerMOD(class CGameContext *pGameServer);
	
	void UpdateHiScore();
	bool LoadHiScore();
	static void SaveHiScoreThread(void *data);
	
	void PreStartGame();
	void StartGame(int PlayerCount);

	void DoAirstrike(int ClientID);
	void ShowHighScore();
	virtual void Tick();
	virtual void PostReset();
	virtual int MutGameRunning();
	virtual int MutPowerSupply();
	virtual void MutSetPowerSupply(int State);
	virtual bool ReapinatorOnline();
	virtual void DoWincheck();
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual bool FoundHammerPickups();

private:
	bool m_PowerSupply;
	int m_ShowCountDownTick;
	
	int m_HighScoreHuman;
	char m_HighScoreNameHuman[MAX_NAME_LENGTH];
	char m_HighScoreClanHuman[MAX_CLAN_LENGTH];
	int m_HighScoreMutant;
	char m_HighScoreNameMutant[MAX_NAME_LENGTH];
	char m_HighScoreClanMutant[MAX_CLAN_LENGTH];
	
	int m_ShowHighScore;
	int m_ShowHighScoreTick;
	bool m_FlipIdleBroadcast;
	
	bool m_GameRunning;
	bool m_GamePreStarting;
	bool m_GamePreStartingDelay;
	int m_PreStartDelayTick;
	
	int m_ActivePlayers;
	int m_ActiveCharacters;
	int m_ActiveMutants;
	int m_ActiveHumans;
	int m_BroadcastTick;
	int m_GameInitDelayTick;
	
	int m_ReapinatorInitTick;
	int m_ReapinatorStatus;

	bool m_FoundHammerPickups;
	
	char m_GenBuf[512];
};
#endif
