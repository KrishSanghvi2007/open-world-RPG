"""Measures the frame rate over a stretch of play, while Play-In-Editor runs.

    run_script("dev/fps.py", ["start", "45"])    time every frame for the next 45 seconds
    ... leave the editor alone while the autopilot plays ...
    run_script("dev/fps.py", ["read"])           average, median and slowest tenth

Every frame's length is noted from inside the editor, so the result does not depend on when "read" is called.
Calling any tool while it measures stalls a frame or two; leave it be.
The editor caps play at about 62 frames a second, so 60 is "as fast as it can be shown here".
"""
import json
import os
import sys
import time

import unreal

RESULT = os.path.join(unreal.Paths.project_saved_dir(), "fps_result.json")
command = sys.argv[1] if len(sys.argv) > 1 else "read"

if command == "start":
    # The editor idles at a few frames a second when it is not the front window. Nobody is at the Mac for these
    # measurements, so that is switched off first, or the number means nothing.
    settings = unreal.find_object(None, "/Script/UnrealEd.Default__EditorPerformanceSettings")
    if settings:
        settings.set_editor_property("bThrottleCPUWhenNotForeground", False)
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 45.0
    if os.path.exists(RESULT):
        os.remove(RESULT)
    state = {"frames": [], "began": time.time(), "handle": None, "skip": 3}

    def tick(delta):
        if state["skip"] > 0:              # the frames this very call stalled
            state["skip"] -= 1
            state["began"] = time.time()
            return
        state["frames"].append(delta)
        if time.time() - state["began"] >= seconds:
            unreal.unregister_slate_post_tick_callback(state["handle"])
            times = sorted(state["frames"])
            n = len(times)
            slow = times[int(n * 0.9):] or times
            json.dump({"seconds": round(sum(times), 1), "frames": n, "average": round(n / sum(times), 1), "median": round(1.0 / times[n // 2], 1),
                       "slowest_tenth": round(len(slow) / sum(slow), 1), "worst_ms": round(times[-1] * 1000.0, 1)}, open(RESULT, "w"))

    state["handle"] = unreal.register_slate_post_tick_callback(tick)
    print("fps: measuring for %.0f s" % seconds)
elif os.path.exists(RESULT):
    r = json.load(open(RESULT))
    print("fps: %(average)s average, %(median)s median, %(slowest_tenth)s in the slowest tenth, worst frame %(worst_ms)s ms (%(frames)s frames over %(seconds)s s)" % r)
else:
    print("fps: still measuring")
