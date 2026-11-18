"""Development: what animation set-up does the player have, and can it play a one-off animation over it?"""
import sys
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
mesh = pawn.mesh
anim = mesh.get_anim_instance()
print("pawn class", pawn.get_class().get_name(), "| mesh", mesh.get_skeletal_mesh_asset().get_name() if mesh.get_skeletal_mesh_asset() else None, "| anim class", anim.get_class().get_name() if anim else None)
path = sys.argv[1] if len(sys.argv) > 1 else "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"
seq = unreal.load_asset(path)
print("sequence", seq.get_name(), "length", seq.get_play_length() if hasattr(seq, "get_play_length") else "?")
for slot in ("DefaultSlot", "UpperBody", "FullBody"):
    montage = anim.play_slot_animation_as_dynamic_montage(seq, unreal.Name(slot), 0.1, 0.1, 1.0)
    print(slot, "->", montage.get_name() if montage else None, "playing:", anim.montage_is_playing(montage) if montage else False)
    if montage:
        break
