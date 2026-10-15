/* Review-only capture using the unchanged upstream GL/player fixture helpers.
 * The screenshots exercise the real selector drawing code and font assets. */
#define main upstream_visual_main
#include SOURCE_VISUAL_TEST
#undef main

static void title_state(void) {
  VideoSimulacao v; memset(&v, 0, sizeof v);
  v.largura = 3840; v.altura = 2160; v.pronto = 1;
  v.pos = 12; v.duracao = 3360; v.nAudio = 1; v.nLeg = 3; v.legAtual = 0;
  faixa(&v.audio[0], 1, "English", "en", "", 0);
  faixa(&v.leg[0], 21, "English", "en", "S_TEXT/ASS", 0);
  faixa(&v.leg[1], 22, "English", "en", "S_TEXT/ASS", 0);
  faixa(&v.leg[2], 23, "English", "en", "S_TEXT/ASS", 1);
  v.leg[0].tipoLeg = v.leg[1].tipoLeg = LING_LEG_COMPLETA;
  v.leg[2].tipoLeg = LING_LEG_LETREIROS;
  snprintf(v.leg[0].nome, sizeof v.leg[0].nome, "Full dialogue");
  snprintf(v.leg[1].nome, sizeof v.leg[1].nome, "Full with honorifics");
  snprintf(v.leg[2].nome, sizeof v.leg[2].nome, "Signs & Songs");
  video_simular(&v);
  player_shot_estado(relogio, 12, 3360, 1, 0, 0, 0);
}

int main(int argc, char **argv) {
  saida = argc > 1 ? argv[1] : "/tmp/nuvio-title-captures";
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *w = SDL_CreateWindow("Nuvio subtitle names", 0, 0, 64, 64,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(w);
  SDL_GLContext gl = SDL_GL_CreateContext(w); assert(gl); SDL_GL_SetSwapInterval(0);
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  const char *data = getenv("NUVIO_DADOS"); assert(data && *data);
  char path[700]; snprintf(path, sizeof path, "%s/ajustes.txt", data);
  FILE *settings = fopen(path, "w"); assert(settings);
  fputs("idioma 1\nselected_theme 2\nfonteInterface 3\nlegendaIdioma 3\n", settings);
  fclose(settings);
  ajustes_dir(data); ajustes_iniciar();
  ling_local_legenda("en"); ling_local_legenda2("");
  CatItem c; memset(&c, 0, sizeof c);
  snprintf(c.tipo, sizeof c.tipo, "movie");
  snprintf(c.titulo, sizeof c.titulo, "Subtitle track names");
  cat_definir(&c, 1);
  player_abrir(0, NULL); player_erro_fonte(); player_limpar_erro_fonte();
  player_shot_video(1); title_state(); addons_shot_legendas(NULL, 0);
  quadros(30); faixas_abrir_em(1); quadros(60);
  assert(!legendasui_mais() && !legendasui_versoes());
  salvar("subtitle-titles-recommended");
  for (int i=0; i<8; i++) tecla(SDLK_DOWN);
  tecla(SDLK_RETURN); quadros(40); assert(legendasui_mais());
  salvar("subtitle-titles-all");
  tecla(SDLK_ESCAPE); quadros(20); assert(!legendasui_mais());
  tecla(SDLK_UP); quadros(10);  /* from All to the English recommendation */
  SDL_Event e; memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN; faixas_evento(&e);
  SDL_Delay(780); quadros(30);
  e.type = SDL_KEYUP; faixas_evento(&e); quadros(30);
  tecla(SDLK_DOWN); tecla(SDLK_DOWN); tecla(SDLK_RETURN);
  quadros(30); assert(legendasui_versoes());
  salvar("subtitle-titles-versions");
  puts("subtitle titles: captured recommended/all/versions through production GL draw path");
  return 0;
}
