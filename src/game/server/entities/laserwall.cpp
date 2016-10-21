/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "laserwall.h"
#include <game/server/gamemodes/mod.h>
#include <engine/shared/config.h>

CLaserWall::CLaserWall(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_From = Pos; 
	m_FromFinal = m_Pos + m_Dir * m_Energy;
	m_EvalTick = 0;

	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(!pOwnerChar)
		return;
	else if(pOwnerChar->GetPlayer()->m_ScienceExplored&SCIENCE_BATTERY)
		m_ConsumePowerTick = Server()->Tick() + g_Config.m_SvLaserWallDelay * Server()->TickSpeed() + g_Config.m_SvSuperBattery * Server()->TickSpeed() / 1000;
	else
		m_ConsumePowerTick = Server()->Tick() + g_Config.m_SvLaserWallDelay * Server()->TickSpeed() + g_Config.m_SvNormalBattery * Server()->TickSpeed() / 1000;
	
	m_FirstTick = Server()->Tick();	
	m_InitDelayTick = Server()->Tick() + Server()->TickSpeed();	
	m_InitDelayCounter = 0;
	GameWorld()->InsertEntity(this);
}


bool CLaserWall::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	// Check if owner is still alive and no Mutant
	if(!pOwnerChar)
	{
		GameServer()->m_World.DestroyEntity(this);
		return false;
	} 
	else if(pOwnerChar->GetPlayer()->m_MutatorTeam <= TEAM_MUTANT)
	{
		pOwnerChar->DestroyLaserWall();
		return false;
	}
	
	// Check absolute timelimit if required	
	if((g_Config.m_SvLWLimitNonAlMap && !GameServer()->m_pController->FoundHammerPickups()) || g_Config.m_SvLWLimit)
	{
		if(pOwnerChar->GetPlayer()->m_ScienceExplored&SCIENCE_BATTERY)
		{
			if(Server()->Tick() - (m_FirstTick + g_Config.m_SvSuperBattery * Server()->TickSpeed() / 100) > 0)
			{	
				GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
				GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false);
				pOwnerChar->DestroyLaserWall();
				return false;
			}
		}
		else
		{
			if(Server()->Tick() - (m_FirstTick + g_Config.m_SvNormalBattery * Server()->TickSpeed() / 100) > 0)
			{
				GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
				GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false);
				pOwnerChar->DestroyLaserWall();
				return false;
			}
		}
	}
	
	// Consume Power 
	if(m_InitDelayCounter == g_Config.m_SvLaserWallDelay && Server()->Tick() - m_ConsumePowerTick > 0)
	{
		if(!pOwnerChar->DecreaseArmor(1))
		{
			pOwnerChar->DestroyLaserWall();
			return false;
		}
		else
		{
			if(pOwnerChar->GetPlayer()->m_ScienceExplored&SCIENCE_BATTERY)
				m_ConsumePowerTick = Server()->Tick() + g_Config.m_SvSuperBattery * Server()->TickSpeed() / 1000;
			else
				m_ConsumePowerTick = Server()->Tick() + g_Config.m_SvNormalBattery * Server()->TickSpeed() / 1000;
		}
	}	
		
	// Check Collision
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);
	if(!pHit)
		return false;

	pHit->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE);
	return true;
}

void CLaserWall::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLaserWall::Tick()
{
	if(Server()->Tick() % (170 * Server()->TickSpeed() / 1000) == 0)
		m_EvalTick = Server()->Tick();
	if(m_InitDelayCounter < g_Config.m_SvLaserWallDelay && Server()->Tick() - m_InitDelayTick > 0)
	{
		m_InitDelayCounter++;
		if(m_InitDelayCounter == g_Config.m_SvLaserWallDelay)
		{
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
			// Test if Laser hit wall (if yes just cut no bounce).
			vec2 TestFrom = m_FromFinal;
			if(GameServer()->Collision()->IntersectLine(m_Pos, TestFrom, 0x0, &TestFrom))
			{
				m_Energy = distance(TestFrom, m_Pos)*1.1f;
				m_From = m_Pos + m_Dir * m_Energy;
			}
			else	
				m_From = m_FromFinal;
			
			HitCharacter(m_Pos, m_From);	
		}
		else
		{
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
			m_InitDelayTick = Server()->Tick() + Server()->TickSpeed();
		}
	}
	else
		HitCharacter(m_Pos, m_From);	
}

void CLaserWall::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
