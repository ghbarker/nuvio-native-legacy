#!/usr/bin/env bash
set -euo pipefail
plugin_bundle="$(cd "$(dirname "$0")" && pwd)"
plugin_product=""
plugin_output=""
while [[ $# -gt 0 ]]; do
  case "$1" in --root) plugin_product="$2"; shift 2;; --output) plugin_output="$2"; shift 2;; *) exit 2;; esac
done
test -n "$plugin_product" && test -n "$plugin_output"
mkdir -p "$plugin_output"
PRELOAD_PLUGINS_SDK="$plugin_bundle/../controller/sdk" bash "$plugin_bundle/tests/plugins_snapshot.sh" --root "$plugin_product"
# Actual vendored QuickJS and HTTP foreground fixture, exact original sources
# plus the bounded new-registry/credential-linkage adapter only.
PLUGINS_PRODUCT_ROOT="$plugin_product" PLUGINS_LEGACY_ADAPTER="$plugin_bundle/tests/legacy_registration.c" NV_TMP="$plugin_output/foreground" bash "$plugin_bundle/tests/legacy_foreground.sh"
