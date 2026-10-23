"""Isolated real-Android fixture. Never applied to a product/PR checkout."""
from pathlib import Path
import sys, shutil
root=Path(sys.argv[1]);bundle=Path(__file__).parent
def edit(name,old,new,count=1):
 p=root/name;s=p.read_text();assert s.count(old)==count,(name,s.count(old),old[:80]);p.write_text(s.replace(old,new))
edit('android/app/build.gradle.kts','listOf("arm64-v8a", "armeabi-v7a")','listOf("x86_64")')
edit('android/app/src/main/cpp/CMakeLists.txt','target_compile_definitions(main PRIVATE NV_ANDROID NV_TOUCH_UI)','target_compile_definitions(main PRIVATE NV_ANDROID NV_TOUCH_UI NV_TRAILER_687_EMULATOR)')
edit('src/app.c','void app_atualizar(float dt, Uint32 agora) {','static void nv687_tick(Uint32 agora);\nvoid app_atualizar(float dt, Uint32 agora) {\n  nv687_tick(agora);')
edit('src/app.c','if (!sessao_logada()\n#ifdef NV_LINUX_DESKTOP','if (!sessao_logada() && 0 /* isolated no-account emulator fixture */\n#ifdef NV_LINUX_DESKTOP')
edit('src/app.c','  if (!login_concluido()) return "login";','  if (!login_concluido() && 0 /* fixture has no account */) return "login";')
(root/'src/app.c').write_text((root/'src/app.c').read_text()+'\n#include "../tools/emulator/issue687/fixture.inc"\n')
(root/'src/ajustes.c').write_text((root/'src/ajustes.c').read_text()+'''\n#ifdef NV_TRAILER_687_EMULATOR
// Set the exact choices otherwise selected in Settings, without altering any
// preference getter, source resolver, trailer selector or player behavior.
void nv687_settings(const char *idioma, const char *modo, const char *audio) {
  rotulosDeIdioma();
  valor[AJ_IDIOMA] = IDIOMA_EN + 1;
  valor[AJ_TRAILER_AUDIO] = 0;
  for (int i = 0; i < nTrAud; i++) if (!strcmp(TRAUD_COD[i], idioma)) valor[AJ_TRAILER_AUDIO] = i;
  valor[AJ_TRAILER_FONTE] = 0;
  valor[AJ_HERO_TRAILER_SOM] = 0;
  valor[AJ_HERO_TRAILER] = strcmp(modo,"home-autoplay") ? 1 : 0;
  valor[AJ_DET_TRAILER_AUTO] = strcmp(modo,"detail-autoplay") ? 1 : 0;
  valor[AJ_HERO_TRAILER_ESPERA] = 5;
  valor[AJ_TRAILER_QUAL] = 3;
  valor[AJ_HERO] = 0;
  valor[AJ_HOME_LAYOUT] = 0;
  ling_local_audio(""); ling_conta_audio(audio);
  gravar();
  printf("[emu687] settings trailer=%s primary=%s interface=%d source=%d hero=%d detail=%d sound=%d\\n",
    ajustes_trailer_audio(),ling_audio_preferencia(),ajustes_idioma(),ajustes_trailer_fonte(),ajustes_trailer_hero(),ajustes_trailer_auto(),ajustes_trailer_som());
  fflush(stdout);
}
#endif
''')
(root/'src/detail.c').write_text((root/'src/detail.c').read_text()+'''\n#ifdef NV_TRAILER_687_EMULATOR
void nv687_manual_trailer(void) { tocarTrailerCheio(0); }
#endif
''')
p='android/app/src/main/java/space/nuvio/nativelegacy/NvPlayer.kt'
edit(p,'    fun iniciar(activity: Activity, camada: FrameLayout) {','    fun iniciar(activity: Activity, camada: FrameLayout) {\n        Nv687Evidence.initialize(activity.filesDir)')
edit(p,'try { nativePos(p.currentPosition, p.bufferedPosition, p.isCurrentMediaItemLive,','try { Nv687Evidence.observe(p); nativePos(p.currentPosition, p.bufferedPosition, p.isCurrentMediaItemLive,')
edit(p,'        override fun onRenderedFirstFrame() {\n            if (!atual(minha)) return','        override fun onRenderedFirstFrame() {\n            if (!atual(minha)) return\n            Nv687Evidence.firstFrame = true')
p='android/app/src/main/java/space/nuvio/nativelegacy/GanhoAudioProcessor.kt'
edit(p,'        val n = inputBuffer.remaining()','        Nv687Evidence.capture(inputBuffer, inputAudioFormat)\n        val n = inputBuffer.remaining()')
shutil.copyfile(bundle/'Nv687Evidence.kt',root/'android/app/src/main/java/space/nuvio/nativelegacy/Nv687Evidence.kt')
(root/'build/android').mkdir(parents=True,exist_ok=True)
(root/'build/android/nuvio-env.cmake').write_text('set(NV_VERSAO "2.0.117-trailer-emulator")\n')
print('Prepared an isolated x86_64 build of Logs107: test inputs/observability only; trailer selection/resolvers unchanged.')
