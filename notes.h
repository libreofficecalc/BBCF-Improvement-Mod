#pragma once
/*


SCENE_CVSInfo that is input to the  touches_on_GameSceneState_FUN_00a8f9b0 func

function to change scene probably in CScene_Controller class


Check GAME_CEventCOntrol task because it probably is the thing that is controlling the order of SCENE change and stuff, and its factory. 
It is one of the arguments for the function that seems to do the scene change 
I can manipulate the scene change sometimes forcing by changing the arguments in the function (and remembering to set the GAMESTATE to be set in the minidump args in cheat engine)




room is created with the class GAME_CNetworkTask
the function that seems to call for the creation of the room(base+a56e0) has param1 = GAMESTEAM_CNetworkServer
*/