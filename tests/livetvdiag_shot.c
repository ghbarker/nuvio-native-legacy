// DIAGNOSTICO DA LIVE TV: rede de verdade contra um servidor Xtream FALSO
// (tests/livetvdiag_servidor.py, porta 8765) e capturas da tela.
//
//   1. o fio de rede (fioRede) roda contra o servidor: conta, .ts e .m3u8 de
//      cada canal, codec pela PMT, 10 bits pelo SPS, vazao (~9 Mbps limitados
//      no servidor). Asserts nos numeros;
//   2. captura do resultado do Mac (sem pipeline: "o player so e testado na TV");
//   3. captura com os campos do PLAYER preenchidos a mao com o que a C4 do
//      pasha mediu (registros 13526/14195: TS com dado e decoder mudo, um que
//      abriu em 14,2 s) e um HLS hipotetico tocando — a tela "na TV";
//   4. captura com o teste no meio.
#include "../src/livetvdiag.c"
#include "rail_shot.h"
#include "ilha.h"
#include "ajustes_ux.h"
#include "../src/dados.h"
#include "../src/tex_cache.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

static GLuint fbo, fboTex;
static void captura(const char *nome) {
  int i;
  rail_shot_aplicar();
  for (i = 0; i < 40; i++) {
    SDL_PumpEvents();
    txt_novo_quadro();
    tex_novo_quadro();
    tex_bombear(6);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1920, 1080);
    glClearColor(0.025f, 0.025f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    livetvdiag_desenhar(SDL_GetTicks());
    rail_shot_desenhar(MENU_AJUSTES);
    if (getenv("NUVIO_SHOT_ILHA")) {
      ilha_relogio_visivel(1); ilha_posicionar(1); ilha_desenhar(SDL_GetTicks()); SDL_Delay(12);
    }
    if (i == 39) {
      unsigned char *pix = malloc(1920 * 1080 * 4);
      SDL_Surface *s;
      int y;
      glFinish();
      glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
      for (y = 0; y < 1080; y++)
        memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
      assert(SDL_SaveBMP(s, nome) == 0);
      SDL_FreeSurface(s);
      free(pix);
    }
  }
  printf("captura: %s\n", nome);
}

static void canal(int i, const char *id, const char *nome) {
  snprintf(L.it[i].id, sizeof L.it[i].id, "%s", id);
  snprintf(L.it[i].nome, sizeof L.it[i].nome, "%s", nome);
  L.it[i].xt = 1;
  L.it[i].alturaNome = nv_res_do_texto(nome);
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nuvio-livetvdiag";
  const char *dir = getenv("NUVIO_DADOS");
  char nome[600];
  SDL_Window *w;
  int pt = getenv("NUVIO_SHOT_PT") && *getenv("NUVIO_SHOT_PT") == '1', i;
  assert(dir && *dir);
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  dados_iniciar(dir);
  { char c[600]; FILE *f; snprintf(c, sizeof c, "%s/ajustes.txt", dir);
    f = fopen(c, "w"); fprintf(f, "idioma %d\nanimacoes 0\n", pt ? 0 : 1);
    if (getenv("NUVIO_SHOT_TEMA")) fprintf(f, "selected_theme %s\n", getenv("NUVIO_SHOT_TEMA"));
    if (getenv("NUVIO_SHOT_VIDRO")) fprintf(f, "vidroLocal %d\n", *getenv("NUVIO_SHOT_VIDRO") == '0' ? 1 : 0);
    fclose(f); }
  ajustes_dir(dir);

#ifdef NV_TEST_PHONE_SHOT_H
  // Phone captures seed display data only. No server, account or decoded
  // media is exercised by this fixture.
  memset(&L, 0, sizeof L);
  L.n = 3; L.estado = E_PRONTO; L.xtConfig = L.contaLida = L.conta.valido = 1;
  L.conta.temTs = L.conta.formatosDeclarados = 1; L.conta.maxConexoes = L.conta.conexoes = 1;
  snprintf(L.conta.status, sizeof L.conta.status, "%s", "Active");
  snprintf(L.grupo, sizeof L.grupo, "%s", "Fixture channels with long translated names");
  L.redeMedida = L.kbpsDoSegmento = 1; L.kbps = 9000; L.kbpsPior = 6000; L.latenciaMs = 1200;
  for (i = 0; i < L.n; i++) {
    char tituloCanal[140];
    snprintf(tituloCanal, sizeof tituloCanal, "Fixture %d: a long channel title for narrow phone layout FHD", i + 1);
    canal(i, "fixture", tituloCanal); L.it[i].pronto = 1;
    L.it[i].f[F_TS].tentado = L.it[i].f[F_HLS].tentado = 1;
    L.it[i].f[F_TS].servido = L.it[i].f[F_HLS].servido = 1;
    L.it[i].f[F_TS].http = L.it[i].f[F_HLS].http = 200;
    snprintf(L.it[i].f[F_TS].codec, sizeof L.it[i].f[F_TS].codec, "H.264 · AAC");
    snprintf(L.it[i].f[F_HLS].codec, sizeof L.it[i].f[F_HLS].codec, "H.264 · AAC");
    L.it[i].f[F_TS].kbps = L.it[i].f[F_HLS].kbps = 9000;
  }
  L.it[2].f[F_TS].dezBits = 1;
  recomendar(); ltdPhoneReiniciar();
#else
  // --- 1. rede contra o servidor falso ---
  xtream_definir_servidor("127.0.0.1:8765");
  xtream_definir_usuario("u");
  xtream_definir_senha("p");
  assert(xtream_configurado());
  memset(&L, 0, sizeof L);
  L.xtConfig = 1; L.latenciaMs = -1;
  snprintf(L.grupo, sizeof L.grupo, "%s", "RO| FILME");
  canal(0, "xtream:101", "RO| CINEMAX FHD");
  canal(1, "xtream:102", "RO| CINEMAX 2 HD");
  canal(2, "xtream:103", "RO| HBO 4K HEVC");
  canal(3, "xtream:104", "RO| EPIC DRAMA FHD");
  L.n = 4;
  for (i = 0; i < L.n; i++) {
    L.atual = i; atomic_store(&L.fioOcupado, 1);
    fioRede(&L.it[i]);
    L.it[i].pronto = 1;
  }
  assert(L.contaLida && L.conta.valido && L.conta.temTs && !L.conta.temM3u8 && L.conta.maxConexoes == 1);
  assert(L.it[0].f[F_TS].http == 200 && L.it[0].f[F_TS].servido);
  assert(strstr(L.it[0].f[F_TS].codec, "H.264") && strstr(L.it[0].f[F_TS].codec, "AAC"));
  assert(!L.it[0].f[F_TS].dezBits);
  assert(L.it[0].f[F_HLS].http == 200 && L.it[0].f[F_HLS].playlist && L.it[0].f[F_HLS].servido);
  assert(L.it[1].f[F_TS].http == 404 && !L.it[1].f[F_TS].servido && L.it[1].f[F_HLS].http == 404);
  assert(strstr(L.it[2].f[F_TS].codec, "HEVC") && strstr(L.it[2].f[F_TS].codec, "AC3") && L.it[2].f[F_TS].dezBits);
  assert(L.it[3].f[F_TS].servido && strstr(L.it[3].f[F_TS].codec, "H.264"));  // pelo UA de player
  assert(L.redeMedida && L.kbpsDoSegmento && L.kbps > 4000 && L.kbps < 14000);
  // Mac: o player nao existe; o passo de player marca "so na TV".
  L.n = 3;   // as capturas seguem com os tres de antes
  memset(&L.it[3], 0, sizeof L.it[3]);
  for (i = 0; i < L.n; i++) {
    int f = proximoFormato(&L.it[i]);
    while (f >= 0) { L.atual = i; iniciarPlayer(f); f = LTD_TEM_PLAYER ? proximoFormato(&L.it[i]) : -1; }
  }
  L.estado = E_PRONTO;
  recomendar();
  assert(L.rec.resolucao == 3 || L.rec.resolucao == 2);   // ~9 Mbps: 720p (ou 1080p no pico)
  assert(L.rec.formato == 0 && L.rec.dezBits == 1);
  puts("ok  rede: conta, .ts e .m3u8, codec, 10 bits, vazao e recomendacao contra o servidor falso");
#endif

  // --- capturas ---
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("Nuvio: diagnostico da Live TV", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(w && SDL_GL_CreateContext(w));
  glGenTextures(1, &fboTex);
  glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  ajustes_recursos("deploy/app/art");
  ajustes_ui_arte(15);

  snprintf(nome, sizeof nome, "%s-1-mac-servidor-falso.bmp", saida);
  captura(nome);

  // --- 3. "na TV": player preenchido com o que a C4 mediu ---
  canal(3, "xtream:104", "RO| DIGI SPORT 1 HD");
  canal(4, "xtream:105", "RO| PRO TV FHD");
  canal(5, "xtream:106", "RO| CINEMAX RAW 1080p");
  L.n = 6;
  for (i = 0; i < L.n; i++) {
    LtdItem *it = &L.it[i];
    it->pronto = 1;
    if (i >= 3) {
      it->f[F_TS].tentado = it->f[F_TS].servido = 1; it->f[F_TS].http = 200;
      snprintf(it->f[F_TS].codec, sizeof it->f[F_TS].codec, "H.264 · AAC");
      it->f[F_TS].kbps = 7200 + i * 300;
      it->f[F_HLS].tentado = 1; it->f[F_HLS].http = i == 4 ? 200 : 404;
      it->f[F_HLS].servido = it->f[F_HLS].playlist = i == 4;
      if (i == 4) { snprintf(it->f[F_HLS].codec, sizeof it->f[F_HLS].codec, "H.264 · AAC"); it->f[F_HLS].kbps = 6900; }
    }
    it->f[F_TS].testou = it->f[F_TS].servido; it->f[F_TS].falha = LTD_SEM_DECODER;
    it->f[F_HLS].testou = it->f[F_HLS].servido; it->f[F_HLS].falha = LTD_SEM_DECODER;
  }
  // 101: TS sem decoder, HLS tocou em 4,1 s. 104: TS abriu em 14,2 s (13526).
  L.it[0].f[F_HLS].tocou = 1; L.it[0].f[F_HLS].falha = LTD_OK; L.it[0].f[F_HLS].quadroMs = 4100;
  L.it[0].f[F_HLS].largura = 1920; L.it[0].f[F_HLS].altura = 1080;
  L.it[3].f[F_TS].tocou = 1; L.it[3].f[F_TS].falha = LTD_OK; L.it[3].f[F_TS].quadroMs = 14200;
  L.it[3].f[F_TS].largura = 1280; L.it[3].f[F_TS].altura = 720;
  L.it[4].f[F_HLS].tocou = 1; L.it[4].f[F_HLS].falha = LTD_OK; L.it[4].f[F_HLS].quadroMs = 5300;
  L.it[4].f[F_HLS].altura = 1080;
  recomendar();
  assert(L.rec.formato == 1 && L.rec.espera == 1 && L.rec.semDecoder >= 2);
  L.botao = 0;
  snprintf(nome, sizeof nome, "%s-2-tv-c4.bmp", saida);
  captura(nome);
  L.aplicado = 1; L.botao = 1;
  snprintf(nome, sizeof nome, "%s-3-aplicado.bmp", saida);
  captura(nome);

  // --- 4. no meio ---
  for (i = 2; i < L.n; i++) L.it[i].pronto = 0;
  L.atual = 2; L.estado = E_REDE;
  snprintf(nome, sizeof nome, "%s-4-testando.bmp", saida);
  captura(nome);
#ifdef NV_TEST_PHONE_SHOT_H
  L.pfVivo = 0; L.estado = E_PRONTO;
  L.n = 0; L.xtConfig = 0; L.redeMedida = 0; L.recModo = -1; memset(&L.rec, 0, sizeof L.rec);
  snprintf(nome, sizeof nome, "%s-5-empty.bmp", saida); captura(nome);
  L.n = 6; L.estado = E_PLAYER; L.atual = 2; L.pfFormato = F_HLS; L.pfVivo = 1; L.pfDesde = SDL_GetTicks();
  snprintf(nome, sizeof nome, "%s-6-player-window-layout.bmp", saida); captura(nome);
  L.pfVivo = 0; L.estado = E_PRONTO; L.redeMedida = 1; L.kbps = 9000;
  recomendar();
  L.rec.dezBits = 1; L.rec.semDecoder = 3; L.latenciaMs = 1200; L.recModo = M_B; L.tocouModo[M_P] = 1; L.enviou = 1;
  snprintf(nome, sizeof nome, "%s-7-full-result.bmp", saida); captura(nome);
  ltdPhone.offset = ltdPhone.rolagem.maximo;
  snprintf(nome, sizeof nome, "%s-8-result-bottom.bmp", saida); captura(nome);
#endif
  puts("livetvdiag: tudo ok");
  return 0;
}
