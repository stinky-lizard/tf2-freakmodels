
/*
 CONFIG FUNCTIONS
 ============================================
 */

/**
 * Refreshes the configuration from the config file. Creates one if necessary.
 */
void RefreshConfigFromFile()
{
	char configFilePath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, configFilePath, sizeof(configFilePath), FM_CONFIGFILEPATH);

	if (!FileExists(configFilePath)) CreateConfigFile();

	if (config != null) delete config;

	config = ReadModelsFromConfig();
	
	if (!config.JumpToKey("named_models"))
		//there is no models section
		LogMessage("Warning: Failed to read from config file. Consider checking the file, or deleting the file to initialize it.");

	config.Rewind();

	//TEMPORARY
	//todo: do correctly
	config.JumpToKey("comments", true);
	if (!config.GetNum("RequiredAdmin"))
	{
		LogMessage("Warning: admin requirements aren't defined in the config file. Defaulting to ADMFLAG_GENERIC.");
		config.SetNum("RequiredAdmin", ADMFLAG_GENERIC);
	}

	config.Rewind();

	//todo: lowercase all names & save case in separate sub-key	
}

/**
 * Creates a config file with the correct format.
 * @param addClasses Add the classes & their models to the config file by default.
 */
void CreateConfigFile(bool addClasses=true)
{
	char filePath[PLATFORM_MAX_PATH];

	BuildPath(Path_SM, filePath, sizeof(filePath), FM_CONFIGFILEPATH);

	KeyValues models = new KeyValues("FreakModels");

	//add settings
	//future?

	//add named models
	models.JumpToKey("named_models", true);

	if (addClasses)
	{
		models.SetString("scout", "models/player/scout.mdl");
		models.SetString("soldier", "models/player/soldier.mdl");
		models.SetString("pyro", "models/player/pyro.mdl");
		models.SetString("demoman", "models/player/demo.mdl");
		models.SetString("demo", "models/player/demo.mdl"); //for backwards compat
		models.SetString("heavy", "models/player/heavy.mdl");
		models.SetString("engineer", "models/player/engineer.mdl");
		models.SetString("medic", "models/player/medic.mdl");
		models.SetString("sniper", "models/player/sniper.mdl");
		models.SetString("spy", "models/player/spy.mdl");
	}

	models.Rewind();

	//add section for non-named precached models
	models.JumpToKey("precache_no_name", true);
	models.SetString("error (placeholder)", "models/error.mdl");
	models.Rewind();


	//add comments
	models.JumpToKey("comments", true);
	models.SetString("Hello! This is the configuration file for FreakModels.", "-");
	models.SetString("Add new named models by adding them to the named_models section.", "-");
	models.SetString("For example,", "if you wanted to add models/bots/headless_hatman.mdl under the name headless_horseman, you would add the line:");
	models.SetString("headless_horseman", "models/bots/headless_hatman.mdl");
	models.SetString("You can also add new models to the precache_no_name section, if you want to add non-named models that'll get precached when the round starts.", "-");

	models.SetString("-", "-");
	models.SetString("You can also group models together by adding new subsections. For instance, you could add this to the named_models section:", "-");

	models.JumpToKey("Bots", true);
	models.SetString("A New Model Name!", "Path/To/New/Model");
	models.GoBack();
	models.SetString("^ Add those four lines, even the lines that are only { and }.", "-");

	models.SetString("--", "-");
	models.SetString("Keep in mind all model names are case-insensitive, for QoL of the user.", "-");
	models.SetString("Also, each model name has to be unique. If there are two models with the same name, only the first one found will be used.", "-");
	models.SetString("Finally...", "No forward slashes in model names. It just doesn't work. I cannot change this. Sorry!");

	//TEMPORARY
	models.SetString("---", "-");
	models.SetString("TEMPORARY:", "This next line is a temporary setting. You can change it and reload the plugin to change the required admin levels.");
	models.SetString("Check this link:", "https://wiki.alliedmods.net/Checking_Admin_Flags_(SourceMod_Scripting)");
	models.SetNum("RequiredAdmin", ADMFLAG_GENERIC);


	models.Rewind();

	models.ExportToFile(filePath);

	delete models;
}

/**
 * Reads the models configuration. Assumes it is created (if it isn't it will return an empty KV.)
 * @return A KeyValues of the models (root node "FreakModels", models stored in key "named_models")
 */
KeyValues ReadModelsFromConfig()
{
	char filePath[PLATFORM_MAX_PATH];

	BuildPath(Path_SM, filePath, sizeof(filePath), FM_CONFIGFILEPATH);

	KeyValues models = new KeyValues("FreakModels");
	models.ImportFromFile(filePath);
	return models;
}

bool SearchForModel(char[] name, char[] path, int pathsize, bool intheweeds = false)
{
	do
	{
		if (config.JumpToKey(name))
		{
			//we jumped to something with the models name...
			if (config.GetDataType(NULL_STRING) == KvData_String)
			{
				//model is here!
				config.GetString(NULL_STRING, path, pathsize);
				return true;
			}
			else if (config.GetDataType(NULL_STRING) == KvData_None)
			{
				//we jumped to a section with the model's name
				config.GoBack();
			}
			else
			{
				//it's set incorrectly...
				LogMessage("Warning: Model %s in the config file is not set to a model path.", name);
			}
		}
		//model isn't here, go thru all the subsections
		if (config.GotoFirstSubKey())
		{
			if (SearchForModel(name, path, pathsize, true)) return true;
		}
	}
	while(intheweeds && config.GotoNextKey());

	config.GoBack();
	return false;
}

/**
 * Gets a model from the configuration, by name.
 * __This will rewind the config to the root node.__
 * 
 * @param name         Name of the model to get.
 * @param path         Buffer to store the path to the model in.
 * @param pathsize     Size of the path buffer.
 * @return             _true_ if a model was successfully found. _false_ if no model was found.
 */
bool GetModelFromConfig(char[] name, char[] path, int pathsize)
{
	if (config.NodesInStack() > 0)
		config.Rewind();
	
	config.JumpToKey("named_models");

	bool out = SearchForModel(name, path, pathsize);

	config.Rewind();
	return out;
	
}

bool CheckModelGood(char[] model)
{
	return IsModelPrecached(model);
}

/**
 * Precaches all the models in a section, and all its sub-sections.
 * 
 * @return            True if a model was precached, false if there were none to precache.
 */
bool PrecacheAllModelsInCurSection()
{
	ArrayList modelNames = new ArrayList(PLATFORM_MAX_PATH);
	ArrayList modelPaths = new ArrayList(PLATFORM_MAX_PATH);

	bool out = GetAllModelsInCurSection(modelNames, modelPaths);

	for (int i = 0; i < modelPaths.Length; i++)
	{
		static char path[PLATFORM_MAX_PATH];
		
		modelPaths.GetString(i, path, sizeof(path));

		if (!IsModelPrecached(path)) PrecacheModel(path);
	}

	delete modelNames;
	delete modelPaths;

	return out;
}

/**
 * Iterates through the config and pushes all model names & paths in the current section to the given lists.
 * 
 * @note The data will be pushed to the end of the lists. Block size should be PLATFORM_MAX_PATH.
 * @param outModelNames     OUTPUT: The model names, in order of their definition in the config file (depth-first).
 * @param outModelPaths     OUTPUT: The model paths, in order of their definition in the config file (depth-first).
 * @return                  false if there were no models, true otherwise.
 */
bool GetAllModelsInCurSection(ArrayList outModelNames, ArrayList outModelPaths, bool includeDuplicatePaths = true)
{
	bool intheweeds = false, out = false;

	do
	{
		if (config.GotoFirstSubKey(false))
		{
			//current key is a filled section

			//if the recursive call returns true, we want that to propogate, so we use ||
			out = GetAllModelsInCurSection_Recur(outModelNames, outModelPaths, includeDuplicatePaths, true) || out;
			config.GoBack();
		}
		else
		{
			//Current key is a "value" key, or an empty section.
			if (config.GetDataType(NULL_STRING) != KvData_None)
			{
				//Current key is a "value" key.
				out = true;

				char modelPath[PLATFORM_MAX_PATH], modelName[PLATFORM_MAX_PATH];

				config.GetString(NULL_STRING, modelPath, sizeof(modelPath));
				config.GetSectionName(modelName, sizeof(modelName));

				//does this model exist?
				if (!FileExists(modelPath, true))
				{
					LogMessage("Warning: The model named %s in the config file does not correspond to a real file.", modelName);
					continue;
				}

				//is this model already added to the list?
				if (outModelNames.FindString(modelName) > -1) continue;
				
				if (!includeDuplicatePaths && outModelPaths.FindString(modelPath) > -1) continue;

				outModelNames.PushString(modelName);
				outModelPaths.PushString(modelPath);
			}
		}
	}
	while(intheweeds && config.GotoNextKey(false));

	return out;
}

/**
 * Same as GetAllModelsInCurSection, is just hidden so it can't be misused.
 */
static bool GetAllModelsInCurSection_Recur(ArrayList outModelNames, ArrayList outModelPaths, bool includeDuplicatePaths = true, bool intheweeds = false)
{
	bool out = false;

	do
	{
		if (config.GotoFirstSubKey(false))
		{
			//current key is a filled section

			//if the recursive call returns true, we want that to propogate, so we use ||
			out = GetAllModelsInCurSection_Recur(outModelNames, outModelPaths, includeDuplicatePaths, true) || out;
			config.GoBack();
		}
		else
		{
			//Current key is a "value" key, or an empty section.
			if (config.GetDataType(NULL_STRING) != KvData_None)
			{
				//Current key is a "value" key.
				out = true;

				char modelPath[PLATFORM_MAX_PATH], modelName[PLATFORM_MAX_PATH];

				config.GetString(NULL_STRING, modelPath, sizeof(modelPath));
				config.GetSectionName(modelName, sizeof(modelName));

				//does this model exist?
				if (!FileExists(modelPath, true))
				{
					LogMessage("Warning: The model named %s in the config file does not correspond to a real file.", modelName);
					continue;
				}

				//is this model already added to the list?
				if (outModelNames.FindString(modelName) > -1)
					continue;

				outModelNames.PushString(modelName);
				outModelPaths.PushString(modelPath);
			}
		}
	}
	while(intheweeds && config.GotoNextKey(false));

	return out;
}