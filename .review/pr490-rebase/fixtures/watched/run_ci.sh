#!/bin/bash
# Run only review fixtures against the current source tree. No device/services.
set -euo pipefail
task_repo="$(pwd)"
task_review="${NUVIO_WATCHED_REVIEW:-$task_repo/.review/hide-watched}"
task_out="${NUVIO_WATCHED_RESULTS:-$task_repo/build/review/watched}"
mkdir -p "$task_out"
task_flags=(-O1 -g -ffunction-sections -fdata-sections -Isrc -Itests -Wno-macro-redefined -Wno-deprecated-declarations)
task_flags+=(-DNV_WATCHED_SETTING_PREDECESSOR="${NUVIO_WATCHED_SETTING_PREDECESSOR:-AJ_PROVEDORES_ESCOPO}")
task_link=(-lm -lpthread)
if [[ "$(uname -s)" == Darwin ]]; then
  task_flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
  task_link+=(-Wl,-dead_strip)
else
  task_link+=(-Wl,--gc-sections)
  if command -v sdl2-config >/dev/null; then
    read -r -a task_sdl_flags <<< "$(sdl2-config --cflags)"
    task_flags+=("${task_sdl_flags[@]}")
  fi
fi
if [[ "${SANITIZE:-0}" == 1 ]]; then
  task_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  task_link+=(-fsanitize=address,undefined)
fi
if [[ "${NUVIO_WATCHED_COMBINED:-0}" == 1 ]]; then
  task_flags+=(-DNV_COMBINED_PREVIEW=1)
fi
task_extra=("$task_review/tests/language-test-stubs.c")
for task_src in src/fontefalha.c src/fontefiltro.c; do
  if [[ -f "$task_src" ]]; then task_extra+=("$task_src"); fi
done

cc "${task_flags[@]}" src/catalogo.c src/cwordem.c \
  "$task_review/tests/watched_discovery.c" src/js.c src/metaprov.c \
  "${task_link[@]}" -o "$task_out/watched_discovery"
"$task_out/watched_discovery" | tee "$task_out/discovery.log"

cc "${task_flags[@]}" src/catalogo.c src/cwordem.c \
  "$task_review/tests/colfiltro_existing.c" src/js.c src/metaprov.c \
  "${task_link[@]}" -o "$task_out/colfiltro-existing"
"$task_out/colfiltro-existing" | tee "$task_out/colfiltro-existing.log"

cc "${task_flags[@]}" -DNV_CAT_TEST_ANTES_TRAVA=nv_cat_teste_antes_trava \
  src/catalogo.c tests/catconsistencia.c "${task_link[@]}" -o "$task_out/history-existing"
"$task_out/history-existing" | tee "$task_out/history-existing.log"

cc "${task_flags[@]}" src/catalogo.c src/progresso.c src/focus.c src/ajustes.c \
  src/ajlog.c src/posterprov.c src/redeurl.c src/colecoes.c src/js.c src/catordem.c \
  src/fileiras.c src/artehero.c src/cwordem.c src/cwretido.c \
  "${task_extra[@]+${task_extra[@]}}" tests/homepos.c "$task_review/tests/discovery-stub.c" \
  tests/amigosfil_stub.c "${task_link[@]}" -o "$task_out/homepos-existing"
"$task_out/homepos-existing" | tee "$task_out/homepos-existing.log"

for task_mode in tv touch; do
  task_touch=()
  if [[ "$task_mode" == touch ]]; then task_touch+=(-DNV_TOUCH_UI=1); fi
  cc "${task_flags[@]}" "${task_touch[@]+${task_touch[@]}}" \
    -DNV_SHOT_HOOKS \
    src/catalogo.c src/progresso.c src/focus.c src/ajustes.c src/ajlog.c \
    src/layout.c \
    src/posterprov.c src/redeurl.c src/colecoes.c src/js.c src/catordem.c \
    src/fileiras.c src/artehero.c src/cwordem.c src/cwretido.c \
    "${task_extra[@]+${task_extra[@]}}" "$task_review/tests/watched_home.c" \
    tests/amigosfil_stub.c "${task_link[@]}" -o "$task_out/home-$task_mode"
  "$task_out/home-$task_mode" | tee "$task_out/home-$task_mode.log"
  cc "${task_flags[@]}" "${task_touch[@]+${task_touch[@]}}" \
    "$task_review/tests/watched_catalog_ui.c" src/descoberta.c src/catalogo.c \
    "${task_link[@]}" -o "$task_out/catalog-ui-$task_mode"
  "$task_out/catalog-ui-$task_mode" | tee "$task_out/catalog-ui-$task_mode.log"
  cc "${task_flags[@]}" "${task_touch[@]+${task_touch[@]}}" \
    "$task_review/tests/settings_contract.c" "${task_extra[@]+${task_extra[@]}}" \
    src/layout.c \
    "${task_link[@]}" \
    -o "$task_out/settings-$task_mode"
  "$task_out/settings-$task_mode" | tee "$task_out/settings-$task_mode.log"
done
