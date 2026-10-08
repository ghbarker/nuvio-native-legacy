#!/bin/bash
# Funcoes reais de Ajustes/posplay, colaboradores sem rede/janela.
set -eu
cd "$(dirname "$0")/.."
flags=(-O2 -ffunction-sections -fdata-sections)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
case "$(uname -s)" in
  Darwin) flags+=(-Wl,-dead_strip -Wl,-undefined,dynamic_lookup);;
  Linux) flags+=(-Wl,--gc-sections);;
  *) echo 'SKIP automatico: use a supported ELF/Mach-O host'; exit 0;;
esac
inc=(-Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
cc "${flags[@]}" "${inc[@]}" tests/ajustes_auto.c src/linguas.c src/js.c -lm -o /tmp/nuvio-ajustes-auto-tests
/tmp/nuvio-ajustes-auto-tests
cc "${flags[@]}" "${inc[@]}" tests/posplay_auto.c src/velocidade.c -lm -o /tmp/nuvio-posplay-auto-tests
/tmp/nuvio-posplay-auto-tests
