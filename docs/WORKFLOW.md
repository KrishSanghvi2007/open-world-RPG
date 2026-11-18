# How to work on Underpainting (owner's guide)

You're the creative director. Claude is the team. That's the whole workflow.

## Daily routine
1. Open **Terminal** and type `rpg`, then press Enter. (This shortcut opens the project and resumes your last conversation with Claude.)
2. Type `/ready`. Claude opens Unreal (and Blender if needed), connects everything, keeps the Mac awake, and gives you a checklist with at most one thing for you to do.
3. Tell Claude what you want, in plain words. Examples:
   - "What's the status?"
   - "Build the village greybox."
   - "The forest feels too bright; make it feel like dusk."
   - "Make the boss harder."
   - "Show me three ideas for the first quest's twist."
4. Claude asks you only for real decisions, with options and a recommendation. Pick one, or say "your call".
5. Claude saves and uploads (commits and pushes) the work itself. You never need git or GitHub Desktop.

## Other ways to talk to it
- **Claude desktop app → Code tab:** the same thing, with no Terminal needed. Pick this folder.
- **From your phone:** in a Mac session, type `/remote-control`, then continue from the Claude app on your phone.
- **Cloud sessions (claude.ai/code):** good for design, writing and planning without your Mac. Their work is merged automatically the next time the Mac session starts.

## Where to look (only if you're curious)
- `docs/STATUS.md`: what's done, what's next, what's waiting on you
- `docs/DECISIONS.md`: every decision, and the list of things Claude must ask you about
- `docs/GDD.md`: the game design
- `data/`: the game's content (pigments, enemies, quests…)

## If something breaks
Tell Claude what you see: paste the text, or a screenshot. It fixes things and can always roll back, because every step is saved in git.
