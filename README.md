# BBCF-Improvement-Mod
Additional features and tweaks for BlazBlue Centralfiction

The goal is to extend the game with community-created content and provide additional graphical options.

Join the [BB Improvement Mod Discord Server](https://discord.gg/j2mCX9s) to discuss about the development and the game itself!

## What this mod provides
- Adds extra game modes
- Adds hitbox overlay
- Adds replay takeover
- Adds P2 State Library
- Adds wakeup action thru state library
- Adds gap action thru state library
- Adds training dummy slot introspection
- Adds training dummy slot saving/loading to/from local files
- Adds wakeup action thru training dummy slots
- Adds gap action thru training dummy slots
- Adds local replay file loading
- more experimental features


- Create and load custom palettes and effects without file modifications
- See each other's custom palettes in online matches
- More flexibility to change the graphics options
- Change avatars and accessories in online rooms/lobbies without going back into menu
- Freely adjustable ingame currency value

## Installing
Download dinput8.dll, settings.ini and optionally palettes.ini from the latest release and put it in your BlazBlue Centralfiction folder. There is no need to compile it yourself for regular usage.


## Compiling and usage
BBCF Improvement Mod is coded using Visual Studio 2019 (toolset v142). <br>
To compile, you should only need to load the sln file and compile as-is. No changes should be needed to the solution or source.<br>
Copy the compiled binary, settings.ini, and palettes.ini files from the bin/ folder into the game's root folder.


### FrameHistory: 
frameHistory is a frame meter with two rows for each character. For each non-idle frame, it displays a column of rectangles. 
- For each player:
    + The first row displays player state. The colors are as follows 
        - Hard landing recovery -> blush
        - Startup -> green
        - Active -> red
        - Recovery -> blue
        - Blockstun -> yellow
        - Hitstun -> purple
        - Special (not so easy to classify states: e.g. dashes) -> Aquamarine
        - Unclassified but unable to act -> Byzantium
        - Unclassified otherwise -> Burgundy
    + Second row is for attribute invulnerability. The colors and shapes are mixed and overlayed respectively.
        - Head -> red   rectangle
        - Body -> green rectangle
        - Foot -> blue  rectangle
        - Projectile -> green disk
        - Throw ->      red   disk


## Requirements
- Visual Studio 2019 (toolset v142) (Windows SDK 10)

## Thanks to the people who have helped the mod along the way
* GrimFlash
* KoviDomi
* Neptune
* Rouzel
* Dormin
* NeoStrayCat
* KDing
* PC_volt
* MorphRed
* Tadatys(sublimacija)
* Everybody in the BlazBlue PC community that has given support or feedback of any kind!

## Extra thanks to
Atom0s for their DirectX9.0 Hooking article<br>
Durante for their dsfix source

## Legal
```
BBCF Improvement Mod is NOT associated with Arc System Works or any of its partners / affiliates.
BBCF Improvement Mod is NOT intended for malicious use.
BBCF Improvement Mod is NOT intended to give players unfair advantages in online matches.
BBCF Improvement Mod is NOT intended to unlock unreleased / unpurchased contents of the game.
BBCF Improvement Mod should only be used on the official version that you legally purchased and own.

Use BBCF Improvement Mod at your own risk.
I, Microsoft Excel/libreofficecalc, am not responsible for what happens while using BBCF Improvement Mod. You take full reponsibility for any outcome that happens to you while using this application.
```
