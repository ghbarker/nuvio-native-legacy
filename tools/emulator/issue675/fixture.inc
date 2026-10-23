/* Isolated test fixture compiled only by the emulator workflow. */
#ifndef NV_EOF_EMULATOR
#error This fixture must never enter a product build
#endif
static void nv675_tick(Uint32 agora) {
  static int stage;
  static Uint32 began, lastLog, readyAt;
  char path[1024];
  if (!stage) {
    CatItem c = {0};
    snprintf(c.imdb, sizeof c.imdb, "tt9999675");
    snprintf(c.tipo, sizeof c.tipo, "movie");
    snprintf(c.titulo, sizeof c.titulo, "Playback end — Android emulator");
    snprintf(c.sinopse, sizeof c.sinopse, "Local test movie. Actual Media3 playback. No account or provider connection.");
    snprintf(c.backdrop, sizeof c.backdrop, "%s", "art/19.jpg");
    snprintf(c.meta, sizeof c.meta, "2026 · 2 min");
    cat_definir(&c, 1);
    tela = TELA_HOME; homePronta = 1;
    menu_fechar();
    abrirPorIndice(0);
    began = agora;stage = 1;
    printf("[eof675] fixture detail ready\n");fflush(stdout);
  }
  if (stage == 1 && agora - began > 6000) {
    char url[1200];
    snprintf(url, sizeof url, "file://%s/eof-test.mp4", dados_dir());
    player_abrir(0, url);
    stage = 2;
    printf("[eof675] actual Android player opened local MP4\n");fflush(stdout);
  }
  if (stage == 2 && video_pronto() && video_tocando() && video_duracao() > 120) {
    if (!readyAt) readyAt = agora;
    if (agora - readyAt > 2000) {
      double target = video_duracao() - 6;
      video_buscar(target);
      stage = 3;
      printf("[eof675] seeking actual Media3 playback to %.3f / %.3f\n", target, video_duracao());fflush(stdout);
    }
  }
  if (agora - lastLog >= 500) {
    lastLog = agora;
    dados_caminho(path, sizeof path, "issue675-state.json");
    FILE *f = fopen(path,"w");
    if (f) {
      fprintf(f,"{\"stage\":%d,\"elapsed_ms\":%u,\"player_open\":%d,\"exit_request\":%d,\"detail_open\":%d,\"video_active\":%d,\"playing\":%d,\"ended\":%d,\"position\":%.3f,\"duration\":%.3f}\n",
        stage, (unsigned)(agora-began), player_aberto(),player_quer_sair(),detail_aberto(),video_ativo(),video_tocando(),video_terminou(),video_pos(),video_duracao());
      fclose(f);
    }
  }
}
