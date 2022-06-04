
#define EF_BONEMERGE 0x001
#define EF_BONEMERGE_FASTCULL 0x080
#define EF_PARENT_ANIMATES 0x200
#define EF_NODRAW 0x020
#define EF_NOSHADOW 0x010
#define EF_NORECEIVESHADOW 0x040

//this is just for clarity of what numbers mean what codes
enum FuncOutput
{
	GOOD,
	TARGETNOTEAM,
	TARGETINVALID,
	GETPATHARG_ALREADYFILLED,
	GETPATHARG_NOVAL,
	REMOVEDEQUIP
}

enum struct ModelChangeData
{
	bool isReset;
	bool useFullpaths;

	int targets[MAXPLAYERS + 1]; //+1 for safety
	int numTargets;

	char animPath[PLATFORM_MAX_PATH];
	char animName[PLATFORM_MAX_PATH];

	char skinPath[PLATFORM_MAX_PATH];
	char skinName[PLATFORM_MAX_PATH];

}

//using an arrayList instead?
// #define NUMALLOWEDWEARABLES 32

enum struct PlayerDataStruct
{
	int userId;

	int rSkinItem;
	
	char skinPath[PLATFORM_MAX_PATH];
	char animPath[PLATFORM_MAX_PATH];

	char skinName[PLATFORM_MAX_PATH];
	char animName[PLATFORM_MAX_PATH];

	KeyValues equippedItems;
	KeyValues equippedItemNames;
}

//used globally - in many modules

ModelChangeData g_clientsCommandData[MAXPLAYERS + 1];

#define PLAYERSDATASIZE (MAXPLAYERS + 1)
#define MAXINTLENGTH 22 //64 bit ints are up to 19 characters long, + 1 for sign, + 1 for terminator, + 1 for safety

//hidden - only accessible with the PlayerData methodmap
static PlayerDataStruct g_playersData[PLAYERSDATASIZE];

Handle g_hDummyItemView = null;
Handle g_hEquipWearable = null;

methodmap PlayerData
{
	public PlayerData(int clientId)
	{
		if (IsValidClient(clientId))
		{
			int uid = GetClientUserId(clientId)
			if (g_playersData[clientId].userId && g_playersData[clientId].userId != uid)
			{
				//there is a user id, but its a different id than what we had before!
				//the actual player has changed, but ClearData was never called!
				//could throw an error, but in the event this does happen there's a cleaner solution

				//had to move this down bc methodmaps arent FREAKING hoisted >:(
				UserIdIsDifferent(clientId);

				//the data is now cleared
			}
			g_playersData[clientId].userId = uid;
		} 
		return view_as<PlayerData>(clientId);
	}
	
	property int clientId
	{
		public get() { return view_as<int>(this); }
	}
	
	property int userId
	{
		public get() { return g_playersData[this.clientId].userId; }
	}

	property int rSkinItem
	{
		public get() { return g_playersData[this.clientId].rSkinItem; }
		public set(int value) { g_playersData[this.clientId].rSkinItem = value; }
	}

	public void InitEquipList()
	{
		if (g_playersData[this.clientId].equippedItems) delete g_playersData[this.clientId].equippedItems;
		g_playersData[this.clientId].equippedItems = new KeyValues("Equips");

		if (g_playersData[this.clientId].equippedItemNames) delete g_playersData[this.clientId].equippedItemNames;
		g_playersData[this.clientId].equippedItemNames = new KeyValues("Equips");
	}

	public void ClearData()
	{
		//cant use SetSkinPath/SetAnimPath bc methodmaps and their members aren't hoisted :(
		//reset skin path
		strcopy(g_playersData[this.clientId].skinPath, PLATFORM_MAX_PATH, "");
		strcopy(g_playersData[this.clientId].skinName, PLATFORM_MAX_PATH, "");
		//reset anim path
		strcopy(g_playersData[this.clientId].animPath, PLATFORM_MAX_PATH, "");
		strcopy(g_playersData[this.clientId].animName, PLATFORM_MAX_PATH, "");

		this.rSkinItem = 0;
		
		g_playersData[this.clientId].userId = 0;

		this.InitEquipList();
	}

	public void CheckValid()
	{
		//if there's no user id, then this PlayerData wasn't valid on creation, and shouldn't be assumed to be valid now
		if (!(this.userId))
		{
			int c = this.clientId;
			this.ClearData();
			ThrowError("PlayerData of client index %i does not correspond to a real player.", c);
		}
	}

	public void GetStr(char[] property, char[] buffer, int bufferSize)
	{
		//we should be allowed to get the properties of a nonexistent player. they should be 0.
		if (StrEqual(property, "skinpath", false))
			strcopy(buffer, bufferSize, g_playersData[this.clientId].skinPath);
		else if (StrEqual(property, "skinname", false))
			strcopy(buffer, bufferSize, g_playersData[this.clientId].skinName);
		else if (StrEqual(property, "animpath", false))
			strcopy(buffer, bufferSize, g_playersData[this.clientId].animPath);		
		else if (StrEqual(property, "animname", false))
			strcopy(buffer, bufferSize, g_playersData[this.clientId].animName);		
		else
			ThrowError("PlayerData.GetStr property is invalid.");
	}

	public void SetStr(char[] property, char[] data)
	{
		//we should not be allowed to set the props of a nonexistent player, for obvious reasons.		
		this.CheckValid();

		if (StrEqual(property, "skinpath", false))
			strcopy(g_playersData[this.clientId].skinPath, PLATFORM_MAX_PATH, data);
		else if (StrEqual(property, "skinname", false))
			strcopy(g_playersData[this.clientId].skinName, PLATFORM_MAX_PATH, data);
		else if (StrEqual(property, "animpath", false))
			strcopy(g_playersData[this.clientId].animPath, PLATFORM_MAX_PATH, data);
		else if (StrEqual(property, "animname", false))
			strcopy(g_playersData[this.clientId].animName, PLATFORM_MAX_PATH, data);
		else
			ThrowError("PlayerData.SetStr property is invalid.");

	}

	public void SetEquippable(int itemReference, int slot, char[] name = "")
	{
		this.CheckValid();

		if (slot < 1) ThrowError("The SetEquippable slot %i is invalid; slots start counting from 1.", slot);

		KeyValues equips = g_playersData[this.clientId].equippedItems;

		char slotStr[MAXINTLENGTH];
		IntToString(slot, slotStr, sizeof(slotStr));

		if (itemReference == 0)
			equips.SetString(slotStr, "");
		else
			equips.SetNum(slotStr, itemReference);
		
		//we're overwriting whatever's here - if name is "" it'll delete the key
		g_playersData[this.clientId].equippedItemNames.SetString(slotStr, name);
	}

	public int GetEquippable(int slot)
	{

		KeyValues equips = g_playersData[this.clientId].equippedItems;

		char slotStr[MAXINTLENGTH];
		IntToString(slot, slotStr, sizeof(slotStr));


		return equips.GetNum(slotStr);
	}

	public void GetEquippableName(int slot, char[] buffer, int bufferSize)
	{
		KeyValues equips = g_playersData[this.clientId].equippedItemNames;

		char slotStr[MAXINTLENGTH];
		IntToString(slot, slotStr, sizeof(slotStr));

		equips.GetString(slotStr, buffer, bufferSize);
	}

	/**
	 * Gets all the equip slots that are filled with models, placing them into slotsList (in filling order).
	 * slotsList will not be overwritten; the slots will be pushed to the end.
	 * Remember to close the ArrayList handle after you're done.
	 * 
	 * @return An ArrayList of the filled slots.
	 */
	public ArrayList GetFilledSlots()
	{
		KeyValues equips = g_playersData[this.clientId].equippedItems;
		ArrayList slotsList = new ArrayList();
		
		if (equips.GotoFirstSubKey(false))
		{
			char slotStr[MAXINTLENGTH];
			do
			{
				equips.GetSectionName(slotStr, sizeof(slotStr));
				slotsList.Push(StringToInt(slotStr));
			} while(equips.GotoNextKey(false));

		}

		equips.Rewind();

		return slotsList;
	}
}

static void UserIdIsDifferent(int clientId)
{
	LogMessage("Warning: The FreakModel data for user %i was not correctly reset. (Error 60)", g_playersData[clientId].userId);

	g_playersData[clientId].userId = 0;
	PlayerData(clientId).ClearData();
}

//mainly used in/for cleanup module

ArrayList allWearables = null;

//mainly used in/for config module

#define FM_CONFIGFILEPATH "configs/freakmodels.models.txt"

KeyValues config = null;
