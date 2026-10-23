#ifndef NV_TRAILER_687_EMULATOR
#error This fixture must never enter a product build
#endif
#include "extras.h"
#include "trailerapple.h"
#include "linguas.h"
extern void nv687_settings(const char *, const char *, const char *);
extern void nv687_manual_trailer(void);
static void nv687_tick(Uint32 agora) {
  static int stage, manualCalls, seeked;
  static Uint32 began, lastLog, readyAt;
  static char mode[24], idioma[16], audio[16];
  char path[1024];
  if (!stage) {
    dados_caminho(path, sizeof path, "issue687-case.txt");
    FILE *cfg = fopen(path,"r");
    if (!cfg) return;
    int n=fscanf(cfg,"%23s %15s %15s",mode,idioma,audio);fclose(cfg);
    if (n!=3) return;
    nv687_settings(!strcmp(idioma,"auto") ? "" : idioma,mode,!strcmp(audio,"-") ? "" : audio);
    CatItem c = {0}; CatFileira f = {0};
    snprintf(c.imdb,sizeof c.imdb,"tt1877830");
    snprintf(c.tipo,sizeof c.tipo,"movie");
    snprintf(c.titulo,sizeof c.titulo,"The Batman");
    snprintf(c.sinopse,sizeof c.sinopse,"Android emulator — real trailer lookup and Media3 playback.");
    snprintf(c.backdrop,sizeof c.backdrop,"art/687-batman.jpg");
    snprintf(c.meta,sizeof c.meta,"2022 · 2h 56min");
    snprintf(f.chave,sizeof f.chave,"emulator_trailers");
    snprintf(f.titulo,sizeof f.titulo,"Trailer playback test");
    snprintf(f.tipo,sizeof f.tipo,"movie");f.ini=0;f.n=1;
    cat_definir_tudo(&c,1,&f,1);
    home_abrir_sessao();tela=TELA_HOME;homePronta=1;
    perfilsel_continuar_ativo();menu_fechar();
    if (strcmp(mode,"home-autoplay")) abrirPorIndice(0);
    began=agora;stage=1;
    printf("[emu687] fixture mode=%s language=%s audio=%s\n",mode,idioma,audio);fflush(stdout);
  }
  if (stage==1 && !strcmp(mode,"manual") && agora-began>3000 &&
      detail_assentado() && trailerapple_respondeu("tt1877830") && trailerapple_url("tt1877830")) {
    nv687_manual_trailer();manualCalls++;stage=2;
    printf("[emu687] activated real Watch trailer handler\n");fflush(stdout);
  }
  if (trailer_tocando() && video_pronto()) {
    if (!readyAt) readyAt=agora;
    if (!seeked && agora-readyAt>2000) {
      video_buscar(20);seeked=1;
      printf("[emu687] seek real trailer to 20s for spoken-audio evidence\n");fflush(stdout);
    }
  }
  if (agora-lastLog>=400) {
    lastLog=agora;dados_caminho(path,sizeof path,"issue687-state.json");
    FILE *f=fopen(path,"w");
    if(f) {
      char pref[16];extras_trailer_audio_idioma(pref,sizeof pref);
      fprintf(f,"{\"stage\":%d,\"mode\":\"%s\",\"requested\":\"%s\",\"resolved_preference\":\"%s\",\"manual_calls\":%d,\"elapsed_ms\":%u,\"detail_open\":%d,\"home_ready\":%d,\"trailer_open\":%d,\"fullscreen\":%d,\"owner\":%d,\"playing\":%d,\"visible\":%d,\"sound\":%d,\"position\":%.3f,\"duration\":%.3f,\"seeked\":%d,\"apple_preferred\":%d}\n",
        stage,mode,idioma,pref,manualCalls,(unsigned)(agora-began),detail_aberto(),homePronta,
        trailer_aberto(),trailer_cheia(),trailer_dono(),trailer_tocando(),trailer_mostra_video(),trailer_com_som(),
        video_pos(),video_duracao(),seeked,trailerapple_idioma_preferido("tt1877830"));
      fclose(f);
    }
  }
}
