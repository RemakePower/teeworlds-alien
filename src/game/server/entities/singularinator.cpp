/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "laser.h"
#include <engine/shared/config.h>
#include "singularinator.h"
#include "singularity.h"
#include <game/server/gamemodes/mod.h>

CSingularinator::CSingularinator(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int LifeSpan)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_LifeSpan = LifeSpan;
	m_SparkTTL = Server()->Tick() + 3 * Server()->TickSpeed();
	m_Pos = Pos;
	m_Fake = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction*4;
	m_EvalTick = 0;
	GameWorld()->InsertEntity(this);
	Move();
}

bool CSingularinator::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);
	if(!pHit)
		return false;
	else
	{
		new CSingularity(&GameServer()->m_World, m_Pos, g_Config.m_SvSingularityRangeW, g_Config.m_SvSingularityKillRange,m_Owner, m_LifeSpan);
		GameServer()->m_World.DestroyEntity(this);
		return true;
	}
}

void CSingularinator::Move()
{
	if(Server()->Tick() - m_SparkTTL > 0)
	{
		new CSingularity(&GameServer()->m_World, m_Pos, g_Config.m_SvSingularityRangeW, g_Config.m_SvSingularityKillRange,m_Owner, m_LifeSpan);
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	vec2 To = m_Pos + m_Dir;
	m_Dir = m_Dir * 1.05;

	if(GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;

			new CSingularity(&GameServer()->m_World, m_Pos, g_Config.m_SvSingularityRangeW, g_Config.m_SvSingularityKillRange,m_Owner, m_LifeSpan);
			GameServer()->m_World.DestroyEntity(this);
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
			return;
		}
	}
	else
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
		}
	}
}

void CSingularinator::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CSingularinator::Tick()
{
	Move();
	m_EvalTick = Server()->Tick() - (5 + Server()->Tick()%5);
	float a = float(Server()->Tick()%900)/2.5f;
	m_Fake = m_Pos + vec2(cosf(a), sinf(a))*(80.f-Server()->Tick()%70);
}

void CSingularinator::TickPaused()
{
	m_EvalTick = Server()->Tick() - (5 + Server()->Tick()%5);
	if(Server()->Tick() % 10 == 1)
	{
		float a = float(Server()->Tick()%900)/2.5f;
		m_Fake = m_Pos + vec2(cosf(a), sinf(a))*(80.f-Server()->Tick()%70);
	}
}

void CSingularinator::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_Fake.x;
	pObj->m_FromY = (int)m_Fake.y;
	pObj->m_StartTick = m_EvalTick;
}
