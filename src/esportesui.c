#include "esportesui.h"
#include "esportes.h"
#include "guia.h"
#include "epg.h"
#include "xtepg.h"
#include "ajustes.h"
#include "idioma.h"
#include "buscanorm.h"
#include "menu.h"
#include "plrui.h"
#include "tex_cache.h"
#include "ponteiro.h"
#include "teclado.h"
#include "teclavoltar.h"
#include "rolagemtoque.h"
#include "telefoneui.h"
#define NV_ESCALA_TELA
#include "escala.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { ESP_HOME, ESP_LIGAS, ESP_EVENTOS, ESP_DETALHE, ESP_CANAIS, ESP_CONFIG };
static int pagina, modalidade, foco, sair, pediuRelogio, soFavoritos, entradaTexto;
static int indices[ESP_MAX_LIGAS], nItens;
static EspLiga liga;
static EspEvento evento;
static char consulta[160], pedidoCanal[128], aviso[160];
static unsigned revisao;
static float rolagem, topoLista, alturaLista, larguraLista, xLista, linhaH;
static Uint32 ultimoRefresh;
static int retornoFoco[6];
#ifdef NV_TOUCH_UI
static ToqueRolagem toque;
static int rolar(const PonteiroRolagem *e) { return toquerol_evento(&toque, e); }
#endif
typedef struct { char id[128], nome[200], fonte[120], programa[240]; time_t ini, fim; int confirmado; } EspCanal;
static EspCanal canais[64];
static int nCanais, cursorCanal, cursorPrograma, procurando;
static unsigned guiaRevisao, epgRevisao;
static Uint32 buscaDesde;

static void limparToque(void) {
#ifdef NV_TOUCH_UI
  toquerol_limpar(&toque);
#endif
}
static int contem(const char *s) {
  if (!consulta[0]) return 1;
  char n[512], q[180]; busca_normalizar(s, n, sizeof n); busca_normalizar(consulta, q, sizeof q);
  return strstr(n, q) != NULL;
}
static void montar(void) {
  nItens = 0;
  if (pagina == ESP_HOME || pagina == ESP_CONFIG) {
    for (int i = 0; i < esportes_modalidades_n(); i++)
      if ((pagina == ESP_CONFIG || esportes_habilitado(i)) && contem(i18n(esportes_modalidade(i)->nome))) indices[nItens++] = i;
  } else if (pagina == ESP_LIGAS) {
    for (int i = 0; i < esportes_ligas_n() && nItens < ESP_MAX_LIGAS; i++)
      if (contem(esportes_liga(i)->nome) || contem(esportes_liga(i)->id)) indices[nItens++] = i;
  } else if (pagina == ESP_EVENTOS) {
    for (int i = 0; i < esportes_eventos_n(); i++) {
      const EspEvento *e = esportes_evento(i);
      if (soFavoritos && !esportes_favorito(e, 0) && !esportes_favorito(e, 1)) continue;
      if (contem(e->nome) || contem(e->liga)) indices[nItens++] = i;
    }
  } else if (pagina == ESP_DETALHE) nItens = 1 + evento.nParticipantes;
  else nItens = nCanais;
  if (foco >= nItens) foco = nItens > 0 ? nItens - 1 : 0;
}
static void abrir(int p) {
  retornoFoco[pagina] = foco;
  pagina = p; foco = 0; rolagem = 0; consulta[0] = aviso[0] = 0;
  limparToque(); montar();
}
void esportesui_iniciar(void) {
  esportes_abrir_sessao(); pagina = ESP_HOME; foco = 0; sair = pediuRelogio = 0;
  entradaTexto = soFavoritos = 0; consulta[0] = aviso[0] = pedidoCanal[0] = 0;
  rolagem = 0; procurando = 0; limparToque(); montar(); revisao = esportes_revisao();
}
void esportesui_fechar(void) { esportes_cancelar(); procurando = entradaTexto = 0; limparToque(); }
int esportesui_quer_sair(void) { int v = sair; sair = 0; return v; }
int esportesui_pediu_relogio(void) { int v = pediuRelogio; pediuRelogio = 0; return v; }
int esportesui_pediu_canal(char *id, unsigned n) {
  if (!pedidoCanal[0]) return 0;
  snprintf(id, n, "%s", pedidoCanal); pedidoCanal[0] = 0; return 1;
}
static void buscarCanais(void) {
  guia_carregar(); epg_iniciar(); nCanais = cursorCanal = cursorPrograma = 0;
  procurando = 1; buscaDesde = SDL_GetTicks();
  guiaRevisao = guia_busca_revisao(); epgRevisao = epg_revisao();
}
static void voltar(void) {
  procurando = 0;
  if (pagina == ESP_HOME) { sair = 1; return; }
  int p = pagina == ESP_CONFIG ? ESP_HOME : pagina == ESP_CANAIS ? ESP_DETALHE : pagina - 1;
  abrir(p); foco = retornoFoco[p]; montar();
}
static void agir(void) {
  if (foco < 0 || foco >= nItens) return;
  if (pagina == ESP_CONFIG) {
    int m = indices[foco];
    if (!esportes_habilitar(m, !esportes_habilitado(m))) snprintf(aviso, sizeof aviso, "%s", i18n("Não foi possível salvar"));
  } else if (pagina == ESP_HOME) {
    modalidade = indices[foco]; abrir(ESP_LIGAS); esportes_pedir_ligas(modalidade, 0);
  } else if (pagina == ESP_LIGAS) {
    const EspLiga *l = esportes_liga(indices[foco]); if (!l) return;
    liga = *l; abrir(ESP_EVENTOS); esportes_pedir_eventos(modalidade, &liga, 0); ultimoRefresh = SDL_GetTicks();
  } else if (pagina == ESP_EVENTOS) {
    const EspEvento *e = esportes_evento(indices[foco]); if (!e) return;
    evento = *e; abrir(ESP_DETALHE);
  } else if (pagina == ESP_DETALHE) {
    if (foco == 0) { abrir(ESP_CANAIS); buscarCanais(); }
    else if (!esportes_alternar_favorito(&evento, foco - 1)) snprintf(aviso, sizeof aviso, "%s", i18n("Não foi possível salvar"));
  } else if (pagina == ESP_CANAIS) {
    EspCanal *c = &canais[foco];
    if (c->confirmado && c->ini > time(NULL)) {
      snprintf(aviso, sizeof aviso, "%s", i18n("Este evento ainda não começou")); return;
    }
    snprintf(pedidoCanal, sizeof pedidoCanal, "%s", c->id);
  }
}
static int nTopo(void) { return pagina == ESP_LIGAS || pagina == ESP_EVENTOS ? 4 : 3; }
static void acaoTopo(int i) {
  if (i == 0) { voltar(); return; }
  if (i == 1 && (pagina == ESP_DETALHE || pagina == ESP_CANAIS)) {
    if (pagina == ESP_DETALHE) abrir(ESP_CANAIS);
    buscarCanais(); return;
  }
  if (i == 1) {
    entradaTexto = 1; teclado_abrir_com("Buscar esportes", "Digite para buscar", 159, teclado_alfabeto(), consulta);
  } else if (i == 2) {
    if (pagina == ESP_CONFIG) voltar(); else { procurando = 0; esportes_cancelar(); abrir(ESP_CONFIG); }
  } else if (i == 3 && pagina == ESP_EVENTOS) { soFavoritos = !soFavoritos; foco = 0; montar(); limparToque(); }
  else if (i == 3) {
    if (pagina == ESP_LIGAS) esportes_pedir_ligas(modalidade, 1);
    else if (pagina == ESP_CANAIS) buscarCanais();
  }
}
static void pointerFoco(int i, int b) { (void)b; foco = i; }
static void pointerAtivar(int i, int b) { pointerFoco(i,b); limparToque(); agir(); }
static void pointerTopo(int i, int b) { (void)b; foco = -i-1; }
static void pointerTopoAgir(int i, int b) { pointerTopo(i,b); limparToque(); acaoTopo(i); }
void esportesui_evento(const SDL_Event *e) {
  if (entradaTexto) { teclado_evento(e); return; }
  if (e->type != SDL_KEYDOWN) return;
  SDL_Keycode k = e->key.keysym.sym;
  if (nv_tecla_voltar(e)) { voltar(); return; }
  limparToque();
  if (k == SDLK_LEFT) { if (foco < -1) foco++; else sair = 1; }
  else if (k == SDLK_RIGHT && foco < 0 && -foco < nTopo()) foco--;
  else if (k == SDLK_UP) { if (foco < 0) pediuRelogio = 1; else foco = foco > 0 ? foco - 1 : -1; }
  else if (k == SDLK_DOWN) { if (foco < 0) foco = nItens ? 0 : -1; else if (foco + 1 < nItens) foco++; }
  else if (k == SDLK_PAGEUP) foco = foco > 8 ? foco - 8 : 0;
  else if (k == SDLK_PAGEDOWN) foco = foco + 8 < nItens ? foco + 8 : nItens - 1;
  else if ((k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) && !e->key.repeat) {
    if (foco < 0) acaoTopo(-foco-1); else agir();
  }
}
static void adicionar(const char *id, const char *nome, const char *fonte, const EpgProg *p, int confirmado) {
  for (int i = 0; i < nCanais; i++) if (!strcmp(canais[i].id, id)) {
    if (!confirmado || canais[i].confirmado) return;
    memmove(canais+i, canais+i+1, (size_t)(--nCanais-i)*sizeof *canais); break;
  }
  if (nCanais >= 64) {
    if (!confirmado || canais[63].confirmado) return;
    nCanais--;
  }
  int pos = nCanais;
  if (confirmado) { for (pos = 0; pos < nCanais && canais[pos].confirmado; pos++); memmove(canais+pos+1, canais+pos, (size_t)(nCanais-pos)*sizeof *canais); }
  EspCanal *c = &canais[pos]; memset(c, 0, sizeof *c);
  snprintf(c->id, sizeof c->id, "%s", id); snprintf(c->nome, sizeof c->nome, "%s", nome);
  snprintf(c->fonte, sizeof c->fonte, "%s", fonte); c->confirmado = confirmado;
  if (p) { c->ini = p->ini; c->fim = p->fim; snprintf(c->programa, sizeof c->programa, "%s", p->titulo ? p->titulo : ""); }
  nCanais++; nItens = nCanais; if (foco < 0 && nItens == 1) foco = 0;
}
static void buscarPasso(void) {
  if (!procurando) return;
  epg_passo(); xtepg_passo();
  if (guiaRevisao != guia_busca_revisao() || epgRevisao != epg_revisao()) {
    guiaRevisao = guia_busca_revisao(); epgRevisao = epg_revisao(); nCanais = cursorCanal = cursorPrograma = 0;
  }
  Uint32 inicio = SDL_GetTicks(); int lote = 0;
  for (; lote++ < 24 && SDL_GetTicks() - inicio < 3; ) {
    const char *id, *nome; const char *fonte;
    if (!guia_canal_campos(cursorCanal, &id, &nome, NULL, NULL, NULL)) {
      if (guia_busca_carregando() || epg_estado() == EPG_BAIXANDO) { cursorCanal = cursorPrograma = 0; return; }
      procurando = 0; montar(); return;
    }
    if (!guia_busca_elegivel(cursorCanal)) { cursorCanal++; cursorPrograma=0; continue; }
    fonte = guia_busca_conta(cursorCanal);
    EpgProg ps[16]; time_t ate = evento.fim > evento.inicio ? evento.fim : evento.inicio + 6*3600;
    int n = guia_busca_programas(cursorCanal, evento.inicio - 3*3600, ate, cursorPrograma, ps, 16);
    for (int k = 0; k < n; k++) if (esportes_corresponde(&evento, ps[k].titulo, ps[k].ini, ps[k].fim)) adicionar(id,nome,fonte,&ps[k],1);
    if (cursorPrograma == 0 && esportes_emissora(&evento, nome)) adicionar(id,nome,fonte,NULL,0);
    if (n == 16) cursorPrograma += n; else { cursorCanal++; cursorPrograma = 0; }
  }
  if (SDL_GetTicks() - buscaDesde > 30000) { procurando = 0; montar(); }
}
void esportesui_atualizar(float dt, Uint32 agora) {
  (void)dt;
  char selecionado[100]="";
  if (pagina == ESP_EVENTOS && foco >= 0 && foco < nItens) {
    const EspEvento *e=esportes_evento(indices[foco]);
    if (e) { memcpy(selecionado,e->id,sizeof selecionado); selecionado[sizeof selecionado-1]=0; }
  }
  esportes_passo();
  if (entradaTexto) {
    teclado_atualizar(dt, agora); int r = teclado_resultado();
    if (r) { if (r > 0) snprintf(consulta, sizeof consulta, "%s", teclado_texto()); teclado_esquecer(); entradaTexto = 0; foco = 0; rolagem = 0; montar(); }
  }
  if (revisao != esportes_revisao()) {
    revisao = esportes_revisao();
    if (pagina == ESP_DETALHE) for (int i=0;i<esportes_eventos_n();i++) {
      const EspEvento *e = esportes_evento(i);
      if (!strcmp(e->id,evento.id)) { evento=*e; break; }
    }
    montar();
    if (selecionado[0]) for (int i=0;i<nItens;i++) {
      if (!strcmp(esportes_evento(indices[i])->id,selecionado)) { foco=i; break; }
    }
  }
  buscarPasso();
  if ((pagina == ESP_EVENTOS || pagina == ESP_DETALHE) && !entradaTexto && !esportes_carregando() && agora - ultimoRefresh >= 60000) {
    /* Refresh only the visible league. Never poll hundreds of leagues or
     * touch a media URL while playback is active. */
    ultimoRefresh = agora; esportes_pedir_eventos(modalidade, &liga, 1);
  }
  if (foco >= 0 && alturaLista > 0) {
#ifdef NV_TOUCH_UI
    if (toquerol_livre(&toque)) return;
#endif
    float y = foco * linhaH;
    if (y < rolagem) rolagem = y;
    if (y + linhaH > rolagem + alturaLista) rolagem = y + linhaH - alturaLista;
  }
}
static void txt(TxtEstilo est, const char *s, float x, float y, float w, int cor) {
  TxtLinha t = txt_linha_corta(est,s,cor,cor,cor,255,w); txt_desenhar(t,x,y);
}
static void horario(time_t quando, char *out, size_t n) {
  struct tm tm; if (!localtime_r(&quando,&tm)) { out[0] = 0; return; }
  strftime(out,n,"%d/%m %H:%M",&tm);
}
static void logo(const char *url, GfxRect r) {
  if (!url[0]) return;
  GLuint t = tex_obter_logo_larg(url,r.w);
  if (t) gfx_rect(r,t,GFX_TEXTO,0,0,0,0,1,1,1,1);
}
static void linhaEvento(const EspEvento *e, GfxRect r, int selecionado) {
  plrui_cartao(r,20,1); if (selecionado) plrui_linha_foco(r,20,1);
  float pad = 22, x = r.x+pad, w=r.w-2*pad;
  char data[40], meta[400]; horario(e->inicio,data,sizeof data);
  const char *s = e->estado == 1 ? i18n("Ao vivo") : e->estado == 2 ? i18n("Encerrado") : e->estado == 3 ? e->detalhe : data;
  snprintf(meta,sizeof meta,"%s · %s%s%s",s,e->liga,e->sessao[0]?" · ":"",e->sessao);
  txt(TXT_CAPTION2,meta,x,r.y+16,w,170);
  if (e->nParticipantes == 2) {
    for (int i=0;i<2;i++) {
      const EspParticipante *p=&e->participantes[i]; float y=r.y+52+i*52;
      logo(p->logo,(GfxRect){x,y,36,36});
      float scoreW=strlen(p->pontos)>3?112:55;
      txt(TXT_BODY,p->nome,x+50,y+3,w-scoreW-65,245);
      if (e->estado == 1 || e->estado == 2) txt(TXT_BODY,p->pontos,r.x+r.w-scoreW-15,y+3,scoreW,245);
    }
  } else txt(TXT_BODY,e->nome,x,r.y+62,w,245);
  if (e->local[0]) txt(TXT_CAPTION2,e->local,x,r.y+r.h-38,w,158);
}
void esportesui_desenhar(Uint32 agora) {
  ESCALA_INI();
  float margem = telefoneui_ativo() ? 42 : 72;
  xLista = fmaxf(margem, menu_barra_borda()/gfx_escala()+30);
  larguraLista = NV_TELA_W-xLista-margem;
  if (larguraLista < 100) larguraLista = 100;
  txt(TXT_TITULO2,i18n("Esportes"),xLista,68,larguraLista,245);
  const char *sub = pagina == ESP_HOME || pagina == ESP_CONFIG ? "ESPN" : pagina == ESP_LIGAS ? i18n(esportes_modalidade(modalidade)->nome) : pagina == ESP_EVENTOS ? liga.nome : evento.nome;
  txt(TXT_CAPTION2,sub,xLista,126,larguraLista,170);
  const char *botoes[]={"Voltar",pagina==ESP_DETALHE?"Encontrar canais":pagina==ESP_CANAIS?"Atualizar":"Buscar","Esportes",pagina==ESP_EVENTOS?(soFavoritos?"Todos":"Favoritos"):"Atualizar"};
  int nBotoes=nTopo();
  float bw=fminf(210,(larguraLista-(nBotoes-1)*12)/nBotoes), by=172;
  for (int i=0;i<nBotoes;i++) {
    GfxRect r={xLista+i*(bw+12),by,bw,60};
    if (foco == -i-1) plrui_pilula_foco(r,1); else plrui_botao_repouso(r,1);
    txt(TXT_CAPTION2,i18n(botoes[i]),r.x+16,r.y+18,r.w-32,245);
    ponteiro_alvo(r.x,r.y,r.w,r.h,pointerTopo,pointerTopoAgir,i,0);
  }
  topoLista=252; alturaLista=NV_TELA_H-topoLista-70;
  linhaH=pagina==ESP_EVENTOS?218:pagina==ESP_CANAIS?120:92;
  if (pagina == ESP_DETALHE) {
    GfxRect r={xLista,topoLista,larguraLista,205}; linhaEvento(&evento,r,0);
    topoLista+=225; alturaLista-=225;
  }
  if (alturaLista<60) alturaLista=60;
#ifdef NV_TOUCH_UI
  toquerol_vincular(&toque,(GfxRect){xLista,topoLista,larguraLista,alturaLista},gfx_escala(),0,nItens*linhaH-alturaLista,1,&rolagem);
  ponteiro_rolagem(rolar);
#endif
  gfx_recorte(xLista-5,topoLista,larguraLista+10,alturaLista);
  for (int i=0;i<nItens;i++) {
    GfxRect r={xLista,topoLista+i*linhaH-rolagem,larguraLista,linhaH-12};
    if (r.y+r.h<topoLista || r.y>topoLista+alturaLista) continue;
    if (pagina==ESP_EVENTOS) {
      const EspEvento *e=esportes_evento(indices[i]);
      if (e) linhaEvento(e,r,foco==i);
    }
    else {
      if (foco==i) plrui_linha_foco(r,18,1);
      const char *nome="", *subt=""; char s[400];
      if (pagina==ESP_HOME || pagina==ESP_CONFIG) {
        int m=indices[i]; nome=i18n(esportes_modalidade(m)->nome);
        subt=pagina==ESP_CONFIG?(esportes_habilitado(m)?i18n("Ligado"):i18n("Desligado")):i18n("Ligas e eventos");
      } else if (pagina==ESP_LIGAS) { nome=esportes_liga(indices[i])->nome; subt=i18n(esportes_modalidade(modalidade)->nome); }
      else if (pagina==ESP_DETALHE) {
        nome=i==0?i18n("Encontrar canais"):evento.participantes[i-1].nome;
        subt=i==0?i18n("Buscar este evento no seu guia"):esportes_favorito(&evento,i-1)?i18n("Remover dos favoritos"):i18n("Adicionar aos favoritos");
      } else if (pagina==ESP_CANAIS) {
        const EspCanal *c=&canais[i]; nome=c->nome;
        snprintf(s,sizeof s,"%s · %s",c->fonte,c->confirmado?c->programa:i18n("Possível emissora; evento não confirmado")); subt=s;
      }
      txt(TXT_BODY,nome,r.x+22,r.y+14,r.w-44,245);
      txt(TXT_CAPTION2,subt,r.x+22,r.y+51,r.w-44,170);
    }
    ponteiro_alvo_faixa(r.x,r.y,r.w,r.h,topoLista,topoLista+alturaLista,pointerFoco,pointerAtivar,i,0);
  }
  if (!nItens) {
    const char *s = procurando ? "Buscando no guia…" : esportes_carregando() ? "Carregando…" : esportes_erro() ? "Não foi possível carregar. Use Atualizar para tentar novamente." : pagina==ESP_CANAIS ? "Nenhum canal encontrado para este evento" : "Nenhum resultado";
    txt(TXT_BODY,i18n(s),xLista+22,topoLista+30,larguraLista-44,170);
  }
  gfx_sem_recorte();
  const char *rodape = aviso[0]?aviso:esportes_erro()?i18n("Não foi possível atualizar os eventos"):pagina==ESP_EVENTOS && esportes_eventos_n()==ESP_MAX_EVENTOS?i18n("Mostrando os primeiros 400 eventos"):pagina==ESP_CANAIS?i18n("Correspondências do guia primeiro; emissoras sugeridas não confirmam o evento"):"ESPN";
  txt(TXT_CAPTION2,rodape,xLista,NV_TELA_H-48,larguraLista,158);
  ESCALA_FIM();
  if (entradaTexto) teclado_desenhar(agora);
}
