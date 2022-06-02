
#include <freakmodels-defs>

#include <freakmodels-config>
#include <freakmodels-util>

// #include <freakmodels-manage>
// #include <freakmodels-cleanup>

// #include <freakmodels-ragdoll> //disabled for now, want to redo it


/*
 * CORE FUNCTIONS
 * This file includes the main functions - 
 */

/**
 * Creates & gives a wearable to a player.
 * @param target client id of target.
 * @param model full path to model.
 * @param rItem OUTPUT: __REFERENCE__ ID of created entity.
 * @return one of FuncOutput.
 */
FuncOutput CreateWearable(int target, char[] model, int& rItem)
{
	//are they in game?
	TFTeam team = TF2_GetClientTeam(target);
	if (team != TFTeam_Blue && team != TFTeam_Red) return TARGETNOTEAM; //not in game

	if (!g_hDummyItemView) ThrowError("Error: g_hDummyItemView was not initialized properly.");

	//create item that will get skin
	int iSkinItem = TF2Items_GiveNamedItem(target, g_hDummyItemView);
	
	int rSkinItem = EntIndexToEntRef(iSkinItem);

	allWearables.Push(rSkinItem);

	float pos[3];
	GetClientAbsOrigin(target, pos);
	
	DispatchKeyValueVector(rSkinItem, "origin", pos);
	DispatchKeyValue(rSkinItem, "model", model);

	SetEntPropString(rSkinItem, Prop_Data, "m_iClassname", "freakmodel_wearable");
	//set team
	SetEntProp(iSkinItem, Prop_Send, "m_iTeamNum", team);
	
	SetEntPropFloat(rSkinItem, Prop_Send, "m_flPlaybackRate", 1.0);
	
	//give effects - bonemerge, animations, shadow
	int effects = GetEntProp(rSkinItem, Prop_Send, "m_fEffects");
	effects |= EF_BONEMERGE|EF_BONEMERGE_FASTCULL;
	effects |= EF_PARENT_ANIMATES;	
	effects &= ~(EF_NOSHADOW|EF_NORECEIVESHADOW);
	SetEntProp(rSkinItem, Prop_Send, "m_fEffects", effects);
	
	//make it conform to the player's animations
	SetVariantString("!activator");
	AcceptEntityInput(rSkinItem, "SetParent", EntIndexToEntRef(target));
	SetEntProp(rSkinItem, Prop_Send, "m_bValidatedAttachedEntity", 1);
	
	SetEntPropEnt(rSkinItem, Prop_Send, "m_hOwnerEntity", target);
	
	if (!CheckModelGood(model))
	{
		LogMessage("Warning: A non-precached skin model was about to be used. This would've caused a crash. (Error 32)");
		SetEntityModel(rSkinItem, "models/error.mdl");
	}
	else SetEntityModel(rSkinItem, model);

	rItem = rSkinItem;
	return GOOD;
}

/**
 * Makes a player invisible.
 * Exactly the reverse of MakePlayerVisible.
 * @param target Client ID of target.
 * @error if target is not a valid client index.
 */
void MakePlayerInvisible(int target)
{
	SetEntityRenderMode(target, RENDER_NONE);
	int tarEffects = GetEntProp(target, Prop_Send, "m_fEffects");
	tarEffects |= (EF_NOSHADOW|EF_NORECEIVESHADOW);
	SetEntProp(target, Prop_Send, "m_fEffects", tarEffects);
}

/**
 * Makes a player visible.
 * Exactly the reverse of MakePlayerInvisible.
 * @param target Client ID of target.
 * @error if target is not a valid client index.
 */
void MakePlayerVisible(int target)
{
	SetEntityRenderMode(target, RENDER_NORMAL);
	int tarEffects = GetEntProp(target, Prop_Send, "m_fEffects");
	tarEffects &= ~(EF_NOSHADOW|EF_NORECEIVESHADOW); //enable shadow
	SetEntProp(target, Prop_Send, "m_fEffects", tarEffects);
}

/**
 * set the player's skin.
 * creates an entity that acts as the skin and attaches it to the player,
 * also makes the player invisible
 * @return one of SetSkinOutput
 */
FuncOutput SetSkin(int target, char[] skinModel)
{
	//remove skin if it exists
	RemoveSkin(target);

	int rSkinItem;
	FuncOutput createWearOutput = CreateWearable(target, skinModel, rSkinItem);
	if (createWearOutput == TARGETNOTEAM) return TARGETNOTEAM;
	
	PlayerData pData = PlayerData(target);
	pData.rSkinItem = rSkinItem;
	pData.SetStr("SkinPath", skinModel);
	
	//make player (i.e. anim model) invisible
	MakePlayerInvisible(target);

	//make skin item visible
	SetEntityRenderMode(rSkinItem, RENDER_NORMAL);
	//Q: arthurdead's plugins use this, reason?
	// SetEntityRenderMode(rSkinItem, RENDER_TRANSCOLOR);
	// SetEntityRenderColor(rSkinItem, 255, 255, 255, 255);
	
	//equip skin
	SDKCall(g_hEquipWearable, target, rSkinItem);

	return GOOD;
}

/**
 * Removes a skin from a player, deleting the skin entity and making the player animation model visible.
 * @param target Client ID of the target player.
 * @return true if a skin was removed, false if there was no skin to begin with.
 */
bool RemoveSkin(int target)
{
	bool skinRemoved = false;

	//make player anim model visible
	MakePlayerVisible(target);

	PlayerData pData = PlayerData(target);

	//if there is a valid skin ent
	if (pData.rSkinItem && IsValidEntity(pData.rSkinItem))
	{
		//delete skin
		int index = EntRefToEntIndex(pData.rSkinItem);
		
		AcceptEntityInput(index, "ClearParent");

		TF2_RemoveWearable(target, index);
		//docs say TF2_RemoveWearable deletes it. hopefully this fixes the bug
		if (IsValidEntity(pData.rSkinItem)) RemoveEntity(pData.rSkinItem);


		skinRemoved = true;
	}

	pData.rSkinItem = 0;
	pData.SetStr("SkinPath", "");

	return skinRemoved;
}

//set the client model, which dictates their animations (and appearance with no skin)
//@error if client is not a valid client id.
void SetAnim(int client, char[] model)
{
	if (!CheckModelGood(model))
	{
		LogMessage("Warning: A non-precached animation model was about to be used. This would've caused a crash. (Error 31)");
		return;
	}
	SetVariantString(model);
	AcceptEntityInput(client, "SetCustomModel");
	SetEntProp(client, Prop_Send, "m_bUseClassAnimations", 1);

	PlayerData(client).SetStr("AnimPath", model);
}

//reset the client model, which dictates their animations (and appearance with no skin)
void RemoveAnim(int target)
{
	SetVariantString("");
	AcceptEntityInput(target, "SetCustomModel");

	PlayerData(target).SetStr("AnimPath", "");
}