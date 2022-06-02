
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