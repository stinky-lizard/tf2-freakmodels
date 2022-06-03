#include <sourcemod>
#include <tf2_stocks>
// ^ tf2_stocks.inc itself includes sdktools.inc and tf2.inc
#include <tf2items>
#include <sdkhooks>

#include <freakmodels-core>

#include <freakmodels-equip>
#include <freakmodels-cleanup>
#include <freakmodels-manage>
#include <freakmodels-menu>
#include <freakmodels-fixes>

#pragma semicolon 1
#pragma newdecls required

#define PLUGIN_VERSION "1.4"


public Plugin myinfo = 
{
	name = "FreakModels",
	author = "Stinky Lizard",
	description = "Allows you to change your model and animations independently of your class. Made for FREAKSERVER 'The Most Fun You Can Have Online' at freak.tf2.host.",
	version = PLUGIN_VERSION,
	url = "freak.tf2.host"
};


/*
 FORWARDS
 =======================================================
 */

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	// No need for the old GetGameFolderName setup.
	EngineVersion g_engineversion = GetEngineVersion();
	if (g_engineversion != Engine_TF2)
	{
		SetFailState("FreakModels was made for use with Team Fortress 2 only.");
	}
} 

public void OnPluginStart()
{
	/**
	 * @note For the love of god, please stop using FCVAR_PLUGIN.
	 * Console.inc even explains this above the entry for the FCVAR_PLUGIN define.
	 * "No logic using this flag ever existed in a released game. It only ever appeared in the first hl2sdk."
	 */
	CreateConVar("sm_freakmodels_version", PLUGIN_VERSION, "Standard plugin version ConVar. Please don't change me!", FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);
	
	LoadTranslations("common.phrases");

	//create the item to give
	g_hDummyItemView = TF2Items_CreateItem(OVERRIDE_ALL|FORCE_GENERATION);
	TF2Items_SetClassname(g_hDummyItemView, "tf_wearable");
	TF2Items_SetItemIndex(g_hDummyItemView, -1); //Q: playermodel2 uses 65535. Is there a reason why?
	TF2Items_SetQuality(g_hDummyItemView, 0);
	TF2Items_SetLevel(g_hDummyItemView, 0);
	TF2Items_SetNumAttributes(g_hDummyItemView, 0);

	//prepare the EquipWearable function call
	GameData hGameConf = new GameData("freakmodels.data");
	if(hGameConf == null) {
		SetFailState("FreakModels: Gamedata (addons/sourcemod/gamedata/freakmodels.data.txt) not found.");
		delete hGameConf;
		return;
	}

	StartPrepSDKCall(SDKCall_Entity);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CBasePlayer::EquipWearable");
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer);
	g_hEquipWearable = EndPrepSDKCall();
	if(g_hEquipWearable == null) {
		SetFailState("FreakModels: Failed to create SDKCall for CBasePlayer::EquipWearable.");
		delete hGameConf;
		return;
	}
	delete hGameConf;
	
	//model configs
	RefreshConfigFromFile();

	//initialize player data equippables
	for (int i = 1; i < PLAYERSDATASIZE; i++)
		PlayerData(i).InitEquipList();

	//initialize cleanup stuff
	allWearables = new ArrayList();

	CreateTimer(120.0, Timer_RegularCleanup, _, TIMER_REPEAT);

	//fix anim resetting after inventory applied
	HookEvent("post_inventory_application", OnInventoryApplied);

	//get req admin flags
	config.Rewind();
	config.JumpToKey("comments");
	int adm = config.GetNum("RequiredAdmin");

	config.Rewind();

	RegAdminCmd("freakmodel", MainCommand, adm);
	RegAdminCmd("freakmodel_manage", ManageCommand, adm);
	
	RegAdminCmd("fakeclass", MainCommand, adm); //for backwards-compatibility QoL

	RegAdminCmd("fm_equip", EquipCommand, adm);
}

//precache models found in the config file
public void OnMapStart()
{
	bool modelsDefined = false;

	config.Rewind();

	if (config.JumpToKey("named_models"))
	{
		bool sectionDefined = PrecacheAllModelsInCurSection();
		modelsDefined = sectionDefined || modelsDefined;

		config.Rewind();
	}
	
	if (config.JumpToKey("precache_no_name"))
	{
		bool sectionDefined = PrecacheAllModelsInCurSection();
		modelsDefined = sectionDefined || modelsDefined;

		config.Rewind();

	}

	if (!modelsDefined)
	{
		//there are no defined models
		LogMessage("Warning: There are no models defined in the configuration file. Consider adding some, or deleting the file to re-initialize it.");
	} 
	
}

//Remove any skins from players (since they're items, they stay after) & re-enable any players
public void OnPluginEnd()
{
	for (int i = 0; i < PLAYERSDATASIZE; i++)
	{
		if (PlayerData(i).userId)
		{
			//there has been data set for this player
			RemoveSkin(i);
			RemoveAnim(i);

			PrintToChat(i, "FreakModels is being unloaded or reloaded. Any customizations of yours have been removed (but feel free to re-apply them!");
		}

		PlayerData(i).ClearData();
	}

	while(allWearables.Length > 0)
		CleanupWearables(false);
}

//Remove any skins from players that disconnect
public void OnClientDisconnect(int client)
{
	RemoveSkin(client);
	//dont need to manage the anim; its data is already reset w cleardata
	PlayerData(client).ClearData();
}

public void OnEntityDestroyed(int entity)
{
	char classname[MAX_NAME_LENGTH];
	GetEntityClassname(entity, classname, sizeof(classname));
	if (!(StrEqual(classname, "tf_wearable", false) || StrEqual(classname, "freakmodel_wearable", false)))
		//it ain't a wearable, def isn't ours
		return;
	
	int usedBy = ItemUsedBy(EntIndexToEntRef(entity));

	if (usedBy != -1 && EntRefToEntIndex(PlayerData(usedBy).rSkinItem) == entity)
	{
		//it was a skin that was destroyed
		if (IsValidClient(usedBy)) MakePlayerVisible(usedBy);
		PlayerData(usedBy).rSkinItem = 0;
	}
}

/*
 INTERFACE FUNCTIONS
 =============================================
 */

Action MainCommand(int client, int args)
{

	ResetClientCommandData(client);

	char skinPath[PLATFORM_MAX_PATH], animPath[PLATFORM_MAX_PATH];
	char skinName[PLATFORM_MAX_PATH], animName[PLATFORM_MAX_PATH];
	char targetinput[256];
	bool useFullpaths = false, validArgEntered = false, reset = false, doAll = false, targetMode = false;

	int targetsFound;
	int targetList[MAXPLAYERS];
	//first read the cmd args to figure out what to do

	char tmparg[128];

	for (int i = 1; i <= args; i++)
	{
		GetCmdArg(i, tmparg, sizeof(tmparg));

		if (StrEqual(tmparg, "-a", false) || StrEqual(tmparg, "-anim", false) || StrEqual(tmparg, "-animation", false))
		{
			validArgEntered = true;
			//read next cmd for anim
			i++;
			FuncOutput out = GetPathArg(args, i, animName, sizeof(animName));
			if (out != GOOD)
			{
				ReplyToCommand(client, out == GETPATHARG_NOVAL ? "Please enter a value for %s." : "You can't specify %s twice!", tmparg);
				return Plugin_Handled;
			}
		}
		else if 
		(
			StrEqual(tmparg, "-m", false) || StrEqual(tmparg, "-model", false) 
			|| StrEqual(tmparg, "-s", false) || StrEqual(tmparg, "-skin", false)
		)
		{
			validArgEntered = true;
			//read next cmd for skin
			i++;
			FuncOutput out = GetPathArg(args, i, skinName, sizeof(skinName));
			if (out != GOOD)
			{
				ReplyToCommand(client, out == GETPATHARG_NOVAL ? "Please enter a value for %s." : "You can't specify %s twice!", tmparg);
				return Plugin_Handled;
			}
		}
		else if
		(
			StrEqual(tmparg, "-t", false) || StrEqual(tmparg, "-target", false)
			|| StrEqual(tmparg, "-p", false) || StrEqual(tmparg, "-player", false)
		)
		{
			validArgEntered = true;
			targetMode = true;
			//read next cmd for player
			i++;
			FuncOutput out = GetPathArg(args, i, targetinput, sizeof(targetinput));
			if (out != GOOD)
			{
				ReplyToCommand(client, out == GETPATHARG_NOVAL ? "Please enter a value for %s." : "You can't specify %s twice!", tmparg);
				return Plugin_Handled;
			}

			//get the player(s) they actually want
			bool foundOne = GetClientsFromUsername(client, targetinput, targetsFound, targetList, sizeof(targetList));

			if (!foundOne) 
			{
				return Plugin_Handled;
			}
		}
		else if (StrEqual(tmparg, "-confirm", false))
		{
			doAll = true;
		}
		else if 
		(
			StrEqual(tmparg, "-f", false) || StrEqual(tmparg, "-full", false) 
			|| StrEqual(tmparg, "-fullpath", false) || StrEqual(tmparg, "-fullpaths", false)
			|| StrEqual(tmparg, "-path", false) || StrEqual(tmparg, "-paths", false)
		)
		{
			validArgEntered = true;
			useFullpaths = true;
		}
		else if (StrEqual(tmparg, "-h", false) || StrEqual(tmparg, "-help", false))
		{
			validArgEntered = true;
			PrintHelp(client);
		}
		else if (StrEqual(tmparg, "-r", false) || StrEqual(tmparg, "-reset", false))
		{
			validArgEntered = true;
			//TODO check if they entered a cmd arg that starts with a hyphen after this; if so set the reset mode to that, if not do both
			//TODO do this correctly
			reset = true;
		}
	}

	if (!validArgEntered)
	{
		if (args > 0)
		{
			ReplyToCommand(client, "Sorry, couldn't understand your arguments.");
			// ReplyToCommand(client, "Enter `freakmodel -help` to print help in the console.");
			ReplyToCommand(client, "Enter `freakmodel -help` to print help in the console, or simply `freakmodel` to use a menu.");
		}
		else 
		{
			MainCommandMenu(client);
			// ReplyToCommand(client, "Enter `freakmodel -help` to print help in the console.");
		}
		return Plugin_Handled;
	}


	if (reset && (animName[0] || skinName[0]))
	{
		ReplyToCommand(client, "Sorry, you can't set the animation or skin & reset it in the same operation.");
		ReplyToCommand(client, "Please use only -anim/-skin or -reset.");
		return Plugin_Handled;
	}

	//translate model names to paths
	if (!useFullpaths)
	{
		if (animName[0])
		{
			ToLowerCase(animName, sizeof(animName)); //make model names case-agnostic
			if (!GetModelFromConfig(animName, animPath, sizeof(animPath)))
			{
				ReplyToCommand(client, "Sorry, we couldn't find your animation model (%s) in our list. Check your spelling?", animName);
				animPath = "";
				animName = "";
			}
			
		}
		if (skinName[0])
		{
			ToLowerCase(animName, sizeof(animName));
			if (!GetModelFromConfig(skinName, skinPath, sizeof(skinPath)))
			{
				ReplyToCommand(client, "Sorry, we couldn't find your skin model (%s) in our list. Check your spelling?", skinName);
				skinPath = "";
				skinName = "";
			}
		}
	}
	else
	{
		if (animName[0]) animPath = animName;
		if (skinName[0]) skinPath = skinName;
	}

	//correct \s to /s
	ReplaceString(animPath, sizeof(animPath), "\\", "/");
	ReplaceString(skinPath, sizeof(skinPath), "\\", "/");

	//check if the models are good
	if (animPath[0] && !IsModelPrecached(animPath))
	{
		if (useFullpaths)
		{
			if (FileExists(animPath, true)) 
				ReplyToCommand(client, "Sorry, your animation model is not precached and we cannot use it.");
			else 
				ReplyToCommand(client, "Unknown animation model!");
		} 
		else 
			//the file is set in the config but it wasn't precached... despite it was verified to exist before
			ReplyToCommand(client, "Sorry, an error occurred. Please report this to the server operator. (FreakModels error code: 21)");
		animPath = "";
	}
	if (skinPath[0] && !IsModelPrecached(skinPath))
	{
		if (useFullpaths)
		{
			if (FileExists(skinPath, true)) 
				ReplyToCommand(client, "Sorry, your skin model is not precached and we cannot use it.");
			else 
				ReplyToCommand(client, "Unknown skin model!");
		} 
		else 
			ReplyToCommand(client, "Sorry, an error occurred. Please report this to the server operator. (FreakModels error code: 22)");
		skinPath = "";
	}

	//now actually do the stuff

	g_clientsCommandData[client].animPath = animPath;
	g_clientsCommandData[client].skinPath = skinPath;
	g_clientsCommandData[client].animName = animName;
	g_clientsCommandData[client].skinName = skinName;

	g_clientsCommandData[client].isReset = reset;
	g_clientsCommandData[client].useFullpaths = useFullpaths;

	if (!targetMode && client > 0)
	{
		targetsFound = 1;
		targetList[0] = client;
	}
	else if (!targetMode && client < 1)
	{
		ReplyToCommand(client, "Please specify a player.");
		ResetClientCommandData(client);
		return Plugin_Handled;
	}


	if (targetsFound < 1)
	{
		ReplyToCommand(client, "Sorry, your target doesn't match any players.");
		ResetClientCommandData(client);
	}
	else if (targetsFound > 1 && !doAll)
	{
		ReplyToCommand(client, "You targeted more than one player. Are you sure?");
		if (client == 0)
		{
			ReplyToCommand(client, "If you are, please input this command again with \"-confirm\" added.");
		}
		else
		{
			Menu menu = new Menu(ConfirmAllMenuHandler, MENU_ACTIONS_DEFAULT);
			menu.SetTitle("Are you sure?");
			menu.AddItem("yes", "YES ✔ ✔ ✔");
			menu.AddItem("no",  "NO X X X");
			menu.ExitButton = false;
			menu.Display(client, MENU_TIME_FOREVER);
		}
	}
	else
	{
		g_clientsCommandData[client].targets = targetList;
		g_clientsCommandData[client].numTargets = targetsFound;
		SetModelsFromData(client);
	}

	return Plugin_Handled;

}

void SetModelsFromData(int client)
{
	ModelChangeData data;
	data = g_clientsCommandData[client];


	int resetMode = 0; //TODO add resetMdoe func

	char animPath[PLATFORM_MAX_PATH];
	animPath = data.animPath;

	char skinPath[PLATFORM_MAX_PATH];
	skinPath = data.skinPath;

	char clientName[65];
	GetClientName(client, clientName, sizeof(clientName));

	for (int i = 0; i < data.numTargets; i++)
	{
		int target = data.targets[i];

		char targetString[65];
		GetTargetString(client, target, targetString, sizeof(targetString));

		if (data.isReset)
		{
			switch (resetMode)
			{
				case 0:
				{
					//reset both
					RemoveAnim(target);
					RemoveSkin(target);
					ReplyToCommand(client, "Successfully reset %s skin and animations.", targetString);

					if (client != target) PrintToChat(target, "%s reset your skin and animations.", clientName);
				}
				case 1: 
				{
					//reset anim
					RemoveAnim(target);
					ReplyToCommand(client, "Successfully reset %s animations.", targetString);

					if (client != target) PrintToChat(target, "%s reset your animations.", clientName);
				}
				case 2:
				{
					//reset model
					RemoveSkin(target);
					ReplyToCommand(client, "Successfully reset %s skin.", targetString);
					
					if (client != target) PrintToChat(target, "%s reset your skin.", clientName);
				}
			}
		}

		if (animPath[0])
		{
			SetAnim(target, animPath);

			GetTargetString(client, target, targetString, sizeof(targetString));
			if (data.useFullpaths) 
			{
				ReplyToCommand(client, "Successfully set %s animation model to %s.", targetString, animPath);

				if (client != target) PrintToChat(target, "%s set your animation model to %s.", clientName, animPath);
			}
			else
			{
				PlayerData(target).SetStr("AnimName", data.animName);

				ReplyToCommand(client, "Successfully set %s animations to \"%s\"", targetString, data.animName);

				if (client != target) PrintToChat(target, "%s set your animations to %s.", clientName, data.animName);
			}

		}
		if (skinPath[0])
		{
			GetTargetString(client, target, targetString, sizeof(targetString));
			
			if (SetSkin(target, skinPath) == TARGETNOTEAM)
				ReplyToCommand(client, "You can't set %s skin; %s aren't in the game!", targetString, (client == target) ? "you" : "they");
			//successfully set the skin confirmations:
			else if (data.useFullpaths) 
			{
				ReplyToCommand(client, "Successfully set %s skin model to %s.", targetString, skinPath);

				if (client != target) PrintToChat(target, "%s set your skin model to %s.", clientName, skinPath);
			}
			else
			{
				PlayerData(target).SetStr("SkinName", data.skinName);

				ReplyToCommand(client, "Successfully set %s skin to \"%s\"", targetString, data.skinName);

				if (client != target) PrintToChat(target, "%s set your skin to %s.", clientName, data.skinName);
			}
		}
	}

	ResetClientCommandData(client);
}


public int ConfirmAllMenuHandler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_End:
		{
			delete menu;
		}
		case MenuAction_Select:
		{
			char info[32];
			menu.GetItem(param2, info, sizeof(info));

			if (StrEqual(info, "yes"))
			{
				SetModelsFromData(param1);
			}
			else
			{
				ResetClientCommandData(param1);
			}
		}
	}
}

public Action Timer_HelpPrint(Handle timer, Handle hndl)
{
	
	//fuck this. replytocommand doesn't print in the right order :(
	ArrayList list = view_as<ArrayList>(hndl);

	int client;
	if (list.Get(0) < 0)
	{
		client = 0;
	}
	else 
	{
		client = GetClientOfUserId(list.Get(0));	
	}
	
	
	switch(list.Get(1))
	{
		case 1:  { PrintToConsole(client, "Use freakmodel to change the model or animations of a player."); }
		case 2:  { PrintToConsole(client, "Important options: "); }
		case 3:  { PrintToConsole(client, "- Enter -skin <model> or -model <model> to set the appearance of a player to a model."); }
		case 4:  { PrintToConsole(client, "- Enter -anim <model> to set the animations of a player to a model's."); }
		// case 4: { PrintToConsole(client, "Enter -reset [anim|model] to reset the target's animation/model. If [anim|model] is omitted both will be reset."); }
		case 5:  { PrintToConsole(client, "- Enter -reset to reset a player's animation & skin."); }
		case 6:  { PrintToConsole(client, "--------------------------------"); }
		case 7:  { PrintToConsole(client, "Less important options: "); }
		case 8:  { PrintToConsole(client, "- Enter -fullpath to use the path to a model instead of a name. This requires knowledge of Source model paths & locations."); }
		case 9:  { PrintToConsole(client, "- Enter -target <username> to target a specific player. The command will target yourself if this is omitted."); }
		case 10: { PrintToConsole(client, "- Enter -help to print this dialogue in your console."); }
		case 11: { PrintToConsole(client, "All options (-skin, -anim, etc.) can also be specified with only the first letter (-s, -a, etc.) if you like."); }
		case 12: { PrintToConsole(client, "--------------------------------"); }
		case 13: { PrintToConsole(client, "Model names & their associated models are defined by the server operator. By default, you can use the classes (scout, medic, etc.), but there may be more!"); }
		case 14: { PrintToConsole(client, "--------------------------------"); }
		case 15:
		{
			PrintToConsole(client, "For example: inputting 'freakmodel -s heavy -t bob' will set bob's model to heavy, without changing their animations; inputting 'freakmodel -r' will reset your own model and animations.");
			delete list;
			return Plugin_Handled;
		}
	}
	list.Set(1, list.Get(1) + 1);
	return Plugin_Handled;
}

void PrintHelp(int client)
{
	ReplyToCommand(client, "Printing help in your console.");
	//this is horrible but ReplyToCommand doesn't want to print in the right order without it :(
	//list is this: [userId, lineNum]

	ArrayList list = new ArrayList();
	if (client < 1)
	{
		list.Push(-1);
	}
	else
	{
		list.Push(GetClientUserId(client));
	}
	
	list.Push(1);
	CreateTimer(0.1, Timer_HelpPrint, list);
	CreateTimer(0.2, Timer_HelpPrint, list);
	CreateTimer(0.3, Timer_HelpPrint, list);
	CreateTimer(0.4, Timer_HelpPrint, list);
	CreateTimer(0.5, Timer_HelpPrint, list);
	CreateTimer(0.6, Timer_HelpPrint, list);
	CreateTimer(0.7, Timer_HelpPrint, list);
	CreateTimer(0.8, Timer_HelpPrint, list);
	CreateTimer(0.9, Timer_HelpPrint, list);
	CreateTimer(1.0, Timer_HelpPrint, list);
	CreateTimer(1.1, Timer_HelpPrint, list);
	CreateTimer(1.2, Timer_HelpPrint, list);
	CreateTimer(1.3, Timer_HelpPrint, list);
	CreateTimer(1.4, Timer_HelpPrint, list);
	CreateTimer(1.5, Timer_HelpPrint, list);
	
	
}

void ResetClientCommandData(int client)
{
	g_clientsCommandData[client].useFullpaths = false;
	g_clientsCommandData[client].isReset = false;
	g_clientsCommandData[client].animPath = "";
	g_clientsCommandData[client].skinPath = "";
	g_clientsCommandData[client].animName = "";
	g_clientsCommandData[client].skinName = "";

	for (int i = 0; i < g_clientsCommandData[client].numTargets; i++)
	{
		g_clientsCommandData[client].targets[i] = 0;
	}
	g_clientsCommandData[client].numTargets = 0;
	
}