# Shared flags for the existing native review fixtures; source from repo root.
nv_test_cflags=(-O1 -g -Isrc -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then
  nv_test_brew=/opt/homebrew
  if command -v brew >/dev/null 2>&1; then nv_test_brew=$(brew --prefix); fi
  nv_test_cflags+=("-I$nv_test_brew/include" "-I$nv_test_brew/include/SDL2" "-L$nv_test_brew/lib" -Wl,-dead_strip)
  nv_test_libs=(-lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
else
  nv_test_cflags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections)
  read -r -a nv_test_deps <<< "$(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)"
  nv_test_libs=("${nv_test_deps[@]}" -ldl -pthread -lm)
fi
if [ -n "${NV_TEST_CFLAGS:-}" ]; then
  read -r -a nv_test_extra <<< "$NV_TEST_CFLAGS"
  nv_test_cflags+=("${nv_test_extra[@]}")
fi
