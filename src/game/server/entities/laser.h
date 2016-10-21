/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_LASER_H
#define GAME_SERVER_ENTITIES_LASER_H

#include <game/server/entity.h>


// Mutator
enum
{
        LASER_SUBTYPE_NORMAL=0,
        LASER_SUBTYPE_RIDDLE,
        LASER_SUBTYPE_TELE,
        LASER_SUBTYPE_HEALING,
	LASER_MAX_SUBTYPE,
};


class CLaser : public CEntity
{
public:
	CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int SubType);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	static const char * m_SubDescription[];

protected:
	bool HitCharacter(vec2 From, vec2 To);
	void DoBounce();

private:
	vec2 m_From;
	vec2 m_Dir;
	float m_Energy;
	int m_Bounces;
	int m_EvalTick;
	int m_Owner;
	int m_SubType;
};

#endif
