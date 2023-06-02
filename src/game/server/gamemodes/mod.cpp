/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "mod.h"
#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/spark.h>
#include <game/server/entities/laserturret.h>
#include <game/server/entities/singularity.h>
#include <game/server/entities/projectile.h>

#include <game/server/player.h>

#include <game/generated/protocol.h>
#include <fstream>
#include <base/tl/threading.h>
#include <string.h>

static LOCK HiScoreMutex = 0;

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "Alien";

	m_GameRunning = 0;
	m_GamePreStarting = 0;
	m_GamePreStartingDelay = 0;
	m_BroadcastTick = Server()->Tick();
	m_GameInitDelayTick = Server()->Tick();
	m_PreStartDelayTick = Server()->Tick();
	mem_zero(m_GenBuf,sizeof(m_GenBuf));
	m_PowerSupply = 1;
	LoadHiScore();
	m_ShowHighScore = 0;
	m_ShowHighScoreTick = Server()->Tick();
	m_FlipIdleBroadcast = 0;
	if(HiScoreMutex == 0)
		HiScoreMutex = lock_create();
	m_ReapinatorInitTick = Server()->Tick();
	m_ReapinatorStatus = 0;
	m_FoundHammerPickups = 0;
	
	dbg_msg("server", "Mod constructor called.");
}

int CGameControllerMOD::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
        if(!pKiller || Weapon == WEAPON_GAME)
                return 0;

        if(pKiller == pVictim->GetPlayer())
                pVictim->GetPlayer()->m_Score--; // suicide - all other scores in character.cpp

        if(Weapon == WEAPON_SELF)
                pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;
        return 0;
}

bool CGameControllerMOD::OnEntity(int Index, vec2 Pos)
{
	if(IGameController::OnEntity(Index, Pos))
                return true;
	
	int Type = -1;
	int SubType = 0;
        
	if(Index == ENTITY_POWERUP_HAMMER && !g_Config.m_SvSkipScienceStations)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_HAMMER;
		m_FoundHammerPickups = 1;
	}
	else if(Index == ENTITY_SPARK)
	{
		new CSpark(&GameServer()->m_World, Pos);
	}
	else if(Index == ENTITY_LASERTURRET)
	{
		new CLaserTurret(&GameServer()->m_World, Pos);
	}
	else if(Index == ENTITY_SINGULARITY)
	{
		new CSingularity(&GameServer()->m_World, Pos, g_Config.m_SvSingularityRangeS, g_Config.m_SvSingularityKillRange,-1, 0);
	}

	if(Type != -1)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, SubType);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

bool CGameControllerMOD::FoundHammerPickups()
{
	return m_FoundHammerPickups;
}

int CGameControllerMOD::MutGameRunning()
{
	return m_GameRunning;
}

int CGameControllerMOD::MutPowerSupply() 
{ 
	return m_PowerSupply;
}

void CGameControllerMOD::MutSetPowerSupply(int State)
{
	m_PowerSupply = State; 	
}

bool CGameControllerMOD::ReapinatorOnline()
{
	if(m_ReapinatorStatus > 0 && Server()->Tick() - m_ReapinatorInitTick > 0)
		return 1;
	else
		return 0;
}

void CGameControllerMOD::DoAirstrike(int ClientID)
{
	CGameWorld *pWorld = &GameServer()->m_World;
        for(int bomb = -320; bomb < (GameServer()->Collision()->GetWidth()+10)*32; bomb += g_Config.m_SvAlicaAirStrikeStep)
        {
                CProjectile *pProj = new CProjectile(pWorld, WEAPON_GRENADE,
                        ClientID,
                        vec2(bomb,0),
                        vec2(0.8f,1.f),
                        g_Config.m_SvAirStrikeTTL,
                        1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

                CNetObj_Projectile p;
                pProj->FillInfo(&p);

                CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
                Msg.AddInt(1);
                for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
                        Msg.AddInt(((int *)&p)[i]);
                Server()->SendMsg(&Msg, 0, ClientID);
        }
}

void CGameControllerMOD::ShowHighScore()
{
	if(Server()->Tick() - m_ShowHighScoreTick > 0)
	{
		if(m_ShowHighScore == 1)
		{
			if(m_HighScoreHuman)
			{
				if(strlen(m_HighScoreClanHuman))
					str_format(m_GenBuf, sizeof(m_GenBuf),"高分tee: %d - %s | %s",m_HighScoreHuman,m_HighScoreNameHuman,m_HighScoreClanHuman);
				else
					str_format(m_GenBuf, sizeof(m_GenBuf),"高分tee: %d - %s",m_HighScoreHuman,m_HighScoreNameHuman);
        			
				GameServer()->SendBroadcast(m_GenBuf,-1);
			}			
			else
        			GameServer()->SendBroadcast("暂时没有高分的幸存者!",-1);

			m_ShowHighScoreTick = Server()->Tick() + 2 * Server()->TickSpeed();
			m_ShowHighScore++;
		}
		else
		{
			if(m_HighScoreMutant)
			{
				if(strlen(m_HighScoreClanMutant))
					str_format(m_GenBuf, sizeof(m_GenBuf),"最可怕的突变体: %d - %s | %s",m_HighScoreMutant,m_HighScoreNameMutant,m_HighScoreClanMutant);
				else
					str_format(m_GenBuf, sizeof(m_GenBuf),"最可怕的突变体: %d - %s",m_HighScoreMutant,m_HighScoreNameMutant);
        			
				GameServer()->SendBroadcast(m_GenBuf,-1);
			}
			else
        			GameServer()->SendBroadcast("暂时没有高分的突变体!",-1);

			m_ShowHighScore = 0;
		}
	}
	return;
}

void CGameControllerMOD::PostReset()
{
	m_RoundStartTick = Server()->Tick();
	mem_zero(m_GenBuf,sizeof(m_GenBuf));

	m_GameRunning = 0;
	m_GamePreStarting = 0;
	m_GamePreStartingDelay = 1;
	m_PowerSupply = 1;
	m_ReapinatorStatus = 0;
	m_ShowHighScore = 0;
	m_ShowHighScoreTick = Server()->Tick();
	for(int i=0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i])
			GameServer()->m_apPlayers[i]->m_ScienceExplored = GameServer()->m_apPlayers[i]->m_ScienceRemembered;

	m_PreStartDelayTick = Server()->Tick() + 1000 * Server()->TickSpeed() / 1000;
}

void CGameControllerMOD::PreStartGame()
{
	int pActivePlayers;
	CCharacter* pChr;
	m_PowerSupply = 1;
	m_ReapinatorStatus = 0;
	
	pActivePlayers = 0;
	for(int i=0; i < MAX_CLIENTS; i++)
        {
                if(GameServer()->m_apPlayers[i])
                {
			if(GameServer()->m_apPlayers[i]->GetTeam() != -1)
				pActivePlayers++;

			GameServer()->m_apPlayers[i]->m_ScienceExplored = GameServer()->m_apPlayers[i]->m_ScienceRemembered;
		
			GameServer()->m_apPlayers[i]->m_MutaticLevels = 0;
			GameServer()->m_apPlayers[i]->m_ScienceCount = 0;
			GameServer()->m_apPlayers[i]->m_Score = 0;
			GameServer()->m_apPlayers[i]->m_MyHumanScore = 0;
			GameServer()->m_apPlayers[i]->m_MyMutantScore = 0;
			GameServer()->m_apPlayers[i]->m_BonusScore = 0;
			GameServer()->m_apPlayers[i]->m_BonusScore2 = 0;
			GameServer()->m_apPlayers[i]->m_MultiScore = 0;
			GameServer()->m_apPlayers[i]->m_MultiScoreTick = Server()->Tick();
				
 			pChr = GameServer()->m_apPlayers[i]->GetCharacter();
			if(pChr)
				pChr->SetPreStartGame();

			GameServer()->m_apPlayers[i]->m_MutatorTeam = TEAM_HUMAN;
                }
        }
	
	if(pActivePlayers > 1)
	{
	        m_GameInitDelayTick = Server()->Tick() + 1000 * g_Config.m_SvPreStartDelay * Server()->TickSpeed() / 1000;
		m_GameRunning = 0;
        	m_GamePreStarting = 1;
		m_GamePreStartingDelay = 0;
	
		GameServer()->SendChatTarget(-1, "Alica-AI: 可能的入侵. 开始扫描...");
		m_ShowHighScoreTick = Server()->Tick() + 5 * Server()->TickSpeed();
		m_ShowHighScore = 1;
	}
}


void CGameControllerMOD::StartGame(int PlayerCount)
{
	int iMutant;
	int pActivePlayers;	
	int pRandMsg = rand()%4;
	CCharacter* pChr;

	iMutant = rand()%PlayerCount;
	pActivePlayers = 0;	
	
	for(int i=0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;

		if(GameServer()->m_apPlayers[i]->GetTeam() > -1)
		{
			if(pActivePlayers == iMutant)
			{
				if(GameServer()->m_apPlayers[i]->m_LastInitMutant)
					return;

				pChr = GameServer()->m_apPlayers[i]->GetCharacter();
				if(pChr)
				{
					GameServer()->SendBroadcast("你这回合是突变体!", GameServer()->m_apPlayers[i]->GetCID());
					GameServer()->m_apPlayers[i]->m_ScienceCount = 0;
					pChr->Mutate();
					GameServer()->CreateSound(pChr->m_Pos,SOUND_PLAYER_SKID);
				
					if(g_Config.m_SvInitAirStrike && rand()%2 == 1)	
					{
						GameServer()->SendChatTarget(-1, "Alica-AI: 警告: 检测到外星人突变剂! 开始使用对策.");
						DoAirstrike(-1);
					}
					else
						GameServer()->SendChatTarget(-1, "Alica-AI: 警告: 检测到外星人突变剂! 对策不可用.");
						
				
					m_GameRunning = 1;
					m_GamePreStarting = 0;
					m_GamePreStartingDelay = 0;
					m_PowerSupply = 1;
					if(!g_Config.m_SvReapinatorDisabled)
					{
						m_ReapinatorInitTick = Server()->Tick() + g_Config.m_SvReapinatorDelay * Server()->TickSpeed();
						m_ReapinatorStatus = 1;
					}

					GameServer()->m_apPlayers[i]->m_LastInitMutant = 1;
					// Reset all non starting mutants
					for(int y = 0; y < MAX_CLIENTS; y++)
					{
						if(y != i && GameServer()->m_apPlayers[y])
						{
							switch(pRandMsg)
							{
								case 0: GameServer()->SendBroadcast(g_Config.m_SvStartMsgA,GameServer()->m_apPlayers[y]->GetCID()); break; 
								case 1: GameServer()->SendBroadcast(g_Config.m_SvStartMsgB,GameServer()->m_apPlayers[y]->GetCID()); break;
								case 2: GameServer()->SendBroadcast(g_Config.m_SvStartMsgC,GameServer()->m_apPlayers[y]->GetCID()); break;
								case 3: GameServer()->SendBroadcast(g_Config.m_SvStartMsgD,GameServer()->m_apPlayers[y]->GetCID()); break;
							}
							GameServer()->m_apPlayers[y]->m_LastInitMutant = 0;
						}
					}
					return;
				}
			}
			pActivePlayers++;
		}
	}
}

void CGameControllerMOD::Tick()
{
	IGameController::Tick();
	if(m_ShowHighScore)
		ShowHighScore();
}

void CGameControllerMOD::DoWincheck()
{
	if(m_GameOverTick != -1 || m_Warmup > 0)
		return;

	m_ActivePlayers = 0;
	m_ActiveMutants = 0;
	m_ActiveHumans = 0;
	m_ActiveCharacters =0;

        for(int i = 0; i < MAX_CLIENTS; i++)
	{
                if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != -1)
		{
                       	m_ActivePlayers++;
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_MutatorTeam <= TEAM_MUTANT)
				m_ActiveMutants++;
			else
				m_ActiveHumans++;
			if(GameServer()->m_apPlayers[i]->GetCharacter())
				m_ActiveCharacters++;
		}
	}

	if(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60)
	{
		for(int u = 0; u < MAX_CLIENTS; u++)
		{
			if(GameServer()->m_apPlayers[u] && GameServer()->m_apPlayers[u]->m_MutatorTeam >= TEAM_HUMAN)
				GameServer()->m_apPlayers[u]->m_Score += g_Config.m_SurviveBonus;
		}

		if(m_ActivePlayers >= g_Config.m_SvMinScorePlayers)
			UpdateHiScore();

		EndRound();
		m_GameRunning = 0;
		m_GamePreStarting = 0;
		m_GamePreStartingDelay = 0;
		GameServer()->SendBroadcast("Tee在外星人的入侵下生存了下来!", -1);
	}

	if(m_GameRunning == 0 && m_ActivePlayers > 1)
	{
		if(m_GamePreStarting == 0 && m_GamePreStartingDelay == 0 && m_ActiveCharacters > 1)
			PostReset();
		else if(m_GamePreStartingDelay == 1 && Server()->Tick() - m_PreStartDelayTick > 0)
			PreStartGame();
		else if(m_GamePreStarting == 1 && Server()->Tick() - m_GameInitDelayTick > 0)
			StartGame(m_ActivePlayers);
	}
	else if(m_GameRunning == 1)
	{
		if(m_ActivePlayers < 2)
		{
			m_GameRunning = 0;
			m_GamePreStarting = 0;
			m_GamePreStartingDelay = 0;
			EndRound();
		}
		else if(Server()->Tick() > m_GameInitDelayTick)
		{	
			if(m_ActiveMutants == m_ActivePlayers)
			{
				if(m_ActivePlayers >= g_Config.m_SvMinScorePlayers)
					UpdateHiScore();

				EndRound();
				m_GameRunning = 0;
				m_GamePreStarting = 0;
				m_GamePreStartingDelay = 0;
				GameServer()->SendBroadcast("突变体占领了世界!", -1);
			}
			else if(m_ActiveHumans == m_ActivePlayers)
			{	
				for(int u = 0; u < MAX_CLIENTS; u++)
				{
					if(GameServer()->m_apPlayers[u] && GameServer()->m_apPlayers[u]->m_MutatorTeam >= TEAM_HUMAN)
						GameServer()->m_apPlayers[u]->m_Score += g_Config.m_SurviveBonus;
				}

				if(m_ActivePlayers >= g_Config.m_SvMinScorePlayers)
					UpdateHiScore();
				
				EndRound();
				m_GameRunning = 0;
				m_GamePreStarting = 0;
				m_GamePreStartingDelay = 0;
				GameServer()->SendBroadcast("Tee在外星人的入侵下生存了下来!", -1);
				m_BroadcastTick = Server()->Tick() + 5000 * Server()->TickSpeed() / 1000;
			}
		}
	}
	else
	{
		m_GamePreStarting = 0;
		m_GamePreStartingDelay = 0;
		if(Server()->Tick() - m_BroadcastTick > 0)
		{
			m_FlipIdleBroadcast = 1 - m_FlipIdleBroadcast; 
			m_BroadcastTick = Server()->Tick() + 4 * Server()->TickSpeed();
			if(m_FlipIdleBroadcast == 1)
				GameServer()->SendBroadcast("至少需要2名玩家，请等等!", -1);
			else
				m_ShowHighScore = 1;
		}
	}
		
}

void CGameControllerMOD::UpdateHiScore()
{
	bool newscore = 0; 

	for(int i=0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;
		
		if(GameServer()->m_apPlayers[i]->m_MutatorTeam >= TEAM_HUMAN && GameServer()->m_apPlayers[i]->m_Score > m_HighScoreHuman)
		{
			newscore = 1;
			m_HighScoreHuman = GameServer()->m_apPlayers[i]->m_Score;
			strcpy(m_HighScoreNameHuman,Server()->ClientName(GameServer()->m_apPlayers[i]->GetCID()));	
			strcpy(m_HighScoreClanHuman,Server()->ClientClan(GameServer()->m_apPlayers[i]->GetCID()));
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "%s 打破了人类最高分记录!",m_HighScoreNameHuman);
			GameServer()->SendChatTarget(-1,aBuf);
		}	

		if(GameServer()->m_apPlayers[i]->m_MutatorTeam <= TEAM_MUTANT && GameServer()->m_apPlayers[i]->m_Score > m_HighScoreMutant)
		{
			newscore = 1;
			m_HighScoreMutant = GameServer()->m_apPlayers[i]->m_Score;
			strcpy(m_HighScoreNameMutant,Server()->ClientName(GameServer()->m_apPlayers[i]->GetCID()));	
			strcpy(m_HighScoreClanMutant,Server()->ClientClan(GameServer()->m_apPlayers[i]->GetCID()));
			char bBuf[512];
			str_format(bBuf, sizeof(bBuf), "%s 打破了可怕的突变体的最高分记录!",m_HighScoreNameMutant);
			GameServer()->SendChatTarget(-1,bBuf);
		}	
	}
	
	if(!newscore)
		return;

	// Probably a bit ugly but first time I use threads in C++ so...
	ScoreData * score = new ScoreData;
	score->m_HighScoreHuman = m_HighScoreHuman;
	strcpy(score->m_HighScoreNameHuman,m_HighScoreNameHuman);
	strcpy(score->m_HighScoreClanHuman,m_HighScoreClanHuman);
	score->m_HighScoreMutant = m_HighScoreMutant;
	strcpy(score->m_HighScoreNameMutant,m_HighScoreNameMutant);
	strcpy(score->m_HighScoreClanMutant,m_HighScoreClanMutant);
	void *pSaveHiScoreThread = thread_create(SaveHiScoreThread, score);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)pSaveHiScoreThread);
#endif
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "SaveHiScoreThread created.");
}

void CGameControllerMOD::SaveHiScoreThread(void *data)
{
	ScoreData * score = (ScoreData *) data;
	lock_wait(HiScoreMutex);

	char fName[4096];
	str_format(fName,sizeof(fName),"%s/%s-score.txt",g_Config.m_SvHighScorePath,g_Config.m_SvMap);

	std::fstream f;
	f.open(fName, std::ios::out | std::ios::trunc);
	if(!f.fail() && score)
	{
		f << score->m_HighScoreHuman << std::endl;
		f << score->m_HighScoreNameHuman << std::endl;
		f << score->m_HighScoreClanHuman << std::endl;
		f << score->m_HighScoreMutant << std::endl;
		f << score->m_HighScoreNameMutant << std::endl;
		f << score->m_HighScoreClanMutant << std::endl;
		delete score;
	}
	f.close();
	lock_release(HiScoreMutex);
}

bool CGameControllerMOD::LoadHiScore()
{
	char Score[16];
	m_HighScoreHuman = 0;
        m_HighScoreMutant = 0;
        mem_zero(m_HighScoreNameHuman,sizeof(m_HighScoreNameHuman));
        mem_zero(m_HighScoreClanHuman,sizeof(m_HighScoreClanHuman));
        mem_zero(m_HighScoreNameMutant,sizeof(m_HighScoreNameMutant));
        mem_zero(m_HighScoreClanMutant,sizeof(m_HighScoreClanMutant));

	char fName[4096];
	str_format(fName,sizeof(fName),"%s/%s-score.txt",g_Config.m_SvHighScorePath,g_Config.m_SvMap);

	std::fstream f;
	f.open(fName, std::ios::in);
	if(!f.fail() && !f.eof())
	{
		f.getline(Score,16);
		m_HighScoreHuman = atoi(Score);
		if(f.eof())
		{
			f.close();
			return 0;
		}
		f.getline(m_HighScoreNameHuman,MAX_NAME_LENGTH);
		if(f.eof())
		{
			f.close();
			return 0;
		}
		f.getline(m_HighScoreClanHuman,MAX_NAME_LENGTH);
		if(f.eof())
		{
			f.close();
			return 0;
		}
		f.getline(Score,16);
		m_HighScoreMutant = atoi(Score);
		if(f.eof())
		{
			f.close();
			return 0;
		}
		f.getline(m_HighScoreNameMutant,MAX_NAME_LENGTH);
		if(f.eof())
		{
			f.close();
			return 0;
		}
		f.getline(m_HighScoreClanMutant,MAX_NAME_LENGTH);
	
		f.close();
		return 1;
	}
	f.close();
	return 0;
}
