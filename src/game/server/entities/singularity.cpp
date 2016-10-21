/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "laser.h"
#include "singularity.h"
#include "projectile.h"
#include <game/server/gamemodes/mod.h>

CSingularity::CSingularity(CGameWorld *pGameWorld, vec2 Pos, float Range, float KillRange, int Owner, int LifeSpan)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Owner = Owner;
	if(LifeSpan)
		m_Temporary = 1;

	m_LifeSpan = LifeSpan;
	m_Range = Range;
	m_KillRange = KillRange;
	m_Pos = Pos;
	m_From = Pos;
	m_EvalTick = 0;
	GameWorld()->InsertEntity(this);
}

void CSingularity::Suck()
{
	float Len;
	
	CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
	{
		if(!p || !p->IsAlive())
			continue;	
		
		Len = distance(m_Pos, p->m_Pos);
		if(Len < m_KillRange)
		{
			if(m_Owner > -1)
				p->Die(m_Owner,WEAPON_RIFLE);
			else
				p->Die(p->GetPlayer()->GetCID(),WEAPON_WORLD);
		}
		else if(Len < m_Range)
			p->SetVel(p->GetVel() + (normalize(m_Pos - p->m_Pos) * exp((m_Range - Len)/(m_Range/2.5f))));
	}

	// save CPU for projectiles
	if(m_SkipTick)
		return;
	
	CProjectile *s = (CProjectile *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE);
	for(; s; s = (CProjectile *)s->TypeNext())
	{
		if(!s)
			continue;
		
		Len = distance(m_Pos, s->m_Pos);
		if(Len < m_Range*1.5)
			s->SetVel(s->GetVel() + ((normalize(m_Pos - s->m_Pos) * exp((m_Range*1.5 - Len)/(m_Range*0.6f)))*0.04f));
	}
}

void CSingularity::Reset()
{
	if(m_Temporary)
		GameServer()->m_World.DestroyEntity(this);
	else
		return;
}

void CSingularity::Tick()
{
	m_EvalTick = Server()->Tick() - (5 + Server()->Tick()%5);
	m_SkipTick = 1 - m_SkipTick;

	float a = float(Server()->Tick()%900)/2.5f;
	m_From = m_Pos + vec2(cosf(a), sinf(a))*(300.f-Server()->Tick()%270);

	if(m_Temporary && Server()->Tick() - m_LifeSpan > 0 )
		GameServer()->m_World.DestroyEntity(this);
	else
		Suck();
}

void CSingularity::TickPaused()
{
        m_EvalTick = Server()->Tick() - (5 + Server()->Tick()%5);
	if(Server()->Tick() % 10 == 1)
	{
        	float a = float(Server()->Tick()%900)/2.5f;
        	m_From = m_Pos + vec2(cosf(a), sinf(a))*(300.f-Server()->Tick()%270);
	}
}

void CSingularity::Snap(int SnappingClient)
{
        if(NetworkClipped(SnappingClient))
		return;
	
        CNetObj_Laser *pObj; 
	pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
        if(!pObj)
                return;

        pObj->m_X = (int)m_Pos.x;
        pObj->m_Y = (int)m_Pos.y;
        pObj->m_FromX = (int)m_From.x;
        pObj->m_FromY = (int)m_From.y;
        pObj->m_StartTick = m_EvalTick;
	return;
}
