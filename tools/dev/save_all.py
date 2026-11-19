"""Saves every changed level and asset in the editor: run_script("dev/save_all.py", [])."""
import unreal

ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("save_all:", ok)
