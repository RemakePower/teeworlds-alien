/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include <game/server/gamemodes/mod.h>

#include "character.h"
#include "laser.h"
#include "laserwall.h"
#include "projectile.h"
#include "singularinator.h"

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;

	// Mutator
	m_LaserWall = 0;
	m_QueuedLaser = LASER_SUBTYPE_NORMAL;	
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_ActiveWeapon = WEAPON_GUN;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;

	m_pPlayer = pPlayer;
	m_Pos = Pos;
	
	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
	m_Core.m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;

	// No nead for next line ... we had to change too much stuff already here anyway ... 
	// GameServer()->m_pController->OnCharacterSpawn(this);

	// Mutator
	
	// Init Eyes
	m_MutanticEmoteTick = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
	if(GetPlayer()->m_MutatorTeam > TEAM_REAPER)
		SetEmote(EMOTE_NORMAL,-1);
	else
		SetEmote(EMOTE_ANGRY,m_MutanticEmoteTick);
	
	m_MutanticEyeOn = 0;

	// Default settings of health and hammer	
	IncreaseHealth(10);
	GiveWeapon(WEAPON_HAMMER, -1);

	// Laser Subtype stuff
	m_QueuedLaser = LASER_SUBTYPE_NORMAL;
	m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo = 0;
	m_aWeapons[WEAPON_RIFLE].m_NormalAmmo = 0;
	m_LaserCycleInfoSwitch = 0;
	mem_zero(&m_OwnLaserSubType,sizeof(m_OwnLaserSubType));
	m_OwnLaserSubType[0] = true; // always own knowledge of normal laser :)
	if(m_pPlayer->m_ScienceExplored&SCIENCE_LASER_RIDDLE)
		m_OwnLaserSubType[1] = true;
	if(m_pPlayer->m_ScienceExplored&SCIENCE_LASER_TELE)
		m_OwnLaserSubType[2] = true;
	if(m_pPlayer->m_ScienceExplored&SCIENCE_LASER_HEALING)
		m_OwnLaserSubType[3] = true;

	// On tile or entity switches
	m_OnTurret = 0;
	m_PrevOnTurret = 0;
	m_TurretAmmo = 0;
	m_TurretActive = 0;
	m_OnScience = 0;
	m_PrevOnScience = 0;
	m_Invisible = 0;
	m_OnEnergyUp = 0;
	m_PrevOnEnergyUp = 0;
	m_OnReapinator = 0;
	m_PrevOnReapinator = 0;

	m_pPlayer->m_MultiScore = 0;

	// Timer
	m_BioHazardDamageTick = Server()->Tick();
	m_pPlayer->m_MultiScoreTick = Server()->Tick();
	m_ShowInfoTick = Server()->Tick();
	m_EnergyUpTick = Server()->Tick();
	
	
	// Multijump
	m_MutanticArmorDJCount = 0;

	// Owner of airstrike
	m_OwnAirstrike = 0;

	// Set right team depending on gamestatus 
	if(GameServer()->m_pController->MutGameRunning() == 0)
		m_pPlayer->m_MutatorTeam = TEAM_HUMAN;
	else if(m_pPlayer->m_MutatorTeam >= TEAM_HUMAN)
		m_pPlayer->m_MutatorTeam = TEAM_MUTANT;

	// Give weapons depending on team
	if(m_pPlayer->m_MutatorTeam <= TEAM_MUTANT)
	{
		SetWeapon(WEAPON_HAMMER);
		m_MutanticSpawnProtection = Server()->Tick() + 1000 * Server()->TickSpeed() / 1000;;
		if(m_pPlayer->m_MutaticLevels&MUTATOR_START_SHIELDS)
			IncreaseArmor(10);
	}
	else if(m_pPlayer->m_MutatorTeam < TEAM_HERO)
	{
		GiveWeapon(WEAPON_GUN, 10);
		SetWeapon(WEAPON_GUN);
	}
	else
	{	
		GiveWeapon(WEAPON_GUN, 5);
		GiveWeapon(WEAPON_SHOTGUN, 10);
		SetWeapon(WEAPON_SHOTGUN);
	}
	return true;
}

void CCharacter::SetPreStartGame()
{
	m_MutanticEmoteTick = Server()->Tick();
}

void CCharacter::Destroy()
{
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);

	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS)
		m_ActiveWeapon = 0;
}

bool CCharacter::IsGrounded()
{
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}

void CCharacter::HandleNinja()
{
	if(m_ActiveWeapon != WEAPON_NINJA)
		return;

	if ((Server()->Tick() - m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		m_aWeapons[WEAPON_NINJA].m_Got = false;
		m_ActiveWeapon = m_LastWeapon;

		SetWeapon(m_ActiveWeapon);
		return;
	}

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir*m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, m_pPlayer->GetCID(), WEAPON_NINJA);
			}
		}

		return;
	}

	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_aWeapons[WEAPON_NINJA].m_Got || m_pPlayer->m_MutatorTeam == TEAM_MUTANT 
				|| (m_pPlayer->m_MutatorTeam == TEAM_REAPER && !(m_QueuedWeapon == WEAPON_HAMMER || m_QueuedWeapon == WEAPON_RIFLE)) || m_OnTurret)
		return;

	// Backup Ammo state if laser (healing laser is different ammo)
	if(m_ActiveWeapon == WEAPON_RIFLE)
	{
		if(m_aWeapons[WEAPON_RIFLE].m_SubType == LASER_SUBTYPE_HEALING)
			m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo = m_aWeapons[WEAPON_RIFLE].m_Ammo;

		else
			m_aWeapons[WEAPON_RIFLE].m_NormalAmmo = m_aWeapons[WEAPON_RIFLE].m_Ammo;
	}

	// switch Weapon
	SetWeapon(m_QueuedWeapon);

	// Handle LaserSubtype Ammo and Broadcast-info-timer
	if(m_ActiveWeapon == WEAPON_RIFLE && m_pPlayer->m_MutatorTeam >= TEAM_HUMAN)
	{
		if(m_aWeapons[WEAPON_RIFLE].m_SubType == LASER_SUBTYPE_HEALING)
		{
			// Drop HEALING_LASER from cycle if no ammo is available anymore
			if(m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo < 1 || m_OwnLaserSubType[LASER_SUBTYPE_HEALING] == false)
			{
				m_OwnLaserSubType[LASER_SUBTYPE_HEALING] = false;
				m_aWeapons[WEAPON_RIFLE].m_SubType = LASER_SUBTYPE_NORMAL;
			}
		}

		// Use better riddle-laser if available
		if(m_aWeapons[WEAPON_RIFLE].m_SubType == LASER_SUBTYPE_NORMAL && m_OwnLaserSubType[LASER_SUBTYPE_RIDDLE])
			m_aWeapons[WEAPON_RIFLE].m_SubType = LASER_SUBTYPE_RIDDLE;

		// Just inform if some special type was choosen
		if(m_aWeapons[WEAPON_RIFLE].m_SubType != LASER_SUBTYPE_NORMAL)
		{
			m_LaserCycleTick = Server()->Tick() + 250 * Server()->TickSpeed() / 1000; // Avoid flood at fast cycling (just inform if he keep it)
			m_LaserCycleInfoSwitch = 1;
		}
		
		// Use right ammo-type
		switch(m_aWeapons[WEAPON_RIFLE].m_SubType)
		{
			case LASER_SUBTYPE_NORMAL:
			case LASER_SUBTYPE_RIDDLE:
			case LASER_SUBTYPE_TELE:
				m_aWeapons[WEAPON_RIFLE].m_Ammo = m_aWeapons[WEAPON_RIFLE].m_NormalAmmo; break;
			case LASER_SUBTYPE_HEALING:
				m_aWeapons[WEAPON_RIFLE].m_Ammo = m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo; break; 
		}
	}
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;
	
	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon+1)%NUM_WEAPONS;
			if(m_aWeapons[WantedWeapon].m_Got)
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_aWeapons[WantedWeapon].m_Got)
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon > 0)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_aWeapons[WantedWeapon].m_Got)
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::CycleLaserSubTypes()
{
	if(m_pPlayer->m_MutatorTeam <= TEAM_MUTANT || !m_aWeapons[WEAPON_RIFLE].m_Got)
		return;

	int ActiveLaser = m_aWeapons[WEAPON_RIFLE].m_SubType;

	// Drop Healing-Laser if not available anymore
	if(m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo == 0)
		m_OwnLaserSubType[LASER_SUBTYPE_HEALING] = false;

	for(int i = 0; i < LASER_MAX_SUBTYPE; i++)
	{
		m_QueuedLaser = ++m_QueuedLaser%LASER_MAX_SUBTYPE;
		
		if(m_QueuedLaser == LASER_SUBTYPE_NORMAL && m_OwnLaserSubType[LASER_SUBTYPE_RIDDLE])
			m_QueuedLaser = LASER_SUBTYPE_RIDDLE;

		if(m_OwnLaserSubType[m_QueuedLaser])
			break;
	}
		
	if(ActiveLaser == m_QueuedLaser)
		return;
	else
	{
		if(ActiveLaser == LASER_SUBTYPE_HEALING)
                        m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo = m_aWeapons[WEAPON_RIFLE].m_Ammo;
		else
			m_aWeapons[WEAPON_RIFLE].m_NormalAmmo = m_aWeapons[WEAPON_RIFLE].m_Ammo;
		
		m_aWeapons[WEAPON_RIFLE].m_SubType = m_QueuedLaser;
	}

	if(m_ActiveWeapon == WEAPON_RIFLE)
		SetWeapon(WEAPON_HAMMER);

	m_QueuedWeapon = WEAPON_RIFLE;
	DoWeaponSwitch();
}

void CCharacter::AirStrike()
{
	if(!m_OwnAirstrike)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(),"空袭不可用");
		return;
	}
	else if(m_pPlayer->m_MutatorTeam <= TEAM_MUTANT)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(),"突变体不需要空袭");
		return;
	}
	
	m_OwnAirstrike--;

	vec2 pStrikeDir = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY)); 
        for(int bomb = -320; bomb < (GameServer()->Collision()->GetWidth()+10)*32; bomb += g_Config.m_SvHumAirStrikeStep)
        {
		CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
			m_pPlayer->GetCID(),
			vec2(bomb,0),
			pStrikeDir,
			g_Config.m_SvAirStrikeTTL,
			1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE,
			g_Config.m_SvAirStrikeClusters, WEAPON_GRENADE, Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime, 1, 1, 0, SOUND_GRENADE_EXPLODE);

		// pack the Projectile and send it to the client Directly
		CNetObj_Projectile p;
		pProj->FillInfo(&p);

		CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
		Msg.AddInt(1);
		for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
			Msg.AddInt(((int *)&p)[i]);
		Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());
        }
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0)
		return;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_ActiveWeapon == WEAPON_GRENADE || m_ActiveWeapon == WEAPON_SHOTGUN || m_ActiveWeapon == WEAPON_RIFLE || m_TurretActive)
		FullAuto = true;

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && m_aWeapons[m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_aWeapons[m_ActiveWeapon].m_Ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
		return;
	}

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;

	switch(m_ActiveWeapon)
	{
		case WEAPON_HAMMER:
		{
			// Create laserwall if human and not on Science station
			if(m_pPlayer->m_MutatorTeam >= TEAM_HUMAN)
			{
				if(!m_OnScience && !m_OnEnergyUp)
				{
					if(m_LaserWall)
					{
						DestroyLaserWall();
						GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
					}
					else if(m_Armor > 0)
					{
						DecreaseArmor(1);
						m_LaserWall = new CLaserWall(GameWorld(), m_Pos, Direction, g_Config.m_SvLaserWallLength ,m_pPlayer->GetCID());
						GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
					}
				}
			}

			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if ((pTarget == this) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
					m_pPlayer->GetCID(), m_ActiveWeapon);
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;

		} break;

		case WEAPON_GUN:
		{
			switch(m_TurretActive)
			{
				// normal gun behavior
				case 0:
				{
					CProjectile *pProj;
					if(m_pPlayer->m_ScienceExplored&SCIENCE_GUN_MK2)
					{
						pProj = new CProjectile(GameWorld(), WEAPON_GUN,
							m_pPlayer->GetCID(),
							ProjStartPos,
							Direction,
							(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
							1, 0, float(g_Config.m_SvGunMK2Force), -1, WEAPON_GUN);
					}
					else
					{
						pProj = new CProjectile(GameWorld(), WEAPON_GUN,
							m_pPlayer->GetCID(),
							ProjStartPos,
							Direction,
							(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
							1, 0, 0, -1, WEAPON_GUN);
					}
						

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
					Msg.AddInt(1);
					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
	
					Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

					GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
				} break;
				// Fire turret
				case 1:
				{
					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
						m_pPlayer->GetCID(),
						ProjStartPos,
						Direction,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
						g_Config.m_SvSGTurretNonExpDmg, 1, 2.f, SOUND_GRENADE_EXPLODE, WEAPON_SHOTGUN);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
					Msg.AddInt(1);
					for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);

					Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

					GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
					GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
					GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
				} break;
			}
		} break;

		case WEAPON_SHOTGUN:
		{
			int ShotSpread = 2;
			float HeroBonus;
			int DoExplode;
			int ExplodeSound;

			if(m_pPlayer->m_MutatorTeam == TEAM_HERO || m_pPlayer->m_ScienceExplored&SCIENCE_SHOTGUN_FORCEX2)
				HeroBonus = float(g_Config.m_SvShotMK2Force);
			else
				HeroBonus = 0;

			if(m_pPlayer->m_ScienceExplored&SCIENCE_SHOTGUN_EXPL)
			{
				DoExplode = 1;
				ExplodeSound = SOUND_GRENADE_EXPLODE;
			}
			else
			{
				DoExplode = 0;
				ExplodeSound = -1;
			}
				
			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(ShotSpread*2+1);

			for(int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
				float a = GetAngle(Direction);
				a += Spreading[i+2];
				float v = 1-(absolute(i)/(float)ShotSpread);
				float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					vec2(cosf(a), sinf(a))*Speed,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
					1, DoExplode, HeroBonus, ExplodeSound, WEAPON_SHOTGUN);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
			}

			Server()->SendMsg(&Msg, 0,m_pPlayer->GetCID());

			GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
		} break;

		case WEAPON_GRENADE:
		{
			CProjectile *pProj;
			// Clusterbombs
			if(m_pPlayer->m_ScienceExplored&SCIENCE_GRENADE_CLUSTER)
			{
				pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
					1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE,
					g_Config.m_SvClusters, WEAPON_GRENADE, Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime, 1, 1, 0, SOUND_GRENADE_EXPLODE);
			}
			// Normal grenades
			else
			{
				pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
					m_pPlayer->GetCID(),
					ProjStartPos,
					Direction,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
					1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);
			}
				
			// pack the Projectile and send it to the client Directly
			CNetObj_Projectile p;
			pProj->FillInfo(&p);

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(1);
			for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
				Msg.AddInt(((int *)&p)[i]);
			Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
		} break;

		case WEAPON_RIFLE:
		{
			if(m_pPlayer->m_MutatorTeam == TEAM_REAPER)
			// Black hole weapon
			{
				new CSingularinator(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), Server()->Tick() + g_Config.m_SvSingularinatorTTL * Server()->TickSpeed());
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
				GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
			}
			else
			// Human lasers
			{
				new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), m_aWeapons[WEAPON_RIFLE].m_SubType);
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
				if(m_aWeapons[WEAPON_RIFLE].m_SubType == LASER_SUBTYPE_HEALING)
				{
					m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo--;
					if(m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo < 1)
						m_OwnLaserSubType[LASER_SUBTYPE_HEALING] = 0;
				}
				else
					m_aWeapons[WEAPON_RIFLE].m_NormalAmmo--;
			}
		} break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			m_NumObjectsHit = 0;

			m_Ninja.m_ActivationDir = Direction;
			m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

			GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);
		} break;

	}

	m_AttackTick = Server()->Tick();
	m_aWeapons[m_ActiveWeapon].m_Ammo--;

	if(m_pPlayer->m_MutatorTeam >= TEAM_HUMAN)
	{
		if(m_TurretActive) // use slower reload time for Turret 
			m_ReloadTimer = Server()->TickSpeed()/10;
		else if(m_pPlayer->m_ScienceExplored&SCIENCE_SHOTGUN_FASTRELOAD && m_ActiveWeapon == WEAPON_SHOTGUN)
			m_ReloadTimer = (g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay/2) * Server()->TickSpeed() / 1000;
		else if(!m_ReloadTimer)
			m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 1000;
	}
	else if(!m_ReloadTimer)
		m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 1000;
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	// ammo regen
	if(m_TurretActive)
		return;

	int AmmoRegenTime = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Ammoregentime;
	if(AmmoRegenTime)
	{
		// If equipped and not active, regen ammo?
		if (m_ReloadTimer <= 0)
		{
			if (m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if ((Server()->Tick() - m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_aWeapons[m_ActiveWeapon].m_Ammo + 1, 10);
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}

	return;
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo, int Special)
{
	if(Weapon == WEAPON_RIFLE)
	{
		if(Special == 1)
		{
			if(m_aWeapons[Weapon].m_SpecialAmmo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo)
			{
				m_aWeapons[Weapon].m_Got = true;
				m_aWeapons[Weapon].m_SpecialAmmo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
				if(m_ActiveWeapon == WEAPON_RIFLE && m_aWeapons[WEAPON_RIFLE].m_SubType == LASER_SUBTYPE_HEALING)
					m_aWeapons[WEAPON_RIFLE].m_Ammo = m_aWeapons[WEAPON_RIFLE].m_SpecialAmmo;
				return true;
			}
		}
		else if(m_aWeapons[Weapon].m_NormalAmmo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo || !m_aWeapons[Weapon].m_Got)
		{
			m_aWeapons[Weapon].m_Got = true;
			m_aWeapons[Weapon].m_NormalAmmo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
			if(m_ActiveWeapon == WEAPON_RIFLE && m_aWeapons[WEAPON_RIFLE].m_SubType != LASER_SUBTYPE_HEALING)
				m_aWeapons[WEAPON_RIFLE].m_Ammo = m_aWeapons[WEAPON_RIFLE].m_NormalAmmo;
			return true;
		}
	}
	else if(m_aWeapons[Weapon].m_Ammo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo || !m_aWeapons[Weapon].m_Got)
	{
		m_aWeapons[Weapon].m_Got = true;
		m_aWeapons[Weapon].m_Ammo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
		return true;
	}
	return false;
}

void CCharacter::GiveNinja()
{
	m_Ninja.m_ActivationTick = Server()->Tick();
	m_aWeapons[WEAPON_NINJA].m_Got = true;
	m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
	if (m_ActiveWeapon != WEAPON_NINJA)
		m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;

	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
	if(Emote == EMOTE_FLASHING)
	{	
		m_MutanticEyeOn = 1;
	        m_MutanticEyeFlip = 1;
		m_MutanticEyeFlipTick = Server()->Tick() + 250 * Server()->TickSpeed() / 1000;
		m_EmoteType = EMOTE_NORMAL;
	}	
	else
		m_MutanticEyeOn = 0;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// or are not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

        // it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::Tick()
{
	// useless for this mod
	/*if(m_pPlayer->m_ForceBalanced)
	{
		char Buf[128];
		str_format(Buf, sizeof(Buf), "You were moved to %s due to team balancing", GameServer()->m_pController->GetTeamName(m_pPlayer->GetTeam()));
		GameServer()->SendBroadcast(Buf, m_pPlayer->GetCID());

		m_pPlayer->m_ForceBalanced = false;
	}*/
	
	// Get Input 	
	m_Core.m_Input = m_Input;
	m_Core.Tick(true);

	// Reset to check again later
	m_OnTurret = 0;
	m_OnEnergyUp = 0;
	m_OnReapinator = 0;
	
	// Get collisions
	m_Col1 = GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f);
	m_Col2 = GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f);
	m_Col3 = GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f);
	m_Col4 = GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f);

	// handle death-tiles and leaving gamelayer
	if(m_Col1&CCollision::COLFLAG_DEATH || m_Col2&CCollision::COLFLAG_DEATH || m_Col3&CCollision::COLFLAG_DEATH || m_Col4&CCollision::COLFLAG_DEATH 
			|| GameLayerClipped(m_Pos))
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);

	// handle invisible tiles
	if(m_Col1&CCollision::COLFLAG_INVIS || m_Col2&CCollision::COLFLAG_INVIS || m_Col3&CCollision::COLFLAG_INVIS || m_Col4&CCollision::COLFLAG_INVIS)
		m_Invisible = 1;
	else
		m_Invisible = 0;

	if(m_Col1&CCollision::COLFLAG_BIOHAZARD || m_Col2&CCollision::COLFLAG_BIOHAZARD 
			|| m_Col3&CCollision::COLFLAG_BIOHAZARD || m_Col4&CCollision::COLFLAG_BIOHAZARD)
	{
		if(m_pPlayer->m_MutatorTeam > TEAM_REAPER && !(m_pPlayer->m_MutatorTeam == TEAM_MUTANT && m_pPlayer->m_MutaticLevels&MUTATOR_NOBIOHAZARD))
		{
		        if(!m_MutanticEyeOn && Server()->Tick() - m_BioHazardDamageTick > 0)
			{
                		TakeMinorDamage();
				GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);
				m_BioHazardDamageTick = Server()->Tick() + 150 * Server()->TickSpeed() / 1000;
				m_EmoteType = EMOTE_PAIN;
				m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
        		}	
		}
	}
	else if(m_Col1&CCollision::COLFLAG_REAPER || m_Col2&CCollision::COLFLAG_REAPER
			|| m_Col3&CCollision::COLFLAG_REAPER || m_Col4&CCollision::COLFLAG_REAPER)
	{
		m_OnReapinator = 1;
	}
	else if(m_Col1&CCollision::COLFLAG_ENERGYUP || m_Col2&CCollision::COLFLAG_ENERGYUP
			|| m_Col3&CCollision::COLFLAG_ENERGYUP || m_Col4&CCollision::COLFLAG_ENERGYUP)
	{
		m_OnEnergyUp = 1;
	}
	// handle human only tiles
	else if(m_pPlayer->m_MutatorTeam >= TEAM_HUMAN)
	{
		// set on turret switch
		if(m_Col1&CCollision::COLFLAG_TURRET || m_Col2&CCollision::COLFLAG_TURRET 
				|| m_Col3&CCollision::COLFLAG_TURRET || m_Col4&CCollision::COLFLAG_TURRET)
			m_OnTurret = 1; 
	}
	
	// handle Weapons
	HandleWeapons();

	// handle human stuff
	if(m_pPlayer->m_MutatorTeam >= TEAM_HUMAN)
	{
		// OnScience Reset
		if(m_OnScience && Server()->Tick() - m_OnScienceOffTick > 0)
			m_OnScience = 0;

		// turret 
		if(m_OnTurret)
		{
			m_TurretActive = 1;
			if(!m_PrevOnTurret)
			{
				m_LastWeapon = m_ActiveWeapon;
				m_PrevTurretSwitchAmmo = m_aWeapons[WEAPON_GUN].m_Ammo;
				m_PrevTurretSwitchGot = m_aWeapons[WEAPON_GUN].m_Got;
				SetWeapon(WEAPON_GUN);
				m_aWeapons[m_ActiveWeapon].m_Ammo = m_TurretAmmo;
				m_aWeapons[m_ActiveWeapon].m_Got = true;
				
			}
			if(m_aWeapons[m_ActiveWeapon].m_Ammo < 1 && m_Armor > 0)
			{
				m_aWeapons[m_ActiveWeapon].m_Ammo = 10;
				m_Armor--;
			}
		}	
		else
		{
			// reset weapon on turret leave 
			if(m_PrevOnTurret)
			{
				m_TurretAmmo = m_aWeapons[WEAPON_GUN].m_Ammo;
				m_aWeapons[WEAPON_GUN].m_Ammo = m_PrevTurretSwitchAmmo;
				m_aWeapons[WEAPON_GUN].m_Got = m_PrevTurretSwitchGot;
				SetWeapon(m_LastWeapon);
				m_PrevOnTurret = 0;
				m_TurretActive = 0;
			}
		}

		// Reapinator 
		if(m_OnReapinator)
		{
			if(!m_PrevOnReapinator && Server()->Tick() - m_ShowInfoTick > 0)
			{
				if(GameServer()->m_pController->ReapinatorOnline())
					GameServer()->SendChatTarget(m_pPlayer->GetCID(),"收割者: 原型限制: 需要突变体.");
				else
					GameServer()->SendChatTarget(m_pPlayer->GetCID(),"收割者: 游戏处于热身. 请过会再访问.");
				m_ShowInfoTick = Server()->Tick() + 2 * Server()->TickSpeed();
			}
		}
		// Energynator
		else if(m_OnEnergyUp)
		{
			if(Server()->Tick() - m_EnergyUpTick > 0)
			{
				if(GameServer()->m_pController->MutPowerSupply())
				{
					if(m_LaserWall)
					{
						GameServer()->SendChatTarget(m_pPlayer->GetCID(),"取消激光墙以修复");
						DestroyLaserWall();
                        GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
					}

					m_EnergyUpTick = Server()->Tick() + 200 * Server()->TickSpeed() / 1000;
					if(m_Armor < 10)
					{
						GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
						IncreaseArmor(1);
					}
	
					if(m_Health < 10)
					{
						GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
						IncreaseHealth(1);
					}
				}
				else if(!m_PrevOnEnergyUp)
				{
					m_EnergyUpTick = Server()->Tick() + 4 * Server()->TickSpeed();
					GameServer()->SendChatTarget(m_pPlayer->GetCID(),"发电机已关闭!");
				}	
			}
		}

		// show laser subtype as broadcast
		if (m_LaserCycleInfoSwitch == 1 && Server()->Tick() - m_LaserCycleTick > 0)
		{
			m_LaserCycleInfoSwitch = 0;
			if(m_ActiveWeapon == WEAPON_RIFLE)
				GameServer()->SendBroadcast(CLaser::m_SubDescription[m_aWeapons[WEAPON_RIFLE].m_SubType], m_pPlayer->GetCID());
				
		}
	}
	else
	{
		// Reapinator
		if(m_OnReapinator)
		{
			if(GameServer()->m_pController->ReapinatorOnline())
			{
				if(m_pPlayer->m_MutatorTeam == TEAM_MUTANT)
				{
					if(m_pPlayer->m_Score >= g_Config.m_SvReapinatorMinScore)
					{
						char aBuf[512];
						str_format(aBuf,sizeof(aBuf),"Alica-KI: 警告: %s 突变了并成为了死神!",Server()->ClientName(m_pPlayer->GetCID()));
						GameServer()->SendChatTarget(-1,aBuf);
						BecameReaper();
					}
					else if(!m_PrevOnReapinator && Server()->Tick() - m_ShowInfoTick > 0)
					{
						GameServer()->SendChatTarget(m_pPlayer->GetCID(),"收割者: 突变分数过低. 请过会再访问.");
						m_ShowInfoTick = Server()->Tick() + 2 * Server()->TickSpeed();	
					}	
				}
				else if(!m_PrevOnReapinator && Server()->Tick() - m_ShowInfoTick > 0)
				{
					if(g_Config.m_SvHappyMsgOn)
					{
						switch(rand()%5)
						{
							case 0: GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvHappyReapinatorA); break;
							case 1: GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvHappyReapinatorB); break;
							case 2: GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvHappyReapinatorC); break;
							case 3:	GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvHappyReapinatorD); break;
							case 4: GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvHappyReapinatorE); break;
						}
						m_ShowInfoTick = Server()->Tick() + 2 * Server()->TickSpeed();	
					}
				}
			}
			else if(!m_PrevOnReapinator && Server()->Tick() - m_ShowInfoTick > 0)
			{
				GameServer()->SendChatTarget(m_pPlayer->GetCID(),"收割者: 正在热身. 请稍后访问.");
				m_ShowInfoTick = Server()->Tick() + 2 * Server()->TickSpeed();	
			}
		}
		// Sceptic Energynator text 
		else if(m_OnEnergyUp && GameServer()->m_pController->MutPowerSupply())
		{
			if(g_Config.m_SvScepticMsgOn && !m_PrevOnEnergyUp && Server()->Tick() - m_ShowInfoTick > 0)
			{
				switch(rand()%5)
				{
					case 0: GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvScepticEnergynatorA); break;
					case 1: GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvScepticEnergynatorB); break;
					case 2: GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvScepticEnergynatorC); break;
					case 3:	GameServer()->SendChatTarget(m_pPlayer->GetCID(),g_Config.m_SvScepticEnergynatorD); break;
					case 4: GameServer()->SendChatTarget(-1,g_Config.m_SvScepticEnergynatorE); break;
				}
				m_ShowInfoTick = Server()->Tick() + 2 * Server()->TickSpeed();	
			}
		}

		// Mutantic emote (add random)
		if(Server()->Tick() - m_MutanticEmoteTick > 0)
		{
			SetEmote(EMOTE_FLASHING, Server()->Tick() + 2000 * Server()->TickSpeed() / 1000);
			m_MutanticEmoteTick = Server()->Tick() + (1000 * g_Config.m_SvMutEmoteDelay + (1000 * rand()%10)) * Server()->TickSpeed() / 1000;
		}
		
		if((m_pPlayer->m_MutatorTeam == TEAM_REAPER || m_pPlayer->m_MutaticLevels&MUTATOR_SELFHEAL) && Server()->Tick() - m_AlienMetaTick > 0)
		{
			IncreaseHealth(1);
			IncreaseArmor(1);
			m_AlienMetaTick = Server()->Tick() + 2 * Server()->TickSpeed();
		}

		// Alien Multijump
		if(m_pPlayer->m_MutatorTeam == TEAM_REAPER || m_pPlayer->m_MutaticLevels&MUTATOR_JUMP)
		{
			if(m_Core.m_Jumped&2 && m_Input.m_Jump && !m_PrevInput.m_Jump && m_LatestInput.m_Jump)
			{
				m_MutanticArmorDJCount++;
				if(m_MutanticArmorDJCount < 5) 
					m_Core.m_Jumped = 1;	
				// skip on step to avoid decrease of shield by mistake
				else if(m_MutanticArmorDJCount > 6 && m_Armor)
				{
					m_Core.m_Jumped = 1;	
					GameServer()->CreateSound(m_Pos, SOUND_BODY_LAND);
					m_Armor--;
					if(m_MutanticArmorDJCount > 10)
						SetEmote(EMOTE_FLASHING, Server()->Tick() + 2000 * Server()->TickSpeed() / 1000);
				}
			}
			else if(m_MutanticArmorDJCount && IsGrounded()) 
				m_MutanticArmorDJCount = 0;
		}
		else
		{
			if(m_Core.m_Jumped&2 && m_Input.m_Jump && !m_PrevInput.m_Jump && m_LatestInput.m_Jump)
			{
				m_MutanticArmorDJCount++;
				if(m_MutanticArmorDJCount < 2)
					m_Core.m_Jumped = 1;
			}
			else if(m_MutanticArmorDJCount && IsGrounded())
			m_MutanticArmorDJCount = 0;
		}
	
		// Alien Hook
		if(m_pPlayer->m_MutaticLevels&MUTATOR_ENDLESSHOOK)
			m_Core.m_HookTick = 0;
	}

	// Previnput
	m_PrevInput = m_Input;
	m_PrevOnTurret = m_OnTurret;	
	m_PrevOnScience = m_OnScience;
	m_PrevOnEnergyUp = m_OnEnergyUp;
	m_PrevOnReapinator = m_OnReapinator;
	return;
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision());
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	m_Core.Move();
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Mask);

	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Mask);


	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

void CCharacter::TickPaused()
{
	if(GameServer()->m_pController->MutGameRunning() == 0)
		return;

	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart > -1)
		++m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= 10)
		return false;
	m_Health = clamp(m_Health+Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if(m_Armor >= 10)
		return false;
	m_Armor = clamp(m_Armor+Amount, 0, 10);
	return true;
}

bool CCharacter::DecreaseArmor(int Amount)
{
        if(m_Armor <= 0)
                return false;
        m_Armor = clamp(m_Armor-Amount, 0, 10);
        return true;
}

void CCharacter::Die(int Killer, int Weapon)
{
	int ModeSpecial = 0;
	// we got to wait 0.5 secs before respawning
	m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	if(Killer < 0 && Weapon > -1)
	{
		Killer = m_pPlayer->GetCID();
		Weapon = -1;
	}

	if(Killer >= 0)
		ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);
		
	/*char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
		Killer, Server()->ClientName(Killer),
		m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);*/

	// send the kill message
	CNetMsg_Sv_KillMsg Msg;
	Msg.m_Killer = Killer;
	Msg.m_Victim = m_pPlayer->GetCID();
	Msg.m_Weapon = Weapon;
	Msg.m_ModeSpecial = ModeSpecial;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);

	// Victim is human
	if(m_pPlayer->m_MutatorTeam >= TEAM_HUMAN)
        {
		if(GameServer()->m_pController->MutGameRunning())
			GameServer()->SendBroadcast("你现在是突变体!", m_pPlayer->GetCID());

		if(Killer != m_pPlayer->GetCID() && Killer >= 0)	
		{
			if(GameServer()->m_apPlayers[Killer])
			{
				// Killer was human (teamkiller)
				if(GameServer()->m_apPlayers[Killer]->m_MutatorTeam > TEAM_MUTANT)
				{
					GameServer()->m_apPlayers[Killer]->m_Score--;
					GameServer()->m_apPlayers[Killer]->m_BonusScore--;
					GameServer()->m_apPlayers[Killer]->m_BonusScore2--;
					GameServer()->m_apPlayers[Killer]->m_MultiScore = 0;
					GameServer()->m_apPlayers[Killer]->m_MultiScoreTick = Server()->Tick();
					Mutate();
					GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
					if(GameServer()->m_pController->MutGameRunning())
                				return;
				}
				// Killer was mutant
				else 
				{	
					if(Server()->Tick() - GameServer()->m_apPlayers[Killer]->m_MultiScoreTick >= 0)
						GameServer()->m_apPlayers[Killer]->m_MultiScore = 1; 
					else
						GameServer()->m_apPlayers[Killer]->m_MultiScore++;

					GameServer()->m_apPlayers[Killer]->m_Score += (1 * GameServer()->m_apPlayers[Killer]->m_MultiScore);

					// Extra bonus for hero
					if(m_pPlayer->m_MutatorTeam == TEAM_HERO)
					{
						GameServer()->m_apPlayers[Killer]->m_Score++;
						GameServer()->m_apPlayers[Killer]->m_BonusScore++;
					}

					GameServer()->m_apPlayers[Killer]->m_MultiScoreTick = Server()->Tick() + 2 * Server()->TickSpeed();
					GameServer()->m_apPlayers[Killer]->m_BonusScore++;
					if(GameServer()->m_apPlayers[Killer]->m_BonusScore >= g_Config.m_SvMetamorphoseCount)
					{
						GameServer()->m_apPlayers[Killer]->m_BonusScore = 0;
						CCharacter * pMutChar = GameServer()->m_apPlayers[Killer]->GetCharacter();
						if(pMutChar)
							pMutChar->Metamorphose();	
					}
					if(Weapon != WEAPON_RIFLE) // if not black-hole weapon
					{
						Mutate();
						GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
                				return;
					}
				}
			}
		}
		else
		// Killer was "world"
		{
			Mutate();
			m_pPlayer->m_Score--;	
		}
        }
	else // Victim is mutant
	{
		if(Killer != m_pPlayer->GetCID() && Killer >= 0)
		{
			if(GameServer()->m_apPlayers[Killer])
			{
				// Killer is human 
				if(GameServer()->m_apPlayers[Killer]->m_MutatorTeam >= TEAM_HUMAN)
				{
					if(Server()->Tick() - GameServer()->m_apPlayers[Killer]->m_MultiScoreTick >= 0)
						GameServer()->m_apPlayers[Killer]->m_MultiScore = 1; 
					else
						GameServer()->m_apPlayers[Killer]->m_MultiScore++;

					GameServer()->m_apPlayers[Killer]->m_Score += (1 * GameServer()->m_apPlayers[Killer]->m_MultiScore);
					GameServer()->m_apPlayers[Killer]->m_MultiScoreTick = Server()->Tick() + 2 * Server()->TickSpeed();
					
					if(GameServer()->m_apPlayers[Killer]->m_MutatorTeam != TEAM_HERO)
						GameServer()->m_apPlayers[Killer]->m_BonusScore++;

					if(GameServer()->m_apPlayers[Killer]->m_BonusScore >= g_Config.m_SvHeroBecameScore && GameServer()->m_apPlayers[Killer]->m_MutatorTeam != TEAM_HERO)
					{
						CCharacter * pHeroChar;
						pHeroChar = GameServer()->m_apPlayers[Killer]->GetCharacter();
						if(pHeroChar)
						{
							GameServer()->SendBroadcast("你成为了英雄", GameServer()->m_apPlayers[Killer]->GetCID());
							char bBuf[512];
							str_format(bBuf,sizeof(bBuf),"%s成为了英雄", 
									Server()->ClientName(GameServer()->m_apPlayers[Killer]->GetCID()));
        						GameServer()->SendChatTarget(-1, bBuf);
							pHeroChar->BecameHero(1);
						}
					}
					else if(g_Config.m_SvAutoResearchScore)
					{
						GameServer()->m_apPlayers[Killer]->m_BonusScore2++;
						if(GameServer()->m_apPlayers[Killer]->m_BonusScore2 >= g_Config.m_SvAutoResearchScore)
						{
							GameServer()->m_apPlayers[Killer]->m_BonusScore2 = 0;
							if(!g_Config.m_SvEmptyResearch || (g_Config.m_SvEmptyResearch && !GameServer()->m_pController->FoundHammerPickups()))
							{
								CCharacter * pHumChar = GameServer()->m_apPlayers[Killer]->GetCharacter();
								if(pHumChar)
									pHumChar->ScienceResearch(1);
							}
						}
					}
				}
			}
		}
	}
		

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();

	m_Alive = false;
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
}

void CCharacter::TakeMinorDamage()
{
	m_DamageTaken++;
	if(Server()->Tick() < m_DamageTakenTick+25)
		GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, 1);
	else
	{
		m_DamageTaken = 0;
		GameServer()->CreateDamageInd(m_Pos, 0, 1);
	}
	m_DamageTakenTick = Server()->Tick();

        m_Health--;
	if(m_Health < 1)
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	m_Core.m_Vel += Force;

	// Friendly fire is checked somewhat different below 
	/*if(GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From) && !g_Config.m_SvTeamdamage)
		return false;*/

	// Mutator 
	if(From >= 0)
	{
		if(!g_Config.m_SvTeamdamage && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From] 
				&& (GameServer()->m_apPlayers[From]->m_MutatorTeam >= TEAM_HUMAN && m_pPlayer->m_MutatorTeam >= TEAM_HUMAN && !(m_Pos.y < 0 && Weapon == WEAPON_HAMMER)))
			return false;	
		if(GameServer()->m_apPlayers[From] && GameServer()->m_apPlayers[From]->m_MutatorTeam <= TEAM_MUTANT) 
		{
			if(Weapon != WEAPON_HAMMER)
				return false;

			if(m_pPlayer->m_MutatorTeam <= TEAM_MUTANT)
			{
				if((IncreaseHealth(3) + IncreaseArmor(3)) > 0)
				{
					m_DamageTaken++;
					if(Server()->Tick() < m_DamageTakenTick+25)
        				{
						// make sure that the damage indicators doesn't group together
						GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, 3);
					}
					else
					{
 						m_DamageTaken = 0;
						GameServer()->CreateDamageInd(m_Pos, 0, 3);
					}
					m_DamageTakenTick = Server()->Tick();
				}
				return false;
			}
			else if(m_pPlayer->m_MutatorTeam == TEAM_HERO)
			{
				Dmg = 8;
			}
			else
			{
				Die(From, Weapon);
				return true;
			}
		}
	}

	if(m_pPlayer->m_MutatorTeam <= TEAM_MUTANT)
	{
		// Spawn Protection 
		if(Server()->Tick() - m_MutanticSpawnProtection < 0)
			return false;
		
		// Mutatic special protection (flashing eyes)
		if(m_MutanticEyeOn == 1)
		{
			if(m_pPlayer->m_MutatorTeam == TEAM_MUTANT)
				Dmg = 1;
			else // Reaper that got flashing eyes increase health when hit...
			{
				IncreaseArmor(Dmg);
				IncreaseHealth(Dmg);
				int Mask = CmaskOne(m_pPlayer->GetCID());
				GameServer()->CreateSound(m_Pos, SOUND_HIT, Mask);
				return false;
			}
		}
		else if(m_pPlayer->m_MutatorTeam == TEAM_REAPER && Weapon != WEAPON_RIFLE)
			Dmg = max(1, Dmg/3);
			
	}

	// m_pPlayer only inflicts half damage on self
	if(From == m_pPlayer->GetCID())
		Dmg = max(1, Dmg/2);

	m_DamageTaken++;

	// create healthmod indicator
	if(Server()->Tick() < m_DamageTakenTick+25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
	}
	else
	{
		m_DamageTaken = 0;
		GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
	}

	if(Dmg)
	{
		if(m_Armor)
		{
			if(Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if(Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
	}

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
	}

	// check for death
	if(m_Health <= 0)
	{
		Die(From, Weapon);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChr->m_EmoteType = EMOTE_HAPPY;
				pChr->m_EmoteStop = Server()->Tick() + 2 * Server()->TickSpeed();
			}
		}

		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;
	

	// Mutator - handle invisible 
	if(m_Invisible)
	{
		if(m_pPlayer->m_MutatorTeam <= TEAM_MUTANT)
		{
			if(GameServer()->m_apPlayers[SnappingClient]->m_MutatorTeam >= TEAM_HUMAN)  
				return;
		}
		else if(GameServer()->m_apPlayers[SnappingClient]->m_MutatorTeam <= TEAM_MUTANT)
			return;
	}

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// write down the m_Core
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	// set emote
	if(m_EmoteStop < Server()->Tick())
	{
		if(m_pPlayer->m_MutatorTeam > TEAM_REAPER)
			m_EmoteType = EMOTE_NORMAL;
		else
			m_EmoteType = EMOTE_ANGRY;

		m_EmoteStop = -1;
		m_MutanticEyeOn = 0;
	} 
	else if(m_MutanticEyeOn == 1)
	{
		if(Server()->Tick() - m_MutanticEyeFlipTick > 0)
		{
			m_MutanticEyeFlip = 1 - m_MutanticEyeFlip;
		
			m_MutanticEyeFlipTick = Server()->Tick() + 200 * Server()->TickSpeed() / 1000;
			switch(m_MutanticEyeFlip)
			{
				case 0: m_EmoteType = EMOTE_NORMAL; break;
				case 1: m_EmoteType = EMOTE_SURPRISE; break;
			}
		}
	}

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = m_ActiveWeapon;
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;
		if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
			pCharacter->m_AmmoCount = m_aWeapons[m_ActiveWeapon].m_Ammo;
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;
}

void CCharacter::Mutate()
{
	m_pPlayer->m_MutatorTeam = TEAM_MUTANT;
	GiveWeapon(WEAPON_HAMMER, -1);
	m_TurretActive = 0;
	m_OnTurret = 0;
	m_PrevOnTurret = 0;
	m_pPlayer->m_ScienceCount = 0;
	SetWeapon(WEAPON_HAMMER);
	SetEmote(EMOTE_FLASHING,Server()->Tick() + 2500 * Server()->TickSpeed() / 1000);
	m_AlienMetaTick = Server()->Tick() + 30 * Server()->TickSpeed();
	IncreaseHealth(10);
	m_Armor = 0;
	m_pPlayer->m_MyHumanScore = m_pPlayer->m_Score;
	m_pPlayer->m_Score = 0;
	m_pPlayer->m_BonusScore = 0;
	m_pPlayer->m_MultiScore = 0;
	m_pPlayer->m_MultiScoreTick = Server()->Tick();
	m_OnReapinator = 0;
	m_PrevOnReapinator = 0;
}

void CCharacter::ReMutate()
{
	m_pPlayer->m_MutatorTeam = TEAM_HUMAN;
	GiveWeapon(WEAPON_GUN,10);
	GiveWeapon(WEAPON_HAMMER,-1);
	m_OnTurret = 0;
	m_PrevOnTurret = 0;
	m_pPlayer->m_ScienceCount = 0;
	SetWeapon(WEAPON_GUN);
	SetEmote(EMOTE_FLASHING,Server()->Tick() + 2500 * Server()->TickSpeed() / 1000);
	IncreaseHealth(10);
	m_aWeapons[WEAPON_GUN].m_Got = 1;
	m_pPlayer->m_MyMutantScore = m_pPlayer->m_Score;
	m_pPlayer->m_Score = m_pPlayer->m_MyHumanScore;
	m_pPlayer->m_BonusScore = 0;
	m_pPlayer->m_MultiScore = 0;
	m_pPlayer->m_MultiScoreTick = Server()->Tick();
}

void CCharacter::BecameHero(bool AirStrike)
{
	m_pPlayer->m_Score += 3;
	m_pPlayer->m_MutatorTeam = TEAM_HERO;
	GiveWeapon(WEAPON_SHOTGUN, 10);
	SetWeapon(WEAPON_SHOTGUN);
	IncreaseHealth(10);
	m_pPlayer->m_BonusScore = 0;
	m_pPlayer->m_MultiScore = 0;
	m_pPlayer->m_MultiScoreTick = Server()->Tick();
	if(AirStrike && g_Config.m_SvHeroAirstrike)
	{
		char bBuf[512];
		str_format(bBuf,sizeof(bBuf),"%s %s", Server()->ClientName(m_pPlayer->GetCID()),g_Config.m_SvInfoGotAirstrike);
		GameServer()->SendChatTarget(-1, bBuf);
		m_OwnAirstrike++;
	}
	m_pPlayer->m_ScienceExplored |= SCIENCE_SHOTGUN_FORCEX2;
}		

void CCharacter::BecameReaper()
{
	m_pPlayer->m_Score += 3;
	m_pPlayer->m_MutatorTeam = TEAM_REAPER;
	IncreaseHealth(10);
	IncreaseArmor(10);
	m_pPlayer->m_BonusScore = 0;
	
	GiveWeapon(WEAPON_RIFLE,1);
	m_aWeapons[WEAPON_RIFLE].m_Ammo = g_Config.m_SvReaperAmmo;
	m_aWeapons[WEAPON_GUN].m_Got = 0;
	m_aWeapons[WEAPON_GUN].m_Ammo = 0;
	m_aWeapons[WEAPON_SHOTGUN].m_Got = 0;
	m_aWeapons[WEAPON_SHOTGUN].m_Ammo = 0;
	m_aWeapons[WEAPON_GRENADE].m_Got = 0;
	m_aWeapons[WEAPON_GRENADE].m_Ammo = 0;
	SetWeapon(WEAPON_HAMMER);
	m_pPlayer->m_MultiScore = 0;
	m_pPlayer->m_MultiScoreTick = Server()->Tick();
}

void CCharacter::Metamorphose()
{
	int Lottery = rand()%8;
	IncreaseHealth(5);
	IncreaseArmor(5);

	if(!(m_pPlayer->m_Score >= g_Config.m_SvReaperMinScore && Lottery == 5))
	{
		Lottery = m_pPlayer->m_ScienceCount;
		m_pPlayer->m_ScienceCount++;
		if(Lottery > 4)
			return;
	}
	
	char aBuf[512];
	char bBuf[512];

	m_pPlayer->m_MutaticLevels |= 1 << Lottery; 

	switch(Lottery)
	{
		case 0:
			{
				str_format(bBuf,sizeof(bBuf),"%s 突变了并且得到了开局护盾", Server()->ClientName(m_pPlayer->GetCID()));
				str_format(aBuf,sizeof(aBuf),"你得到了开局护盾");
			}
			break;
		case 1:
			{
				str_format(bBuf,sizeof(bBuf),"%s 突变了并且免疫毒水", Server()->ClientName(m_pPlayer->GetCID()));
				str_format(aBuf,sizeof(aBuf),"你免疫了毒水");
			}
			break;
		case 2:
			{
				str_format(bBuf,sizeof(bBuf),"%s 突变了并得到了外星人多段跳", Server()->ClientName(m_pPlayer->GetCID()));
				str_format(aBuf,sizeof(aBuf),"你得到了外星人多段跳");
			}
			break;
		case 3:
			{
				str_format(bBuf,sizeof(bBuf),"%s 突变了并且得到了外星人超级钩", Server()->ClientName(m_pPlayer->GetCID()));
				str_format(aBuf,sizeof(aBuf),"你得到了外星人超级钩");
			}
			break;
		case 4:
			{
				str_format(bBuf,sizeof(bBuf),"%s 突变了并且能够自我治愈", Server()->ClientName(m_pPlayer->GetCID()));
				str_format(aBuf,sizeof(aBuf),"你能够自我治愈了");
			}
			break;
		case 5:
			{
				str_format(bBuf,sizeof(bBuf),"Alica-KI: 警告: %s 突变并成为了死神!", Server()->ClientName(m_pPlayer->GetCID()));
				str_format(aBuf,sizeof(aBuf),"你现在是一个死神!");
				BecameReaper();
			}
			break;
	}	

	GameServer()->SendBroadcast(aBuf, m_pPlayer->GetCID());
	if(Lottery == 5 || g_Config.m_SvBonusMsgOn)
		GameServer()->SendChatTarget(-1, bBuf);
}

void CCharacter::ScienceResearch(bool AutoResearch)
{
	int Lottery = rand()%16;
	int remember_me = 0;
	char aBuf[512];
	char bBuf[512];

	m_pPlayer->m_ScienceCount = 0;


	if(Lottery < 13)
	{
		if(m_pPlayer->m_ScienceExplored&(1 << Lottery) && Lottery < 9)
		{
			if(g_Config.m_SvSaveScienceScore && m_pPlayer->m_Score > g_Config.m_SvSaveScienceScore)
			{
				remember_me = 1; 
				m_pPlayer->m_ScienceRemembered |= 1 << Lottery;
				m_pPlayer->m_ScienceExplored |= 1 << Lottery;
			}
			else
				Lottery = -1;
		}
		else
			m_pPlayer->m_ScienceExplored |= 1 << Lottery;
	}

	if(Lottery > 12 || Lottery == -1 || (Lottery == 9 && m_pPlayer->m_MutatorTeam == TEAM_HERO))
	{
		if(!AutoResearch)
		{
			str_format(aBuf,sizeof(aBuf),"科学研究: 失败!");
			GameServer()->SendBroadcast(aBuf, m_pPlayer->GetCID());
		}
		return;
	}

	switch(Lottery)
	{
		case 0:
			{
				m_OwnLaserSubType[LASER_SUBTYPE_TELE] = true;
				m_QueuedLaser = LASER_SUBTYPE_TELE;
				m_aWeapons[WEAPON_RIFLE].m_SubType = LASER_SUBTYPE_TELE;
				GiveWeapon(WEAPON_RIFLE,10);
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了传送激光");
					str_format(bBuf,sizeof(bBuf),"%s %s传送激光", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"传送激光: 保存!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 传送激光", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 1:
			{
				m_OwnLaserSubType[LASER_SUBTYPE_HEALING] = true;
				m_QueuedLaser = LASER_SUBTYPE_HEALING;
				if(m_ActiveWeapon != WEAPON_RIFLE)
					m_aWeapons[WEAPON_RIFLE].m_SubType = LASER_SUBTYPE_HEALING;

				GiveWeapon(WEAPON_RIFLE,g_Config.m_SvHealingAmmo,1);
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了治愈激光");
					str_format(bBuf,sizeof(bBuf),"%s %s治愈激光", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"治愈激光: 已拥有!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 治愈激光", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 2:
			{
				m_OwnLaserSubType[LASER_SUBTYPE_RIDDLE] = true;
				m_QueuedLaser = LASER_SUBTYPE_RIDDLE;
				m_aWeapons[WEAPON_RIFLE].m_SubType = LASER_SUBTYPE_RIDDLE;
				GiveWeapon(WEAPON_RIFLE,10);
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了穿透激光");
					str_format(bBuf,sizeof(bBuf),"%s %s穿透激光", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"穿透激光: 已拥有!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 穿透激光", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 3:
			{
				GiveWeapon(WEAPON_SHOTGUN,10);
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了霰弹-MKII");
					str_format(bBuf,sizeof(bBuf),"%s %s霰弹-MKII", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"霰弹MKII: 已拥有!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 霰弹-MKII", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 4:
			{ 
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了霰弹-弹药 MKII");
					str_format(bBuf,sizeof(bBuf),"%s %s霰弹-弹药 MKII", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"霰弹-弹药 MKII: 已拥有!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 霰弹-淡雅 MKII", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 5:
			{
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了集簇榴弹炮");
					str_format(bBuf,sizeof(bBuf),"%s %s集簇榴弹炮", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"集簇榴弹炮: 已拥有!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 集簇榴弹炮", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 6:
			{
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了手枪-弹药 MKII");
					str_format(bBuf,sizeof(bBuf),"%s %s手枪-弹药 MKII", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"手枪-弹药 MKII: 已拥有!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 手枪-弹药 MKII", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 7:
			{
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了霰弹-速度-kit");
					str_format(bBuf,sizeof(bBuf),"%s %s 霰弹-速度-kit", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"霰弹-速度-kit: 已拥有!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 霰弹-速度-kit", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 8:
			{
				if(!remember_me)
				{
					str_format(aBuf,sizeof(aBuf),"你得到了超级蓄电池");
					str_format(bBuf,sizeof(bBuf),"%s %s超级蓄电池", Server()->ClientName(m_pPlayer->GetCID()), AutoResearch == 1 ? g_Config.m_SvInfoGot : g_Config.m_SvInfoResearched);
				}
				else
				{
					str_format(aBuf,sizeof(aBuf),"超级蓄电池: 已拥有!");
					str_format(bBuf,sizeof(bBuf),"%s 已拥有: 超级蓄电池", Server()->ClientName(m_pPlayer->GetCID()));
				}
			}
			break;
		case 9:
			{
				str_format(aBuf,sizeof(aBuf),"你成为了英雄");
				str_format(bBuf,sizeof(bBuf),"%s 得到了: 英雄遗传", Server()->ClientName(m_pPlayer->GetCID()));
				BecameHero(0);
			}
			break;
		case 10:
			{
				str_format(aBuf,sizeof(aBuf),"你得到了武器装弹.");
				str_format(bBuf,sizeof(bBuf),"%s 得到了: 武器装弹", Server()->ClientName(m_pPlayer->GetCID()));
				GiveWeapon(WEAPON_GUN,10);
				GiveWeapon(WEAPON_SHOTGUN,10);
				GiveWeapon(WEAPON_GRENADE,10);
				GiveWeapon(WEAPON_RIFLE,10);
				if(m_pPlayer->m_ScienceExplored&SCIENCE_LASER_HEALING)
				{
					if(m_ActiveWeapon != WEAPON_RIFLE)
						m_aWeapons[WEAPON_RIFLE].m_SubType = LASER_SUBTYPE_HEALING;

					GiveWeapon(WEAPON_RIFLE,g_Config.m_SvHealingAmmo,1);
				}
			}
			break;
		case 11:
			{
				str_format(aBuf,sizeof(aBuf),"你得到了血盾恢复");
				str_format(bBuf,sizeof(bBuf),"%s 得到了: 血盾恢复", Server()->ClientName(m_pPlayer->GetCID()));
				IncreaseHealth(10);
				IncreaseArmor(10);
			}
			break;
		case 12:
			{
				if(!AutoResearch)
					str_format(bBuf,sizeof(bBuf),"%s %s", Server()->ClientName(m_pPlayer->GetCID()), g_Config.m_SvInfoHackedAirstrike);
				else
					str_format(bBuf,sizeof(bBuf),"%s %s", Server()->ClientName(m_pPlayer->GetCID()), g_Config.m_SvInfoGotAirstrike);

				str_format(aBuf,sizeof(aBuf),"你得到了空袭. (使用 /airstrike)");
				m_OwnAirstrike++;
			}
			break;
	}	
	GameServer()->SendBroadcast(aBuf, m_pPlayer->GetCID());
	if(g_Config.m_SvBonusMsgOn)
		GameServer()->SendChatTarget(-1, bBuf);
}

vec2 CCharacter::GetVel()
{
	return m_Core.m_Vel;
}
                     
void CCharacter::SetVel(vec2 Vel)
{
	m_Core.m_Vel = Vel;
}

void CCharacter::SetPos(vec2 Pos)
{
	m_Core.m_Pos = Pos;
}

void CCharacter::DestroyLaserWall()
{
	if(m_LaserWall)
	{
		GameServer()->m_World.DestroyEntity(m_LaserWall);
		m_LaserWall = 0;
	}
}

int CCharacter::GetActiveWeapon()
{
	return m_ActiveWeapon;
}

bool CCharacter::IsFiring()
{
	if((m_Input.m_Fire&1) != 0)
		return 1;
	else
		return 0;
}

void CCharacter::SetOnScience(bool val)
{
	m_OnScience = val;
	m_OnScienceOffTick = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
}

bool CCharacter::IsOnScience()
{
	return m_OnScience;
}

bool CCharacter::PrevOnScience()
{
	return m_PrevOnScience;
}

bool CCharacter::BreakCircuit()
{
	if(m_pPlayer->m_MutatorTeam <= TEAM_MUTANT)
	{
		GameServer()->m_pController->MutSetPowerSupply(0);
		char aBuf[512];
		str_format(aBuf,sizeof(aBuf),"\nAlica-AI: 警告! 发电机被%s摧毁了!\n",Server()->ClientName(m_pPlayer->GetCID()));
		GameServer()->SendChatTarget(-1, aBuf);
		m_pPlayer->m_Score += 3;
		SetEmote(EMOTE_FLASHING, Server()->Tick() + 2000 * Server()->TickSpeed() / 1000);
		Metamorphose();
		return true;
	}
	else
		return false;
}

void CCharacter::SetShowInfoTick(int Tick)
{
	m_ShowInfoTick = Tick;
}

int CCharacter::GetShowInfoTick()
{
	return m_ShowInfoTick;
}

bool CCharacter::MutanticEyes()
{
	return m_MutanticEyeOn;
}
