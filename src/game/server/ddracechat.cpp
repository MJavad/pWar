/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "gamecontext.h"
#include "ddracechat.h"
#include <engine/shared/config.h>
#include <engine/server/server.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/version.h>
#include <game/generated/nethash.cpp>
#if defined(CONF_SQL)
#include <game/server/score/sql_score.h>
#endif
bool CheckClientID(int ClientID);
bool CheckRights(int ClientID, int Victim, CGameContext *GameContext);
char* TimerType(int TimerType);

void CGameContext::ConCredits(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Teeworlds Team takes most of the credits also");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"This mod was originally created by \'3DA\'");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Now it is maintained & re-coded by:");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"\'[Egypt]GreYFoX@GTi\' and \'[BlackTee]den\'");
	pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"credit",
			"Others Helping on the code: \'heinrich5991\', \'ravomavain\', \'Trust o_0 Aeeeh ?!\', \'noother\', \'<3 fisted <3\' & \'LemonFace\'");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Documentation: Zeta-Hoernchen & Learath2, Entities: Fisico");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Code (in the past): \'3DA\' and \'Fluxid\'");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credit",
			"Modded mod by Parham & MJavad.");
}

void CGameContext::ConJoin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int ID = pResult->m_ClientID;
	if (!pSelf->m_apPlayers[ID]->m_IsLoggedIn){
		pSelf->SendChatTarget(ID, "Please register or login first");
		return;
	}
	if (pResult->GetInteger(0) < 0 || pResult->GetInteger(0) > 4){
		pSelf->SendChatTarget(ID, "You can only join teams 0~4");
		return;
	}
	if (pSelf->m_apPlayers[ID]->m_LastJoin - pSelf->Server()->Tick() > 0){
		pSelf->SendChatTarget(ID, "You can't join a team that often");
		return;
	}
	
	pSelf->m_apPlayers[ID]->m_LastJoin = pSelf->Server()->Tick() + pSelf->Server()->TickSpeed()*g_Config.m_SvTeamChangeDelay;
	char aBuf[512];
	int Num = 0;
	for (int i=0; i < MAX_CLIENTS; i++){
		if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->m_Team2 == pResult->GetInteger(0) && i != ID){
			Num++;
			if (Num == 1)
				str_format(aBuf, sizeof(aBuf), "%s", pSelf->Server()->ClientName(i));
			else
				str_format(aBuf, sizeof(aBuf), "%s, %s", aBuf, pSelf->Server()->ClientName(i));
		}
	}
	char aChatMsg[512];
	str_format(aChatMsg, sizeof(aChatMsg), "Your teammates : %s", aBuf);
	if (Num == 0 || pResult->GetInteger(0) == 0){
		if (Num == 0 && pResult->GetInteger(0) != 0)
			pSelf->SendChatTarget(ID, "You haven't any teammates now");
		if (pResult->GetInteger(0) == 0)
			pSelf->SendChatTarget(ID, "You are alone to fight");
	}
	else
		pSelf->SendChatTarget(ID, aChatMsg);
	pSelf->m_apPlayers[ID]->m_Team2 = pResult->GetInteger(0);
	pSelf->m_apPlayers[ID]->SetTeam(TEAM_RED);
	
}

void CGameContext::ConPay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int ID = pResult->m_ClientID;
	int To = -1;
	int Amount = pResult->GetInteger(0);
	char aBuf[128];
	str_format(aBuf, 128, "%s", pResult->GetString(1));
	int len = str_length(aBuf);
	if (aBuf[len-1] == ' ')
		aBuf[len-1] = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (!str_comp_nocase(pSelf->Server()->ClientName(i), aBuf))
		{
			To = i;
			break;
		}
	if (To == -1)
	{
		pSelf->SendChatTarget(ID, "Player not found");
		return;
	}
	if (To == ID)
	{
		pSelf->SendChatTarget(ID, "You can't give money to yourself");
		return;
	}
	if (Amount <= 0)
	{
		pSelf->SendChatTarget(ID, "Wrong amount given");
		return;
	}
	if (pSelf->m_apPlayers[ID]->m_Money < Amount)
	{
		pSelf->SendChatTarget(ID, "You don't have enough money");
		return;
	}
	pSelf->m_apPlayers[ID]->m_Money -= Amount;
	pSelf->m_apPlayers[To]->m_Money += Amount;
	char aChatMsg[512];
	str_format(aChatMsg, 512, "You paid '%s' %d$", pSelf->Server()->ClientName(To), Amount);
	pSelf->SendChatTarget(ID, aChatMsg);
	str_format(aChatMsg, 512, "'%s' gave you %d$", pSelf->Server()->ClientName(ID), Amount);
	pSelf->SendChatTarget(To, aChatMsg);
}

void CGameContext::ConBuy(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int ID = pResult->m_ClientID;
	char aBuf[128];str_copy(aBuf, pResult->GetString(0), 128);
	int Money = 0;
	char ItemName[128];
	char Chat[512];
	bool *pBool;
	int *pInt;
	int Mode = BUY_MODE_BOOL;
	int Max = 100;
	int Num = 1;
	int Weapon = WEAPON_SHOTGUN;
	if (!pSelf->m_apPlayers[ID]->m_IsLoggedIn)
	{
		pSelf->SendChatTarget(ID, "Register or login first");
		return;
	}
	if (!str_comp_nocase(aBuf, "armor"))
	{
		Mode = BUY_MODE_ADD_INT;
		pInt = &pSelf->m_apPlayers[ID]->GetCharacter()->m_Armor;
		Money = 10;
		Max = 10;
		str_copy(ItemName, "armor", 128);
		if (pResult->NumArguments() == 2)
			Num = pResult->GetInteger(1);
	}
	else if (!str_comp_nocase(aBuf, "bloody")) //Added just for test
	{
		Mode = BUY_MODE_BOOL;
		pBool = &pSelf->m_apPlayers[ID]->GetCharacter()->m_Bloody;
		Money = 30;
		str_copy(ItemName, "bloody", 128);
	}
	else if (!str_comp_nocase(aBuf, "shotgun"))
	{
		Mode = BUY_MODE_ADD_WEAPON;
		Weapon = WEAPON_SHOTGUN;
		Money = 10;
		str_copy(ItemName, "shotgun", 128);
	}
	else if (!str_comp_nocase(aBuf, "grenade"))
	{
		Mode = BUY_MODE_ADD_WEAPON;
		Weapon = WEAPON_GRENADE;
		Money = 15;
		str_copy(ItemName, "grenade", 128);
	}
	else if (!str_comp_nocase(aBuf, "rifle"))
	{
		Mode = BUY_MODE_ADD_WEAPON;
		Weapon = WEAPON_RIFLE;
		Money = 20;
		str_copy(ItemName, "rifle", 128);
	}
	else
	{
		pSelf->SendChatTarget(ID, "Unknow item.Items : armor, bloody, shotgun, grenade, rifle");
		return;
	}
	if (Mode == BUY_MODE_BOOL)
		if (*pBool)
		{
			*pBool = false;
			str_format(Chat, 512, "You removed your %s", ItemName);
			pSelf->SendChatTarget(ID, Chat);
			return;
		}
	if (Mode == BUY_MODE_ADD_WEAPON)
	{
		if (!pSelf->m_apPlayers[ID]->GetCharacter())
			return;
		if (!pSelf->m_apPlayers[ID]->GetCharacter()->GiveWeapon(Weapon, 10))
		{
			str_format(Chat, 512, "You can't carry more than one of %s", ItemName);
			pSelf->SendChatTarget(ID, Chat);
			return;
		}
	}
	if (Mode == BUY_MODE_ADD_INT && Num > Max-*pInt)
	{
		str_format(Chat, 512, "Max of this item is %d", Max);
		pSelf->SendChatTarget(ID, Chat);
		return;
	}
	if (pSelf->m_apPlayers[ID]->m_Money < Money*Num)
	{
		str_format(Chat, 512, "You don't have enough money (Need %d$)", Money*Num);
		pSelf->SendChatTarget(ID, Chat);
		return;
	}
	if (Mode == BUY_MODE_ADD_INT)
	{
		if (*pInt >= Max)
		{
			str_format(Chat, 512, "You have max of %s", ItemName);
			pSelf->SendChatTarget(ID, Chat);
			return;
		}
		*pInt = *pInt + Num;
		pSelf->m_apPlayers[ID]->m_Money -= Money*Num;
		str_format(Chat, 512, "You bought %d %ss", Num, ItemName);
		pSelf->SendChatTarget(ID, Chat);
	}
	if (Mode == BUY_MODE_ADD_WEAPON)
	{
		pSelf->m_apPlayers[ID]->m_Money -= Money;
		pSelf->m_apPlayers[ID]->GetCharacter()->GiveWeapon(Weapon, 10);
		str_format(Chat, 512, "You bought a %s", ItemName);
		pSelf->SendChatTarget(ID, Chat);
	}
	if (Mode == BUY_MODE_BOOL)
	{
		*pBool = true;
		pSelf->m_apPlayers[ID]->m_Money -= Money;
		str_format(Chat, 512, "You bought %s", ItemName);
		pSelf->SendChatTarget(ID, Chat);
	}
}

void CGameContext::ConInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"-->pWar<--");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Fight For Money");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Mod by Parham & MJavad");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Ideas By Poya (jiks)");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"-------- How To play: --------");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Use /team <team num> to change your team");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Team 0:      Alone (Enemy with all)");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Team 1~4: With teammates");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Block each other, drop money and collect");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Use /register <pass> to save your money in database");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"Use /login <pass> to login");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"You can pay money to a player with /pay <amount> <player name>");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"TODO: /buy");
}

void CGameContext::ConHelp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"/cmdlist will show a list of all chat commands");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"/help + any command will show you the help for this command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"Example /help settings will display the help about ");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		const IConsole::CCommandInfo *pCmdInfo =
				pSelf->Console()->GetCommandInfo(pArg, CFGFLAG_SERVER, false);
		if (pCmdInfo && pCmdInfo->m_pHelp)
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
					pCmdInfo->m_pHelp);
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"help",
					"Command is either unknown or you have given a blank command without any parameters.");
	}
}

void CGameContext::ConSettings(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"setting",
				"to check a server setting say /settings and setting's name, setting names are:");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "setting",
				"teams, cheats, collision, hooking, endlesshooking, me, ");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "setting",
				"hitting, oldlaser, timeout, votes, pause and scores");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		char aBuf[256];
		float ColTemp;
		float HookTemp;
		pSelf->m_Tuning.Get("player_collision", &ColTemp);
		pSelf->m_Tuning.Get("player_hooking", &HookTemp);
		if (str_comp(pArg, "teams") == 0)
		{
			str_format(
					aBuf,
					sizeof(aBuf),
					"%s %s",
					g_Config.m_SvTeam == 1 ?
							"Teams are available on this server" :
							!g_Config.m_SvTeam ?
									"Teams are not available on this server" :
									"You have to be in a team to play on this server", /*g_Config.m_SvTeamStrict ? "and if you die in a team all of you die" : */
									"and if you die in a team only you die");
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
					aBuf);
		}
		else if (str_comp(pArg, "collision") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					ColTemp ?
							"Players can collide on this server" :
							"Players Can't collide on this server");
		}
		else if (str_comp(pArg, "hooking") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					HookTemp ?
							"Players can hook each other on this server" :
							"Players Can't hook each other on this server");
		}
		else if (str_comp(pArg, "endlesshooking") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvEndlessDrag ?
							"Players can hook time is unlimited" :
							"Players can hook time is limited");
		}
		else if (str_comp(pArg, "hitting") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvHit ?
							"Player's weapons affect each other" :
							"Player's weapons has no affect on each other");
		}
		else if (str_comp(pArg, "oldlaser") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvOldLaser ?
							"Lasers can hit you if you shot them and that they pull you towards the bounce origin (Like DDRace Beta)" :
							"Lasers can't hit you if you shot them, and they pull others towards the shooter");
		}
		else if (str_comp(pArg, "me") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvSlashMe ?
							"Players can use /me commands the famous IRC Command" :
							"Players Can't use the /me command");
		}
		else if (str_comp(pArg, "timeout") == 0)
		{
			str_format(aBuf, sizeof(aBuf),
					"The Server Timeout is currently set to %d",
					g_Config.m_ConnTimeout);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
					aBuf);
		}
		else if (str_comp(pArg, "votes") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvVoteKick ?
							"Players can use Callvote menu tab to kick offenders" :
							"Players Can't use the Callvote menu tab to kick offenders");
			if (g_Config.m_SvVoteKick)
				str_format(
						aBuf,
						sizeof(aBuf),
						"Players are banned for %d second(s) if they get voted off",
						g_Config.m_SvVoteKickBantime);
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvVoteKickBantime ?
							aBuf :
							"Players are just kicked and not banned if they get voted off");
		}
		else if (str_comp(pArg, "pause") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvPauseable ?
							g_Config.m_SvPauseTime ?
									"/pause is available on this server and it pauses your time too" :
									"/pause is available on this server but it doesn't pause your time"
									:"/pause is NOT available on this server");
		}
		else if (str_comp(pArg, "scores") == 0)
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"settings",
					g_Config.m_SvHideScore ?
							"Scores are private on this server" :
							"Scores are public on this server");
		}
	}
}

void CGameContext::ConRules(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	bool Printed = false;
	if (g_Config.m_SvDDRaceRules)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No blocking.");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No insulting / spamming.");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No fun voting / vote spamming.");
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"rules",
				"Breaking any of these rules will result in a penalty, decided by server admins.");
		Printed = true;
	}
	if (g_Config.m_SvRulesLine1[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine1);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine2[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine2);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine3[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine3);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine4[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine4);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine5[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine5);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine6[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine6);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine7[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine7);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine8[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine8);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine9[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine9);
		Printed = true;
	}
	if (g_Config.m_SvRulesLine10[0])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				g_Config.m_SvRulesLine10);
		Printed = true;
	}
	if (!Printed)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No Rules Defined, Kill em all!!");
}

void CGameContext::ConTogglePause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	char aBuf[128];

	if(!g_Config.m_SvPauseable)
	{
		ConToggleSpec(pResult, pUserData);
		return;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pPlayer->GetCharacter() == 0)
	{
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pause",
	"You can't pause while you are dead/a spectator.");
	return;
	}
	if (pPlayer->m_Paused == CPlayer::PAUSED_SPEC && g_Config.m_SvPauseable)
	{
		ConToggleSpec(pResult, pUserData);
		return;
	}

	if (pPlayer->m_Paused == CPlayer::PAUSED_FORCE)
	{
		str_format(aBuf, sizeof(aBuf), "You are force-paused. %ds left.", pPlayer->m_ForcePauseTime/pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pause", aBuf);
		return;
	}

	pPlayer->m_Paused = (pPlayer->m_Paused == CPlayer::PAUSED_PAUSED) ? CPlayer::PAUSED_NONE : CPlayer::PAUSED_PAUSED;
}

void CGameContext::ConToggleSpec(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	char aBuf[128];

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if(!pPlayer)
		return;

	if (pPlayer->GetCharacter() == 0)
	{
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec",
	"You can't spec while you are dead/a spectator.");
	return;
	}

	if(pPlayer->m_Paused == CPlayer::PAUSED_FORCE)
	{
		str_format(aBuf, sizeof(aBuf), "You are force-paused. %ds left.", pPlayer->m_ForcePauseTime/pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec", aBuf);
		return;
	}

	pPlayer->m_Paused = (pPlayer->m_Paused == CPlayer::PAUSED_SPEC) ? CPlayer::PAUSED_NONE : CPlayer::PAUSED_SPEC;
}

void CGameContext::ConTop5(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (g_Config.m_SvHideScore)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "top5",
				"Showing the top 5 is not allowed on this server.");
		return;
	}

	if (pResult->NumArguments() > 0 && pResult->GetInteger(0) >= 0)
		pSelf->Score()->ShowTop5(pResult, pResult->m_ClientID, pUserData,
				pResult->GetInteger(0));
	else
		pSelf->Score()->ShowTop5(pResult, pResult->m_ClientID, pUserData);

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

#if defined(CONF_SQL)
void CGameContext::ConTimes(IConsole::IResult *pResult, void *pUserData)
{
	if(!CheckClientID(pResult->m_ClientID)) return;
	CGameContext *pSelf = (CGameContext *)pUserData;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if(g_Config.m_SvUseSQL)
	{
		CSqlScore *pScore = (CSqlScore *)pSelf->Score();
		CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
		if(!pPlayer)
			return;

		if(pResult->NumArguments() == 0)
		{
			pScore->ShowTimes(pPlayer->GetCID(),1);
			return;
		}

		else if(pResult->NumArguments() < 3)
		{
			if (pResult->NumArguments() == 1)
			{
				if(pResult->GetInteger(0) != 0)
					pScore->ShowTimes(pPlayer->GetCID(),pResult->GetInteger(0));
				else
					pScore->ShowTimes(pPlayer->GetCID(), (str_comp(pResult->GetString(0), "me") == 0) ? pSelf->Server()->ClientName(pResult->m_ClientID) : pResult->GetString(0),1);
				return;
			}
			else if (pResult->GetInteger(1) != 0)
			{
				pScore->ShowTimes(pPlayer->GetCID(), (str_comp(pResult->GetString(0), "me") == 0) ? pSelf->Server()->ClientName(pResult->m_ClientID) : pResult->GetString(0),pResult->GetInteger(1));
				return;
			}
		}

		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "/times needs 0, 1 or 2 parameter. 1. = name, 2. = start number");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "Example: /times, /times me, /times Hans, /times \"Papa Smurf\" 5");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "times", "Bad: /times Papa Smurf 5 # Good: /times \"Papa Smurf\" 5 ");

#if defined(CONF_SQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
			pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
	}
}
#endif

void CGameContext::ConRank(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments() > 0)
		if (!g_Config.m_SvHideScore)
			pSelf->Score()->ShowRank(pResult->m_ClientID, pResult->GetString(0),
					true);
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"rank",
					"Showing the rank of other players is not allowed on this server.");
	else
		pSelf->Score()->ShowRank(pResult->m_ClientID,
				pSelf->Server()->ClientName(pResult->m_ClientID));

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConJoinTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];

	if (pSelf->m_VoteCloseTime && pSelf->m_VoteCreator == pResult->m_ClientID)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"join",
				"You are running a vote please try again after the vote is done!");
		return;
	}
	else if (g_Config.m_SvTeam == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
				"Admin has disabled teams");
		return;
	}
	else if (g_Config.m_SvTeam == 2 && pResult->GetInteger(0) == 0 && pPlayer->GetCharacter()->m_LastStartWarning < pSelf->Server()->Tick() - 3 * pSelf->Server()->TickSpeed())
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"join",
				"You must join a team and play with somebody or else you can\'t play");
		pPlayer->GetCharacter()->m_LastStartWarning = pSelf->Server()->Tick();
	}

	if (pResult->NumArguments() > 0)
	{
		if (pPlayer->GetCharacter() == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					"You can't change teams while you are dead/a spectator.");
		}
		else
		{
			if (pPlayer->m_Last_Team
					+ pSelf->Server()->TickSpeed()
					* g_Config.m_SvTeamChangeDelay
					> pSelf->Server()->Tick())
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"You can\'t change teams that fast!");
			}
			else if (((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.SetCharacterTeam(
					pPlayer->GetCID(), pResult->GetInteger(0)))
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s joined team %d",
						pSelf->Server()->ClientName(pPlayer->GetCID()),
						pResult->GetInteger(0));
				pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
				pPlayer->m_Last_Team = pSelf->Server()->Tick();
			}
			else
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"You cannot join this team at this time");
			}
		}
	}
	else
	{
		char aBuf[512];
		if (!pPlayer->IsPlaying())
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"join",
					"You can't check your team while you are dead/a spectator.");
		}
		else
		{
			str_format(
					aBuf,
					sizeof(aBuf),
					"You are in team %d",
					((CGameControllerDDRace*) pSelf->m_pController)->m_Teams.m_Core.Team(
							pResult->m_ClientID));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					aBuf);
		}
	}
}

void CGameContext::ConMe(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	char aBuf[256 + 24];

	str_format(aBuf, 256 + 24, "'%s' %s",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			pResult->GetString(0));
	if (g_Config.m_SvSlashMe)
		pSelf->SendChat(-2, CGameContext::CHAT_ALL, aBuf, pResult->m_ClientID);
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"me",
				"/me is disabled on this server, admin can enable it by using sv_slash_me");
}

void CGameContext::ConSetEyeEmote(IConsole::IResult *pResult,
		void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if(pResult->NumArguments() == 0) {
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				(pPlayer->m_EyeEmote) ?
						"You can now use the preset eye emotes." :
						"You don't have any eye emotes, remember to bind some. (until you die)");
		return;
	}
	else if(str_comp_nocase(pResult->GetString(0), "on") == 0)
		pPlayer->m_EyeEmote = true;
	else if(str_comp_nocase(pResult->GetString(0), "off") == 0)
		pPlayer->m_EyeEmote = false;
	else if(str_comp_nocase(pResult->GetString(0), "toggle") == 0)
		pPlayer->m_EyeEmote = !pPlayer->m_EyeEmote;
	pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"emote",
			(pPlayer->m_EyeEmote) ?
					"You can now use the preset eye emotes." :
					"You don't have any eye emotes, remember to bind some. (until you die)");
}

void CGameContext::ConEyeEmote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (g_Config.m_SvEmotionalTees == -1)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "emote",
				"Server admin disabled emotes.");
		return;
	}

	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Emote commands are: /emote surprise /emote blink /emote close /emote angry /emote happy /emote pain");
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Example: /emote surprise 10 for 10 seconds or /emote surprise (default 1 second)");
	}
	else
	{
			if(pPlayer->m_LastEyeEmote + g_Config.m_SvEyeEmoteChangeDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
				return;

			if (!str_comp(pResult->GetString(0), "angry"))
				pPlayer->m_DefEmote = EMOTE_ANGRY;
			else if (!str_comp(pResult->GetString(0), "blink"))
				pPlayer->m_DefEmote = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "close"))
				pPlayer->m_DefEmote = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "happy"))
				pPlayer->m_DefEmote = EMOTE_HAPPY;
			else if (!str_comp(pResult->GetString(0), "pain"))
				pPlayer->m_DefEmote = EMOTE_PAIN;
			else if (!str_comp(pResult->GetString(0), "surprise"))
				pPlayer->m_DefEmote = EMOTE_SURPRISE;
			else if (!str_comp(pResult->GetString(0), "normal"))
				pPlayer->m_DefEmote = EMOTE_NORMAL;
			else
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,
						"emote", "Unknown emote... Say /emote");

			int Duration = 1;
			if (pResult->NumArguments() > 1)
				Duration = pResult->GetInteger(1);

			pPlayer->m_DefEmoteReset = pSelf->Server()->Tick()
							+ Duration * pSelf->Server()->TickSpeed();
			pPlayer->m_LastEyeEmote = pSelf->Server()->Tick();
	}
}

void CGameContext::ConShowOthers(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if (g_Config.m_SvShowOthers)
	{
		if (pPlayer->m_IsUsingDDRaceClient)
		{
			if (pResult->NumArguments())
				pPlayer->m_ShowOthers = pResult->GetInteger(0);
			else
				pPlayer->m_ShowOthers = !pPlayer->m_ShowOthers;
		}
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"showotherschat",
					"Showing players from other teams is only available with DDRace Client, http://DDRace.info");
	}
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"showotherschat",
				"Showing players from other teams is disabled by the server admin");
}

bool CheckClientID(int ClientID)
{
	dbg_assert(ClientID >= 0 || ClientID < MAX_CLIENTS,
			"The Client ID is wrong");
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;
	return true;
}

char* TimerType(int TimerType)
{
	char msg[3][128] = {"game/round timer.", "broadcast.", "both game/round timer and broadcast."};
	return msg[TimerType];
}
void CGameContext::ConSayTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;
	if(pChr->m_DDRaceState != DDRACE_STARTED)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime), "Your Time is %s%d:%s%d",
			((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
			((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "time", aBuftime);
}

void CGameContext::ConSayTimeAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;
	if(pChr->m_DDRaceState != DDRACE_STARTED)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime),
			"%s\'s current race time is %s%d:%s%d",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
			((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuftime, pResult->m_ClientID);
}

void CGameContext::ConTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuftime[64];
	int IntTime = (int) ((float) (pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float) pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime), "Your Time is %s%d:%s%d",
				((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
				((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->SendBroadcast(aBuftime, pResult->m_ClientID);
}

void CGameContext::ConSetTimerType(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	char aBuf[128];
	if(pPlayer->m_TimerType <= 2 && pPlayer->m_TimerType >= 0)
		str_format(aBuf, sizeof(aBuf), "Timer is displayed in", TimerType(pPlayer->m_TimerType));
	else if(pPlayer->m_TimerType == 3)
		str_format(aBuf, sizeof(aBuf), "Timer isn't displayed.");

	if(pResult->NumArguments() == 0) {
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,"timer",aBuf);
		return;
	}
	else if(str_comp_nocase(pResult->GetString(0), "gametimer") == 0) {
		pSelf->SendBroadcast("", pResult->m_ClientID);
		pPlayer->m_TimerType = 0;
	}
	else if(str_comp_nocase(pResult->GetString(0), "broadcast") == 0)
			pPlayer->m_TimerType = 1;
	else if(str_comp_nocase(pResult->GetString(0), "both") == 0)
			pPlayer->m_TimerType = 2;
	else if(str_comp_nocase(pResult->GetString(0), "none") == 0)
			pPlayer->m_TimerType = 3;
	else if(str_comp_nocase(pResult->GetString(0), "cycle") == 0) {
		if(pPlayer->m_TimerType < 3)
			pPlayer->m_TimerType++;
		else if(pPlayer->m_TimerType == 3)
			pPlayer->m_TimerType = 0;
	}
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,"timer",aBuf);
}

//XXLDDRace

void CGameContext::ConRescue(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter* pChr = pSelf->m_apPlayers[pResult->m_ClientID]->GetCharacter();
	char aBuf[256];

	if (!g_Config.m_SvRescue){
		pSelf->SendChatTarget(pResult->m_ClientID, "Rescue is not activated.");
		return;
	}

	if(pChr)
	{
		if (!pChr->m_LastRescue){

			float RescueDelay = 1.25;
			if (pChr->m_FreezeTime == 0)
				pSelf->SendChatTarget(pResult->m_ClientID, "You are not freezed!");
			else if (pChr->m_DeepFreeze)
				pSelf->SendChatTarget(pResult->m_ClientID, "You are deepfreezed, undeepfreeze first!");
			else if (!pChr->IsAlive())
				pSelf->SendChatTarget(pResult->m_ClientID, "You are not alive!");
			else if (pChr->m_RescuePos == vec2 (0,0)) //hum
				pSelf->SendChatTarget(pResult->m_ClientID, "No position saved!");
			else
			{
				//not freezed
				for(int i = 0;i <=(int)MAX_CLIENTS-1 ; i++)
				{
					if ( pSelf->m_apPlayers[i])
					{
						CCharacter* pChr2 = pSelf->m_apPlayers[i]->GetCharacter();
						//who hooks me?
						if (pChr2 && pChr2->Core()->m_HookedPlayer == pResult->m_ClientID)
						{
							//Release hook
							pChr2->Core()->m_HookedPlayer = -1;
							pChr2->Core()->m_HookState = HOOK_RETRACTED;
							pChr2->Core()->m_HookPos = pChr2->Core()->m_Pos;
						}
					}
				}
				if(g_Config.m_SvRescueEffects)
				{
					//Blood effect
					pChr->GameServer()->CreateDeath(pChr->m_Pos, pResult->m_ClientID);
					//Spawn effect
					pChr->GameServer()->CreatePlayerSpawn(pChr->m_RescuePos);
				}
				//"save" last rescue time
				pChr->m_LastRescue = RescueDelay * pSelf->Server()->TickSpeed();
				//Teleport player
				pChr->Core()->m_Pos = pChr->m_RescuePos;
			}
		}
		else
		{
			if (pChr->m_TileIndex != TILE_FREEZE && pChr->m_TileFIndex != TILE_FREEZE)
				pChr->UnFreeze();
		}
	}
	else
		pSelf->SendChatTarget(pResult->m_ClientID, "You are not alive!");
}

void CGameContext::ConHelper(IConsole::IResult *pResult, void *pUserData)
{
	int helper = 0;
	char aBuf[128];
	int Seconds;
	int HelperTime = 60;

	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter* pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
	CServer* pServ = (CServer*)pSelf->Server();

	if (pResult->NumArguments() == 1 && pSelf->m_apPlayers[pResult->m_ClientID]->m_Authed >= pServ->AUTHED_MOD)
	{
		if(!CheckClientID(pResult->GetInteger(0))) return;
		CCharacter* pChr2 = pSelf->GetPlayerChar(pResult->GetInteger(0));
		if(pSelf->m_apPlayers[pResult->GetInteger(0)]
				&& pChr2 && pResult->m_ClientID != pResult->GetInteger(0)
				&& pSelf->m_apPlayers[pResult->GetInteger(0)]->m_Authed == pServ->AUTHED_NO)
			pServ->SetRconLevel(pResult->GetInteger(0), pServ->AUTHED_HELPER);

		return;
	}

	if(pChr && g_Config.m_SvHelper)
	{

		if (pSelf->m_apPlayers[pResult->m_ClientID]->m_Helped)
		{

			Seconds = pSelf->m_apPlayers[pResult->m_ClientID]->m_Helped / pSelf->Server()->TickSpeed();
			str_format(aBuf, sizeof(aBuf), "Please wait %d seconds to call a helper again.",  Seconds);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			return;
		}
		for(int i = 0;i <=(int)MAX_CLIENTS-1 ; i++)
		{
			CCharacter* pChr3 = pSelf->GetPlayerChar(i);
			if(pChr3)
			{
				if (pSelf->m_apPlayers[i]->m_Authed >= pServ->AUTHED_HELPER)
				{
					str_format(aBuf, sizeof(aBuf), "%s needs help. ID:%i", pSelf->Server()->ClientName(pResult->m_ClientID), pResult->m_ClientID);
					pSelf->SendChatTarget(i, aBuf);
					pServ->SendRconLine(i, aBuf);
					helper++;

				}
			}
		}
		if (helper == 0 )
			str_format(aBuf, sizeof(aBuf), "Sorry, but there is no helper online.");
		else if (helper == 1)
		{
			str_format(aBuf, sizeof(aBuf), "You called one helper!");
			pSelf->m_apPlayers[pResult->m_ClientID]->m_Helped = HelperTime * pSelf->Server()->TickSpeed();
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "%i helpers called!", helper);
			pSelf->m_apPlayers[pResult->m_ClientID]->m_Helped = HelperTime * pSelf->Server()->TickSpeed();
		}
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "sv_helper not activated.");
	}
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
}

void CGameContext::ConJumps(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;
	CCharacter *pChr = pSelf->m_apPlayers[pResult->m_ClientID]->GetCharacter();
	if (!pChr)
		return;

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "You can jump %d times.", pChr->Core()->m_MaxJumps);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
}

void CGameContext::ConRegister(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->MemberList->Register(pResult, pResult->m_ClientID, pResult->GetString(0), pSelf);
}

void CGameContext::ConLogin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->MemberList->Login(pResult, pResult->m_ClientID, pResult->GetString(0), pSelf);
	if (pSelf->m_apPlayers[pResult->m_ClientID]->m_IsLoggedIn && pSelf->m_apPlayers[pResult->m_ClientID]->GetTeam() == TEAM_SPECTATORS)
		pSelf->m_apPlayers[pResult->m_ClientID]->SetTeam(TEAM_RED);
}

void CGameContext::ConLogOut(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CServer* pServer = (CServer*)pSelf->Server();

	int Victim;
	if(pResult->NumArguments() == 0)
		Victim = pResult->m_ClientID;
	else
		Victim = pResult->GetInteger(0);

	if(!CheckClientID(Victim)) return;

	if (g_Config.m_SvRconScore)
		pSelf->m_apPlayers[Victim]->m_Score = 0;

	// logout from membertile
	pSelf->m_apPlayers[Victim]->m_IsMember = false;
	pSelf->m_apPlayers[Victim]->m_IsLoggedIn = false;
	pSelf->m_apPlayers[Victim]->SetTeam(TEAM_SPECTATORS);

	// DDRace level reset
	pSelf->OnSetAuthed(Victim, pServer->AUTHED_NO);

	// from server.cpp
	if(pServer->m_aClients[Victim].m_State != CServer::CClient::STATE_EMPTY)
	{
		CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
		Msg.AddInt(0);	//authed
		Msg.AddInt(0);	//cmdlist
		pServer->SendMsgEx(&Msg, MSGFLAG_VITAL, Victim, true);

		pServer->m_aClients[Victim].m_Authed = pServer->AUTHED_NO;
		pServer->m_aClients[Victim].m_pRconCmdToSend = 0;
		pServer->SendRconLine(Victim, "Logout successful.");
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "ClientID=%d logged out", Victim);
		pServer->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}
}

