BBCF Improvement Mod (v3.1) by Libreofficecalc -- README
==========================

What this mod provides
==========================
- Unlocks the game's region restricted multiplayer
- Adds extra game modes
- Adds hitbox overlay
- Adds framedata advantage info
- Create and load custom palettes and effects without file modifications
- See each other's custom palettes in online matches
- Options to improve the performance of the game even further
- More flexibility to change the graphics options
- Change avatars and accessories in online rooms/lobbies without going back into menu
- Freely adjustable ingame currency value

Requirements
==============
The game must be up-to-date

How to install
==========================
1. Download and extract the latest BBCF_IM.zip
2. Put dinput8.dll and settings.ini into the game's folder (where BBCF.exe is located)
(Should be at "..\Steam\SteamApps\common\BlazBlue Centralfiction")
3. Adjust the values in settings.ini as desired

Uninstall
==========================
Delete or rename dinput8.dll. The mod makes no permanent changes to anything.

How to use the mod's overlay
==========================
- By default the mod's main window can be toggled with the F1 key (can be changed in the settings.ini file).
- Double-click on title bar to collapse windows.
- Click and drag on any empty space to move the windows.
- Click and drag on lower right corner to resize the palette editor window (double-click to auto fit window to its contents).
- CTRL + Click on a slider or drag box to input value as text.
- Mouse wheel to scroll.

How to start a custom game mode
==========================
- The custom game mode selector can be accessed under the "Gameplay settings" section on the main window while you are on the character selection screen in the following modes: Training, Versus, Online.
- The custom game mode selector is also available in the Replay Theater menu screen, so replays saved after after custom game modes can be replayed. (If a custom game mode that the replay's match was not played on is selected then the replay will desync)
- Custom game modes can be played in online player rooms or lobbies with other Improvement Mod users as well. For this both player 1 and player 2 must select the same custom game mode while on the character selection screen. If the players have not settled on the same custom game mode, then it will default back to "Normal".

How to access the palette editor
==========================
- The palette editor's button can be accessed under the "Custom palettes" section on the main window while you are in a match in the following modes: Training, Versus.
- Custom palettes saved in the editor can be found at "..\BlazBlue Centralfiction\BBCF_IM\Palettes\"

How to switch between custom palettes
==========================
- The palette switching buttons can be accessed under the "Custom palettes" section whenever you are in a match.
- In online matches you can only switch your own character's palette. (You can reset your opponent's palette with the button found next to the palette switching button).
- Using the "palettes.ini" file in the game's root folder you can assign custom palettes to the ingame palette slots.

How to set a custom palette as default so it gets automatically switched to upon the start of a match
==========================
See "palettes.ini" file that you placed in the game's root folder.

How to access the hitbox overlay
==========================
- The hitbox overlay can be accessed under the "Hitbox overlay" section on the main window while you are in a match in the following modes: Training, Versus, Replay.

Where to place and find your custom palette files (.cfpl and .hpl files)
==========================
- The palette folders are created in the path "..\BlazBlue Centralfiction\BBCF_IM\Palettes\" upon the first launch of the mod.
- Place your .cfpl or .hpl files into the character's corresponding palette folder (at "..\BlazBlue Centralfiction\BBCF_IM\Palettes\") to have the mod automatically load them upon start, and making them selectable ingame via the mod's menu.

For legacy .hpl formats:
- Effect files for the .hpl format must end with the naming convention of "_effectXX.hpl". Where XX is the index of the effect file. 
(For example, if you have a custom palette file named "Nyx_Izanami.hpl", then in the same palette folder a file named "Nyx_Izanami_effect01.hpl" will be loaded as that palette's first effect, and a file named "Nyx_Izanami_effect06.hpl" will be loaded as sixth, etc.)
- A file created with its name ending with "_effectbloom.hpl" will turn on the bloom effect for that custom palette in the game. (Once activated, the bloom effect will keep the color it was first activated on, and can't be changed unless a new round is started)

Known issues
==========================
1. Vampire and Extreme Vampire desync online. Avoid using these two custom game modes when online.

2. Platinum keeps swapping between her default and the selected custom palette whenever she has her drive active.
	- To prevent this, assign her custom palette in palettes.ini before the match starts, and do not switch it during match.

3. Hitbox overlay is not aligned properly or is off-screen when the game's window resolution mismatches the rendering resolution.
	- To prevent this, open up the settings.ini file, and set Viewport to 2, while setting the RenderingWidth and RenderingHeight to the resolution values you have in the ingame display options.

4. Hitbox overlay shows phantom hitboxes that are disabled by the engine (shoutout to Shtkn for discovering this)

5. Screen not displaying properly when Viewport in settings.ini is set to 2 or 3 with keep aspect ratio enabled in in-game settings. This is still being investigated. For now, either
disable keep aspect ratio or keep the Viewport option set to 1. 

Troubleshooting
==========================
First make sure that your issue doesn't persist if you remove/rename dinput8.dll
If the issue still persists, then it has nothing to do with the mod.

1. The mod's UI is very small, unreadable:
This might happen if you run the game in windowed or full-window mode with a resolution that's higher than what your monitor supports.
Try changing your display setting to fullscreen, or lowering the resolution ingame.

2. Blackscreen problem:
Make sure that RenderingWidth and RenderingHeight values in the .ini file are correctly set to the resolution of your game. 
If their values are higher than your monitor's native resolution, then you might get blackscreen.

3. Game crashes when launched:
Make sure that the settings.ini file is present in the game's folder.
Check if the problem still occurs after you remove, or rename dinput8.dll
Turn off tools such as MSI Afterburner, Plays.tv, or other recording softwares and overlays that manipulate D3D.
Make sure you are running both Steam and the game with administrator privileges, and remove any steam_appid.txt file if present in the game's directory.
Restore the default settings in the settings.ini file.
Reboot your system.

4. The mod's windows are off-screen or have disappeared:
Delete the "menus.ini" file found in the game's root folder to reset the positions of the mod's windows.

Keep in mind that the mod may partially, or completely stop working whenever a new official patch of the game is released. (Though unlikely)

Changelog

===============================
02-11-2025 -- version 3.101
===============================
Features:
- Frame history bar can now be used in mirrors (+@Unisectyn)
- Frame history bar now distinguishes between guard point and invul

Bugfixes:
- Inactive hitboxes(from bbscript 23047) will not show up on the hitbox viewer anymore, ex: es's j.5a, nine's 5a.
- Inactive hitboxes(from bbscript 23047) are now listed as inactive on the state's frame breakdown viewer.
- Bugs related to frame history bar in mirrors (such as crashes when changing to a mirror with it open) are solved.
- Toggling the replay data upload using the main menu will no longer overwrite the following settings:
- - UploadReplayDataHost
- - UploadReplayDataEndpoint
- - UploadReplayDataPort

Dev/DEBUG stuff:
- Fixed alignment issue on unknown_status2, which wasn't causing issues by luck(default align). In the future we should pack this stuct.

===============================
04-10-2025 -- version 3.100
===============================
Features:
- Added Real time frame viewer (+@Unisectyn)

Bugfixes:
- Room rematch setting changed while in room will not reset when a new player enters anymore.



===============================
16-07-2025 -- version 3.099
===============================
Features:
- Hitbox Overlay:
- - Added collision, throw range and move range box viewers.
- - Added brief descriptions of how the checks for throws work on the help markers "(?)"

Replay database download/archive replace (@Tadatys):
- Focused on populating the replay theater with other replays
- - Experimental: Added direct replay search+download+load from website db to the game.
- - Experimental: Added direct loading of replays from archive to replay theater.

- Combo data window:
- - Added heat cooldown counter.
- - Last heat gain now persists until a new combo starts.

Bugfixes:
- Hitbox displays should now work properly with "keep aspect ratio" in fullscreen/borderless. It used to bug out in corners.(#45 @MorphRed)
- Frame advantage viewer now works correctly on a jumping dummy(6f68d0eb78783e53570c244af0a01568929eb3ca).
- Fixed heat gain on combo data window not taking into account heat cooldown(49b3340fa4c33a410d36c0f344fcb28ec45281a7).

Dev/DEBUG stuff:
- Added charpos control for p1 and p2 in DEBUG window.


===============================
02-11-2024 -- version 3.098
===============================
Bugfixes:
- Fixed some input buffer names showing the incorrect motion.

===============================
17-08-2024 -- version 3.097
===============================
Features:
- Removal of the need to be connected to the network in order to watch replays in the replay theater

Bugfixes:
- #42 : Archived replay files would skip the first 2 letter of non-CJK names, fixed.
Changes:
- Removed "::experimental" suffix from replay rewind, it is not experimental anymore. Refer to the release https://github.com/libreofficecalc/BBCF-Improvement-Mod/releases/tag/v3.095 for more in-depth info on how it works if needed.

===============================
15-06-2024 -- version 3.096)bugfix
===============================
Bugfixes:
- If both players had IM BO BLEIS version v3.095, and both had replay upload ENABLED, it wouldn't upload the replay. It should work properly now.

===============================
30-05-2024 -- version 3.095
===============================
Changes:
- Replay takeover's "load replay state" button will be disabled when in the replay itself and not in the taken over training stage.
- Rewind until you reach the checkpoint at second 9
- Now as it reaches the desired position it will have recorded the previous checkpoints in smaller intervals, allowing you to wait less to reach the desired part / analyze the lead up to it.
- You can see the frames of all saved checkpoints and more advanced info on the Saved Checkpoints Advanced Info" section located above the "rewind" button.

===============================
21-05-2024 -- version 3.094
===============================
Features:
- Added button to toggle replay upload in game without needing to use settings.ini
- You now need to choose whether to have the replay database enabled or not on first boot
Fixes:
- Disabling replay upload to db previously would not stop someone who had it enabled from uploading, now if you are either p1 or p2 and has it disabled it will disable the uploading from the other players.
- ps: The website's backend and frontend code is in this repository, anyone that wants to help feel free: https://github.com/libreofficecalc/bbcf-im-replay-server

===============================
05-05-2024 -- version 3.093
===============================
Features:
- Rough draft of a simple frontend for the database is available using the button "Replay Database" in the main IM window.
- I will open the code being used for the backend and the scratch frontend later this week once I get more time, any help is appreciated.

===============================
17-03-2024 -- version 3.092
===============================
Features:
- Removal(for now) of experimental replay rewind that didn't work anyway(for now) and all its baggage.

===============================
10-03-2024 -- version 3.091
===============================
Features:
- Replay Takeover:
- Replay takeover now actually works.
- P1 and P2 takeover.
- Option to set a delay from load state to it actually running to reposition hands etc.

Bugfixes:
- Added guardrails to disable save states/replay takeover if searching for a ranked match, due to the issue listed here: #35
- *: only between F1-F9. Currently there's no support for controllers, use autohotkey to make a simple script to trigger it if you really need it, should be trivial.
Changes:
- Does not work when searching for a ranked match(for now).
- If playback of takeover seems wrong use the "FIX PLAYBACK" button before reporting an issue.
- Replay Rewind:
- Only here because I forgot to make a different branch for it, still experimental.
- A lot of progress but still not 100%, some random crashes still happen and you must keep its tab open for it to record in order to rewind.
- Should allow you to rewind in 30 second intervals(for now).

===============================
08-03-2024 -- version 3.090
===============================
Features:
- Replay Takeover:
- Replay takeover now actually works.
- P1 and P2 takeover.
- Option to set a delay from load state to it actually running to reposition hands etc.

Bugfixes:
- Added guardrails to disable save states/replay takeover if searching for a ranked match, due to the issue listed here: #35
- *: only between F1-F9. Currently there's no support for controllers, use autohotkey to make a simple script to trigger it if you really need it, should be trivial.
Changes:
- Does not work when searching for a ranked match(for now).
- If playback of takeover seems wrong use the "FIX PLAYBACK" button before reporting an issue.
- Replay Rewind:
- Only here because I forgot to make a different branch for it, still experimental.
- A lot of progress but still not 100%, some random crashes still happen and you must keep its tab open for it to record in order to rewind.
- Should allow you to rewind in 30 second intervals(for now).

===============================
25-02-2024 -- version 3.080_guanabara_remembers
===============================
Features:
- Save states:
- You can now save and playback states in training mode, actually works this time.
- Option to set a delay from load state to it actually running to reposition hands etc.

Bugfixes:
- Hitboxes:
- Fixing longstanding issue with inactive hitboxes showing as active arising from ID code 2001, these include but are no limited to rachel's 4b, jin's 5b and kagura's 6C.
- States:
- Fixed on hit state activation on the following "trigger states": "CmnActSlideAir" , "CmnActSkeleton", "CmnActBlowoff". These were found in izanami's 6A, 5C air, 41236D and 214A.
- Playbacks:
- Fixed off by one error, making random playbacks in slot 2 refer to the recording in slot1 and so on.
- *: only between F1-F9. Currently there's no support for controllers, use autohotkey to make a simple script to trigger it if you really need it, should be trivial.

===============================
28-01-2024 -- version watch=T_soDyOFu9c
===============================
Features:
- Combo Data Window:
- Added a window containing hitstun, hitstun decay, combo time, proration, starter type and heat gained on combo as well as the same move proration stack. It is usable on training and replays alike for both players.
- Playback actions:
- You can now buffer random wakeup actions.
- Palettes(oJay):
- **check palettes.ini for further explanation**
- Added the ability to randomly select between a list of palette names in pallete config file.
- e.g.
- [Hakumen]
- 1="Default"
- 2="Crimson_Glow,Fulgore,Jetstream_Hakumen"
- choosing palette 2 will random between "Crimson_Glow", "Fulgore" and "Jetstream_Hakumen"
- Added the option to exclude the default palette when using random with the use of a new keyword.
Fixes:
- Frame advantage fixes(PC_volt):
- Air _NEUTRAL is not idle with Izanami float as a special case
- Air Barrier is considered not idle

===============================
05-11-2023 -- version 3.060
===============================
Features:
- Palettes:
- Added an undo and redo button to the palette editor(DixiE).
- You may now use a comma seperated list of palette filenames for a palette index and the mod will load a random palette from this list each match. See the provided palettes.ini file in this release for more information.(OJay)
- Playback Editor:
- Added a button to switch the side a playback was recorded using the playback editor.

Bugfixes:
- Frame advantage fixes(PC_volt):
- A toggle for stagger on hit and on recovery was added. This allows you to know whether an attack that staggers can be combo'd from, or at which frame advantage it leaves you at if they recover.
- Frame advantage was fixed on neutral wakeup, it was previously 1F too early because the first frame of landing after teching is still part of the tech. Example: Mai 2B on hit displays as +32F instead of +31F
- State action fixes:
- Stagger should now be properly triggered as a **wakeup action**, it is being defined as such due to being an Ukemi state. If you want an action on states after a stagger, set a wakeup action, not a on hit action.
- Fixed incorrect state onhit actions triggering on certain hard knockdowns, such as tager's j2c and es's double stacked projectile.

===============================
28-10-2023 -- version 3.056
===============================

===============================
14-10-2023 -- version 3.055
===============================
Features:
- Added frame breakdown in the states tab, allowing you to check for active frames and invul properties of moves throughout their duration.
- Added support for wakeup actions on quick rise for both states and playback.*
- Added loop recording, to loop a slot recording.
- Added a fourth option for "MenuSize" in setting.ini(MenuSize=4), for people with higher resolution screens. I do not have a >1080p monito so need to rely on feedback more than ever on this one.

Bugfixes:
- Fixed longstanding bug with playbacks that would turn any recording you tried to load/modify into taunts and jibberish.
- Fixed hitbox editor window's frame table skipping all over the place anytime you added or removed an entry. You are now edit the playbacks without wanting to jump off a bridge.
- *:In states, if you wish to set a normal as a wakeup action for quick rise and have it behave as it would in game you must add a 6 frame delay, this is not necessary for special. This happens because quick rise allows for specials from frame 14, while for normals only from frame 20. The objective of the states tab is to be exploratory and not necessary perfect to the game so this is why "invalid states" like a normal triggering at frame 14 of quick rise are accepted and not auto-corrected, that is down to the user. And yes I know that as of now 14/10/2023 dustloop claims that you can cancel into normals at frame 14, from my testing so far it appears they are incorrect.
- **this is not relevant to the mod: Little tavo if you have some time pls send us some combo into oki recordings

===============================
22-09-2023 -- version 3.0540
===============================
Features:
- Playback Editor:
- Allows for hand editing playbacks to create frame perfect recordings
- Wakeup Delay and Randomizer:
- Adds option to delay wakeup both fixed and randomized within a range(skew)
- Allows for selecting randomization of wakeups (ex:you can choose just to randomize forward and backward rolls)
- Does not include emergency tech
- Motion Input Buffer Viewer for P1 and P2
- Option in settings.ini to set whether the ""ranked fix""" aka load foreign pallets is activated by default or not. Either replace your settings.ini with the one provided in this release or add the line "LoadForeignPalettesToggleDefault =" with either 1 or 0 after the "=", depending on the desired behaviour.

Bugfixes:
- The dropdown display on the rematch settings should now properly reflect the correct setting after a room is remade

===============================
02-09-2023 -- version 3.0510
===============================
Features:
- Added throw tech actions for both states and playback

Bugfixes:
- Fixed playback on hit not triggering for actions in the air or that caused launch states
- Fixed "Enable" button label conflict that didn't allow the framedata window to open if the hitbox section was open
- Fixed size of the frame data window resetting to the default every frame
- Added small indentation to the "Enable" button on the framedata section on the main window
- A SuperVia deseja a todos uma boa viagem.

===============================
01-09-2023 -- version 3.05
===============================
- Added Framedata window for Practice Mode and Replays Theater
- Displays gaps on block and on hit, up to 30F gaps
- Displays frame advantage on block and on hit

===============================
12-08-2023 -- version quebraaluneta
===============================

===============================
05-08-2023 -- version canseiderefazersala
===============================

===============================
02-08-2023 -- version fixing_logging
===============================

===============================
30-07-2023 -- version semspoiler
===============================

===============================
28-07-2023 -- version preguica
===============================

===============================
25-07-2023 -- version superia
===============================

===============================
13-05-2023 -- version 3.046_atrasada
===============================

===============================
04-05-2023 -- version iagra
===============================

===============================
01-05-2023 -- version asdf.asdf2
===============================

===============================
26-04-2023 -- version australia.canada.newzealand.uk.us.stopwatchingmyfiles
===============================

===============================
16-04-2023 -- version prince.of.persia2.5.apple2
===============================

===============================
05-04-2023 -- version prince.of.persia.apple2
===============================

===============================
31-03-2023 -- version hate.arcsys
===============================

===============================
25-03-2023 -- version alocando.combos.baixados.aleatoriamente
===============================

===============================
21-03-2023 -- version alocando.combos.baixados
===============================

===============================
20-03-2023 -- version baixando.mais.combos
===============================

===============================
19-03-2023 -- version baixando.combos
===============================

===============================
18-03-2023 -- version grade.do.zoologico.release
===============================

===============================
15-03-2023 -- version erf
===============================

===============================
17-03-2023 -- version asdfe
===============================

===============================
15-03-2023 -- version asdf
===============================

===============================
15-03-2023 -- version t
===============================

===============================
13-03-2023 -- version bbcf2
===============================

===============================
13-03-2023 -- version bbcf
===============================

==========================
23-07-2022 -- version 3.04
===============================
- Updated to be compatible with the ver 2103 patch

08-03-2022 -- version 3.03
===============================
- Implemented fix for additional rare rematch crash

25-02-2022 -- version 3.02
===============================
- Implemented fix for crashes when rematching in lobbies

20-01-2022 -- version 3.01
===============================
- Updated to be compatible with beta branch
- All FT5 modes have been set to FT3. This is to avoid a crash that happens with FT5 game modes online. Once it's fixed they will be reverted back to FT5
- Vsync is now set to off by default. You can turn it on in the settings.ini file
- Removed CpuUsageFix, fix for CPU usage is already in publictest builds officially
- Removed Region lock settings as they are no longer needed
- With the aformentioned changes, it is highly reccomended to replace your existing settings.ini file if you have a previous version of the mod

11-01-2021 -- version 3.00
===============================
- Complete ground up rework of BBCFIM, online features are incompatible with previous versions
- Added hitbox overlay in Training, Versus, and Replay modes
- Added "palettes.ini" file for assigning custom palettes to the ingame palette slots
- Added Highlight mode in the palette editor, to make it easier to find the corresponding colors
- Added dinput dll chaining to settings.ini to use other dinput8.dll wrappers together with BBCFIM
- Added option to toggle the visibility of the ingame HUD
- Added "Online window" for BBCFIM detection and quick access to certain features in online games
- Added frame freezing option to the palette editor
- Added gradient generator to the palette editor
- Added color box indexing to the palette editor
- Enabled palette editor in Versus mode
- Palettes are now previewed when hovered over their selections
- Custom palettes now use a new CFPL file format (.hpl format is still supported)
- "Drag and drop" of the color boxes now work in the palette editor
- Players now send custom palettes and game mode requests to spectators as well
- All custom game modes are now playable in Training mode as well
- Custom game modes are applied on spectators as well in online matches
- Disabled stage selection slider in game modes other than online/training/versus
- Keyboard inputs are now not being passed to the game while any of BBCFIM's windows are focused
- BBCFIM windows are now hidden when the Steam overlay is active
- Detecting other BBCFIM players in online games is now consistent
- Players you play online games with are now added to Steam's "Recent games" list
- Removed palette placeholders in palette editor
- Fixed the game with BBCFIM not launching on Windows 10 in some cases
- Fixed the title of the main window being always visible
- Fixed online BBCFIM detection mistaking spectators as the opponent player
- Fixed Jubei's stage missing from BBCFIM's stage selection slider
- Fixed Steroid game mode starting a different game mode
- Fixed Vampire game mode's health draining not working when the timer is set to infinite in Versus mode

31-03-2018 -- version 2.06
===============================
- Added new custom game modes:
	Five Punch Mode:
		* Taking damage five times results in death, unless blocked with barrier
		* Each round 50% of the Burst Gauge is regenerated
		* Each round lasts 60 seconds
		* This game mode is always 5 rounds
	Tug of War Mode:
		* Start with 50% HP
		* All damages hurt/heal 10% HP, unless blocked with barrier
		* Each round 50% of the Burst Gauge is regenerated
		* Each round lasts 60 seconds
		* This game mode is always 5 rounds
	Infinite Heat Mode:
		* Infinite Heat Gauge
- Added MenuSize option
- Added buttons for picking custom palettes randomly
- Added Stage select slider, now you can select any stages online
- Added "Show transparency values" checkbox on the palette editor
- Added "Save with bloom effect" tickbox to the "Character file" page in the palette editor 
- Added logging to file
- Added Crash Dump generation on unhandled exceptions to track down crash causing bugs
- Custom palettes can now be placed in subfolders
- Custom game modes now can be applied on replays
- Transparency/Alpha values are now hidden by default on the palette editor
- Fixed receiving BBCFIM ID packets outside of the character selection screen
- Fixed some custom game mode modifications not resetting back to normal when gone back to the charselect screen without quitting to the main menu in offline modes
- Fixed spectators being able to switch the players' palettes
- Fixed crash when overwriting an existing palette file with upper-case/under-case differences in the palette editor

11-03-2018 -- version 2.05
===============================
- Fixed some players being unable the set their custom palettes in online matches

10-03-2018 -- version 2.04
===============================
- Added custom game modes that are playable in training, versus, and online: 
	Steroid Mode:
		* Max HP: +150%
		* Max Heat Gauge: +50%
		* Heat gain: +20%
		* Automatic Heat gain on low HP: +100%
		* Burst Gauge gain: +100%
	Vampire Mode:
		* 60% of damage dealt is healed back
		* Players lose 1% max HP per second
	Extreme Vampire Mode:
		* 200% of damage dealt is healed back
		* Healing beyond full HP increases max HP
		* Losing 1.5% max HP per second
	One Punch Mode:
		* Any damage results in instakill, unless blocked with barrier
		* Each round 50% of the Burst Gauge is regenerated
		* Each round lasts 15 seconds
		* This game mode is always 5 rounds
	Two Punch Mode:
		* Taking damage twice results in death, unless blocked with barrier
		* Each round 50% of the Burst Gauge is regenerated
		* Each round lasts 15 seconds
		* This game mode is always 5 rounds
- Added notifications
- Added NotificationPopups option
- BBCFIM now lets you know if the opponent is using BBCFIM as well (detecting versions only v2.04 and above)
- Added overwrite confirmation when a palette with the same name already exists
- Saving a custom palette in the palette editor will now automatically load it into the palettes menu. No longer needed to manually reload them
- Saving custom palettes now support capital letters as well
- Saving custom palette won't clear the filename's textinput box anymore
- Loading custom effects now have better error-logging
- Fixed player 1 and player 2 palettes being switched up after "quick-changing" characters while playing as player 2 in the tutorial mode
- Fixed characters using palettes from previous replays in the Replay Theater mode
- Fixed players occasionally not sending the custom palettes to the opponent in online matches
- Fixed rare crash in lobby
- Fixed crash whenever losing in the Grim of Abyss mode

20-02-2018 -- version 2.03
===============================
- Fixed crash that occured whenever a custom palette was selected then switching to an another character while the BBCFIM menu was closed
- Fixed crash when attempting to select Platinum or Valkenhayn in the palette editor.
- Fixed bloom effect not being reactivated on the 22nd palettes upon rematches in offline modes
- Fixed default palettes changing to different ones upon rematches in offline modes

18-02-2018 -- version 2.02
===============================
- Added ingame palette editor
- Added support for bloom effect on custom palettes (see "Where to place your custom palettes" section for how-to)
- Added NMC's winning palette of Azure Abyss' february palette contest
- Fixed bloom effect not being visible on the 22nd palettes
- Fixed crash in arcade and versus modes when playing as player 2
- Fixed palette swapping bugs in arcade and versus modes when playing as player 2
- Fixed crash in versus mode when locally playing with two players

08-02-2018 -- version 2.01
===============================
- Fixed crashing in tutorial mode
- Fixed a palette bug that occurred whenever characters were "quick-changed" in training mode

07-02-2018 -- version 2.00
===============================
- New overlay GUI framework
- Palette files (.hpl) can be loaded without any file modification
- Can freely switch palettes anytime during matches
- Can freely switch avatars and accessories on the fly
- Can freely edit the value of the ingame currency
- Palettes are now exchanged at the start of the matches (instead at the "wheel of fate" part)
- Added ToggleButton option
- Added CpuUsageFix option to reduce the game's CPU usage
- Added CheckUpdates option to check if newer version of BBCFIM is available
- Removed CustomPalettes option as it will always be turned on
- This version's custom palette sharing is incompatible with BBCFIM version 1.23

28-11-2017 -- version 1.23
===============================
- Added CustomPalettes option (HIGHLY EXPERIMENTAL! Makes it possible to see each other's custom color palettes)

19-06-2017 -- version 1.22
===============================
- Rewritten certain parts of the code to eliminate any possible input lag, and fix some controller incompatibilities

12-06-2017 -- version 1.21
===============================
- Fixed blackscreen issue that occurred when RenderingWidth and RenderingHeight were set above 1920x1152

08-06-2017 -- version 1.20
===============================
- Added Regionlock options
- Added Viewport options
- Fixed the misbehaving texture filter
- Fixed background sign being misaligned on the stage Duel Field

07-06-2017 -- version 1.11
===============================
- Adjusted D-card and Chat text placements some more

06-06-2017 -- version 1.10
===============================
- Fixed D-card and Chat text placements

28-05-2017 -- version 1.00
===============================
- Initial release


Special thanks to
==========================
KoviDomi
corpse warblade
Dormin
Euchre
NeoStrayCat
Rouzel
SegGel2009
Everybody in the BlazBlue PC community that has given support or feedback of any kind!