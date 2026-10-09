#!/usr/bin/env bash
# APK de teste touch, separado do app Android TV instalado. Nao publica releases.
# NUVIO_TOUCH_ABIS=arm64-v8a,armeabi-v7a (padrao) ou x86_64 para o emulador.
# Configuracao publica do app: NUVIO_PROPERTIES=<arquivo usado por tools/env.sh>.
set -euo pipefail
cd "$(dirname "$0")/.."
RAIZ="$PWD"
case "$(uname -s)" in
  Darwin) SDK_DEFAULT="$HOME/Library/Android/sdk" ;;
  Linux) SDK_DEFAULT="$HOME/Android/Sdk" ;;
  *) echo "android-touch.sh: execute no Linux ou macOS com o SDK Android" >&2; exit 1 ;;
esac
export ANDROID_HOME="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$SDK_DEFAULT}}"
export ANDROID_SDK_ROOT="$ANDROID_HOME"
export ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-$ANDROID_HOME/ndk/27.2.12479018}"
CACHE="${NUVIO_ANDROID_CACHE:-$HOME/.cache/nuvio-android}"
EST="$RAIZ/build/android-touch"
command -v java >/dev/null || { echo "android-touch.sh: JDK 17 ausente" >&2; exit 1; }
for d in ndk/27.2.12479018 platforms/android-35 build-tools/35.0.0 cmake/3.22.1; do
  [ -d "$ANDROID_HOME/$d" ] || { echo "android-touch.sh: SDK ausente: $d" >&2; exit 1; }
done
IFS=', ' read -r -a ABIS <<< "${NUVIO_TOUCH_ABIS:-arm64-v8a,armeabi-v7a}"
[ ${#ABIS[@]} -gt 0 ] || { echo "android-touch.sh: nenhuma ABI" >&2; exit 1; }
for abi in "${ABIS[@]}"; do
  case "$abi" in arm64-v8a|armeabi-v7a|x86|x86_64) ;;
    *) echo "android-touch.sh: ABI desconhecida: $abi" >&2; exit 1 ;; esac
done
ABI_PROP=$(IFS=,; echo "${ABIS[*]}")
tools/env.sh --require-core >/dev/null
VER=$(sed -n 's/^[[:space:]]*"version":[[:space:]]*"\([^"]*\)".*/\1/p' deploy/app/appinfo.json | head -1)
[ -n "$VER" ] || { echo "android-touch.sh: versao ausente" >&2; exit 1; }

mkdir -p "$CACHE/src" "$EST"
for u in https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz \
         https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-2.8.2.tar.gz \
         https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-2.22.0.tar.gz \
         https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.4.0.tar.gz; do
  d="$CACHE/src/$(basename "$u" .tar.gz)"
  [ -d "$d" ] || curl -fsSL --retry 3 "$u" | tar xz -C "$CACHE/src"
done
if [ "${NUVIO_TOUCH_SKIP_DEPS:-0}" != "1" ]; then bash tools/android/deps.sh "${ABIS[@]}"; fi
FFMPEG_AAR="$EST/media3-ffmpeg-touch.aar"
bash tools/android/ffmpeg-touch.sh "$FFMPEG_AAR" "${ABIS[@]}"

rm -rf "$EST/assets" "$EST/jnilibs"
mkdir -p "$EST/assets/fonts" "$EST/assets/licencas"
NUVIO_ARTE_ESTAGIO=build/android-touch/assets/art bash tools/tizen-art.sh >/dev/null
cp deploy/app/fonts/* "$EST/assets/fonts/"
cp deploy/app/licencas/* "$EST/assets/licencas/"
for abi in "${ABIS[@]}"; do
  mkdir -p "$EST/jnilibs/$abi"
  for so in libcurl.so libjpeg.so libwebp.so; do
    f="$CACHE/prefix/$abi/lib/$so"
    [ -s "$f" ] || { echo "android-touch.sh: dependencia ausente: $abi/$so" >&2; exit 1; }
    cp "$f" "$EST/jnilibs/$abi/"
  done
done
ENVF=$(mktemp "${TMPDIR:-/tmp}/nuvio-touch-env.XXXXXXXX")
trap 'rm -f "$ENVF"' EXIT
tools/env.sh --env-file "$ENVF"
: > "$EST/nuvio-env.cmake"; chmod 600 "$EST/nuvio-env.cmake"
while IFS= read -r l; do
  k="${l%%=*}"; v="${l#*=}"
  v=$(printf '%s' "$v" | sed 's/[\\"$]/\\&/g')
  printf 'set(%s "%s")\n' "$k" "$v" >> "$EST/nuvio-env.cmake"
done < "$ENVF"

bash android/gradlew -p android --console=plain --max-workers=2 \
  -Pnuvio.touchPreview=true -Pnuvio.abis="$ABI_PROP" \
  -Pnuvio.ffmpegAar="$FFMPEG_AAR" \
  -Pnuvio.sdlSrc="$CACHE/src" -Pnuvio.estagio="$EST" assembleDebug
APK="$EST/Nuvio-$VER-touch-${ABI_PROP//,/-}.apk"
cp android/app/build/outputs/apk/debug/app-debug.apk "$APK"
L=$(unzip -Z1 "$APK")
for abi in "${ABIS[@]}"; do
  for so in libmain.so libSDL2.so libcurl.so libjpeg.so libwebp.so; do
    grep -qx "lib/$abi/$so" <<< "$L" || { echo "android-touch.sh: APK sem $abi/$so" >&2; exit 1; }
  done
done
for need in assets/art/marcas/abertura.jpg assets/fonts/InterDisplay-Regular.ttf assets/licencas/p2p-avisos.txt; do
  grep -qx "$need" <<< "$L" || { echo "android-touch.sh: APK sem $need" >&2; exit 1; }
done
SEGREDO='(^|/)(trakt|addons|tmdb|mdblist|sessao|simkl[^/]*|fanart|diag-token)\.txt$|collections\.json$|catalogo-rede\.bin|local\.properties|\.env$|(^|/)(trakt|stalker|xtream|listas)-p[0-9]|(^|/)conta-[^/]*\.txt(\.tmp)?$'
if grep -E "$SEGREDO" <<< "$L"; then
  echo "android-touch.sh: arquivo pessoal no APK" >&2; exit 1
fi
AAPT="$ANDROID_HOME/build-tools/35.0.0/aapt"
BADGING=$("$AAPT" dump badging "$APK")
PKG=$(sed -n "s/^package: name='\([^']*\)'.*/\1/p" <<< "$BADGING")
[ "$PKG" = space.nuvio.nativelegacy.touch ] || { echo "android-touch.sh: applicationId de teste incorreto" >&2; exit 1; }
python3 tools/android-apk-pages.py "$APK"
echo "android-touch.sh: $APK ($(du -h "$APK" | cut -f1))"
