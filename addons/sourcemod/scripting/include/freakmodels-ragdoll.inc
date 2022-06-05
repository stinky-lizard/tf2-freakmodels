/*
 * Everything for keeping the skin on the ragdoll after death.
 */

#define freakmodels_ragdoll_included

KeyValues g_trackedRagdolls = null;
ArrayList g_allRagdollWearables = null;

public void OnEntityCreated(int entity, const char[] classname)
{
	if (StrEqual(classname, "tf_ragdoll"))
	{
		RequestFrame(RagdollMade, entity);
	}
}

void RagdollMade(int ragdollEnt)
{
	// SetEntPropFloat(ragdollEnt, Prop_Send, "m_flHeadScale", 20.0);
}/*	
	ragdollEnt = EntIndexToEntRef(ragdollEnt);

	if (!g_trackedRagdolls)
		g_trackedRagdolls = new KeyValues("Ragdolls");

	if (!g_allRagdollWearables)
		g_allRagdollWearables = new ArrayList();

	if (GetEntProp(ragdollEnt, Prop_Send, "m_bGib"))
		//causes errors, and the skin doesnt even show anyway
		return;

	int owner = GetEntProp(ragdollEnt, Prop_Send, "m_iPlayerIndex");
	PlayerData pData = PlayerData(owner);

	//keep track of it
	char ragdollEntStr[MAXINTLENGTH];
	IntToString(ragdollEnt, ragdollEntStr, sizeof(ragdollEntStr));

	if (pData.rSkinItem)
	{

		char model[PLATFORM_MAX_PATH];
		pData.GetStr("skinpath", model, sizeof(model));
		int rSkin = CreateRagdollWearable(ragdollEnt, model);

		g_trackedRagdolls.JumpToKey("wearables", true);

		int wearablesPoint = g_trackedRagdolls.GetNum(ragdollEntStr);
		ArrayList wearables;

		if (wearablesPoint)
		{
			wearables = view_as<ArrayList>(wearablesPoint);
		}
		else
		{
			wearables = new ArrayList();
			g_trackedRagdolls.SetNum(ragdollEntStr, view_as<int>(wearables));
		}
		wearables.Push(rSkin);
		
		g_trackedRagdolls.Rewind();
	}


	//todo: equips


	
	
	g_trackedRagdolls.JumpToKey("timers", true);
	Handle fadeTimer = CreateTimer(10.0, Timer_FadeRagdollWearablesOut, ragdollEnt);
	g_trackedRagdolls.SetNum(ragdollEntStr, view_as<int>(fadeTimer));
	g_trackedRagdolls.Rewind();

}

int CreateRagdollWearable(int rRagdollEnt, char[] model)
{
	int rSkin = EntIndexToEntRef(CreateEntityByName("prop_physics_multiplayer"));	

	//using our own cleanup function
	// allWearables.Push(rSkin);
	g_allRagdollWearables.Push(rSkin);

	TFTeam team = TF2_GetClientTeam(GetEntProp(rRagdollEnt, Prop_Send, "m_iPlayerIndex"));
	if (team == TFTeam_Blue)
	{
		SetEntProp(rSkin, Prop_Send, "m_nSkin", 1);

	}
	
	//make skin conform to parent
	int effects = GetEntProp(rSkin, Prop_Send, "m_fEffects");
	effects |= EF_BONEMERGE|EF_BONEMERGE_FASTCULL;
	effects &= ~(EF_NOSHADOW|EF_NORECEIVESHADOW);
	SetEntProp(rSkin, Prop_Send, "m_fEffects", effects);

	//set parent of skin
	SetVariantString("!activator");
	AcceptEntityInput(rSkin, "SetParent", rRagdollEnt);
	
	SetEntPropEnt(rSkin, Prop_Send, "m_hOwnerEntity", rRagdollEnt);

	if (!CheckModelGood(model))
	{
		LogMessage("Warning: A non-precached skin model was about to be used. This would've caused a crash. (Error 32)");
		SetEntityModel(rSkin, "models/error.mdl");
	}
	else SetEntityModel(rSkin, model);

	return rSkin;
}

//fade wearables out
Action Timer_FadeRagdollWearablesOut(Handle timer, int ragdoll)
{
	char entStr[MAXINTLENGTH];
	IntToString(ragdoll, entStr, sizeof(entStr));


	if (g_trackedRagdolls && g_trackedRagdolls.JumpToKey("wearables"))
	{
		//it was a ragdoll we're tracking
		ArrayList ragdollWearables = view_as<ArrayList>(g_trackedRagdolls.GetNum(entStr));
		if (ragdollWearables)
		{
			//delete all the wearables and clear the arraylist
			for (int i = 0; i < ragdollWearables.Length; i++)
			{
				int wearableItem = ragdollWearables.Get(i);
				SetEntProp(wearableItem, Prop_Send, "m_nRenderFX", 5);
			}
		}
		g_trackedRagdolls.Rewind();
	}

	if (g_trackedRagdolls && g_trackedRagdolls.JumpToKey("timers"))
	{
		g_trackedRagdolls.SetString(entStr, "");
	}
}