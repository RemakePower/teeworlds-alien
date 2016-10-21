/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "projectile.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, 
		int Clusters, int ClusterType, int ClusterLifeSpan, int ClusterDamage, bool ClusterExplosive, float ClusterForce,
                int ClusterSoundImpact)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;

	m_Clusters = Clusters;
	m_ClusterType = ClusterType;
	m_ClusterLifeSpan = ClusterLifeSpan;
	m_ClusterDamage = ClusterDamage;
	m_ClusterExplosive = ClusterExplosive;
	m_ClusterForce = ClusterForce;
	m_ClusterSoundImpact = ClusterSoundImpact;

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;

		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	m_LifeSpan--;

	if(TargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
			GameServer()->CreateSound(CurPos, m_SoundImpact);

		if(m_Explosive)
		{
			// Mutator
			if(m_Weapon == WEAPON_SHOTGUN)
			{
				GameServer()->CreateSound(CurPos, m_SoundImpact);
				GameServer()->CreateSound(CurPos, SOUND_RIFLE_BOUNCE);
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false);
			}
			else
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false);
		}

		else if(TargetChr)
			TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon);
		
		if(m_Clusters > 0)
		{
			int pOwner;	
			vec2 pSpread;
			CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
			if(OwnerChar)
			{
				pOwner = OwnerChar->GetPlayer()->GetCID();
			
				CProjectile * pCluster;
				for(int i =0; i < m_Clusters; i++)
				{
					//float a = GetAngle(PrevPos);
					float a = float(i*(360.f/float((m_Clusters+1))));
					pSpread = CurPos;
					pCluster = new CProjectile(GameWorld(), m_ClusterType,
						pOwner,
						PrevPos,
						vec2(cosf(a), sinf(a))*1.1f,
						m_ClusterLifeSpan,
						m_ClusterDamage,
						m_ClusterExplosive, 
						m_ClusterForce,
						m_ClusterSoundImpact, 
						m_ClusterType);
			
					CNetObj_Projectile p;
					pCluster->FillInfo(&p);

					CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
					Msg.AddInt(1);
					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
					Server()->SendMsg(&Msg, 0, pOwner);
				}
			}
		}
		GameServer()->m_World.DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
}

vec2 CProjectile::GetVel()
{
	return m_Direction;
}

void CProjectile::SetVel(vec2 Vel)
{
	m_Direction = Vel;
}
