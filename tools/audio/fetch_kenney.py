"""Downloads the free sound packs the game uses from kenney.nl (all Creative Commons Zero) and unpacks them into
Saved/ThirdParty/kenney/. Run it once on a new machine, then run tools/audio/import_audio.py inside Unreal.

    python3 tools/audio/fetch_kenney.py

The packs are not kept in git (see docs/ASSETS.md). Standard library only.
"""

import io
import json
import os
import re
import sys
import urllib.request
import zipfile

PACKS = ["impact-sounds", "rpg-audio", "interface-sounds"]
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Saved", "ThirdParty", "kenney")
AGENT = {"User-Agent": "Mozilla/5.0"}


def get(url, referer=None):
    headers = dict(AGENT)
    if referer:
        headers["Referer"] = referer
    with urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=120) as response:
        return response.read()


def main():
    os.makedirs(ROOT, exist_ok=True)
    for pack in PACKS:
        target = os.path.join(ROOT, pack)
        if os.path.isdir(os.path.join(target, "Audio")):
            print(pack, "already here")
            continue
        page = "https://kenney.nl/assets/" + pack
        html = get(page).decode("utf-8", "replace")
        if "CC0" not in html:
            sys.exit("%s: the page no longer says CC0. Check the licence by hand before using it." % pack)
        found = re.search(r"data-download-id='([0-9a-f]+)'", html)
        if not found:
            sys.exit("%s: no download link on the page" % pack)
        link = json.loads(get("https://kenney.nl/download-link/" + found.group(1), page))
        data = get(link.get("url") or link.get("link"), page)
        zipfile.ZipFile(io.BytesIO(data)).extractall(target)
        print(pack, "downloaded,", len(data) // 1024, "KB")


if __name__ == "__main__":
    main()
