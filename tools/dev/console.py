"""Runs one console command in the editor: run_script("dev/console.py", ["r.AllowOcclusionQueries 0"]).

Useful before taking pictures from many viewpoints: with occlusion queries on, the first frame from a new
viewpoint leaves out things that were hidden from the last one.
"""
import sys

import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
command = " ".join(sys.argv[1:])
unreal.SystemLibrary.execute_console_command(world, command)
print("ran:", command)
