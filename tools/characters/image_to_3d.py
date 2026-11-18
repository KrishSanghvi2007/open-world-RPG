"""Step 2 of /make-character: turn the painted front view into a rough 3D shape with a free online service.

    python tools/characters/image_to_3d.py <id> [--service hunyuan] [--views]

Uses free Hugging Face Spaces through gradio_client, with no login (anonymous). Nothing about the owner or the
project is sent: only the picture. Free anonymous use is rationed by the hour and the day; if the service says the
quota is used up this stops with the message QUOTA and nothing is spent. Never pay to get past it.

  hunyuan   tencent/Hunyuan3D-2.1, shape only (the fastest thing it offers; we make our own materials)
            With --views it also sends back.png, left.png and right.png from art_source/characters/<id>/ if they exist.

Writes art_source/characters/<id>/raw/<service>.glb and a line in raw/log.json saying what was asked for.
Needs gradio_client (free; `pip install gradio_client`).
"""

import json
import os
import shutil
import sys
import time

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))


def hunyuan(folder, use_views, seed):
    from gradio_client import Client, handle_file

    client = Client("tencent/Hunyuan3D-2.1", verbose=False)

    def view(name):
        path = os.path.join(folder, name + ".png")
        return handle_file(path) if use_views and os.path.exists(path) else None

    front = handle_file(os.path.join(folder, "front_white.png"))
    multi = use_views and any(view(n) for n in ("back", "left", "right"))
    result = client.predict(
        image=None if multi else front, mv_image_front=front if multi else None, mv_image_back=view("back"),
        mv_image_left=view("left"), mv_image_right=view("right"), steps=30, guidance_scale=5.0, seed=seed,
        octree_resolution=256, check_box_rembg=True, num_chunks=8000, randomize_seed=False, api_name="/shape_generation")
    mesh = result[0]
    mesh = mesh.get("value", mesh.get("path")) if isinstance(mesh, dict) else mesh
    return mesh, {"space": "tencent/Hunyuan3D-2.1", "endpoint": "/shape_generation", "steps": 30, "octree_resolution": 256,
                  "seed": seed, "views": ["front"] + [n for n in ("back", "left", "right") if view(n)], "stats": result[2]}


def main(args):
    char_id = args[0]
    service = args[args.index("--service") + 1] if "--service" in args else "hunyuan"
    seed = int(args[args.index("--seed") + 1]) if "--seed" in args else 1234
    folder = os.path.join(ROOT, "art_source", "characters", char_id)
    os.makedirs(os.path.join(folder, "raw"), exist_ok=True)
    started = time.time()
    try:
        mesh, record = {"hunyuan": hunyuan}[service](folder, "--views" in args, seed)
    except Exception as error:      # the client raises its own types; the message is what matters
        text = str(error)
        if "quota" in text.lower() or "exceeded" in text.lower():
            print("QUOTA:", text[:300])
            sys.exit(3)
        print("FAILED:", type(error).__name__, text[:500])
        sys.exit(2)
    suffix = "_views" if "--views" in args else ""
    out = os.path.join(folder, "raw", service + suffix + os.path.splitext(mesh)[1])
    shutil.copy(mesh, out)
    record.update({"seconds": round(time.time() - started, 1), "file": os.path.relpath(out, ROOT), "when": time.strftime("%Y-%m-%d %H:%M")})
    log_path = os.path.join(folder, "raw", "log.json")
    log = json.load(open(log_path)) if os.path.exists(log_path) else []
    log.append(record)
    json.dump(log, open(log_path, "w"), indent=2)
    print("wrote", out, "in", record["seconds"], "s", json.dumps(record.get("stats"))[:200])


if __name__ == "__main__":
    main(sys.argv[1:])
