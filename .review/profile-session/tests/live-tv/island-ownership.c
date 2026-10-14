#include "support.h"
#include "ilha.c"
const char *i18n(const char *s) { return s; }
Uint32 SDL_GetTicks(void) { return 100; }
static void fake_body(GfxRect rect, float opacity, void *ctx) { (void)rect; (void)opacity; (void)ctx; }
static void aviso(const char *key, const char *text) {
  IlhaAvisoEx e = {0}; e.chave = key; e.texto = text; e.tipo = ILHA_INFO;
  ilha_avisar_ex(&e);
}
int main(void) {
  fixture_init(); perfis_ao_abrir_sessao(ilha_abrir_sessao);
  ilha_relogio_visivel(1); ilha_ancorar(700, 40, 1);
  aviso("lembrete-curto", "Private A programme starts now");
  aviso("pedido:friendA", "Private A friend and avatar");
  aviso("av:update:2.0.6", "Device update available");
  aviso("av:crash:last-session", "Device closed unexpectedly");
  aviso("av:canal:notice", "Public developer announcement");
  mostraA = cur; mostra = M_AVISO; strcpy(mostraChave, cur.chave);
  for (int i = 0; i < ILHA_N_CARTOES; i++) {
    IlhaCartao c = {0}; strcpy(c.chave, "private_A"); strcpy(c.titulo, "Private A title");
    strcpy(c.imdb, "private_A"); ilha_cartao(i, &c);
  }
  mostraC = modalC = pedidoC = cartoes[0]; mostraQual = cartaoVez = modalQual = pedidoQual = 0;
  modalAberto = modalAviso = pedido = avPedido = pontoEnquete = pontoPediu = 1;
  strcpy(avPedidoChave, "pedido:friendA"); strcpy(modalM.rosto, "https://a.example/private_avatar");
  strcpy(modalM.texto, "Private A modal"); strcpy(modalChave, "pedido:friendA");
  strcpy(atvTexto, "Private A addon loading"); strcpy(atvDetalhes, "Private A loading details"); atvVisto = 100;
  corpoFn = fake_body; corpoU = &modalC; corpoPed = 1;
  voo = vooDissolve = svoo = svooQuer = 1; strcpy(vooArte, "https://a.example/private_poster");
  strcpy(svooPoster, "https://a.example/private_poster"); marcarMostrado("pedido:friendA");
  perfis_definir_ativo(2);
  assert(!ilha_tem("lembrete-curto") && !ilha_tem("pedido:friendA"));
  assert(!mostraA.texto[0] && !mostraChave[0] && !modalM.texto[0] && !modalM.rosto[0]);
  assert(!atvTexto[0] && !atvDetalhes[0] && !corpoFn && !corpoU);
  assert(!voo && !svoo && !vooArte[0] && !svooPoster[0]);
  for (int i = 0; i < ILHA_N_CARTOES; i++) assert(!temCartao[i] && !cartoes[i].titulo[0]);
  assert(!mostraC.titulo[0] && !modalC.titulo[0] && !pedidoC.titulo[0]);
  IlhaCartao c; char key[80]; assert(!ilha_pediu(&c, NULL) && !ilha_aviso_pediu(key, sizeof key));
  assert(!ilha_modal_visivel() && !ilha_mostrou("pedido:friendA") && !ilha_ponto_pediu());
  assert(relogioQuer && ancDef && ancX == 700 && ancY == 40 && ancDir);
  puts("PASS: real profile selection removes private queued/rendered notices, cards, requests, activity and artwork");
  assert(ilha_tem("av:update:2.0.6") && ilha_tem("av:crash:last-session") && ilha_tem("av:canal:notice"));
  assert(!strcmp(ilha_aviso_vez(), "av:update:2.0.6"));
  aviso("perfil", "Now in profile B");
  assert(nFila == 3); for (int i = 0; i < nFila; i++) assert(!strstr(fila[i].texto, "Private A"));
  puts("PASS: update, crash and public developer alerts survive; clock placement is unchanged");
  // An already visible device alert also survives a fresh opening of the
  // same profile, while its old profile-name notice cannot survive.
  uint64_t old = perfis_sessao_geracao(); perfis_definir_ativo(2);
  assert(perfis_sessao_geracao() != old && !ilha_tem("perfil"));
  assert(ilha_tem("av:update:2.0.6") && ilha_tem("av:crash:last-session"));
  puts("PASS: reopening the same profile discards its previous UI while retaining device alerts");
  fixture_cleanup();
}
