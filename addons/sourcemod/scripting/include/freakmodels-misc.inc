
public void OnInventoryApplied(Event event, const char[] name, bool dontBroadcast)
{
	int userId = event.GetInt("userid", -1);
	if (userId == -1) return;

	int clientId = GetClientOfUserId(userId);
	if (!IsValidClient(clientId)) return;
		
	//does this client have an animation set?
	char animPath[PLATFORM_MAX_PATH];
	PlayerData(clientId).GetStr("AnimPath", animPath, sizeof(animPath));
	
	if (animPath[0])
	{	
		CreateTimer(0.2, Timer_InventoryApplied, userId);
		CreateTimer(2.0, Timer_CheckAnimAfterInventoryApplied, userId);
	}
}

public Action Timer_InventoryApplied(Handle timer, int userId)
{
	int client = GetClientOfUserId(userId);
	if (!IsValidClient(client)) return Plugin_Handled;
	
	char animPath[PLATFORM_MAX_PATH];
	PlayerData(client).GetStr("AnimPath", animPath, sizeof(animPath));
	SetAnim(client, animPath);
	
	return Plugin_Handled;
}

public Action Timer_CheckAnimAfterInventoryApplied(Handle timer, int userId)
{
	int client = GetClientOfUserId(userId);
	if (!IsValidClient(client)) return Plugin_Handled;

	char setAnimPath[PLATFORM_MAX_PATH];
	PlayerData(client).GetStr("AnimPath", setAnimPath, sizeof(setAnimPath));

	char curAnimPath[PLATFORM_MAX_PATH];
	GetClientModel(client, curAnimPath, sizeof(curAnimPath));

	if (!StrEqual(curAnimPath, setAnimPath))
		SetAnim(client, setAnimPath);

	return Plugin_Handled;
}

public void OnTeamChanged(Event event, const char[] name, bool dontBroadcast)
{
	int userId = event.GetInt("userid", -1);
	if (userId == -1) return;

	int clientId = GetClientOfUserId(userId);
	if (!IsValidClient(clientId)) return;

	TFTeam team = view_as<TFTeam>(event.GetInt("team", -1));

	PlayerData pData = PlayerData(clientId);
	ArrayList slots = pData.GetFilledSlots();

	if (!(team == TFTeam_Blue || team == TFTeam_Red))
	{
		//they switched to observer or smth
		RemoveAnim(clientId);
		RemoveSkin(clientId);

		int s;
		for (int i = 0; i < slots.Length; i++)
		{
			s = slots.Get(i);

			RemoveEquippable(clientId, s);
		}

	}
	else
	{
		SetEntProp(pData.rSkinItem, Prop_Send, "m_iTeamNum", team);

		int s;
		for (int i = 0; i < slots.Length; i++)
		{
			s = slots.Get(i);
			SetEntProp(pData.GetEquippable(s), Prop_Send, "m_iTeamNum", team);
		}
	}
	
	delete slots;
}

int g_disguiseTargets[PLAYERSDATASIZE];

//switch wearable team when disguising & disguise removed
public void TF2_OnConditionAdded(int client, TFCond condition)
{
	if (condition == TFCond_Disguised)
	{
	 	if (!IsValidClient(client)) return;

		// TFTeam newTeam = view_as<TFTeam>(GetEntProp(client, Prop_Send, "m_nDisguiseTeam"));
		int disguiseTarget = GetEntProp(client, Prop_Send, "m_iDisguiseTargetIndex");

		//check every 100ms if the player has disguised as someone else
		g_disguiseTargets[client] = disguiseTarget;
		SDKHook(client, SDKHook_PostThinkPost, CheckDisguiseChanged);

		DisguiseEquips(client, disguiseTarget);
	}
}

public void TF2_OnConditionRemoved(int client, TFCond condition)
{
	if (condition == TFCond_Disguised)
	{
		SDKUnhook(client, SDKHook_PostThinkPost, CheckDisguiseChanged);
		g_disguiseTargets[client] = 0;
		UndisguiseEquips(client);
	}
}

void CheckDisguiseChanged(int client)
{
	if (GetEntProp(client, Prop_Send, "m_iDisguiseTargetIndex") != g_disguiseTargets[client])
	{
		//theyve changed who they're disguised as
		int dt = GetEntProp(client, Prop_Send, "m_iDisguiseTargetIndex");
		g_disguiseTargets[client] = dt;
		DisguiseEquips(client, dt);
	}
}

void DisguiseEquips(int disguiser, int disguiseTarget)
{
	if (!g_disguiseWearables)
		g_disguiseWearables = new KeyValues("DisguiseEquips");

	PlayerData dtData = PlayerData(disguiseTarget);

	char idStr[MAXINTLENGTH];
	IntToString(GetClientUserId(disguiser), idStr, sizeof(idStr));
	char curSection[MAXINTLENGTH];
	g_disguiseWearables.GetSectionName(curSection, sizeof(curSection));
	if (!StrEqual(curSection, idStr))
	{
		g_disguiseWearables.Rewind();
		g_disguiseWearables.JumpToKey(idStr, true);
	}

	//we're in the right place
	if (g_disguiseWearables.GotoFirstSubKey(false))
	{
		//we already have a disguise on
		UndisguiseEquips(disguiser);
		//undisguiseequips rewound us & deleted the key
		g_disguiseWearables.JumpToKey(idStr, true);
	}

	SetEquipsHidden(disguiser, true);

	//now actually add equips to the player
	char tmppath[PLATFORM_MAX_PATH];
	
	//set anim
	dtData.GetStr("animpath", tmppath, sizeof(tmppath));
	g_disguiseWearables.SetString("anim", tmppath);
	SetVariantString(tmppath);
	AcceptEntityInput(disguiser, "SetCustomModel");
	SetEntProp(disguiser, Prop_Send, "m_bUseClassAnimations", 1);

	//set skin
	dtData.GetStr("skinpath", tmppath, sizeof(tmppath));
	if (tmppath[0])
	{
		int rDisguiseSkin;
		CreateWearable(disguiser, tmppath, rDisguiseSkin);
		g_disguiseWearables.SetNum("skin", rDisguiseSkin);
		MakePlayerInvisible(disguiser);
	}

	//add wearables
	ArrayList equipSlots = dtData.GetFilledSlots();
	for (int i = 0; i < equipSlots.Length; i++)
	{
		int s = equipSlots.Get(i);
		int rDisguiseEquip;

		GetEntPropString(dtData.GetEquippable(s), Prop_Data, "m_ModelName", tmppath, sizeof(tmppath));
		CreateWearable(disguiser, tmppath, rDisguiseEquip);
		
		char slotStr[MAXINTLENGTH];
		IntToString(s, slotStr, sizeof(slotStr));
		g_disguiseWearables.SetNum(slotStr, rDisguiseEquip);
	}

	delete equipSlots;
	
}

void UndisguiseEquips(int client)
{
	if (!g_disguiseWearables)
		g_disguiseWearables = new KeyValues("DisguiseEquips");

	g_disguiseWearables.Rewind();

	//check if its used in disguises
	char idStr[MAXINTLENGTH];
	IntToString(GetClientUserId(client), idStr, sizeof(idStr));
	if (g_disguiseWearables && g_disguiseWearables.JumpToKey(idStr))
	{
		//this client has an entry in the disguises kv
		if (g_disguiseWearables.GotoFirstSubKey(false))
		{
			do
			{
				int rItem = g_disguiseWearables.GetNum(NULL_STRING)
				if (IsValidEntity(rItem)) TF2_RemoveWearable(client, EntRefToEntIndex(rItem));
				if (IsValidEntity(rItem)) RemoveEntity(rItem);
			} while (g_disguiseWearables.DeleteThis() == 1);
		}
		g_disguiseWearables.Rewind();
		g_disguiseWearables.SetString(idStr, "");
	}

	SetEquipsHidden(client, false);
}

void SetEquipsHidden(int client, int hidden)
{
	PlayerData pData = PlayerData(client);

	SetItemDraw(pData.rSkinItem, !hidden);
	
	//dont need to hide anim/real model, the disguise process already does that

	ArrayList equips = pData.GetFilledSlots();

	for (int i = 0; i < equips.Length; i++)
		SetItemDraw(pData.GetEquippable(equips.Get(i)), !hidden);

	delete equips;
}

static void SetItemDraw(int item, int draw)
{
	// int eff = GetEntProp(item, Prop_Send, "m_fEffects");
	// if (draw) eff &= ~EF_NODRAW;
	// else eff |= EF_NODRAW;
	// SetEntProp(item, Prop_Send, "m_fEffects", eff);

	if (draw)
		MakePlayerVisible(item);
	else
		MakePlayerInvisible(item);
}
