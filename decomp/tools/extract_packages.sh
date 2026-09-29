#!/usr/bin/env bash
# Regenerate decomp/assets/USA/packages/ from DATA.BIN + SLUS_009.02 and
# re-render the splat overlay configs.
#
# Needs DATA.BIN and SLUS_009.02 from your own dump (not committed).
# Defaults: ../silent_bomber/{DATA.BIN,SLUS_009.02} relative to this repo;
# override with SB_DATA_BIN / SB_SLUS. Python: SB_PYTHON, else
# ../silent_bomber/venv/bin/python (needs splat64/spimdisasm/rabbitizer).
set -euo pipefail
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
PY="${SB_PYTHON:-$REPO/../silent_bomber/venv/bin/python}"
"$PY" "$REPO/decomp/tools/extract_packages.py" "$@"
"$PY" "$REPO/decomp/tools/gen_overlay_configs.py"
