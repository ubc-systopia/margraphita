#!/bin/bash
# Load all remaining Aster artifact datasets into FlexoGraph (adj + split_ekey).
#
# Usage: ./load_all_datasets.sh
#
# Loads: wikitalk, cit-patents, wikipedia (in order of increasing size).
# Exits immediately on any failure.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOADER="$SCRIPT_DIR/load_aster_dataset.sh"

DATASETS=(wikitalk cit-patents wikipedia)

for ds in "${DATASETS[@]}"; do
  echo ""
  echo "============================================"
  echo "  Loading: $ds (adj + split_ekey)"
  echo "============================================"
  bash "$LOADER" "$ds"
done

echo ""
echo "=== All datasets loaded successfully ==="
