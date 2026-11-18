# Devlog 008: crafting, gear and the palette

2026-11-18. Milestone **M7**. Everything on the two crafting stations can be made and does something, and all twelve attunement nodes can be bought.

## What is new

- **All 10 recipes craft**, five at the Grinding Slab and five at the Armature Bench.
- **Gear.** Mahlstick, Bone Scraper, Moss Cloak and Chitin Jerkin can be made and worn. New gear goes on as soon as you make it. Open your pack (**Tab**) and press a number to change back. What you wear is saved.
- **Braided Wick.** A one-off upgrade: +20 maximum Glim for good. It costs the same 4 Ochre an attunement would.
- **Two new potions.**
  - **K**, Sanguine Draught: +20% damage for 30 seconds.
  - **L**, Blue Tincture: you weigh less for 20 seconds, so jumps carry further.
- **Second-tier attunements now do their second thing**: Scumble gives back 25 Stamina, Sfumato slows for 5 seconds, a kill with Alla Prima refunds its Glim, and Clean Ground eases the lead toll from 15 Health to 10.
- **A debug key, F7**: fills your pack with 20 of everything and teaches every recipe. It is there so you can try builds without grinding. It will not ship.

![The Armature Bench](img/008-armature-bench.png)
*The Armature Bench after making one of everything. It refuses a second of each because you can only carry one. (The pop-up notices sitting on top of the menu are a known mess for the UI milestone.)*

## How it was checked

- 10 automated tests pass. The new one fills a pack, crafts all ten recipes, checks the gear goes on and can be swapped, and buys all twelve attunement nodes in order.
- In the running game, from a debug-filled pack:
  - All five Slab recipes and all five Bench recipes crafted, with the right amounts spent.
  - Maximum Glim went from 100 to 120 with the Braided Wick.
  - The Bone Scraper and Chitin Jerkin went on (damage taken down 15%).
  - The Draught started its 30 seconds and the Tincture changed how you fall.
  - Attuning Terre Verte and Bone Black filled both palette wells and raised Stamina recovery by 20%.

**Not checked in play:** changing gear from the pack screen with the number keys (the test covers the rule, not the keys), the Moss Cloak's Stamina bonus, and whether the Draught's extra damage shows on a hit. Nobody has yet *felt* two builds against each other; that is yours to judge (see the morning notes).

**Not built:** three small attunement effects that need things the game does not have yet.
- Golden Hour's "Glazed enemies light the room more": Glaze does not light the room at all yet.
- Lamp-Black Weight's "cannot be flinched out of an Impasto": the player is never flinched.
- Blank Slate's "harmful effects last half as long": no enemy puts a lasting effect on you yet.

## Test it (5 minutes)

1. Start in the village and press **F7**.
2. Use the Grinding Slab (left of the square) and press 1 to 5. Use the Armature Bench (right) and press 1 to 5.
3. Press **Tab** to see your pack and gear. Press a gear number to swap.
4. Rest at the Lampstand and attune two colours. Go and hit something. Come back, pick two different colours, and compare.
