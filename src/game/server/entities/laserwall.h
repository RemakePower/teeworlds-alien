/* (c) Martin Grasberger - See alien-license.txt and licence.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_ENTITIES_LASERWALL_H
#define GAME_SERVER_ENTITIES_LASERWALL_H

#include <game/server/entity.h>

class CLaserWall : public CEntity
{
public:
	CLaserWall(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

protected:
	bool HitCharacter(vec2 From, vec2 To);

private:
	vec2 m_From;
	vec2 m_FromFinal;
	vec2 m_Dir;
	float m_Energy;
	int m_EvalTick;
	int m_FirstTick;
	int m_Owner;
	int m_ConsumePowerTick;
	int m_InitDelayTick;
        int m_InitDelayCounter;

};

#endif
