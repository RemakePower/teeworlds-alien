/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

class CProjectile : public CEntity
{
public:
	CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon, 
		int Clusters=0, int ClusterType=0, int ClusterLifeSpan=0, int ClusterDamage=0, bool ClusterExplosive=0, float ClusterForce=0, 
		int ClusterSoundImpact=0);

	vec2 GetPos(float Time);
	vec2 GetVel();
	void SetVel(vec2 Vel);
	void FillInfo(CNetObj_Projectile *pProj);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_Direction;
	int m_LifeSpan;
	int m_Owner;
	int m_Type;
	int m_Damage;
	int m_SoundImpact;
	int m_Weapon;
	float m_Force;
	int m_StartTick;
	bool m_Explosive;
	
	int m_Clusters;
	int m_ClusterType;
	int m_ClusterLifeSpan;
	int m_ClusterDamage;
	int m_ClusterExplosive;
	int m_ClusterForce;
	int m_ClusterSoundImpact;
};

#endif
