
Action EquipCommand(int client, int args)
{
	if (args < 1)
	{
		Equip_PrintHelp(client);
		return Plugin_Handled;
	}

	int targetList[MAXPLAYERS];
	int targetsFound = 1;
	int slot;

	char operation[32];
	GetCmdArg(1, operation, sizeof(operation));

	operation[0] = CharToLower(operation[0]);

	if (operation[0] == 'h')
		Equip_PrintHelp(client);
	else if (operation[0] == 'i')
	{
		//get target if exists
		if (args < 2 && IsValidClient(client))
			targetList[0] = client;
		else if (args < 2)
		{
			ReplyToCommand(client, "Please enter a player to view the FreakModels equip info of.");
			return Plugin_Handled;
		}
		else
		{
			char targetInput[MAX_NAME_LENGTH];
			GetCmdArg(2, targetInput, sizeof(targetInput));
			bool foundOne = GetClientsFromUsername(client, targetInput, targetsFound, targetList, sizeof(targetList));
			if (!foundOne) return Plugin_Handled; //client was already alerted their input was invalid by GetClientsFromUsername
		}

		for (int p = 0; p < targetsFound; p++)
		{
			int size = GetEquippablesInfoSize(client, targetList[p]);
			char[] playerInfo = new char[size];

			GetEquippablesInfo(client, targetList[p], playerInfo, size);

			ReplyToCommand(client, "%s", playerInfo);
		}
	}
	else if (operation[0] == 'r')
	{
		//want to reset
	
		bool doAll = false;

		if (args < 2)
		{
			//theres no slot arg
			ReplyToCommand(client, "Please enter a slot to remove the FreakModels equip from, or 'all' to remove all of them.");
			return Plugin_Handled;
		}

		char slotArg[] = "000";
		GetCmdArg(2, slotArg, sizeof(slotArg));
		if (slotArg[0] == 'a')
		{
			//lets assume they meant all
			doAll = true;
		}
		else
		{
			slot = StringToInt(slotArg);
			if (slot < 1)
			{
				ReplyToCommand(client, "Sorry, FreakModels equip slots start at 1; you entered %i.", slot);
				return Plugin_Handled;
			}
			else if (slot > 99)
			{
				ReplyToCommand(client, "Sorry, you can only have up to 99 FreakModels equip slots. You entered %i.", slot);
				return Plugin_Handled;
			}
		}

		//get target if exists
		if (args < 3 && IsValidClient(client))
			targetList[0] = client;
		else if (args < 3)
		{
			ReplyToCommand(client, "Please enter a player to target as a third argument.");
			return Plugin_Handled;
		}
		else
		{
			char targetInput[MAX_NAME_LENGTH];
			GetCmdArg(3, targetInput, sizeof(targetInput));
			bool foundOne = GetClientsFromUsername(client, targetInput, targetsFound, targetList, sizeof(targetList));
			if (!foundOne) return Plugin_Handled; //client was already alerted their input was invalid by GetClientsFromUsername
		}

		char playerName[MAX_NAME_LENGTH];

		//reset it
		if (doAll)
		{
			for (int i = 0; i < targetsFound; i++)
			{
				if (targetList[i] == client) ReplyToCommand(client, "Resetting all FreakModels equips on you.");
				else 
				{
					GetClientName(targetList[i], playerName, sizeof(playerName));
					ReplyToCommand(client, "Resetting all FreakModels equips on %s.", playerName);
				}

				PlayerData pData = PlayerData(targetList[i]);
				ArrayList slots = pData.GetFilledSlots();
				for (int j = 0; j < slots.Length; j++)
				{
					if (IsValidEntity(pData.GetEquippable(slots.Get(j))))
						RemoveEquippable(targetList[i], slots.Get(j));
				}
				pData.InitEquipList();

				delete slots;
			}
		}
		else
		{
			char modelPath[PLATFORM_MAX_PATH];

			for (int i = 0; i < targetsFound; i++)
			{
				int equip = PlayerData(targetList[i]).GetEquippable(slot);
				if (!equip)
				{
					if (targetList[i] == client) ReplyToCommand(client, "You have no FreakModels equip in slot %i.", slot);
					else
					{
						GetClientName(targetList[i], playerName, sizeof(playerName));
						ReplyToCommand(client, "%s has no FreakModels equip in slot %i.", playerName, slot);
					} 
					continue;
				}

				RemoveEquippable(targetList[i], slot);

				GetEntPropString(equip, Prop_Data, "m_ModelName", modelPath, sizeof(modelPath));
				if (targetList[i] == client) ReplyToCommand(client, "Removing your FreakModels equip in slot %i. (%s)", slot, modelPath);
				else 
				{
					GetClientName(targetList[i], playerName, sizeof(playerName));
					ReplyToCommand(client, "Removing %s's FreakModels equip in slot %i. (%s)", playerName, slot, modelPath);
				}

			}
		}
	}
	else if (operation[0] == 'e' || operation[0] == 'a' || operation[0] == 's')
	{
		if (args < 2)
		{
			//theres no model arg
			ReplyToCommand(client, "Please enter a model to add and a slot to add the model to (in that order).");
			return Plugin_Handled;
		}
		else if (args < 3)
		{
			//theres no slot arg
			ReplyToCommand(client, "Please enter a number as a slot to add the model to.");
			return Plugin_Handled;
		}

		//get model
		char modelName[PLATFORM_MAX_PATH];
		GetCmdArg(2, modelName, sizeof(modelName));

		char modelPath[PLATFORM_MAX_PATH];
		strcopy(modelPath, sizeof(modelPath), modelName);
		
		//get & check slot
		char slotArg[] = "000";
		GetCmdArg(3, slotArg, sizeof(slotArg));
		slot = StringToInt(slotArg);

		if (slot < 1)
		{
			ReplyToCommand(client, "Sorry, FreakModels equip slots start at 1; you entered %i.", slot);
			return Plugin_Handled;
		}
		else if (slot > 99)
		{
			ReplyToCommand(client, "Sorry, you can only have up to 99 FreakModels equip slots. You entered %i.", slot);
			return Plugin_Handled;
		}

		//get target if exists
		if (args < 4 && IsValidClient(client))
			targetList[0] = client;
		else if (args < 4)
		{
			ReplyToCommand(client, "Please enter a player to target as a third argument.");
			return Plugin_Handled;
		}
		else
		{
			char targetInput[MAX_NAME_LENGTH];
			GetCmdArg(4, targetInput, sizeof(targetInput));
			bool foundOne = GetClientsFromUsername(client, targetInput, targetsFound, targetList, sizeof(targetList));
			if (!foundOne) return Plugin_Handled;
		}

		bool useFullpaths = false;

		//get if full paths are used & translate them if so
		if (args >= 5)
		{
			char optionArg[64];

			GetCmdArg(5, optionArg, sizeof(optionArg));

			if
			(
				StrEqual(optionArg, "-f", false) || StrEqual(optionArg, "-full", false) 
				|| StrEqual(optionArg, "-fullpath", false) || StrEqual(optionArg, "-fullpaths", false)
				|| StrEqual(optionArg, "-path", false) || StrEqual(optionArg, "-paths", false)
			)
			{
				//if there's a -f option
				//modelName and modelPath will be the name
				useFullpaths = true;
			}
		}

		if (!useFullpaths)
		{
			ToLowerCase(modelName, sizeof(modelName)); //make model names case-agnostic
			if (!GetModelFromConfig(modelName, modelPath, sizeof(modelPath)))
			{
				ReplyToCommand(client, "Sorry, we couldn't find your model (%s) in our list. Check your spelling?", modelName);
				return Plugin_Handled;
			}	
		}
		else
		{
			//they didn't enter a name
			modelName = "";
		}

		//replace \s with /s
		ReplaceString(modelPath, sizeof(modelPath), "\\", "/");

		//check if the models are good
		if (!IsModelPrecached(modelPath))
		{
			if (useFullpaths)
			{
				if (FileExists(modelPath, true))
					ReplyToCommand(client, "Sorry, your model is not precached and we cannot use it.");
				else 
					ReplyToCommand(client, "Unknown model!");
			} 
			else 
				//the file is set in the config but it wasn't precached... despite it was verified to exist before
				ReplyToCommand(client, "Sorry, an error occurred. Please report this to the server operator. (FreakModels error code: 50)");
			return Plugin_Handled;
		}

		//add the equippable

		char playerName[MAX_NAME_LENGTH];

		for (int i = 0; i < targetsFound; i++)
		{
			FuncOutput out = AddEquippable(targetList[i], modelPath, slot, modelName);

			GetClientName(targetList[i], playerName, sizeof(playerName));
			if (targetList[i] == client)
			{
				if (modelName[0]) ReplyToCommand(client, "You are now wearing \"%s\" in slot %i.", modelName, slot);
				else ReplyToCommand(client, "You are now wearing %s in slot %i.", modelPath, slot);
				if (out == REMOVEDEQUIP) ReplyToCommand(client, "Let it be known: you already had something equipped in slot %i - this has been overwritten.", slot);
			}
			else
			{
				if (modelName[0]) ReplyToCommand(client, "%s is now wearing \"%s\" in slot %i.", playerName, modelName, slot);
				else ReplyToCommand(client, "%s is now wearing %s in slot %i.", playerName, modelPath, slot);
				if (out == REMOVEDEQUIP) ReplyToCommand(client, "Let it be known: %s already had something equipped in slot %i - this has been overwritten.", playerName, slot);
			}
		}
	}

	return Plugin_Handled;
}

void Equip_PrintHelp(int client)
{
	ReplyToCommand(client, "Printing fm_equip help in your console.");

	PrintToConsole
	(
		client,
		"\nUse fm_equip to add a model to a player as a wearable.\nThe player will not actually receive the item in their inventory, nor will it change their stats in any way.\nOptions:\n- Enter `fm_equip add` to add a model to a player.\n- Enter `fm_equip remove` to remove a model from a player.\n- Enter `fm_equip info` to view a player's current models.\n\n\nAdd mode usage: `fm_equip add <model> <slot> [target] [-fullpath]\n- In place of <model>, add the name of the model as defined by the server operator.\n- In place of <slot>, add the slot (between 1 and 99) to add the wearable to on the player.\n- Optional: In place of [target], enter a player username (or a multi-target, e.g. @all) to target another player.\n- Optional: Add -fullpath or -f at the end to use the path to a model instead of a model's name. This can use any loaded model.\n\n\nRemove mode usage: `fm_equip remove <slot> [target]\n- In place of <slot>, add the slot containing the wearable you want to remove.\n- Optional: In place of [target], enter a player username (or a multi-target, e.g. @all) to target another player.\n\n\nInfo mode usage: `fm_equip info <target>\n- In place of <target>, enter a player username (or a multi-target, e.g. @all) to view that player's info."
	)

}

FuncOutput AddEquippable(int target, char[] modelPath, int slot, char[] modelName = "")
{
	bool removedOne = false;

	PlayerData pData = PlayerData(target);

	//if there's an existing item, remove it
	if (pData.GetEquippable(slot))
	{
		RemoveEquippable(target, slot);
		removedOne = true;
	}

	int rItem = 0;
	
	CreateWearable(target, modelPath, rItem);
	
	pData.SetEquippable(rItem, slot, modelName);

	return removedOne ? REMOVEDEQUIP : GOOD;
}

void RemoveEquippable(int target, int slot)
{
	PlayerData pData = PlayerData(target);
	int rItem = pData.GetEquippable(slot);
	if (!rItem) return; //its already 0

	if (IsValidEntity(rItem)) TF2_RemoveWearable(target, EntRefToEntIndex(rItem));

	if (IsValidEntity(rItem)) RemoveEntity(rItem);

	pData.SetEquippable(0, slot);
}

/**
 * Gets the amount of bytes written to the string buffer with GetEquippablesInfo.
 * Obviously this will change when adding/removing/changing equippables.
 *
 * @return number of bytes written by GetEquippablesInfo.
 */
int GetEquippablesInfoSize(int client, int target)
{
	int size = 2; //terminator + safety

	char targetName[MAX_NAME_LENGTH];
	GetClientName(target, targetName, sizeof(targetName));

	char modelName[PLATFORM_MAX_PATH];
	char modelPath[PLATFORM_MAX_PATH];

	PlayerData pData = PlayerData(target);

	ArrayList slots = pData.GetFilledSlots();

	//first check if they even have anything
	if (slots.Length < 1)
	{
		if (client == target)
			size += strlen("You have no equipped models.");
		else
		{
			size += strlen(" has no equipped models.");
			size += strlen(targetName);
		}
	}
	else
	{
		if (client == target)
		{
			size += strlen("You have these models equipped:");	//"You have:"
		}
		else
		{
			size += strlen(targetName);
			size += strlen(" has these models equipped:");	//" has:"
		}
		
		size += 2;	//\n
		
		int s;

		for (int i = 0; i < slots.Length; i++)
		{
			s = slots.Get(i);
			//- Slot n: <modelName>|<modelPath>\n
			size += 7;	//"- Slot "
			size += RoundToFloor(Logarithm(float(s))) + 1; //length of slot num - doesn't work for numbers below 1 but slots start at 1 so is ok
			size += 2;	//": "
			
			pData.GetEquippableName(s, modelName, sizeof(modelName));

			if (modelName[0])
			{
				//the model has a name
				size += 4; //\"\"
				size += strlen(modelName);
			}
			else
			{
				GetEntPropString(pData.GetEquippable(s), Prop_Data, "m_ModelName", modelPath, sizeof(modelPath));
				size += strlen(modelPath);
			}

			size += 2; //\n
		}
	}

	delete slots;
	return size;
}

/**
 * Gives a string of information about a client's equippables, spanning multiple lines.
 * 
 * @param client         Client ID of caller.
 * @param target         Client ID of target.
 * @param buffer         Buffer to store the string in.
 * @param bufferSize     Size of buffer. See GetEquippablesInfoSize.
 */
void GetEquippablesInfo(int client, int target, char[] buffer, int bufferSize)
{
	PlayerData pData = PlayerData(target);
	ArrayList slots = pData.GetFilledSlots();

	char modelName[PLATFORM_MAX_PATH];
	char modelPath[PLATFORM_MAX_PATH];

	char targetName[MAX_NAME_LENGTH];
	GetClientName(target, targetName, sizeof(targetName));

	strcopy(buffer, bufferSize, "");

	if (slots.Length < 1)
	{
		if (client == target)
			StrCat(buffer, bufferSize, "You have no equipped models.");
		else
		{
			StrCat(buffer, bufferSize, targetName);
			StrCat(buffer, bufferSize, " has no equipped models.");
		}
	}
	else
	{
		if (client == target)
			StrCat(buffer, bufferSize, "You have these models equipped:");
		else
		{
			StrCat(buffer, bufferSize, targetName);
			StrCat(buffer, bufferSize, " has these models equipped:");
		}

		StrCat(buffer, bufferSize, "\n");

		int s;
		char sStr[MAXINTLENGTH];

		for (int i = 0; i < slots.Length; i++)
		{
			s = slots.Get(i);
			//- Slot n: <modelName>|<modelPath>\n
			StrCat(buffer, bufferSize, "- Slot ");

			IntToString(s, sStr, sizeof(sStr));
			StrCat(buffer, bufferSize, sStr);

			StrCat(buffer, bufferSize, ": ");
			
			pData.GetEquippableName(s, modelName, sizeof(modelName));

			if (modelName[0])
			{
				//the model has a name
				StrCat(buffer, bufferSize, "\"");
				StrCat(buffer, bufferSize, modelName);
				StrCat(buffer, bufferSize, "\"");
			}
			else
			{
				GetEntPropString(pData.GetEquippable(s), Prop_Data, "m_ModelName", modelPath, sizeof(modelPath));
				StrCat(buffer, bufferSize, modelPath);
			}

			StrCat(buffer, bufferSize, "\n");
		}
	}

	delete slots;

}