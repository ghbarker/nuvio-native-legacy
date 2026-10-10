#!/usr/bin/env python3
"""Exercise main.c's actual TV calibration gate and viewport-change hook.

Use the real layout implementation and inert SDL/GL boundaries. Android's
holder restore is represented by separate mode and drawable events, including
a restore delayed past several reports. No SDL, Android SDK or network needed.
"""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/main.c").read_text()
start = source.index("static int resolucaoDaTv(void)")
end = source.index("// PORTA DE TESTE DO MOTOR P2P", start)
production = source[start:end]
assert "resolucaoTvViewportMudou(SDL_GetTicks(), novoModo != *modo);" in production
assert production.index("resolucaoTvViewportMudou(SDL_GetTicks(),") < production.index("*width = novoW")
assert "if (autoPediu && janelaTv)" in source
assert "if (pediu4k && janelaTv &&" in source
harvest = source.index("int gN = gputempo_colher(&gMed, &gPior, &gUlt)")
gate = source.index("int janelaTv = resolucaoTvJanela(agora, ultRelato, dw, dh, pedeW, pedeH)")
assert harvest < gate < source.index("if (autoPediu && janelaTv)")

boundary = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "layout.h"
typedef uint32_t Uint32;
#ifdef NV_TOUCH_UI
typedef struct { int w, h; } SDL_Window;
static Uint32 clockMs;
static int capW, capH;
static Uint32 SDL_GetTicks(void) { return clockMs; }
static void SDL_GL_GetDrawableSize(SDL_Window *win, int *w, int *h) { *w = win->w; *h = win->h; }
static void ponteiro_cancelar_toque(void) {}
static void gpun_redimensionar(int w, int h) { (void)w; (void)h; }
static void glViewport(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
static void gfx_tamanho_alvo(int w, int h) { (void)w; (void)h; }
static void video_escala_definir(int w, int h) { (void)w; (void)h; }
static void gfx_snap_encerrar(void) {}
static void gfx_snap_iniciar(int w, int h) { (void)w; (void)h; }
#endif
'''

checks = r'''
#ifdef NV_TOUCH_UI
static SDL_Window drawable;
static int viewportW, viewportH, viewportMode;
static void reset(int w, int h, int mobile) {
  tvViewportMudouEm = 0; tvViewportMudou = tvModoMudou = 0;
  layout_tela_definir(w, h); layout_modo_definir(mobile);
  drawable = (SDL_Window){w, h};
  viewportW = w; viewportH = h; viewportMode = mobile;
}
static void change(Uint32 when, int w, int h, int mobile) {
  clockMs = when; drawable = (SDL_Window){w, h};
  layout_modo_definir(mobile);
  interfaceViewport(&drawable, &viewportW, &viewportH, &viewportMode);
}
static int report(Uint32 when, Uint32 since, int requestW, int requestH) {
  return resolucaoTvJanela(when, since, viewportW, viewportH, requestW, requestH);
}
#endif
int main(void) {
  assert(resolucaoDaTv());
  // Ordinary TV startup can still persist "4K was refused" immediately.
  assert(resolucaoTvJanela(3000u, 0u, 1920, 1080, 3840, 2160));
#ifdef NV_TOUCH_UI
  reset(1920, 1080, 0);
  change(14999u, 1920, 1080, 0); // duplicate SDL event does not invalidate TV
  assert(!tvViewportMudou && report(15000u, 12000u, 3840, 2160));
  reset(1080, 2340, 1);
  assert(!resolucaoDaTv() && !report(12000u, 9000u, 3840, 2160));

  reset(3840, 2160, 0);
  change(10000u, 3840, 2160, 1); // mode change without a resize
  assert(tvViewportMudou && tvModoMudou);
  assert(!report(12000u, 9000u, 3840, 2160));
  change(13000u, 1080, 2340, 1);
  change(14990u, 1080, 2340, 0); // native TV precedes asynchronous holder restore
  assert(!report(15000u, 12000u, 3840, 2160));
  assert(!report(21000u, 18000u, 3840, 2160));
  assert(!report(24000u, 21000u, 3840, 2160)); // even a delayed restore cannot record 1080
  change(25000u, 3840, 2160, 0);
  assert(!report(27000u, 24000u, 3840, 2160)); // mixed old/new drawable samples
  assert(!report(30000u, 27000u, 3840, 2160)); // report began during the settle interval
  assert(report(33000u, 30000u, 3840, 2160)); // settled, wholly TV window; GPU history flushed

  // Both modes can occur between reports with unchanged physical dimensions.
  reset(3840, 2160, 0);
  change(10001u, 3840, 2160, 1);
  change(10002u, 3840, 2160, 0);
  assert(!report(12000u, 9000u, 3840, 2160));
  assert(!report(16001u, 13001u, 3840, 2160)); // settle delay is followed by a whole report
  assert(report(16002u, 13002u, 3840, 2160));
  // A restored width with the wrong height is still the wrong holder.
  change(17000u, 3840, 1080, 0);
  assert(!report(25000u, 22000u, 3840, 2160));

  // No requested 4K holder: live TV can use the ordinary physical viewport.
  reset(1920, 1080, 0);
  change(10000u, 1920, 1080, 1);
  change(10001u, 1920, 1080, 0);
  assert(report(16001u, 13001u, 1920, 1080));
  // SDL ticks wrap after 49 days; interval comparisons remain unsigned.
  reset(3840, 2160, 0);
  change(UINT32_MAX - 1999u, 3840, 2160, 1);
  change(UINT32_MAX - 999u, 3840, 2160, 0);
  assert(!report(4999u, 1999u, 3840, 2160));
  assert(report(5000u, 2000u, 3840, 2160));
#else
  // Non-touch TV targets keep their original eligibility and no layout deps.
  assert(resolucaoTvJanela(3000u, 0u, 1080, 2340, 3840, 2160));
#endif
  return 0;
}
'''

compiler = os.environ.get("CC") or shutil.which("cc")
assert compiler, "host C compiler required"
with tempfile.TemporaryDirectory(prefix="nuvio-tv-window-") as directory:
    directory = Path(directory)

    def run(body, name, touch=True, succeeds=True):
        file = directory / (name + ".c")
        file.write_text(boundary + body + checks)
        executable = directory / name
        command = [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-Isrc"]
        if touch:
            command.append("-DNV_TOUCH_UI")
        subprocess.run(command + [str(file), "src/layout.c", "-o", str(executable)],
                       cwd=root, check=True)
        result = subprocess.run([str(executable)], capture_output=True, text=True)
        assert (result.returncode == 0) == succeeds, (name, result.stderr)

    run(production, "touch")
    run(production, "tv", touch=False)
    # Each negative control changes the actual helper, not a second policy.
    stale_holder = re.sub(r"  if \(tvModoMudou && pedidoW.*?\n      \(w != pedidoW \|\| h != pedidoH\)\) return 0;",
                          "  (void)w; (void)h; (void)pedidoW; (void)pedidoH;", production, flags=re.S)
    assert stale_holder != production
    run(stale_holder, "stale-holder", succeeds=False)
    mixed_window = production.replace("  if (tvViewportMudou && agora - tvViewportMudouEm < agora - inicio + 3000u) return 0;",
                                      "  (void)agora; (void)inicio;")
    assert mixed_window != production
    run(mixed_window, "mixed-window", succeeds=False)

print("tv_resolution_window: actual viewport hook and calibration gate preserve TV startup, reject Mobile/stale/mixed windows, await restored holder, handle tick wrap; negative controls PASS")
