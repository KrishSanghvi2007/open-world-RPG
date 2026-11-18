# Devlog 014: making it feel like something, and making it fair

2026-11-18. Not a milestone: a pass over the whole of Act 1 before the owner plays it for the first time.
Ten steps, in two halves. No new story, no new models, no environment art.

**Watch first:** [`clips/014-fight-before-after.mp4`](clips/014-fight-before-after.mp4), 24 seconds. The same
scripted fight, the same button presses at the same moments, before and after.

## Part A: feel

![A hit landing](img/014-hit-feedback.png)
*A Pentiment's sweep about to land (the orange mark on the floor shows where), paint flying off it from the
last blow, the damage number, the enemy's two bars.*

1. **Animation.** The engine's free mannequin animations drive the player: three swings that chain, a charged
   swing, a dash, flinches, deaths. Enemies have no models yet, so their placeholder shapes gather, strike, rock
   back and fall on the same clock as the rules.
2. **Hit feedback.** A hit freezes both fighters for a few hundredths of a second, shakes the camera, knocks the
   enemy back, throws paint in the enemy's colour, and floats the number. Your bars leave a pale trail when they
   drop and jolt red when you ask for Stamina or Glim you do not have.
3. **Sound.** 46 events (swings, impacts, footsteps per zone, dodges, menus, wind and room tone), from three free
   CC0 packs by Kenney and 22 sounds made by script. **I cannot hear. Nobody has listened to this yet.**
4. **Responsiveness.** A press made a little early is remembered. A swing can be cut short by a dodge, but never
   its wind-up. You keep your footing for a moment past a ledge. The camera trails and, locked on, sits over your shoulder.
5. **Clarity.** Every menu row can be clicked. A mark grows on the ground where an enemy's attack will land. A
   red flag at the screen's edge points at what hit you.

## Part B: playable

![A teaching prompt](img/014-prompt.png)
*One line, once, when it first matters: here, just after the first harvest.*

6. **Onboarding.** 22 one-line prompts (`data/hints.json`), each shown once at the moment the thing it teaches
   first matters, naming your own keys. A thing you have already done is never taught. Nobody needs the design document.
7. **Wayfinding.** The journal line was always there. Now a small marker hangs over the thing the current step is
   about, or over the door that leads to it when it is in another zone (it can be switched off). Lit Lampstands
   shine twice as far. Five dead ends that had nothing in them have a small cache.
8. **Pacing.** The act written down as 23 beats (`docs/PACING.md`), and its enemies moved so the beats are what
   they say: one enemy before two, a new kind met alone, the jumping puzzle without a fight on top of it, a safe
   place to rest before the boss.
9. **Difficulty and balance.** Measured, not guessed (`docs/BALANCE.md`): the game fought each enemy over and over
   at three levels of skill, and the pigment economy was played 2,000 times.
10. **Robustness.** You wake where you last rested *or arrived*. What you have done is saved as you go. A fall
    with no bottom puts you back. The game played its own main path from Hesper's first line to "Quest complete".

![Crossing the Cartoon Gallery](img/014-gallery.png)
*Mid-jump over the pit. The lit Lampstand on the far ledge is the brightest thing in the room, with the marker
over it. The platform behind is solid because the Flare is burning.*

## What playing it found

The useful part of this pass was not the polish. It was that the game, played by itself, kept dying in places a
person would have too, and each time the reason was a real fault.

- **Dodging did not protect you from any attack longer than the dodge.** The boss hit a sharp player as often as
  a poor one. A dodged blow is now spent.
- **The Cartoon Gallery, which the main quest must cross, could not honestly be crossed.** Eight seconds of light
  for six jumps, no way to relight, and the floor vanished the instant the light did. Every earlier check had
  teleported over it. A Flare can now be renewed as it gutters, and platforms hold a moment after the light goes.
- **You could wake up being killed.** After losing to the boss you woke with a Blotter four metres away. Every
  Lampstand and every zone door is now clear for 15 metres, and the level builder refuses to build one that is not.
- **The boss could see the Lampstand you rest at before fighting it.** Its notice range came in from 40 m to 15.
- **Closing the game lost everything since your last rest.** Now nothing.
- Three enemies arriving as one in two places; enemies gathering under you as you crossed above them; retrying
  the boss with no poultices. All in `docs/BALANCE.md`.

![The boss](img/014-boss.png)
*Old Palimpsest in its second phase. Locked on, the camera sits very close to something this big: one for the
owner to judge.*

## Numbers

- Main path at the autopilot's pace: **11 min 26 s**. Estimate for a first-time player: **33 to 48 minutes**; about an hour with the side stories.
- The boss, final numbers, middling skill, the first knife, no colour: **won in 53 seconds with 17 Health left.** One run.
- A typical player ends the act able to afford about **three quarters** of everything pigment buys. Ceruse and Slumber Blue are always short.
- Frame rate: **55 on average** in the editor's play window (median 57, slowest tenth 52), with the editor,
  Blender and the test tools all running and the Mac swapping. I could not confirm a steady 60 and did not measure a packaged game.
- 12 automated tests pass.

## Not done, and not hidden

- No person has played any of this. No one has heard the sound. The mouse was checked by one scripted click.
- The main path was run in nine pieces with fixes between them, not in one sitting. The enemy moved onto the Rib
  Stair and the one moved at the Rib Gate were placed after the runs that passed them.
- Only the third ending at the Heart wall was replayed in this pass.
- The prompts for lock-on, low Glim and the second palette well never came up in the runs (the autopilot locks on
  by itself and never fills a second well).
- "I am stuck" in Settings is built and untested. The fall rescue was seen working once.

## Free things used

Kenney's Impact Sounds, RPG Audio and Interface Sounds (CC0), and Unreal's own mannequin animations. Nothing paid.
Everything is listed in `docs/ASSETS.md`.
