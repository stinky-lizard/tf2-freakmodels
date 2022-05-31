
void MainCommandMenu(int client)
{
    //first check if the client is the server. can't show a menu to the server
    if (!IsValidClient(client))
    {
        ReplyToCommand(client, "Please use the command options, which can be seen with `freakmodel -h`, when using this command from the server console.");
        return;
    }

    Menu mainMenu = new Menu(MenuHandler_MainCommand);

    mainMenu.SetTitle("Customize your model");
    mainMenu.AddItem("#info", "View your info");
    mainMenu.AddItem("spacer1", "spacer1", ITEMDRAW_SPACER);
    mainMenu.AddItem("#animation", "Change your animations");
    mainMenu.AddItem("#skin", "Change your skin");
    mainMenu.AddItem("#reset", "Reset your model");
    mainMenu.AddItem("spacer2", "spacer2", ITEMDRAW_SPACER);
    mainMenu.AddItem("#addequip", "Equip a model");
    mainMenu.AddItem("#removeequip", "Remove an equipped model");

    mainMenu.Pagination = MENU_NO_PAGINATION;
    mainMenu.ExitButton = true;

    mainMenu.Display(client, 30);
}

static int MenuHandler_MainCommand(Menu menu, MenuAction action, int param1, int param2)
{
    switch (action)
    {
        case MenuAction_End:
        {
            delete menu;
        }
        case MenuAction_Select:
        {
            char selectedItem[32];

            menu.GetItem(param2, selectedItem, sizeof(selectedItem));

            if (StrEqual(selectedItem, "#info"))
            {

            }
            else if (StrEqual(selectedItem, "#animation"))
            {
                
            }
            else if (StrEqual(selectedItem, "#skin"))
            {

            }
            else if (StrEqual(selectedItem, "#reset"))
            {

            }
            else if (StrEqual(selectedItem, "#addequip"))
            {

            }
            else if (StrEqual(selectedItem, "#removeequip"))
            {

            }
        }
    }
}

static void SetModeModelSelect(Menu menu, int client, int operation)
{


    menu.RemoveAllItems();

    //get all models
    config.Rewind();
    config.JumpToKey("named_models");

    ArrayList modelNames = new ArrayList(PLATFORM_MAX_PATH);
    ArrayList modelPaths = new ArrayList(PLATFORM_MAX_PATH);

    GetAllModelsInCurSection(modelNames, modelPaths, false);
    config.Rewind();

    //add model names to menu
    char modelName[PLATFORM_MAX_PATH];
    for (int i = 0; i < modelNames.Length; i++)
    {
        modelNames.GetString(i, modelName, sizeof(modelName));
        //todo: when getting config from file, lowercase all model names & save the case within a separate key "name_cases" as lowername:casedname
        //so we can use the casing from the file
        modelName[0] = CharToUpper(modelName[0]);
        menu.AddItem(modelName, modelName);
    }

    delete modelPaths;
    delete modelNames;

    menu.AddItem("#custom", "Custom Model Path");

    menu.Display(client, MENU_TIME_FOREVER);
}