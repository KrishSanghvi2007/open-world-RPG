"""Stops the editor idling at a few frames a second when it is not the front window (nobody is at the Mac
during unattended work, so it never is). Run once after the editor opens: run_script("dev/unthrottle.py", [])."""
import unreal

settings = unreal.find_object(None, "/Script/UnrealEd.Default__EditorPerformanceSettings")
if settings is None:
    print("unthrottle: settings object not found")
else:
    for name in ("throttle_cpu_when_not_foreground", "bThrottleCPUWhenNotForeground"):
        try:
            settings.set_editor_property(name, False)
            print("unthrottle: set", name, "to", settings.get_editor_property(name))
            break
        except Exception as error:
            print("unthrottle:", name, "failed:", str(error)[:120])
