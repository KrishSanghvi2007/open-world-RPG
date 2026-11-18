"""Brings the game's sounds into Unreal. Run inside the editor (through the dev toolset's run_script):

    audio/import_audio.py

Imports every sound that data/sounds.json names and nothing else:
  /Game/ThirdParty/Audio/Kenney/...   from Saved/ThirdParty/kenney (run tools/audio/fetch_kenney.py first); not kept in git
  /Game/Audio/Made/...                from art_source/audio (tools/audio/make_sounds.py); ours, kept in git
Rows whose name starts with "amb_" or "rumble" loop.
"""

import json
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
PACKS = {"ImpactSounds": "impact-sounds", "RPGAudio": "rpg-audio", "InterfaceSounds": "interface-sounds"}


def source_for(asset_path):
    folder, name = asset_path.rsplit("/", 1)
    name = name.split(".")[0]
    if folder.startswith("/Game/ThirdParty/Audio/Kenney/"):
        pack = folder.rsplit("/", 1)[1]
        return os.path.join(PROJECT, "Saved", "ThirdParty", "kenney", PACKS[pack], "Audio", name + ".ogg"), folder, name
    return os.path.join(PROJECT, "art_source", "audio", name + ".wav"), folder, name


def main():
    rows = json.load(open(os.path.join(PROJECT, "data", "sounds.json")))
    loops = set()
    wanted = {}
    for row in rows:
        for asset in row["Assets"]:
            wanted[asset] = source_for(asset)
            if row["Name"].startswith("amb_"):
                loops.add(asset)
    tasks, missing = [], []
    for asset, (source, folder, name) in sorted(wanted.items()):
        if not os.path.exists(source):
            missing.append(source)
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source)
        task.set_editor_property("destination_path", folder)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    imported = 0
    for asset in wanted:
        wave = unreal.load_asset(asset.split(".")[0])
        if wave:
            imported += 1
            if asset in loops and not wave.get_editor_property("looping"):
                wave.set_editor_property("looping", True)
                unreal.EditorAssetLibrary.save_loaded_asset(wave)
    print("imported %d of %d sounds; missing source files: %d" % (imported, len(wanted), len(missing)))
    for path in missing[:8]:
        print("  missing", path)


main()
