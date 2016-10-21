/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_ENTITIES_SINGULARITY_H
#define GAME_SERVER_ENTITIES_SINGULARITY_H

#include <game/server/entity.h>


class CSingularity : public CEntity
{
public:
	CSingularity(CGameWorld *pGameWorld, vec2 Pos, float Range, float KillRange, int Owner, int LifeSpan);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	static const char * m_SubDescription[];

protected:
	void Suck();

private:
	vec2 m_From;
	int m_Owner;
	bool m_Temporary;
	int m_LifeSpan;
	float m_Range;
	float m_KillRange;
	bool m_SkipTick;
	int m_EvalTick;
};

#endif
