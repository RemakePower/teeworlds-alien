/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_ENTITIES_LASERTURRET_H
#define GAME_SERVER_ENTITIES_LASERTURRET_H

#include <game/server/entity.h>


class CLaserTurret : public CEntity
{
public:
	CLaserTurret(CGameWorld *pGameWorld, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	static const char * m_SubDescription[];

protected:
	bool AimVictim();

private:
	int m_FireDelayTick;
};

#endif
