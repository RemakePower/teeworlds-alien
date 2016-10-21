/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_ENTITIES_SINGULARINATOR_H
#define GAME_SERVER_ENTITIES_SINGULARINATOR_H

#include <game/server/entity.h>

class CSingularinator : public CEntity
{
public:
	CSingularinator(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int LifeSpan);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	static const char * m_SubDescription[];

protected:
	bool HitCharacter(vec2 From, vec2 To);
	void Move();

private:
	vec2 m_From;
	vec2 m_Fake;
	vec2 m_Dir;
	float m_Energy;
	int m_EvalTick;
	int m_Owner;
	int m_SparkTTL;
	int m_LifeSpan;
};

#endif
