/* (c) XXLTomate (xxltomate@v93.eu) -- MemberList */

#ifndef GAME_SERVER_MEMBERLIST_H
#define GAME_SERVER_MEMBERLIST_H

#include "gamecontext.h"
#include <base/tl/sorted_array.h>
#include <engine/server/server.h>

class CMemberList
{
private:
	CGameContext *m_pGameServer;
	CServer *m_pServer;

	class CPlayerMember
	{
	public:
		char m_aName[MAX_NAME_LENGTH];
		char m_aPass[32+1];
		int m_AuthLvl;
		int m_Money;

		CPlayerMember() {};
		CPlayerMember(const char *pName, const char *pPass, int AuthLvl, int Money);

		bool operator<(const CPlayerMember& other) { return (this->m_AuthLvl < other.m_AuthLvl); }
	};

	sorted_array<CPlayerMember> m_List;

	CPlayerMember *SearchName(const char *pName, int *pPosition, bool MatchCase);
	CPlayerMember *SearchList(int ID, int *pPosition){ return SearchName(Server()->ClientName(ID), pPosition, 0 );};

	void Init();
	void UpdatePlayer(int ClientID, const char* pPass, int AuthLvl, int Money);
	void LoadMember(int ClientID,CGameContext *pSelf);
	void SaveList(int ClientID, const char* pPass , CGameContext *pSelf, bool ForceAuth, int Money);

	static void SaveListThread(void *pUser);
public:
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() { return m_pServer; }

	CMemberList(CGameContext *pGameServer);
	~CMemberList();
	
	void Save();
	void Register(IConsole::IResult *pResult, int ClientID, const char* pPass , CGameContext *pSelf);
	void Login(IConsole::IResult *pResult, int ClientID, const char* pPass , CGameContext *pSelf);
	void Save(int ClientID, CGameContext *pSelf);
	void Member(int ClientID, CGameContext *pSelf);
	void UnMember(int ClientID, CGameContext *pSelf);
	void Check(int ClientID, CGameContext *pSelf);
};

#endif
