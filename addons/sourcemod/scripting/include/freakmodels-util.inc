/*
 HELPER FUNCTIONS
 ============================================
 */

bool GetTargetString(int client, int target, char[] buffer, int buffersize)
{
	if (client == target) strcopy(buffer, buffersize, "your");
	else
	{
		GetClientName(target, buffer, buffersize);
		StrCat(buffer, buffersize, "'s");
	}
	return client == target;
}

/**
 * Gets a value arg, performing user input checks along the way
 * @param args how many args are in the full command
 * @param i arg to get value of
 * @param path buffer to store value string into
 * @param pathsize size of path
 * @return one of GETPATHARG_ALREADYFILLED, GETPATHARG_NOVAL, or GOOD
 */
FuncOutput GetPathArg(int args, int i, char[] path, int pathsize)
{
	if (path[0]) return GETPATHARG_ALREADYFILLED;
	if (i > args || !checkArgIsVal(i)) return GETPATHARG_NOVAL;

	GetCmdArg(i, path, pathsize);
	return GOOD;
}

bool checkArgIsVal(int i)
{
	char arg[128];

	GetCmdArg(i, arg, sizeof(arg));
	return (arg[0] != '-');
}

void ToLowerCase(char[] str, int strsize)
{
	for (int i = 0; i < strsize; i++)
	{
		if (str[i])
			str[i] = CharToLower(str[i]);
	}
}

/**
 * Returns if a client ID is valid.
 * Note a client index of 0 - the server - is not considered valid.
 * @return true if client is a valid client index, false if not.
 */
public bool IsValidClient(int client) 
{ 
	return client > 0 && client <= MaxClients && IsClientInGame(client); 
}

/**
 * Checks if a client index is valid, and if not, throws an error.
 * Note a client index of 0 - the server - is not considered valid and will throw an error.
 * @error if client is not a valid client index.
 */
public void CheckClientValid(int client) 
{ 
	if (!IsValidClient(client)) ThrowError("Client index %i is not a valid client.", client); 
}


/**
 * gets client id of specified username.
 * @param client Client ID of caller.
 * @param user String to search against.
 * @param targetsFound OUTPUT: number of targets found.
 * @param targetList OUTPUT: list to hold the targets.
 * @return if a target was found.
 */
bool GetClientsFromUsername(int client, char[] user, int& targetsFound, int[] targetList, const int targetListSize)
{
	//find a player to match the entered user
	bool tn_is_ml;
	char foundName[65];

	//trim quotes in case they added them
	StripQuotes(user);

	int deadTargetsFound;
	int[] deadTargetList = new int[targetListSize];
	
	targetsFound = ProcessTargetString(user, client, targetList, targetListSize, COMMAND_FILTER_ALIVE, foundName, sizeof(foundName), tn_is_ml);

	deadTargetsFound = ProcessTargetString(user, client, deadTargetList, targetListSize, COMMAND_FILTER_DEAD, foundName, sizeof(foundName), tn_is_ml);

	if (deadTargetsFound > 0)
	{
		if (targetsFound < 1)
		{
			for (int i = 0; i < deadTargetsFound; i++)
			{
				targetList[i] = deadTargetList[i];
			}
			targetsFound = deadTargetsFound;
		} 
		else
		{
			//add all dead targets to alive targets
			for (int i = 0; i < deadTargetsFound; i++)
			{
				targetList[targetsFound + i] = deadTargetList[i];
			}
			targetsFound += deadTargetsFound;
			
		}
	}

	if (targetsFound < 1)
	{
		//couldn't find one
		ReplyToTargetError(client, targetsFound);
		return false;
	}
	else
	{
		bool atLeastOneValid = false;
		//could find one
		for (int i = 0; i < targetsFound; i++)
		{
			if (!IsValidClient(targetList[i]))
			{
				targetList[i] = 0;
			}
			else atLeastOneValid = true;
			
		}
		if (!atLeastOneValid)
		{
			//shouldn't happen - processtargetstring should have checked they're all valid
			ReplyToCommand(client, "Sorry, something went wrong. Try again? (Freakmodels error code: 10)");
			return false;
		}
		return true;
	}
}