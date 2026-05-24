# Unlimited Playback

Unlimited Playback is a BBCF Improvement Mod training feature that lets you keep and use more than the game's normal 4 playback slots.

The normal game only gives you 4 dummy playback slots at a time. Unlimited Playback adds a separate playback library where you can store many playback entries, organize them into profiles, export/import them, and play them back in training without permanently replacing all of your normal CF slots.

## What This Feature Does

Unlimited Playback lets you:

- Save recordings from the normal CF playback slots into a larger library.
- Keep more than 4 playback recordings available.
- Play a library entry immediately in training mode.
- Send a library entry back into one of the normal CF slots.
- Import and export individual `.playback` files.
- Save and load whole Unlimited Playback profiles as `.upl` files.
- Capture short input sequences from Replay Theater.
- Randomize or rotate through enabled playback entries.
- Trigger playback from training events such as wakeup, block, hit, throw tech, or a key/button press.

## Basic Idea

The feature works like this:

1. You still record dummy actions using the game's normal playback system.
2. You open the Unlimited Playback window.
3. You copy a normal CF slot into the Unlimited Playback library.
4. You repeat that as many times as you want.
5. You save the library as a profile if you want to keep it for later.
6. In training mode, you can play any saved entry directly or send it back into a normal CF slot.

Think of the normal 4 CF slots as temporary recording slots, and the Unlimited Playback library as your larger storage box.

## Opening Unlimited Playback

1. Launch BBCF with the mod installed.
2. Enter Training Mode.
3. Open the mod menu with `F1` unless you changed the menu bind.
4. Go to the `Playback` section.
5. Click `Unlimited Playback (BETA)`.

The popup can be opened outside training mode, but most runtime actions only work in the correct context.

## Adding Playbacks to the Library

### Add from CF Slot

Use this when you already recorded something into one of the game's normal 4 playback slots.

1. Record a dummy action into CF slot `1`, `2`, `3`, or `4`.
2. Open `Unlimited Playback (BETA)`.
3. Click `Add from CF Slot`.
4. Choose the CF slot number.
5. Give it a name.
6. Click `Save`.

The recording is copied into the Unlimited Playback library as a new entry.

This does not remove the original CF slot recording.

### Add from File

Use this to import a compatible Unlimited Playback `.playback` file.

1. Open `Unlimited Playback (BETA)`.
2. Click `Add from file`.
3. Select a `.playback` file.
4. If the file is compatible, it is added to the library.

Important: this is for Unlimited Playback `.playback` files. If a file was made by an older or different playback system, it may be rejected by the compatibility check.

### Add from Replay

Use this to create a playback entry by recording inputs from Replay Theater.

1. Go to Replay Theater.
2. Start watching a replay.
3. Open `Unlimited Playback (BETA)`.
4. Click `Add from replay`.
5. Choose `Record P1 Inputs` or `Record P2 Inputs`.
6. Let the replay play through the sequence you want to capture.
7. Click `Stop and Save`.
8. Give the entry a name.

Replay capture only works while a replay match is currently active.

## Using Library Entries

Each library entry has:

- A checkbox to enable or disable it.
- A name.
- A play button.
- An edit button.
- A delete button.
- A right-click menu with extra actions.

### Play an Entry Immediately

In Training Mode:

1. Open `Unlimited Playback (BETA)`.
2. Find the entry in the library.
3. Click the `>` play button.

The mod temporarily loads that entry into the runtime playback slot and starts it.

### Send an Entry to a CF Slot

Use this when you want the entry available through the normal CF playback slot system.

1. Right-click the library entry.
2. Choose `Send to CF Slot`.
3. Choose slot `1`, `2`, `3`, or `4`.
4. Click `Send`.

This writes the library entry into the selected normal CF slot.

This only works during a training match.

### Edit an Entry

Click the edit button on an entry to change:

- The entry name.
- Its random selection weight.
- Its playback data, through the playback editor.

The weight matters when using random selection. Higher weight means that entry is more likely to be chosen.

### Delete an Entry

Click the delete button on an entry and confirm.

Deleting removes it from the current library. If you already saved a profile before deleting, the old profile file still has the previous version until you save over it.

## Saving Your Library

Unlimited Playback entries are not something you should rely on keeping forever unless you save them.

Use profiles.

### Save a Profile

1. Open `Unlimited Playback (BETA)`.
2. Add the entries you want.
3. Configure selection and trigger settings if needed.
4. Click `Save` in the library section.
5. Save the file as a `.upl` profile.

A profile stores the library entries and their playback data.

### Load a Profile

1. Open `Unlimited Playback (BETA)`.
2. Click `Load`.
3. Select a `.upl` profile.

Loading a profile replaces the current Unlimited Playback library with the profile contents.

### Reset the Current Library

Click `Default` in the library section to clear the currently loaded library.

This clears the current Unlimited Playback setup in memory. It does not automatically delete profile files you already saved on disk.

## Exporting and Sharing Entries

### Export One Entry

1. Right-click a library entry.
2. Choose `Save to File`.
3. Save it as a `.playback` file.

You can send that `.playback` file to someone else.

### Import One Entry

1. Put the `.playback` file somewhere easy to find.
2. Open `Unlimited Playback (BETA)`.
3. Click `Add from file`.
4. Select the file.

If the file is compatible, it becomes a new library entry.

## File Locations

Unlimited Playback uses these folders inside the game directory:

```txt
BBCF_IM/unlimited_playbacks/profiles
BBCF_IM/unlimited_playbacks/imports
BBCF_IM/unlimited_playbacks/exports
BBCF_IM/unlimited_playbacks/library
```

Common file types:

```txt
.upl       Unlimited Playback profile
.playback  Single Unlimited Playback entry
```

For normal use, the most important thing is:

- Save `.upl` files when you want a full setup.
- Export `.playback` files when you want to share one entry.

## Selection Modes

Selection mode controls how the library chooses an entry when playback is triggered automatically.

### Random

Chooses randomly from the eligible enabled entries.

Entry weight affects the chance of being selected.

### Sequential

Goes through eligible entries in order.

This is useful when you want to practice against a fixed rotation.

### Non-repeating Random

Chooses randomly, but tries not to repeat the same entry until the current pool has been used.

This is useful when you want variety without getting the same option repeatedly.

## Auto-Mirror on Side Swap

`Auto-mirror on side swap` is enabled by default.

When enabled, the mod tries to mirror directional inputs if the dummy is on the opposite side compared to where the playback was recorded.

Example:

- You record a left/right mixup with the dummy on one side.
- Later, the dummy is on the other side.
- Auto-mirror helps the recorded directions still make sense.

If you turn this off, the playback is loaded using the current facing side instead of mirroring the input sequence.

## Trigger Types

Unlimited Playback can trigger entries automatically in Training Mode.

Only one trigger type is selected in the UI at a time.

### Wakeup

Triggers around dummy wakeup situations.

Use this for practicing okizeme, meaty timing, wakeup options, and post-knockdown situations.

### Gap

Triggers when the dummy enters a guard/blockstun gap-style detection state.

Use this for practicing pressure gaps and defensive playback responses.

### On Block

Triggers when the dummy reaches a blocking loop state.

Use this for practicing block pressure responses.

### On Hit

Triggers when the dummy is in hitstun or related hit reactions.

Use this for practicing hit-confirm or combo-response situations.

### Throw Tech

Triggers around throw tech detection.

Use this for practicing post-throw-tech situations.

### Key Press

Triggers when you press a mapped key or controller button.

Use this when you want manual control over when the library chooses and plays an entry.

To set it:

1. Select `Key Press` as the trigger type.
2. Click `Map Playback Bind`.
3. Press the key or controller button you want to use.

## Cooldown Frames

Each trigger has a `Cooldown Frames` value.

This prevents the same trigger from firing again immediately.

A low cooldown makes the feature respond quickly. A higher cooldown prevents repeated activations when the same situation lasts multiple frames.

The minimum is `1`.

## Fix Triggers

Use `Fix Triggers` if automatic triggers temporarily stop working after a training reset or state change.

It clears trigger cooldown and edge-detection state.

This is mainly a recovery button for when the game state changes but the trigger logic still thinks an old condition is active.

## Practical Workflows

### Build a Big Defensive Playback Library

1. Go to Training Mode.
2. Record a defensive option into CF slot 1.
3. Open Unlimited Playback.
4. Click `Add from CF Slot`.
5. Name the entry, such as `DP`, `Backdash`, `Throw Tech`, or `Jump`.
6. Repeat for as many options as you want.
7. Set selection mode to `Random` or `Non-repeating Random`.
8. Save the profile.

Now you can keep far more than 4 options in one setup.

### Practice Against Random Wakeup Options

1. Record several wakeup options into CF slots and add each one to the library.
2. Enable the entries you want.
3. Set selection mode to `Random` or `Non-repeating Random`.
4. Set trigger type to `Wakeup`.
5. Set a reasonable cooldown.
6. Save the profile.

Now the dummy can choose from your larger wakeup library instead of being limited to 4 slots.

### Practice a Specific Set of Responses

1. Add several entries to the library.
2. Disable any entries you do not want for this drill.
3. Set selection mode to `Sequential`.
4. Use `Play` manually or configure a trigger.

This is useful for checking one option at a time without deleting anything.

### Share a Setup With Someone Else

To share the whole setup:

1. Save the library as a `.upl` profile.
2. Send the `.upl` file.

To share only one playback:

1. Right-click the entry.
2. Choose `Save to File`.
3. Send the `.playback` file.

## Scope and Limitations

Unlimited Playback is a training/lab feature.

Current limitations:

- Runtime playback actions only work in Training Mode.
- `Add from replay` only works during an active Replay Theater match.
- Normal CF slots still exist and are still limited to 4.
- Unlimited Playback does not permanently expand the game's built-in slot UI.
- Entries should be saved to a `.upl` profile if you want to keep them.
- Individual `.playback` import expects a compatible Unlimited Playback file format.
- Each playback is capped at about 1200 frames.
- The feature is marked BETA.
- Automatic trigger behavior depends on the current training state and may need `Fix Triggers` after resets.
- Some complex recordings may still behave differently depending on character side, spacing, or game state.
- This is separate from Unlimited Replay Takeover.

## Troubleshooting

### The Play Button Is Disabled

You are probably not in a Training Mode match.

Enter Training Mode, start the match, then try again.

### Add from Replay Is Disabled

You need to be inside an active Replay Theater match.

Start watching a replay first.

### My Library Disappeared After Restart

You probably did not save a profile.

Open Unlimited Playback, build your library, then click `Save` and save a `.upl` file.

### A Playback File Will Not Import

The file may not be an Unlimited Playback `.playback` file, or it may be from an incompatible format version.

If the compatibility popup appears, only use `Import Anyway` when you trust the file and understand it may not work correctly.

### The Wrong Direction Comes Out

Check `Auto-mirror on side swap`.

If the dummy changed sides compared to the original recording, auto-mirror usually helps. If your setup intentionally depends on raw directions, try turning it off.

### Automatic Triggers Stopped Working

Click `Fix Triggers`.

If that does not help, leave and re-enter training mode, then load the profile again.

### The Entry Plays, But Not Like I Expected

Check these things:

- Was the original CF slot recorded correctly?
- Was the dummy on the same side?
- Is `Auto-mirror on side swap` right for this recording?
- Is the playback longer than expected?
- Did you save the correct profile after editing?
- Are you testing in Training Mode, not Replay Theater?

## Quick Start

If someone just wants to use the feature:

1. Go to Training Mode.
2. Record something into CF playback slot 1.
3. Open the mod menu with `F1`.
4. Open `Playback`.
5. Click `Unlimited Playback (BETA)`.
6. Click `Add from CF Slot`.
7. Choose slot `1`, name it, and save.
8. Repeat for more recordings.
9. Click the `>` button next to an entry to play it.
10. Click `Save` to save the library as a `.upl` profile.

That is the core loop: record into one of the normal 4 slots, copy it into Unlimited Playback, then keep building your library.
