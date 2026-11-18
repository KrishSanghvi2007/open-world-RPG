#!/bin/bash
# Restores engine-supplied content that git ignores (see docs/ASSETS.md).
# Safe to run again: it only copies when the folder is missing.
set -euo pipefail
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
REPO="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$UE/Templates/TemplateResources/High/Characters/Content"
DST="$REPO/Content/Characters"
if [ -d "$DST" ]; then
  echo "Content/Characters already present."
else
  cp -R "$SRC" "$DST"
  chmod -R u+w "$DST"
  echo "Restored Content/Characters from the engine."
fi
