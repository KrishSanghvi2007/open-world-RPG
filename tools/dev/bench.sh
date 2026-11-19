#!/bin/bash
# Measures the frame rate of the game on its own (not inside the editor), drawing exactly 1920 x 1080 pixels.
# This Mac's screen cannot fit a 1080p window, so the game draws 1920 x 1080 and shows it in a 1280 x 720 window
# (screen percentage 150). The work done is the same as a 1080p screen's; the result file reports the window's size.
#
#   tools/dev/bench.sh <level> "<autopilot script>" [seconds]
#   tools/dev/bench.sh L_Matriarch_Village_A "walk -22 5 4; walk -22 19 8; walk 0 30 8" 40
#
# Close the Unreal editor first: two copies fight over the graphics chip and both numbers come out wrong.
# The game plays the script, times every frame after an 8 second warm-up, writes Saved/Bench/<level>.json and quits.
# The screen's own limit is switched off for the test, so the number shows how much is in hand (in normal play the
# game keeps step with the screen). The level must be one the save allows: with no save, that is the A versions.
# Run it twice the first time a level has changed: the first run is still compiling the paint.
# To try a rendering setting without changing the project: BENCH_CVARS="r.AntiAliasingMethod 2, r.SSR.Quality 0" tools/dev/bench.sh ...
set -e
cd "$(dirname "$0")/../.."
LEVEL="$1"; SCRIPT="$2"; SECONDS_TO_RUN="${3:-40}"
rm -f "Saved/Bench/$LEVEL.json"
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$PWD/Underpainting.uproject" \
  "/Game/Maps/Matriarch/$LEVEL" -game -windowed -ResX=1280 -ResY=720 "-ExecCmds=r.ScreenPercentage 150, r.VSync 0, t.MaxFPS 0${BENCH_CVARS:+, $BENCH_CVARS}" -nosplash -unattended -nosound \
  "-Autopilot=\"$SCRIPT\"" "-Bench=$SECONDS_TO_RUN" > /dev/null 2>&1 || true
cat "Saved/Bench/$LEVEL.json" 2>/dev/null || echo "bench: no result for $LEVEL (did the game start?)"
