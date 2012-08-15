/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUP_H
#define GAME_SERVER_ENTITIES_PICKUP_H

#include <game/server/entity.h>

const int PickupPhysSize = 14;

class CPickup : public CEntity
{
public:
	CPickup(CGameWorld *pGameWorld, int Type, int SubType = 0, int Layer = 0, int Number = 0, bool Move = false, vec2 Vel = vec2(0.f, 0.f), int LifeSpan = -1, int InactiveTime = 0, int Owner = -1);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:

	int m_Type;
	int m_Subtype;
	bool m_Move;
	vec2 m_Vel;
	int m_Owner;
	int m_InactiveTime;
	int m_LifeSpan;
	//int m_SpawnTick;

	// DDRace

	void Move();
	vec2 m_Core;
};

#endif
