
/*
 * Stuff for the regular cleanup, by default every 3 minutes.
 */

public Action Timer_RegularCleanup(Handle timer)
{
	CleanupWearables();
}

KeyValues g_cleanedEnts = null;

void CleanupWearables(bool onlyLost = true)
{
	if (!g_cleanedEnts)
		g_cleanedEnts = new KeyValues("CleanedEntities");


	for (int i = 0; i < allWearables.Length; i++)
	{
		int rItem = allWearables.Get(i);

		char rItemStr[32];
		IntToString(rItem, rItemStr, sizeof(rItemStr));

		if ((onlyLost && ItemUsedBy(rItem) == -1) || !onlyLost)
		{
			//should be removed
			if (!IsValidEntity(rItem) || g_cleanedEnts.GetNum(rItemStr))
			{
				//is deleted
				allWearables.Erase(i);
				//we've shifted the array down, so we need to shift i down as well
				i--;

				g_cleanedEnts.DeleteKey(rItemStr);
			}
			else
			{

				if (ItemUsedBy(rItem) != -1)
				{
					//will only happen if onlyLost is false
					TF2_RemoveWearable(ItemUsedBy(rItem), EntRefToEntIndex(rItem));
					g_cleanedEnts.SetNum(rItemStr, 1);
				}

				//is not already deleted
				if (IsValidEntity(rItem)) 
				{
					RemoveEntity(rItem);	
					g_cleanedEnts.SetNum(rItemStr, 1);
				}
			}
		}
	}
}

/**
 * Returns the player using an item, meaning if it's referenced in g_playersData
 * @param itemReference Reference ID of the item to check.
 * @return Client index of the client using the item, or -1 if it's not used.
 */
int ItemUsedBy(const int itemReference)
{
	int usedBy = -1;

	bool done = false;

	for(int i = 1; i < PLAYERSDATASIZE; i++)
	{
		PlayerData pData = PlayerData(i);
		if (itemReference == pData.rSkinItem) 
		{
			usedBy = i;
			break;
		}
		
		ArrayList slots = pData.GetFilledSlots();
		
		for (int j = 0; j < slots.Length; j++)
		{
			if (itemReference == pData.GetEquippable(slots.Get(j)))
			{
				usedBy = i;
				done = true;
				break;
			}
		}

		delete slots;

		if (done) break;
	}

	return usedBy;
}
