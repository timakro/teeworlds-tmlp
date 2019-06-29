/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "gameworld.h"
#include "entity.h"
#include "gamecontext.h"
#include "entities/pickup.h"
#include "entities/laser.h"
#include "entities/projectile.h"

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
CGameWorld::CGameWorld()
{
	m_pGameServer = 0x0;
	m_pServer = 0x0;

	m_Paused = false;
	m_ResetRequested = false;
	for(int i = 0; i < NUM_ENTTYPES; i++)
		m_apFirstEntityTypes[i] = 0;
}

CGameWorld::~CGameWorld()
{
	// delete all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		while(m_apFirstEntityTypes[i])
			delete m_apFirstEntityTypes[i];
}

void CGameWorld::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
}

CEntity *CGameWorld::FindFirst(int Type)
{
	return Type < 0 || Type >= NUM_ENTTYPES ? 0 : m_apFirstEntityTypes[Type];
}

int CGameWorld::FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type)
{
	if(Type < 0 || Type >= NUM_ENTTYPES)
		return 0;

	int Num = 0;
	for(CEntity *pEnt = m_apFirstEntityTypes[Type];	pEnt; pEnt = pEnt->m_pNextTypeEntity)
	{
		if(distance(pEnt->m_Pos, Pos) < Radius+pEnt->m_ProximityRadius)
		{
			if(ppEnts)
				ppEnts[Num] = pEnt;
			Num++;
			if(Num == Max)
				break;
		}
	}

	return Num;
}

void CGameWorld::InsertEntity(CEntity *pEnt)
{
#ifdef CONF_DEBUG
	for(CEntity *pCur = m_apFirstEntityTypes[pEnt->m_ObjType]; pCur; pCur = pCur->m_pNextTypeEntity)
		dbg_assert(pCur != pEnt, "err");
#endif

	// insert it
	if(m_apFirstEntityTypes[pEnt->m_ObjType])
		m_apFirstEntityTypes[pEnt->m_ObjType]->m_pPrevTypeEntity = pEnt;
	pEnt->m_pNextTypeEntity = m_apFirstEntityTypes[pEnt->m_ObjType];
	pEnt->m_pPrevTypeEntity = 0x0;
	m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt;
}

void CGameWorld::DestroyEntity(CEntity *pEnt)
{
	pEnt->m_MarkedForDestroy = true;
}

void CGameWorld::RemoveEntity(CEntity *pEnt)
{
	// not in the list
	if(!pEnt->m_pNextTypeEntity && !pEnt->m_pPrevTypeEntity && m_apFirstEntityTypes[pEnt->m_ObjType] != pEnt)
		return;

	// remove
	if(pEnt->m_pPrevTypeEntity)
		pEnt->m_pPrevTypeEntity->m_pNextTypeEntity = pEnt->m_pNextTypeEntity;
	else
		m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt->m_pNextTypeEntity;
	if(pEnt->m_pNextTypeEntity)
		pEnt->m_pNextTypeEntity->m_pPrevTypeEntity = pEnt->m_pPrevTypeEntity;

	// keep list traversing valid
	if(m_pNextTraverseEntity == pEnt)
		m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;

	pEnt->m_pNextTypeEntity = 0;
	pEnt->m_pPrevTypeEntity = 0;
}

//
void CGameWorld::Snap(int SnappingClient)
{
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Snap(SnappingClient);
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Reset()
{
	// reset all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Reset();
			pEnt = m_pNextTraverseEntity;
		}
	RemoveEntities();

	GameServer()->m_pController->PostReset();
	RemoveEntities();

	m_ResetRequested = false;
}

void CGameWorld::RemoveEntities()
{
	// destroy objects marked for destruction
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			if(pEnt->m_MarkedForDestroy)
			{
				RemoveEntity(pEnt);
				pEnt->Destroy();
			}
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Tick()
{
	if(m_ResetRequested)
		Reset();

	if(!m_Paused)
	{
		if(GameServer()->m_pController->IsForceBalanced())
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Teams have been balanced");
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->Tick();
				pEnt = m_pNextTraverseEntity;
			}

		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickDefered();
				pEnt = m_pNextTraverseEntity;
			}
	}
	else
	{
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickPaused();
				pEnt = m_pNextTraverseEntity;
			}
	}

	RemoveEntities();
}


// TODO: should be more general
CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		float Len = distance(p->m_Pos, IntersectPos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}


CCharacter *CGameWorld::ClosestCharacter(vec2 Pos, float Radius, CEntity *pNotThis)
{
	// Find other players
	float ClosestRange = Radius*2;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		float Len = distance(Pos, p->m_Pos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			if(Len < ClosestRange)
			{
				ClosestRange = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}

void CGameWorld::RenderTMLPFrame(uint8_t *RenderBuffer, vec2 Center)
{
	auto SetPixel = [RenderBuffer](int x, int y, int v)
	{
		if(x >= 0 && x < 90 && y >= 0 && y < 50)
			RenderBuffer[y*90+x] = v;
	};

	auto DrawLine = [SetPixel](int x1, int y1, int x2, int y2, int v)
	{
		bool steep = abs(y2 - y1) > abs(x2 - x1);
		if(steep)
		{
			std::swap(x1, y1);
			std::swap(x2, y2);
		}
		if(x1 > x2)
		{
			std::swap(x1, x2);
			std::swap(y1, y2);
		}
		int dx = x2 - x1;
		int dy = abs(y2 - y1);
		float error = dx / 2.0f;
		int ystep = y1 < y2 ? 1 : -1;
		int y = y1;
		for(int x = x1; x < x2; x++)
		{
			if(steep)
				SetPixel(y, x, v);
			else
				SetPixel(x, y, v);
			error -= dy;
			if(error < 0)
			{
				y += ystep;
				error += dx;
			}
		}
	};

	int CenterX = ((int)Center.x+8)/16;
	int CenterY = ((int)Center.y+8)/16;

	// Pickups
	{
		CPickup *p = (CPickup *)FindFirst(ENTTYPE_PICKUP);
		for(; p; p = (CPickup *)p->TypeNext())
		{
			if(p->m_SpawnTick != -1)
				continue;

			int v;
			switch(p->m_Type)
			{
				case POWERUP_HEALTH: v = 4; break;
				case POWERUP_ARMOR:  v = 5; break;
				case POWERUP_WEAPON:
					switch(p->m_Subtype)
					{
						case WEAPON_SHOTGUN: v = 6; break;
						case WEAPON_GRENADE: v = 7; break;
						case WEAPON_RIFLE:   v = 8; break;
						default: continue;
					}
					break;
				case POWERUP_NINJA:  v = 9; break;
				default: continue;
			}

			int x = ((int)p->m_Pos.x-8)/16 - CenterX + 45;
			int y = ((int)p->m_Pos.y-8)/16 - CenterY + 25;

			SetPixel(x,   y, v);
			SetPixel(x+1, y, v);
			SetPixel(x,   y+1, v);
			SetPixel(x+1, y+1, v);
			if(p->m_Type == POWERUP_WEAPON || p->m_Type == POWERUP_NINJA)
			{
				SetPixel(x-1, y,   v);
				SetPixel(x+2, y,   v);
				SetPixel(x-1, y+1, v);
				SetPixel(x+2, y+1, v);
			}
		}
	}

	// Player hook
	{
		CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
		for(; p; p = (CCharacter *)p->TypeNext())
		{
			if(p->m_Core.m_HookState == HOOK_RETRACTED || p->m_Core.m_HookState == HOOK_IDLE)
				continue;

			int x = ((int)p->m_Pos.x-8)/16 - CenterX + 45;
			int y = ((int)p->m_Pos.y-8)/16 - CenterY + 25;
			int hx = ((int)p->m_Core.m_HookPos.x)/16 - CenterX + 45;
			int hy = ((int)p->m_Core.m_HookPos.y)/16 - CenterY + 25;
			DrawLine(x, y, hx, hy, 20);
		}
	}

	// Laser beam
	{
		CLaser *p = (CLaser *)FindFirst(ENTTYPE_LASER);
		for(; p; p = (CLaser *)p->TypeNext())
		{
			int x = ((int)p->m_Pos.x)/16 - CenterX + 45;
			int y = ((int)p->m_Pos.y)/16 - CenterY + 25;
			int fx = ((int)p->m_From.x)/16 - CenterX + 45;
			int fy = ((int)p->m_From.y)/16 - CenterY + 25;
			DrawLine(fx, fy, x, y, 13);
		}
	}

	// Projectile
	{
		CProjectile *p = (CProjectile *)FindFirst(ENTTYPE_PROJECTILE);
		for(; p; p = (CProjectile *)p->TypeNext())
		{
			int v;
			switch(p->m_Type)
			{
				case WEAPON_GUN:     v = 10; break;
				case WEAPON_SHOTGUN: v = 11; break;
				case WEAPON_GRENADE: v = 12; break;
				default: continue;
			}
			int x = ((int)p->m_CurPos.x)/16 - CenterX + 45;
			int y = ((int)p->m_CurPos.y)/16 - CenterY + 25;
			SetPixel(x, y, v);
		}
	}

	// Character
	{
		CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
		for(; p; p = (CCharacter *)p->TypeNext())
		{
			int v = p->m_ActiveWeapon + 14;
			int x = ((int)p->m_Pos.x-8)/16 - CenterX + 45;
			int y = ((int)p->m_Pos.y-8)/16 - CenterY + 25;
			SetPixel(x,   y, v);
			SetPixel(x+1, y, v);
			SetPixel(x,   y+1, v);
			SetPixel(x+1, y+1, v);
		}
	}
}
