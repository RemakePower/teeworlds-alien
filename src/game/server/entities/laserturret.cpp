/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "laser.h"
#include "laserturret.h"
#include <game/server/gamemodes/mod.h>
#include <engine/shared/config.h>

CLaserTurret::CLaserTurret(CGameWorld *pGameWorld, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_FireDelayTick = Server()->Tick();
	GameWorld()->InsertEntity(this);
}

bool CLaserTurret::AimVictim()
{
	float ClosestRange = 9999; // just dummy maxval out of range to get some start-value for searching nearest
	CCharacter *pClosest = 0;
	vec2 CheckVel;

	CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
	{
		// just aim what you see
		if(!p || !p->IsAlive() || GameServer()->Collision()->IntersectLine(m_Pos, p->m_Pos, 0x0, 0x0))
			continue;	
		
		// just aim if victim moved fast enough (slow movement = no detection)
		if(length(p->GetVel()) < g_Config.m_SvMaxTurretAimVel)
			continue;	

		// just fire on mutants? 
		if(g_Config.m_SvTurretAimNoHumans && p->GetPlayer()->m_MutatorTeam >= TEAM_HUMAN)
			continue;
		
		float Len = distance(m_Pos, p->m_Pos);
		if(Len < p->m_ProximityRadius+g_Config.m_SvTurretRange)
		{
			if(Len < ClosestRange)
			{
				ClosestRange = Len;
				pClosest = p;
			}
		}
	}
	
	if(pClosest)
	{
		GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
		new CLaser(GameWorld(), m_Pos, normalize(pClosest->m_Pos - m_Pos), GameServer()->Tuning()->m_LaserReach, -1, LASER_SUBTYPE_NORMAL);
		return true;
	}
	else
		return false;
}

void CLaserTurret::Reset()
{
	return;
}

void CLaserTurret::Tick()
{
	if(Server()->Tick() - m_FireDelayTick > 0)
	{
		if(GameServer()->m_pController->MutPowerSupply() == 1 && AimVictim())
			m_FireDelayTick = Server()->Tick() + 600 * Server()->TickSpeed() / 1000;
		else
			m_FireDelayTick = Server()->Tick() + 10 * Server()->TickSpeed() / 1000;
	}
}

void CLaserTurret::Snap(int SnappingClient)
{
	return;
}
