
/*
 * Everything for keeping the skin on the ragdoll after death.
 * Technically works but is underwhelming.
 */

public void OnEntityCreated(int entity, const char[] classname)
{
	if (StrEqual(classname, "tf_ragdoll"))
		RequestFrame(RagdollMade, entity);
}

void RagdollMade(int ragdollEnt)
{
// 	int owner = GetEntProp(ragdollEnt, Prop_Send, "m_iPlayerIndex");

// 	PlayerData pData = PlayerData(owner);

// 	//go thru all the player's datas & move their attachments to the ragdoll
// 	//then reset them on spawn

// 	if (pData.rSkinItem)
// 	{
// 		SetVariantString("!activator");
// 		AcceptEntityInput(pData.rSkinItem, "SetParent", EntIndexToEntRef(ragdollEnt));
// 		SetEntProp(pData.rSkinItem, Prop_Send, "m_bValidatedAttachedEntity", 1);

// 		SetEntPropEnt(pData.rSkinItem, Prop_Send, "m_hOwnerEntity", ragdollEnt);

// 		// SetEntityRenderMode(pData.rSkinItem, RENDER_NORMAL);
// 		MakePlayerVisible(pData.rSkinItem);
// 		MakePlayerVisible(owner);
// 	}

	//todo: equips
	
}

// public Action ResetPlayerModelAfterDeath(Handle timer, int owner)
// {
// 	owner = GetClientOfUserId(owner);
// 	SetVariantString(g_playersData[owner].animPath);
// 	AcceptEntityInput(owner, "SetCustomModel");
// }
