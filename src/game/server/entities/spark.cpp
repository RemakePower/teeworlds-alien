/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "spark.h"
#include <game/server/gamemodes/mod.h>

CSpark::CSpark(CGameWorld *pGameWorld, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_EvalTick = 0;
	m_DieTick = 0;
	m_NextDetonationTick = 0;
	m_InitDie = 0;
	GameWorld()->InsertEntity(this);
}

bool CSpark::HitCharacter()
{
	vec2 At;
		
	// Check Collision
	CCharacter *Hit = GameServer()->m_World.ClosestCharacter(m_Pos, 5.0f, 0);
	if(!Hit)
		return false;

	if(GameServer()->m_pController->MutPowerSupply())
		Hit->BreakCircuit();

	Hit->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage, -1, WEAPON_RIFLE);
	return true;
}

void CSpark::Reset()
{
	m_InitDie = 0;
}

void CSpark::Tick()
{
	if(Server()->Tick() % (100 * Server()->TickSpeed() / 1000) == 0)
		m_EvalTick = Server()->Tick();
	else
	{

		if(GameServer()->m_pController->MutPowerSupply() == 0)
		{
			if(!m_InitDie)
			{
				m_InitDie = 1;
				m_DieTick = Server()->Tick() + 5 * Server()->TickSpeed();
				m_NextDetonationTick = Server()->Tick();
				GameServer()->CreateExplosion(m_Pos, -1, WEAPON_GRENADE, true);
				GameServer()->CreateExplosion(m_Pos, -1, WEAPON_GRENADE, true);
			}
			else if(m_InitDie < 2)
			{
				if(Server()->Tick() - m_DieTick > 0)
				{
					GameServer()->CreateExplosion(m_Pos, -1, WEAPON_GRENADE, true);
					GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
					m_InitDie = 2;
				}
				else if(Server()->Tick() - m_NextDetonationTick > 0)
				{
					GameServer()->CreateExplosion(m_Pos, -1, WEAPON_GRENADE, true);
					GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
					m_NextDetonationTick = Server()->Tick() + (50 + rand()%500) * Server()->TickSpeed() / 1000;
				}
			}
		}
		else
			HitCharacter();
	}
}

void CSpark::Snap(int SnappingClient)
{
	if(GameServer()->m_pController->MutPowerSupply() == 0)
		return;

	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_Pos.x;
	pObj->m_FromY = (int)m_Pos.y;
	pObj->m_StartTick = m_EvalTick;
}
