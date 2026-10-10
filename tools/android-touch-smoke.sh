#!/usr/bin/env bash
# Instala, abre e exercita entrada/rotacao no emulador; deixa capturas e logs.
set -euo pipefail
cd "$(dirname "$0")/.."
APK="${1:?passe o APK x86_64 de android-touch.sh}"
OUT=build/android-touch/smoke
PKG="${NUVIO_SMOKE_PACKAGE:-space.nuvio.nativelegacy}"
case "$PKG" in space.nuvio.nativelegacy|space.nuvio.nativelegacy.touch) ;; *) exit 1 ;; esac
mkdir -p "$OUT"
adb wait-for-device
adb shell getconf PAGE_SIZE > "$OUT/page-size.txt"
grep -qx '16384' "$OUT/page-size.txt" || { echo "smoke: emulador nao usa paginas de 16KB" >&2; exit 1; }
adb shell wm size "${NUVIO_TOUCH_SCREEN:-1080x2340}"
adb shell settings put system accelerometer_rotation 0
adb shell settings put system user_rotation 0
adb shell settings put secure immersive_mode_confirmations confirmed
adb shell settings put secure show_ime_with_hard_keyboard 1
# Let first-boot services finish before starting the app and the keyboard.
sleep 30
adb install -r "$APK"
adb shell pm clear "$PKG" >/dev/null
adb shell am force-stop "$PKG"
adb logcat -c
capturar_saida() {
  adb logcat -d > "$OUT/logcat.txt" || true
  adb exec-out screencap -p > "$OUT/last.png" || true
  adb shell dumpsys window > "$OUT/window-exit.txt" || true
  adb shell settings get system accelerometer_rotation > "$OUT/rotation-mode-exit.txt" || true
  adb shell settings get system user_rotation > "$OUT/rotation-exit.txt" || true
}
trap capturar_saida EXIT
adb shell am start -W -n "$PKG/space.nuvio.nativelegacy.NuvioActivity" > "$OUT/start.txt"
# A abertura inclui SDL/fontes e pode pedir o catalogo. O processo precisa
# continuar vivo e com a Activity em primeiro plano depois dessa espera.
sleep 20
PID=$(adb shell pidof "$PKG" | tr -d '\r' || true)
[ -n "$PID" ] || { adb logcat -d > "$OUT/logcat.txt"; echo "smoke: app fechou na abertura" >&2; exit 1; }
# O aviso de tela cheia do Android pode cobrir os alvos no primeiro arranque.
# Se ainda apareceu, fecha o botao do sistema antes de testar os toques do app.
adb shell uiautomator dump /sdcard/nuvio-touch-window.xml >/dev/null
adb pull /sdcard/nuvio-touch-window.xml "$OUT/window.xml" >/dev/null
CONFIRM=$(python3 - "$OUT/window.xml" <<'PY'
import re
import sys
import xml.etree.ElementTree as ET
for node in ET.parse(sys.argv[1]).iter('node'):
    if node.get('resource-id') == 'com.android.systemui:id/ok':
        coords = list(map(int, re.findall(r'\d+', node.get('bounds', ''))))
        if len(coords) == 4:
            print((coords[0] + coords[2]) // 2, (coords[1] + coords[3]) // 2)
            break
PY
)
if [ -n "$CONFIRM" ]; then
  read -r x y <<< "$CONFIRM"
  adb shell input tap "$x" "$y"
  sleep 2
fi
tap_logico() {
  local viewport ponto x y
  viewport=$(adb logcat -d --pid="$PID" | sed -n 's/.*touch viewport=\([0-9]*,[0-9]*,[0-9]*,[0-9]*\).*/\1/p' | tail -1)
  [ -n "$viewport" ] || { echo "smoke: viewport nao foi registrado" >&2; exit 1; }
  ponto=$(python3 - "$viewport" "$1" "$2" <<'PY'
import sys
x, y, w, h = map(int, sys.argv[1].split(','))
print(round(x + float(sys.argv[2]) * w / 1920), round(y + float(sys.argv[3]) * h / 1080))
PY
  )
  read -r x y <<< "$ponto"
  adb shell input tap "$x" "$y"
}
conferir_viewport() {
  local viewport
  viewport=$(adb logcat -d --pid="$PID" | sed -n 's/.*touch viewport=\([0-9]*,[0-9]*,[0-9]*,[0-9]*\).*/\1/p' | tail -1)
  adb shell wm size > "$OUT/display-size.txt"
  python3 - "$viewport" "$OUT/display-size.txt" "$1" <<'PY'
from pathlib import Path
import re
import sys
assert sys.argv[1], 'smoke: viewport nao foi registrado'
x, y, w, h = map(int, sys.argv[1].split(','))
sizes = re.findall(r'(\d+)x(\d+)', Path(sys.argv[2]).read_text())
dw, dh = map(int, sizes[-1])
expected = (min(dw, dh), max(dw, dh)) if sys.argv[3] == 'portrait' else (max(dw, dh), min(dw, dh))
assert (x, y, w, h) == (0, 0, *expected), 'smoke: app nao preenche a tela inteira na orientacao pedida'
print(f'smoke: viewport {sys.argv[3]} {w}x{h} em {x},{y}')
PY
}
pedir_rotacao() {
  # UiAutomation pode restaurar a rotacao anterior ou liberar o sensor ao
  # desconectar. Reafirma o bloqueio e a direcao em cada pedido, incluindo
  # o primeiro retrato depois de selecionar Mobile.
  adb shell settings put system accelerometer_rotation 0
  adb shell settings put system user_rotation "$1"
}
# A clean standard APK starts in TV mode. Seed only the local mode preference
# to exercise persisted Mobile without account credentials or another APK.
conferir_viewport landscape
adb logcat -d --pid="$PID" | grep -F '[interface] modo=tv' >/dev/null
adb exec-out screencap -p > "$OUT/tv-default.png"
definir_modo_gravado() {
  local valor="$1" modo
  case "$valor" in 0) modo=tv ;; 1) modo=mobile ;; *) return 1 ;; esac
  adb shell am force-stop "$PKG"
  adb shell run-as "$PKG" cat files/dados/ajustes.txt > "$OUT/settings-before.txt" 2>/dev/null || true
  python3 - "$OUT/settings-before.txt" "$OUT/settings-mode.txt" "$valor" <<'PY'
from pathlib import Path
import sys
lines = [s for s in Path(sys.argv[1]).read_text().splitlines()
         if not s.startswith('interfaceModoLocal ')]
Path(sys.argv[2]).write_text('\n'.join(lines + ['interfaceModoLocal ' + sys.argv[3]]) + '\n')
PY
  adb push "$OUT/settings-mode.txt" /data/local/tmp/nuvio-smoke-mode.txt >/dev/null
  adb shell chmod 644 /data/local/tmp/nuvio-smoke-mode.txt
  adb shell run-as "$PKG" cp /data/local/tmp/nuvio-smoke-mode.txt files/dados/ajustes.txt
  adb logcat -c
  adb shell am start -W -n "$PKG/space.nuvio.nativelegacy.NuvioActivity" >/dev/null
  sleep 8
  PID=$(adb shell pidof "$PKG" | tr -d '\r')
  [ -n "$PID" ] || { echo 'smoke: app failed after saved mode change' >&2; return 1; }
  adb logcat -d --pid="$PID" | grep -F "[interface] modo=$modo" >/dev/null
}
definir_modo_gravado 1
pedir_rotacao 0
sleep 4
conferir_viewport portrait
adb exec-out screencap -p > "$OUT/open-portrait.png"
adb shell dumpsys window > "$OUT/window-portrait.txt"
pedir_rotacao 1
sleep 4
conferir_viewport landscape
adb exec-out screencap -p > "$OUT/open.png"
adb shell dumpsys window > "$OUT/window-start.txt"
pedir_rotacao 0
sleep 4
conferir_viewport portrait
adb exec-out screencap -p > "$OUT/return-portrait.png"
pedir_rotacao 1
sleep 4
conferir_viewport landscape
[ "$(adb shell pidof "$PKG" | tr -d '\r')" = "$PID" ] || { echo 'smoke: rotacao recriou o processo' >&2; exit 1; }
# login.c: o botao do e-mail muda de altura entre QR pronto, erro e pedido
# pendente. Nenhum destes pontos envia credenciais. O campo e-mail fica em 406.
ime_mostrado() {
  adb shell dumpsys input_method > "$OUT/ime.txt"
  grep -E 'mInputShown=true' "$OUT/ime.txt" >/dev/null
}
abrir_campo_email() {
  local y
  for y in 928 470 384; do
    tap_logico 960 "$y"
    sleep 1
    # A primeira acao pode abrir o campo. O segundo toque cairia numa tecla
    # do IME ja visivel, em vez de no campo que antes ocupava este ponto.
    if ime_mostrado; then return 0; fi
    tap_logico 960 406
    sleep 2
    if ime_mostrado; then return 0; fi
  done
  return 1
}
aguardar_campo_email() {
  local tentativa
  for tentativa in 1 2 3 4 5; do
    if adb shell uiautomator dump /sdcard/nuvio-touch-editor-ready.xml >/dev/null &&
       adb pull /sdcard/nuvio-touch-editor-ready.xml "$OUT/editor-ready.xml" >/dev/null &&
       python3 - "$OUT/editor-ready.xml" "$PKG" <<'PY'
import re
import sys
import xml.etree.ElementTree as ET
fields = [n for n in ET.parse(sys.argv[1]).iter('node')
          if n.get('class') == 'android.widget.EditText'
          and n.get('package') == sys.argv[2]]
if len(fields) != 1 or fields[0].get('focused') != 'true' or fields[0].get('text', '') != '':
    sys.exit(1)
coords = list(map(int, re.findall(r'\d+', fields[0].get('bounds', ''))))
if len(coords) != 4:
    sys.exit(1)
x1, y1, x2, y2 = coords
sys.exit(0 if x2-x1 >= 100 and y2-y1 >= 30 and y2 < 540 else 1)
PY
    then return 0; fi
    sleep 1
  done
  echo 'smoke: campo Android visivel e focado nao ficou pronto' >&2
  return 1
}
digitar_fresco() {
  local texto="$1" i
  # `input text` cria os eventos da palavra com o mesmo instante. Um IME
  # lento pode entregar os primeiros e deixar os demais antigos demais.
  # Cada comando curto cria eventos novos, como a digitacao real.
  for ((i=0; i<${#texto}; i++)); do
    adb shell input text "${texto:i:1}"
    sleep 0.1
  done
}
if ! abrir_campo_email; then
  adb exec-out screencap -p > "$OUT/email-failed.png"
  echo "smoke: toque nao abriu o teclado do e-mail" >&2
  exit 1
fi
aguardar_campo_email
digitar_fresco 'touch-preview@example.invalid'
sleep 1
adb shell uiautomator dump /sdcard/nuvio-touch-editor.xml >/dev/null
adb pull /sdcard/nuvio-touch-editor.xml "$OUT/editor.xml" >/dev/null
python3 - "$OUT/editor.xml" "$PKG" <<'PY'
import re
import sys
import xml.etree.ElementTree as ET
fields = [n for n in ET.parse(sys.argv[1]).iter('node')
          if n.get('class') == 'android.widget.EditText'
          and n.get('package') == sys.argv[2]]
assert len(fields) == 1, 'smoke: campo Android visivel nao encontrado'
field = fields[0]
assert field.get('focused') == 'true', 'smoke: campo nao tem foco'
assert field.get('text', '') == 'touch-preview@example.invalid', 'smoke: digitacao exata nao chegou ao campo'
x1, y1, x2, y2 = map(int, re.findall(r'\d+', field.get('bounds', '')))
assert x2 - x1 >= 100 and y2 - y1 >= 30 and y2 < 540, 'smoke: campo pequeno ou coberto pelo teclado'
print('smoke: texto digitado visivel acima do teclado')
PY
adb exec-out screencap -p > "$OUT/email-ime.png"
adb shell input keyevent KEYCODE_BACK
sleep 2
adb shell dumpsys input_method > "$OUT/ime-back.txt"
# Android 16's IME-service dump can retain mIsInputViewShown=true after its
# window has closed. The system manager's mInputShown is the current state.
if grep -E 'mInputShown=true' "$OUT/ime-back.txt" >/dev/null; then
  echo "smoke: Voltar nao fechou o teclado" >&2
  exit 1
fi
adb exec-out screencap -p > "$OUT/ime-back.png"
pedir_rotacao 3
sleep 2
adb exec-out screencap -p > "$OUT/rotate.png"
adb logcat -d --pid="$PID" > "$OUT/logcat.txt"
adb shell dumpsys activity activities > "$OUT/activity.txt"
[ "$(adb shell pidof "$PKG" | tr -d '\r')" = "$PID" ] || { echo "smoke: processo mudou ou fechou" >&2; exit 1; }
grep -E '(mResumedActivity|ResumedActivity)' "$OUT/activity.txt" | grep -F "$PKG" >/dev/null || {
  echo "smoke: Activity nao esta em primeiro plano" >&2; exit 1; }
if grep -E 'FATAL EXCEPTION|Fatal signal|UnsatisfiedLinkError|ANR in space\.nuvio\.nativelegacy' "$OUT/logcat.txt"; then
  echo "smoke: falha no processo do app" >&2; exit 1
fi
definir_modo_gravado 0
conferir_viewport landscape
adb exec-out screencap -p > "$OUT/tv-restored.png"
definir_modo_gravado 1
conferir_viewport landscape
adb exec-out screencap -p > "$OUT/mobile-restored.png"
python3 - "$OUT" <<'PY'
from pathlib import Path
from PIL import Image
import sys

for path in sorted(Path(sys.argv[1]).glob('*.png')):
    img = Image.open(path).convert('RGB')
    if 'portrait' in path.stem:
        assert img.height > img.width, f'{path}: tela nao esta em retrato'
    else:
        assert img.width > img.height, f'{path}: tela nao esta em paisagem'
    # Barra do sistema nao prova que o canvas do app chegou a desenhar.
    inner = img.crop((img.width // 10, img.height // 10, img.width * 9 // 10, img.height * 9 // 10))
    colors = inner.resize((160, 90)).getcolors(14400)
    assert colors is None or len(colors) >= 8, f'{path}: conteudo vazio ou uniforme'
    print(f'{path}: {img.width}x{img.height}, conteudo renderizado')
PY
echo "smoke: instalacao, toque no e-mail, teclado, Voltar e rotacao sem queda; capturas em $OUT"

