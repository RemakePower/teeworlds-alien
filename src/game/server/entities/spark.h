/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_ENTITIES_SPARK_H
#define GAME_SERVER_ENTITIES_SPARK_H

#include <game/server/entity.h>

class CSpark : public CEntity
{
public:
	CSpark(CGameWorld *pGameWorld, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

protected:
	bool HitCharacter();

private:
	int m_EvalTick;
	int m_DieTick;
	int m_NextDetonationTick;
	int m_InitDie;
};

#endif
