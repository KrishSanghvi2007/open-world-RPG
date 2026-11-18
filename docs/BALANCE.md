# Balance: Act 1, as measured

Written 2026-11-18, before the owner's first playtest. Everything here was measured by the game playing itself
(the autopilot, `tools/dev/golden_path.py`) or by simulation (`tools/sim/economy.py`). No person has played it yet,
so read these as a starting point to argue with, not as proof.

## How it was measured

- **The autopilot** plays with the same inputs a person has. Its one dial is *skill*: the share of enemy attacks it
  reads and dodges at the last moment (0.3 poor, 0.6 middling, 0.9 sharp). It never retreats, never uses a colour
  well, and fights with the starting knife. That makes it a pessimistic player in tactics and an optimistic one in reflexes.
- **Trials** (`golden_path.py trials <Enemy>`): the same fight against a fresh enemy several times over, in a test
  room, counting the damage taken. The player cannot die in a trial.
- **In place**: the real levels, start to finish, at skill 0.6.

## What the measuring found (and what changed because of it)

These mattered more than any number.

1. **Dodging did not work against long attacks.** An attack kept testing for a hit for as long as it was live. A
   dodge protects for 0.30 s; the boss's sweeps are live for 0.3 to 0.5 s. So a perfectly timed dodge was hit anyway
   when its protection ran out. In trials the boss did *more* damage to a sharp player than to a poor one.
   **Fix:** a dodged blow is spent. After the fix, damage taken from the boss fell by more than half and skill
   started to matter (table below).
2. **The Cartoon Gallery could not really be crossed.** A Flare lasts 8 s (two jumps of six), could not be lit
   again while burning, and the platform under you vanished the instant it ended. Earlier checks teleported across.
   **Fix:** a Flare can be renewed in its last 3 s (it gutters as the cue), and a sketched platform holds for 1.2 s
   after the light leaves it. The autopilot now crosses in 35 s, and back again after the Turning.
3. **Waking next to an enemy.** After dying to the boss you woke at the Lampstand before the Heart with a Blotter
   4 m away; idle, the player was killed in a loop. The same at the Rib Gate door. **Fix:** nothing hostile within
   15 m of any Lampstand or zone door, checked by the level builder every time it runs.
4. **The boss noticed you from 40 m**, which reached that same Lampstand. Now 15 m: it wakes when you step into its room.
5. **Retrying the boss with nothing to heal with.** Resting tops Moss Poultices up to 2; waking after a death did
   not. Now it does.
6. **Three at once.** The Hollow Vertebra's three Madderbacks stood together and always came together; the Approach
   had three more. Now they stand further apart than they notice at, so they can be drawn one at a time, and the
   Approach is a pair and a single. Groups also take turns: at most two attacking, never starting within 0.7 s of each other.
7. **Enemies in the pit gathered under you** as you crossed the gallery overhead, and mobbed whoever fell. Enemies
   now ignore anyone a storey above or below, and the pit's enemies stand at its walls beside what is worth finding
   there. A fall costs time and at most one fight, which is what the design says it should.
8. **The very first fight was two Blotters.** Now one, standing alone in the road in plain sight; the second waits
   at the First Well.

## Enemies: damage taken in one fight (starting knife, no colour, mean of 3 to 5 fights)

| Enemy | Fight lasts | Skill 0.3 | Skill 0.6 | Skill 0.9 |
|---|---|---|---|---|
| Blotter (before the dodge fix) | 5 s | 25 | 25 | 25 |
| Blotter (after) | 5 s | not re-run | not re-run | 0 |
| Madderback (before) | 7 to 8 s | 38 | 44 | 49 |
| Madderback (after) | 6 to 9 s | 27 | not re-run | 10 |
| Pentiment (before) | 7 s | 44 | 33 | 23 |
| Old Palimpsest (before; 600 Health) | 60 s | 487 (skill 0.5) | 581 (skill 0.7) | not run |
| Old Palimpsest (after the fix; 600 Health) | 62 to 71 s | 218 (skill 0.5) | 207 (skill 0.7) | 133 |

You have 100 Health and, rested, 2 Moss Poultices of 40: about 180 to spend. Read the last row against that.
One enemy alone is cheap (0 to 27). The cost of the forest is in pairs and in what you carry between rests.

**Numbers changed:** the Blotter's lunge 26 to 22 (it is the first attack most players ever take). The boss:
Health 600 to 480, every attack about 15% lighter (28/36/24/18/45 to 24/31/20/15/38), notice range 40 m to 15 m.
Nothing else: the standard enemies were fair alone once dodging worked.

## The boss, in place (skill 0.6)

| Boss numbers | Poultices | Result |
|---|---|---|
| Original (600 Health) | 2 | lost at 51 s |
| Original | few or none left (the bug in 5 above) | lost, lost |
| Health 540, damage -15% | 0 (same bug) | lost in phase 2 |
| Health 540, damage -15% | 2 | lost in phase 2, having dealt about 370 |
| **Health 480, damage -15% (now)** | 2 | **won in 53 s with 17 Health left** |

One win is one win: a small sample. What it supports is "a middling player with no colour and the first knife can
just do it". A player who brings Bone Black (its weakness: +30% damage, +50% stagger) or a better weapon, or who
has learned the attacks by the second try, has more room. If the owner beats it first time without trouble, put
some Health back first (540), then the damage.

## The forest at skill 0.6, in place

| Stretch | Health after |
|---|---|
| The lone Blotter in the road, then the one at the First Well | 100 |
| The Bend (one Madderback) | 100 |
| Rest at the Sunward Clearing | 100 |
| The Descent (two Blotters) | 87 |
| The Hollow Vertebra (three Madderbacks, drawn apart) | 87 |
| The Approach (two Madderbacks) and the last Blotters | 13, with poultices drunk |
| Rest at the Rib Gate | 100 |

Before the changes the same autopilot died at the First Well, and again at the Hollow Vertebra.
The last stretch (nine enemies between two Lampstands) is the forest's test. It was survived, narrowly.

## The pigment economy

`python3 tools/sim/economy.py` plays the act 2,000 times as three kinds of player. Each goes through once.

- **hurried**: finds half the harvest nodes, kills what stands in the road, never rests more than it must, dies 4 times.
- **typical**: finds three quarters, a few caches, rests once more per zone, dies 3 times.
- **thorough**: finds everything, rests twice more per zone (so nodes regrow twice), dies twice.

### What there is to spend pigment on (each once)

| | Ochre | TerreVerte | Sanguine | BoneBlack | Ceruse | SlumberBlue |
|---|---|---|---|---|---|---|
| Tier 1 attunements (6) | 3 | 3 | 3 | 3 | 3 | 3 |
| Tier 2 attunements (6) | 8 | 8 | 8 | 8 | 8 | 8 |
| Weapons, armour, the wick (5) | 6 | 4 | 3 | 4 | 0 | 0 |
| One batch of each consumable (5) | 1 | 1 | 2 | 0 | 1 | 1 |
| Bartram's limewash | 0 | 0 | 0 | 0 | 3 | 0 |
| **Everything** | **18** | **16** | **16** | **15** | **15** | **12** |

### What one pass through the act brings in (average of 2000 runs)

| Player | Ochre | TerreVerte | Sanguine | BoneBlack | Ceruse | SlumberBlue | Lost to deaths | Share of everything they could buy |
|---|---|---|---|---|---|---|---|---|
| hurried | 4.0 | 5.0 | 5.4 | 5.6 | 7.5 | 7.3 | 1.1 | 38% |
| typical | 12.0 | 14.8 | 11.9 | 16.6 | 8.0 | 9.2 | 4.7 | 77% |
| thorough | 24.0 | 29.5 | 21.5 | 35.3 | 9.4 | 11.5 | 7.2 | 93% |

### Is anyone starving?

- hurried: every first tier within reach but 0 (6 of 6); 0.0 Ochre left after what the first quest takes; scarcest is Ochre.
- typical: every first tier within reach but 0 (6 of 6); 8.0 Ochre left after what the first quest takes; scarcest is Ceruse.
- thorough: every first tier within reach but 0 (6 of 6); 20.0 Ochre left after what the first quest takes; scarcest is Ceruse.

**Reading it.** Nobody starves: even the hurried player can afford every first tier and what the main path asks
for. Nobody gets everything: the thorough player is still short of Ceruse (9 of 15) because it only drops from
Blotters, and of Slumber Blue (11.5 of 12) because it never regrows. The typical player ends with about three
quarters, which leaves real choices in the second tiers, and two pigments that are always tight:

- **Ceruse (8 of 15):** Bartram's limewash (3), the Erase stroke (3), Fixative Flasks, or tempering Bone Black's second tier. Not all of them.
- **Slumber Blue (9 of 12):** the Sfumato stroke (3), Blue Tincture, or tempering Ochre's second tier (2). Six for its own second tier is a decision.

Common pigments (Ochre, Terre Verte, Bone Black) are comfortable for anyone who rests and goes back. If the
owner wants them to bite too, the lever is the yield of a node (2 today), not the prices.

Dying costs little here (1 to 7 units over the act), because the rule only halves *wet* pigment and the first
death is free. That is the design working: the fear is real, the loss is small.

### Materials (what gear is made of)

| Material | Needed for everything | In plain sight or from the road | Hidden or off the road |
|---|---|---|---|
| Shed Hide | 8 | 3 (the Hem, the Descent) + 2 (Corin's quest) | 3 (behind one-sided walls) |
| Rib Splinter | 5 | 3 (the Bend, the Descent) | 2 in the pit, 3 behind one-sided walls |
| Tick Chitin | 4 | about 4 (Madderbacks) + 2 (Ilka's quest) | |
| Graphite Shard | 2 | about 1 (the Pentiment on the stair) | 1 in the pit, more from the pit's Pentiments |

Before this pass every Shed Hide and Rib Splinter in the act was behind a one-sided wall, so a player who missed
them could craft no gear at all. Five small caches now sit in plain sight at dead ends. A typical player can make
one weapon and the wick, or the cloak: a choice. The Bone Scraper is the reward for going down into the pit.

## Glim in the Vault (no light: Strokes cost full price)

| | Glim |
|---|---|
| Lantern, with Ochre in a well | 120 |
| A Flare | -20 |
| Crossing the gallery (measured: three Flares) | -60 |
| A Stroke | -6 (Glaze) to -20 (Erase) |
| A Blotter finished | +10 |
| A last-moment dodge | +5 |
| Lamp Oil | +40 (two from Hesper, one hidden in the forest, craftable) |
| Rest | full |
| The boss finished | +100 |

You rest at the Rib Gate (full), spend half crossing, and rest again before the Heart (full). Nobody can be
stranded: with no Glim and no oil, the pit's Blotters pay 10 each, and the way back to the Rib Gate Lampstand is open.

## How to run it again

- `python3 tools/sim/economy.py`
- In Unreal, Play in `Maps/Dev/L_Combat_Boss`, then `golden_path.py trials Blotter` (or `Madderback`, `Pentiment`, `ThePalimpsest`).
- The whole path: `golden_path.py village`, `forest`, `vault`, then `vault_home`, `forest_home`, `village_home`; `forest2` and `vault2` carry on from the last rest after a death.
