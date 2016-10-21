/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>

#include <game/gamecore.h>
#include <game/server/entities/laser.h>

enum
{
	WEAPON_GAME = -3, // team switching etc
	WEAPON_SELF = -2, // console kill command
	WEAPON_WORLD = -1, // death tiles etc
};

class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()

public:
	//character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld);

	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	bool IsGrounded();

	void SetWeapon(int W);
	void HandleWeaponSwitch();
	void DoWeaponSwitch();

	void HandleWeapons();
	void HandleNinja();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	void Die(int Killer, int Weapon);
	bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);
	void TakeMinorDamage();

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);
	bool Remove();

	bool IncreaseHealth(int Amount);
	bool IncreaseArmor(int Amount);

	bool GiveWeapon(int Weapon, int Ammo, int Special=0);
	void GiveNinja();

	void SetEmote(int Emote, int Tick);

	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }

	// Mutator
	void SetPreStartGame();
	void Mutate();
	void ReMutate();
	void Metamorphose();
	void ScienceResearch(bool AutoResearch);
	void BecameHero(bool AirStrike);
	void BecameReaper();
	void AirStrike();

	vec2 GetVel();
	void SetVel(vec2 Vel);
	void SetPos(vec2 Pos);
	bool DecreaseArmor(int Amount);
	int GetActiveWeapon();
	bool IsFiring();
	void CycleLaserSubTypes();
	void DestroyLaserWall();
	void SetOnScience(bool val);
	bool IsOnScience();
	bool PrevOnScience();
	bool BreakCircuit();
	void ShowScanResults();
	void SetShowInfoTick(int Tick);
	int GetShowInfoTick();
	bool MutanticEyes();

private:
	
	// player controlling this character
	class CPlayer *m_pPlayer;

	// Mutator
	class CLaserWall *m_LaserWall;
	int m_MutanticEmoteTick;
	int m_BioHazardDamageTick;
	int m_AlienMetaTick;
	int m_MutanticSpawnProtection;
	int m_MutanticArmorDJCount;
	bool m_OnScience;
	int m_OnScienceOffTick;
	bool m_PrevOnScience;
	bool m_Invisible;
	bool m_PrevOnReapinator;
	bool m_OnReapinator;

	bool m_OnTurret;
	bool m_TurretActive;
	int m_TurretAmmo;
	int m_PrevTurretSwitchAmmo;
	bool m_PrevTurretSwitchGot;
	bool m_PrevOnTurret;
	int m_OwnAirstrike;
	
	int m_EnergyUpTick;
	int m_OnEnergyUp;
	int m_PrevOnEnergyUp;

	int m_ShowInfoTick;

        int m_Col1;
        int m_Col2;
        int m_Col3;
        int m_Col4;
	
	bool m_Alive;

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;

	struct WeaponStat
	{
		int m_AmmoRegenStart;
		int m_Ammo;
		int m_Ammocost;
		bool m_Got;
		int m_SubType;
		int m_SpecialAmmo;
		int m_NormalAmmo;

	} m_aWeapons[NUM_WEAPONS];

	int m_ActiveWeapon;
	int m_LastWeapon;

	// Mutator 
	int m_QueuedWeapon;
	int m_QueuedLaser;
	int m_LaserCycleTick;
	bool m_LaserCycleInfoSwitch;
	bool m_OwnLaserSubType[LASER_MAX_SUBTYPE];

	int m_ReloadTimer;
	int m_AttackTick;

	int m_DamageTaken;

	int m_EmoteType;
	int m_EmoteStop;

	// Mutator
	bool m_MutanticEyeOn;
	bool m_MutanticEyeFlip;
	int m_MutanticEyeFlipTick;

	// last tick that the player took any action ie some input
	int m_LastAction;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;

	int m_DamageTakenTick;

	int m_Health;
	int m_Armor;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CCharacterCore m_Core;

	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core

};

#endif
