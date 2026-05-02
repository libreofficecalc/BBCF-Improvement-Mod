# BBCF Ranked LP Guide

This is a player-facing explanation of how BBCF ranked progression appears to work.

The short version: ranked is not just one LP number. The game also keeps hidden promotion and demotion counters at some ranks. Those counters explain cases where you rank up or down before the LP bar reaches the obvious endpoint.

Each character has separate ranked progress.

## Ranked Prediction Window

The mod can show a ranked prediction window during ranked match confirmation.
This is the popup state after selecting a ranked opponent, before both players
confirm with OK.

The window shows the opponent name, opponent rank, and two outcomes:

- **Win**: predicted LP gain, or `RANK UP` when the current rules predict a rank-up.
- **Loss**: predicted LP loss, or `RANK DOWN` when the current rules predict a rank-down.

When a rank change is predicted, the window explains which rule caused it:

- `Automatic Promotion Reached`: hidden promotion counter reaches its limit.
- `LP Threshold Reached`: raw LP reaches the rank's upper or lower bound.
- `Automatic Demotion Reached`: hidden demotion counter reaches its limit.

For Leader and higher ranks, a lower-ranked opponent can leave you capped at
the top of the current rank without allowing a rank-up. In that case the win
prediction says `Nothing.` and explains that you can only rank up against your
rank or higher.

## Common Rules

- A rank change resets LP to the middle of the new rank.
- A rank-up resets promotion and demotion counters.
- A rank-down resets promotion and demotion counters.
- Same-rank wins and losses are worth about `1024` raw LP after LV10.
- Bigger rank gaps reduce LP loss (-1024 on same rank, halved per rank difference. aka -512 | -256 | -128, etc etc.). Losing to someone far away from your rank can cost 1 LP. Crazy i know.
- Lower-rank wins give reduced LP. (Same rule, +1024 on same rank, then halved per rank difference. aka +512 | +256 | +128, etc etc.)
- "Promotion counter" means hidden progress toward a rank-up from qualifying wins.
- "Demotion counter" means hidden strikes toward a rank-down from qualifying losses.

## LV1-LV10

This is the beginner bucket.

Wins:

- Each win gives `+100` LP.
- You rank up when LP reaches the top of the current rank.

Losses:

- Current code does not show normal LP loss or rank-down behavior here.

Counters:

- No promotion counter.
- No demotion counter.

## LV11-LV18

Normal ranked LP starts here.

Wins:

- Same-rank win gives `+1024` LP.
- Win against lower rank gives less. (+512 | +256 | +128 | +64 | +32 | +16 | +8 | +4 | +2 | +1)
- Win against higher rank gives `+1024` LP + roughly `+25%` multiplier per rank above you. So you can have more than 1024.

Losses:

- Same-rank loss costs about `-1024` LP. 
- Loss against a different rank costs less as rank gap gets bigger. (-512 | -256 | -128 | -64 | -32 | -16 | -8 | -4 | -2 | -1)
- You rank down if LP reaches the bottom of the rank.

Promotion counter:

- Promotion counter is now active.
- Qualifying wins (when the opponent is within 2 ranks of you) can add hidden LP promotion-counter progress.
- This hidden gain is not always the same as LP gain. Same-rank wins add `+1024`, one-rank-lower wins add `+686`, and two-ranks-lower wins add `+459`.
- Wins against one/two ranks higher in this bucket add much more hidden promotion progress: `+2048` / `+4096`.
- If this counter fills to its internal rank-dependant limit, you can rank up before the LP bar hits the top.

Demotion counter:

- Not active.

## LV19-LV24

This mostly works like LV11-LV18, but promotion requirements are bigger.

Wins:

- Same rank LP gain based on downward rank difference. Including the `+25%` multiplier per rank above you. (+1024 | +512 | +256 | +128 | +64 | +32 | +16 | +8 | +4 | +2 | +1)

Losses:

- Same rank LP loss based on rank difference. (-1024 | -512 | -256 | -128 | -64 | -32 | -16 | -8 | -4 | -2 | -1)

Promotion counter:

- Still active for wins against opponents within 2 ranks.
- One-rank-lower wins add `+686` hidden promotion progress, and two-ranks-lower wins add `+459`, even though LP gain is only `+512` / `+256`.
- Ceiling/trigger point is higher than earlier ranks.

Demotion counter:

- No demotion counter technically (i say technically because internally it has a value but it's not used)

## LV25-LV29

Demotion counter starts mattering.

Wins:

- Win LP is now hard capped at `+1024`. No more +25%. The halving per rank difference still applies. (+1024 | +512 | +256 | +128 | +64 | +32 | +16 | +8 | +4 | +2 | +1)

Losses:

- Same rank LP loss based on rank difference. (-1024 | -512 | -256 | -128 | -64 | -32 | -16 | -8 | -4 | -2 | -1)
- Rank-down can happen from LP floor or demotion counter.

Promotion counter:

- Still active.
- Builds on wins against opponents within 2 ranks.
- Hidden promotion progress uses `+1024` for same-rank or higher-ranked wins, `+686` for one-rank-lower wins, and `+459` for two-ranks-lower wins.

Demotion counter:

- Active.
- Same-rank losses add 1 demotion strike.
- At 5 strikes, you rank down even if LP is not at the floor.
- Wins reset demotion strikes only if opponent is LV23 or higher.

## LV30-LV35

High-rank demotion rules get stricter.

Wins:

- Same-rank or higher-rank wins can advance LP and trigger rank-up.
- Wins against lower-ranked opponents can still count in match stats and can leave LP capped, but they do not trigger rank-up from this named-rank bucket.
- LP gain against lower ranks still scales down. (+1024 | +512 | +256 | +128 | +64 | +32 | +16 | +8 | +4 | +2 | +1)

Losses:

- Same rank LP loss based on rank difference. (-1024 | -512 | -256 | -128 | -64 | -32 | -16 | -8 | -4 | -2 | -1)
- Rank-down can happen from LP floor or demotion counter.

Promotion counter:

- Still active.
- Builds on wins against opponents within 2 ranks.
- LV34 and LV35 have larger promotion-counter targets.
- Same hidden gain pattern as LV25-LV29: `+1024` same/higher, `+686` one lower, `+459` two lower.

Demotion counter:

- Active.
- Now losses against opponents within 2 ranks add 1 demotion strike.
- At 5 strikes, you rank down even if LP is not at the floor.
- Wins reset demotion strikes only if opponent is LV25 or higher.

## Leader-Hero

This bucket starts after LV35.

Wins:

- Same rank LP gain based on downward rank difference. (+1024 | +512 | +256 | +128 | +64 | +32 | +16 | +8 | +4 | +2 | +1)

Losses:

- LP loss still scales down with rank gap. (-1024 | -512 | -256 | -128 | -64 | -32 | -16 | -8 | -4 | -2 | -1)
- Rank-down can happen from LP floor or demotion counter.

Promotion counter:

- Nah. Not anymore. Do it the old fashioned way.

Demotion counter:

- Active.
- Losses against LV30 or higher add 1 demotion strike.
- At 5 strikes, you rank down.
- Wins against LV25 or higher reset demotion strikes.

## Kisshin-Ruler

This bucket is still being live-validated, but current code shows these rules.

Wins:

- Same rank LP gain based on downward rank difference. (+1024 | +512 | +256 | +128 | +64 | +32 | +16 | +8 | +4 | +2 | +1)

Losses:

- LP loss still scales down with rank gap. (-1024 | -512 | -256 | -128 | -64 | -32 | -16 | -8 | -4 | -2 | -1)
- Rank-down can happen from LP floor or demotion counter.

Demotion counter:

- Active.
- Losses against LV25 or higher add 1 demotion strike.
- Losses against LV24 or lower likely do not add a demotion strike.
- At 5 strikes, you rank down.

Resetting demotion counter:

- Hades and Ruler appear to need wins against LV30 or higher to reset demotion strikes.
- Kisshin appears to use the previous bucket's LV25-or-higher reset rule.
- This named-rank area still needs more live testing.
- Hades has no promotion counter. If the mod shows Hades at `305128 / 305128 LP`, that is cumulative UI LP for Hades' raw upper bound (`48127`). A full Hades bar is valid if the player has not won against Hades-or-higher since reaching that cap.




## Still Being Verified

- Exact named-rank behavior above Ruler.
- Exact opponent-rank requirement for every named-rank edge above Ruler.
- Whether every online result path passes opponent rank the same way.
- How often promotion-counter rank-ups happen in real player history.
- Whether draw/no-result paths only reset counters and never change LP.
