/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_DDRACECOMMANDS_H
#define GAME_SERVER_DDRACECOMMANDS_H
#undef GAME_SERVER_DDRACECOMMANDS_H // this file can be included several times
#ifndef CHAT_COMMAND
#define CHAT_COMMAND(name, params, flags, callback, userdata, help)
#endif

enum
{
	BUY_MODE_BOOL=0,
	BUY_MODE_ADD_INT,
	BUY_MODE_ADD_WEAPON,
};

CHAT_COMMAND("credits", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConCredits, this, "Shows the credits of the DDRace mod")
CHAT_COMMAND("buy", "s?i", CFGFLAG_CHAT|CFGFLAG_SERVER, ConBuy, this, "Shop")
CHAT_COMMAND("emote", "?si", CFGFLAG_CHAT|CFGFLAG_SERVER, ConEyeEmote, this, "Sets your tee's eye emote")
CHAT_COMMAND("eyeemote", "?s", CFGFLAG_CHAT|CFGFLAG_SERVER, ConSetEyeEmote, this, "Toggles use of standard eye-emotes on/off, eyeemote s, where s = on for on, off for off, toggle for toggle and nothing to show current status")
CHAT_COMMAND("settings", "?s", CFGFLAG_CHAT|CFGFLAG_SERVER, ConSettings, this, "Shows gameplay information for this server")
CHAT_COMMAND("help", "?r", CFGFLAG_CHAT|CFGFLAG_SERVER, ConHelp, this, "Shows help to command r, general help if left blank")
CHAT_COMMAND("info", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConInfo, this, "Shows info about this server")
CHAT_COMMAND("me", "r", CFGFLAG_CHAT|CFGFLAG_SERVER, ConMe, this, "Like the famous irc command '/me says hi' will display '<yourname> says hi'")
CHAT_COMMAND("pause", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConTogglePause, this, "Toggles pause (if not activated on the server, it toggles spec)")
CHAT_COMMAND("spec", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConToggleSpec, this, "Toggles spec")
CHAT_COMMAND("rank", "?r", CFGFLAG_CHAT|CFGFLAG_SERVER, ConRank, this, "Shows the rank of player with name r (your rank by default)")
CHAT_COMMAND("rules", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConRules, this, "Shows the server rules")
CHAT_COMMAND("team", "?i", CFGFLAG_CHAT|CFGFLAG_SERVER, ConJoin, this, "Join a team")
CHAT_COMMAND("top5", "?i", CFGFLAG_CHAT|CFGFLAG_SERVER, ConTop5, this, "Shows five ranks of the ladder beginning with rank i (1 by default)")
CHAT_COMMAND("showothers", "?i", CFGFLAG_CHAT|CFGFLAG_SERVER, ConShowOthers, this, "Whether to showplayers from other teams or not (off by default), optional i = 0 for off else for on")
CHAT_COMMAND("saytime", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConSayTime, this, "Privately messages you your current time in this current running race")
CHAT_COMMAND("saytimeall", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConSayTimeAll, this, "Publicly messages everyone your current time in this current running race")
CHAT_COMMAND("time", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConTime, this, "Privately shows you your current time in this current running race in the broadcast message")
CHAT_COMMAND("timer", "?s", CFGFLAG_CHAT|CFGFLAG_SERVER, ConSetTimerType, this, "Personal Setting of showing time in either broadcast or game/round timer, timer s, where s = broadcast for broadcast, gametimer for game/round timer, cycle for cycle, both for both, none for no timer and nothing to show current status")
CHAT_COMMAND("pay", "ir", CFGFLAG_CHAT|CFGFLAG_SERVER, ConPay, this, "Give money to a player")

CHAT_COMMAND("rescue", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConRescue, this, "Rescue from freeze")
CHAT_COMMAND("r", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConRescue, this, "Rescue from freeze")
CHAT_COMMAND("jumps", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConJumps, this, "Shows your jumps")
CHAT_COMMAND("register", "r", CFGFLAG_CHAT|CFGFLAG_SERVER, ConRegister, this, "Register to memberlist")
CHAT_COMMAND("login", "r", CFGFLAG_CHAT|CFGFLAG_SERVER, ConLogin, this, "Login to memberlist")
CHAT_COMMAND("helper", "?i", CFGFLAG_CHAT|CFGFLAG_SERVER, ConHelper, this, "Calls a helper./Authenticates player v to the level of 1")
CHAT_COMMAND("logout", "?i", CFGFLAG_CHAT|CFGFLAG_SERVER, ConLogOut, this, "Logout of rcon and membertile")

#if defined(CONF_SQL)
CHAT_COMMAND("times", "?s?i", CFGFLAG_CHAT|CFGFLAG_SERVER, ConTimes, this, "/times ?s?i shows last 5 times of the server or of a player beginning with name s starting with time i (i = 1 by default)")
#endif
#undef CHAT_COMMAND

#endif
