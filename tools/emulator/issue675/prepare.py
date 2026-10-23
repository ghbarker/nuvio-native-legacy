"""Prepare an isolated Android emulator build; never a product patch."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
def edit(name,old,new,count=1):
 p=root/name;s=p.read_text();assert s.count(old)==count,(name,s.count(old),old[:60]);p.write_text(s.replace(old,new))
edit('android/app/build.gradle.kts','listOf("arm64-v8a", "armeabi-v7a")','listOf("x86_64")')
edit('android/app/src/main/cpp/CMakeLists.txt','target_compile_definitions(main PRIVATE NV_ANDROID NV_TOUCH_UI)','target_compile_definitions(main PRIVATE NV_ANDROID NV_TOUCH_UI NV_EOF_EMULATOR)')
edit('src/app.c','void app_atualizar(float dt, Uint32 agora) {','static void nv675_tick(Uint32 agora);\nvoid app_atualizar(float dt, Uint32 agora) {\n  nv675_tick(agora);')
edit('src/app.c','if (!sessao_logada()\n#ifdef NV_LINUX_DESKTOP','if (!sessao_logada() && 0 /* isolated emulator fixture, no account */\n#ifdef NV_LINUX_DESKTOP')
(root/'src/app.c').write_text((root/'src/app.c').read_text()+'\n#include "../tools/emulator/issue675/fixture.inc"\n')
# Real Media3 plays to completion. Only the clock exposed to native UI is offset
# in the fault-injection scenarios, before the real completion signal is sent.
p='android/app/src/main/java/space/nuvio/nativelegacy/NvPlayer.kt'
edit(p,'    private fun ev(tipo: Int, a: Int = 0, b: Int = 0) {','''    private var eofEmulatorGapMs = -1L
    private fun eofEmulatorPosition(p: ExoPlayer): Long {
        if (eofEmulatorGapMs < 0) eofEmulatorGapMs = try {
            java.io.File(activity!!.filesDir, "dados/issue675-gap.txt").readText().trim().toLong().coerceIn(0, 15000)
        } catch (_: Exception) { 0L }
        return if (p.playbackState == Player.STATE_ENDED && eofEmulatorGapMs > 0)
            (p.duration - eofEmulatorGapMs).coerceAtLeast(0) else p.currentPosition
    }
    private fun ev(tipo: Int, a: Int = 0, b: Int = 0) {''')
edit(p,'try { nativePos(p.currentPosition, p.bufferedPosition, p.isCurrentMediaItemLive,','try { nativePos(eofEmulatorPosition(p), p.bufferedPosition, p.isCurrentMediaItemLive,')
edit(p,'                Player.STATE_ENDED -> ev(EV_FIM)','''                Player.STATE_ENDED -> {
                    val observed = eofEmulatorPosition(p)
                    val evidence = "{\\"actual_position_ms\\":${p.currentPosition},\\"duration_ms\\":${p.duration},\\"injected_gap_ms\\":$eofEmulatorGapMs,\\"reported_position_ms\\":$observed}"
                    Log.i("EOF675", evidence)
                    try { java.io.File(activity!!.filesDir,"dados/issue675-eos.json").writeText(evidence) } catch (_: Exception) { }
                    nativePos(observed, p.bufferedPosition, p.isCurrentMediaItemLive,
                        recursosAtuais?.carga?.allocator?.totalBytesAllocated ?: -1, geracaoNative)
                    ev(EV_FIM)
                }''')
Path(root/'build/android').mkdir(parents=True,exist_ok=True)
(root/'build/android/nuvio-env.cmake').write_text('set(NV_VERSAO "2.0.116-eof-emulator")\n')
print('Prepared real Android player plus isolated, labelled fixture and end-clock fault injection.')
