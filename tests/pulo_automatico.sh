#!/bin/bash
# Testes JVM do ciclo de vida dos pulos automaticos.
set -eu
cd "$(dirname "$0")/.."
if ! command -v kotlinc >/dev/null 2>&1 || ! command -v java >/dev/null 2>&1; then
  echo 'SKIP pulo_automatico: kotlinc and java are required for JVM lifecycle tests'
  exit 0
fi
tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-pulo-automatico.XXXXXXXX")
jar="$tmp/test.jar"
trap 'rm -f "$jar"
rmdir "$tmp"' EXIT
kotlinc android/app/src/main/java/space/nuvio/nativelegacy/PuloAutomatico.kt \
  tests/PuloAutomaticoTeste.kt -jvm-target 17 -include-runtime -d "$jar"
java -jar "$jar"
