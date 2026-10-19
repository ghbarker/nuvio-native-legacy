#include "esportesui.h"
#include "esportes.h"
#include "streams.h"
#include "fontescanais.h"
#include "ctxlista.h"
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
#include "horafmt.h"
#define NV_ESCALA_TELA
#include "escala.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { ESP_LIGAS, ESP_EVENTOS, ESP_DETALHE, ESP_CONFIG };
#define FOCO_MODALIDADE 100
static int pagina, modalidade, foco, sair, pediuRelogio, soFavoritos, entradaTexto;
static int indices[ESP_MAX_LIGAS], nItens;
static EspLiga liga;
static EspEvento evento;
static char consulta[160], aviso[160];
static unsigned revisao;
static float rolagem, menuRolagem, topoLista, alturaLista, larguraLista, xLista, linhaH;
static int colunas = 1;
static float vao = 24;
static int modalidadeFoco(int f) { return -f - FOCO_MODALIDADE; }
static int primeiroEsporte(void) {
  for (int i=0;i<esportes_modalidades_n();i++) if (esportes_habilitado(i)) return i;
  return -1;
}
static int esporteVizinho(int m, int d) {
  for (int i=m+d;i>=0 && i<esportes_modalidades_n();i+=d) if (esportes_habilitado(i)) return i;
  return m;
}
static void medidas(void) {
  float margem=telefoneui_ativo()?48:72;
  xLista=fmaxf(margem,menu_barra_borda()/gfx_escala_ui()+30);
  larguraLista=fmaxf(100,NV_TELA_W-xLista-margem);
  float minimo=pagina==ESP_LIGAS?270:470;
  colunas=(pagina==ESP_LIGAS || pagina==ESP_EVENTOS)?(int)((larguraLista+vao)/(minimo+vao)):1;
  if (colunas<1) colunas=1;
  if (colunas>5) colunas=5;
  linhaH=pagina==ESP_LIGAS?292:pagina==ESP_EVENTOS?286:92;
}
static int linhas(void) { return (nItens+colunas-1)/colunas; }
static Uint32 ultimoRefresh;
static int retornoFoco[4];
static CtxHold eventoHold;
#ifdef NV_TOUCH_UI
static ToqueRolagem toque, toqueMenu;
static int rolar(const PonteiroRolagem *e) {
  return toquerol_evento(&toqueMenu,e) || toquerol_evento(&toque,e);
}
#endif
typedef struct { char id[128], nome[200], fonte[120], programa[240]; time_t ini, fim; int confirmado; } EspCanal;
static EspCanal canais[64];
static int nCanais, cursorCanal, cursorPrograma, procurando, canaisSujos;
static unsigned guiaRevisao, epgRevisao;
static Uint32 buscaDesde;

static void limparToque(void) {
#ifdef NV_TOUCH_UI
  toquerol_limpar(&toque);
  toquerol_limpar(&toqueMenu);
#endif
}
static int contem(const char *s) {
  if (!consulta[0]) return 1;
  char n[512], q[180]; busca_normalizar(s, n, sizeof n); busca_normalizar(consulta, q, sizeof q);
  return strstr(n, q) != NULL;
}
static void montar(void) {
  nItens = 0;
  if (pagina == ESP_CONFIG) {
    for (int i = 0; i < esportes_modalidades_n(); i++)
      if (contem(i18n(esportes_modalidade(i)->nome))) indices[nItens++] = i;
  } else if (pagina == ESP_LIGAS && modalidade>=0) {
    for (int i = 0; i < esportes_ligas_n() && nItens < ESP_MAX_LIGAS; i++)
      if (contem(esportes_liga(i)->nome) || contem(esportes_liga(i)->id)) indices[nItens++] = i;
  } else if (pagina == ESP_EVENTOS) {
    for (int i = 0; i < esportes_eventos_n(); i++) {
      const EspEvento *e = esportes_evento(i);
      if (soFavoritos && !esportes_favorito(e, 0) && !esportes_favorito(e, 1)) continue;
      if (contem(e->nome) || contem(e->liga)) indices[nItens++] = i;
    }
  } else if (pagina == ESP_DETALHE) nItens = 1 + evento.nParticipantes;
  if (foco >= nItens) foco = nItens > 0 ? nItens - 1 : 0;
}
static void abrir(int p) {
  retornoFoco[pagina] = foco;
  pagina = p; foco = 0; rolagem = 0; consulta[0] = aviso[0] = 0;
#ifdef NV_TOUCH_UI
  memset(&toqueMenu,0,sizeof toqueMenu);
#endif
  medidas();
  ctxhold_cancelar(&eventoHold);limparToque(); montar();
}
void esportesui_iniciar(void) {
  esportes_abrir_sessao(); pagina = ESP_LIGAS; modalidade=primeiroEsporte();
  foco = modalidade>=0?-FOCO_MODALIDADE-modalidade:0; sair = pediuRelogio = 0;
  entradaTexto = soFavoritos = 0; consulta[0] = aviso[0] = 0;
  ctxhold_cancelar(&eventoHold);
  rolagem = menuRolagem = 0; procurando = 0; limparToque(); medidas(); montar();
  if (modalidade>=0) esportes_pedir_ligas(modalidade,0);
  montar(); revisao = esportes_revisao();
}
void esportesui_fechar(void) { stream_folha_canais_fechar();ctxhold_cancelar(&eventoHold);esportes_cancelar(); procurando = entradaTexto = 0; limparToque(); }
int esportesui_quer_sair(void) { int v = sair; sair = 0; return v; }
int esportesui_pediu_relogio(void) { int v = pediuRelogio; pediuRelogio = 0; return v; }
int esportesui_pediu_canal(char *id, unsigned n) { return fontescanais_consumir(id,n); }
static void publicarCanais(void) {
  if(!canaisSujos)return;
  FonteCanal lista[FONTESCANAIS_MAX];
  for(int i=0;i<nCanais;i++) {
    FonteCanal *c=&lista[i];memset(c,0,sizeof *c);
    memcpy(c->id,canais[i].id,sizeof c->id);
    memcpy(c->nome,canais[i].nome,sizeof c->nome);
    memcpy(c->conta,canais[i].fonte,sizeof c->conta);
    memcpy(c->programa,canais[i].programa,sizeof c->programa);c->inicio=canais[i].ini;c->fim=canais[i].fim;
  }
  fontescanais_publicar(lista,nCanais,procurando);canaisSujos=0;
}
static void buscarCanais(void) {
  guia_carregar(); epg_iniciar(); nCanais = cursorCanal = cursorPrograma = 0;
  procurando = canaisSujos = 1; buscaDesde = SDL_GetTicks();
  guiaRevisao = guia_busca_revisao(); epgRevisao = epg_revisao();
  publicarCanais();
}
static void voltar(void) {
  procurando = 0;
  if (pagina == ESP_LIGAS) { sair = 1; return; }
  int p = pagina == ESP_CONFIG ? ESP_LIGAS : pagina - 1;
  int configurar=pagina==ESP_CONFIG;
  abrir(p); foco=retornoFoco[p];
  if (p==ESP_LIGAS && configurar) {
    if (!esportes_habilitado(modalidade)) modalidade=primeiroEsporte();
    if (modalidade>=0) { esportes_pedir_ligas(modalidade,0);foco=-FOCO_MODALIDADE-modalidade; }
    else foco=0;
  }
  montar();
}
static void agir(void) {
  if (foco < 0 || foco >= nItens) return;
  if (pagina == ESP_CONFIG) {
    int m = indices[foco];
    if (!esportes_habilitar(m, !esportes_habilitado(m))) snprintf(aviso, sizeof aviso, "%s", i18n("Não foi possível salvar"));
  } else if (pagina == ESP_LIGAS) {
    const EspLiga *l = esportes_liga(indices[foco]); if (!l) return;
    liga = *l; abrir(ESP_EVENTOS); esportes_pedir_eventos(modalidade, &liga, 0); ultimoRefresh = SDL_GetTicks();
  } else if (pagina == ESP_EVENTOS) {
    const EspEvento *e = esportes_evento(indices[foco]); if (!e) return;
    evento = *e;stream_folha_canais_abrir(evento.nome);buscarCanais();
  } else if (pagina == ESP_DETALHE) {
    if (foco == 0) { stream_folha_canais_abrir(evento.nome);buscarCanais(); }
    else if (!esportes_alternar_favorito(&evento, foco - 1)) snprintf(aviso, sizeof aviso, "%s", i18n("Não foi possível salvar"));
  }
}
static int nTopo(void) { return pagina == ESP_LIGAS || pagina == ESP_EVENTOS ? 4 : 3; }
static void acaoTopo(int i) {
  if (i == 0) { voltar(); return; }
  if (i == 1 && pagina == ESP_DETALHE) {
    stream_folha_canais_abrir(evento.nome);buscarCanais();return;
  }
  if (i == 1) {
    entradaTexto = 1; teclado_abrir_com("Buscar esportes", "Digite para buscar", 159, teclado_alfabeto(), consulta);
  } else if (i == 2) {
    if (pagina == ESP_CONFIG) voltar(); else { procurando = 0; esportes_cancelar(); abrir(ESP_CONFIG); }
  } else if (i == 3 && pagina == ESP_EVENTOS) { soFavoritos = !soFavoritos; foco = 0; montar(); limparToque(); }
  else if (i == 3) {
    if (pagina == ESP_LIGAS && modalidade>=0) esportes_pedir_ligas(modalidade, 1);
  }
}
static void pointerFoco(int i, int b) { (void)b; foco = i; }
static void pointerAtivar(int i, int b) { pointerFoco(i,b); limparToque(); agir(); }
static void pointerTopo(int i, int b) { (void)b; foco = -i-1; }
static void pointerTopoAgir(int i, int b) { pointerTopo(i,b); limparToque(); acaoTopo(i); }
static void escolherModalidade(int m) {
  if (!esportes_habilitado(m)) return;
  if (modalidade!=m) {
    modalidade=m; abrir(ESP_LIGAS); esportes_pedir_ligas(m,0);
    montar(); /* A previous sport must never remain selectable while loading. */
  }
  foco=-FOCO_MODALIDADE-m;
}
static void pointerModalidade(int m,int b) { (void)b; foco=-FOCO_MODALIDADE-m; }
static void pointerModalidadeAgir(int m,int b) { (void)b; limparToque(); escolherModalidade(m); }
static void pointerDetalhe(int i,int b) {
  (void)b;foco=i;
  const EspEvento *e=i>=0 && i<nItens?esportes_evento(indices[i]):NULL;
  if (e) { evento=*e;abrir(ESP_DETALHE); }
}
void esportesui_evento(const SDL_Event *e) {
  if (entradaTexto) { teclado_evento(e); return; }
  int gesto=ctxhold_evento(&eventoHold,e,pagina==ESP_EVENTOS && foco>=0 && foco<nItens);
  if (gesto==CTXH_CONSUMIDO) return;
  if (gesto==CTXH_TOQUE) { agir();return; }
  if (gesto==CTXH_LONGO) { pointerDetalhe(foco,0);return; }
  if (e->type != SDL_KEYDOWN) return;
  SDL_Keycode k = e->key.keysym.sym;
  if (k==SDLK_MENU && pagina==ESP_EVENTOS && foco>=0) { pointerDetalhe(foco,0);return; }
  if (nv_tecla_voltar(e)) { voltar(); return; }
  limparToque(); medidas();
  if (foco<=-FOCO_MODALIDADE) {
    int m=modalidadeFoco(foco);
    if (k==SDLK_LEFT || k==SDLK_RIGHT) {
      int outro=esporteVizinho(m,k==SDLK_LEFT?-1:1);
      if (outro==m && k==SDLK_LEFT) sair=1;
      else escolherModalidade(outro);
    } else if (k==SDLK_UP) foco=-1;
    else if (k==SDLK_DOWN || k==SDLK_RETURN || k==SDLK_KP_ENTER || k==SDLK_SPACE) foco=nItens?0:foco;
    return;
  }
  if (k == SDLK_LEFT) {
    if (foco < -1) foco++;
    else if (foco>=0 && foco%colunas>0) foco--;
    else sair=1;
  } else if (k == SDLK_RIGHT) {
    if (foco<0 && -foco<nTopo()) foco--;
    else if (foco>=0 && foco%colunas+1<colunas && foco+1<nItens) foco++;
  } else if (k == SDLK_UP) {
    if (foco<0) pediuRelogio=1;
    else if (foco>=colunas) foco-=colunas;
    else foco=pagina==ESP_LIGAS && modalidade>=0?-FOCO_MODALIDADE-modalidade:-1;
  } else if (k == SDLK_DOWN) {
    if (foco<0) foco=pagina==ESP_LIGAS && modalidade>=0?-FOCO_MODALIDADE-modalidade:nItens?0:-1;
    else if (foco+colunas<nItens) foco+=colunas;
    else if (foco/colunas<(nItens-1)/colunas) foco=nItens-1;
  } else if (k == SDLK_PAGEUP) foco = foco > colunas*3 ? foco-colunas*3 : 0;
  else if (k == SDLK_PAGEDOWN) foco = nItens ? (foco+colunas*3<nItens?foco+colunas*3:nItens-1) : -1;
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
  nCanais++;canaisSujos=1;
}
static void buscarPasso(void) {
  if (!procurando) return;
  epg_passo(); xtepg_passo();
  if (guiaRevisao != guia_busca_revisao() || epgRevisao != epg_revisao()) {
    guiaRevisao = guia_busca_revisao(); epgRevisao = epg_revisao(); nCanais = cursorCanal = cursorPrograma = 0;canaisSujos=1;
  }
  Uint32 inicio = SDL_GetTicks(); int lote = 0;
  for (; lote++ < 24 && SDL_GetTicks() - inicio < 3; ) {
    const char *id, *nome; const char *fonte;
    if (!guia_canal_campos(cursorCanal, &id, &nome, NULL, NULL, NULL)) {
      if (guia_busca_carregando() || epg_estado() == EPG_BAIXANDO) { cursorCanal = cursorPrograma = 0; return; }
      procurando = 0;canaisSujos=1; return;
    }
    if (!guia_busca_elegivel(cursorCanal)) { cursorCanal++; cursorPrograma=0; continue; }
    fonte = guia_busca_conta(cursorCanal);
    EpgProg ps[16]; time_t ate = evento.fim > evento.inicio ? evento.fim : evento.inicio + 6*3600;
    int n = guia_busca_programas(cursorCanal, evento.inicio - 3*3600, ate, cursorPrograma, ps, 16);
    for (int k = 0; k < n; k++) if (esportes_corresponde(&evento, ps[k].titulo, ps[k].ini, ps[k].fim)) adicionar(id,nome,fonte,&ps[k],1);
    /* Broadcaster names alone (ESPN, ESPN+, etc.) do not identify a game.
     * A confirmed programme must name both opponents and overlap its time. */
    if (n == 16) cursorPrograma += n; else { cursorCanal++; cursorPrograma = 0; }
  }
  if (SDL_GetTicks() - buscaDesde > 30000) { procurando=0;canaisSujos=1; }
}
void esportesui_atualizar(float dt, Uint32 agora) {
  (void)dt; medidas();
  if (ctxhold_passo(&eventoHold,agora,1)) pointerDetalhe(foco,0);
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
  if (stream_folha_canais_aberta()) {
    if (fontescanais_pediu_atualizar()) buscarCanais();
    buscarPasso();publicarCanais();
  } else procurando=0;
  if ((pagina == ESP_EVENTOS || pagina == ESP_DETALHE) && !entradaTexto && !esportes_carregando() && agora - ultimoRefresh >= 60000) {
    /* Refresh only the visible league. Never poll hundreds of leagues or
     * touch a media URL while playback is active. */
    ultimoRefresh = agora; esportes_pedir_eventos(modalidade, &liga, 1);
  }
  if (foco >= 0 && alturaLista > 0) {
#ifdef NV_TOUCH_UI
    if (toquerol_livre(&toque)) return;
#endif
    float y = (foco / colunas) * linhaH;
    if (y < rolagem) rolagem = y;
    if (y + linhaH > rolagem + alturaLista) rolagem = y + linhaH - alturaLista;
  }
}
static void txt(TxtEstilo est, const char *s, float x, float y, float w, int cor) {
  TxtLinha t = txt_linha_corta(est,s,cor,cor,cor,255,w); txt_desenhar(t,x,y);
}
static void horario(time_t quando, char *out, size_t n) {
  struct tm tm; char data[24], hora[24];
  if (!localtime_r(&quando,&tm)) { out[0]=0; return; }
  strftime(data,sizeof data,"%d/%m",&tm); hora_tela(hora,sizeof hora,&tm);
  snprintf(out,n,"%s %s",data,hora);
}
/* The same asynchronous, size-bounded image cache as Home and collections.
 * Fit transparent league/team marks rather than stretching them as posters. */
static void logo(const char *url, const char *nome, GfxRect r) {
  GLuint t=url[0]?tex_obter_larg(url,r.w):0;
  float ap=url[0]?tex_aspecto(url):0;
  if (t && ap>0) {
    float w=r.w,h=w/ap;
    if (h>r.h) { h=r.h; w=h*ap; }
    GfxRect arte={r.x+(r.w-w)*0.5f,r.y+(r.h-h)*0.5f,w,h};
    gfx_rect(arte,t,tex_marca_escura(url)?GFX_MARCA:GFX_TEXTO,0,0,0,0,1,1,1,1);
  } else {
    char inicial[8]="";
    /* ASCII short labels are a fallback, never invented remote artwork. */
    for (int i=0,k=0;nome[i] && k<3;i++) if ((unsigned char)nome[i]<128 && nome[i]!=' ' && (i==0 || nome[i-1]==' ')) inicial[k++]=nome[i];
    TxtLinha t=txt_linha_corta(TXT_HEADLINE,inicial,158,158,158,255,r.w);
    txt_desenhar(t,r.x+(r.w-t.w)*0.5f,r.y+(r.h-t.h)*0.5f);
  }
}
static void linhaEvento(const EspEvento *e, GfxRect r, int selecionado) {
  plrui_cartao(r,20,1); if (selecionado) plrui_linha_foco(r,20,1);
  float pad=22,x=r.x+pad,w=r.w-2*pad;
  char data[48],meta[400]; horario(e->inicio,data,sizeof data);
  const char *estado=e->estado==1?i18n("Ao vivo"):e->estado==2?i18n("Encerrado"):e->estado==3?e->detalhe:data;
  snprintf(meta,sizeof meta,"%s%s%s",estado,e->sessao[0]?" · ":"",e->sessao);
  txt(TXT_CAPTION2,meta,x,r.y+16,w,170);
  if (e->nParticipantes==2) {
    for (int i=0;i<2;i++) {
      const EspParticipante *p=&e->participantes[i]; float y=r.y+59+i*72;
      logo(p->logo,p->sigla[0]?p->sigla:p->nome,(GfxRect){x,y,54,54});
      float scoreW=strlen(p->pontos)>3?112:55;
      float reservado=e->estado==1 || e->estado==2?scoreW:0;
      const char *nome=p->curto[0] && r.w<550?p->curto:p->nome;
      txt(TXT_BODY,nome,x+68,y+11,w-reservado-78,245);
      if (reservado) txt(TXT_BODY,p->pontos,r.x+r.w-scoreW-pad,y+11,scoreW,245);
    }
  } else {
    logo(liga.logo,liga.nome,(GfxRect){x,r.y+62,74,74});
    txt(TXT_BODY,e->nome,x+92,r.y+72,w-92,245);
  }
  txt(TXT_CAPTION2,e->local[0]?e->local:e->liga,x,r.y+r.h-38,w,158);
}
static void cartaoLiga(const EspLiga *l,GfxRect r,int selecionado) {
  float arteH=r.h-90,raio=ajustes_raio_poster_px();
  if (raio<8) raio=8;
  plrui_cartao(r,raio,1);
  if (selecionado) plrui_linha_foco(r,raio,1);
  logo(l->logo,l->nome,(GfxRect){r.x+34,r.y+18,r.w-68,arteH-20});
  txt(TXT_BODY,l->nome,r.x+18,r.y+arteH+4,r.w-36,245);
  txt(TXT_CAPTION2,l->id,r.x+18,r.y+r.h-38,r.w-36,170);
}
static void menuModalidades(void) {
  float pos[32],larg[32],total=0;
  for (int m=0;m<esportes_modalidades_n();m++) {
    pos[m]=total;larg[m]=0;
    if (!esportes_habilitado(m)) continue;
    TxtLinha t=txt_linha(TXT_CAPTION2,i18n(esportes_modalidade(m)->nome),245,245,245,255);
    larg[m]=t.w+44;total+=larg[m]+12;
  }
  int alvo=foco<=-FOCO_MODALIDADE?modalidadeFoco(foco):modalidade;
#ifdef NV_TOUCH_UI
  if (!toquerol_livre(&toqueMenu))
#endif
  if (alvo>=0 && alvo<esportes_modalidades_n()) {
    if (pos[alvo]<menuRolagem) menuRolagem=pos[alvo];
    if (pos[alvo]+larg[alvo]>menuRolagem+larguraLista) menuRolagem=pos[alvo]+larg[alvo]-larguraLista;
  }
  menuRolagem=fmaxf(0,fminf(menuRolagem,fmaxf(0,total-larguraLista)));
#ifdef NV_TOUCH_UI
  toquerol_vincular(&toqueMenu,(GfxRect){xLista,252,larguraLista,64},gfx_escala(),0,total-larguraLista,0,&menuRolagem);
#endif
  gfx_recorte(xLista,250,larguraLista,70);
  for (int m=0;m<esportes_modalidades_n();m++) if (larg[m]>0) {
    GfxRect r={xLista+pos[m]-menuRolagem,254,larg[m],56};
    if (r.x+r.w<=xLista || r.x>=xLista+larguraLista) continue;
    if (foco==-FOCO_MODALIDADE-m) plrui_pilula_foco(r,1);
    else if (m==modalidade) plrui_linha_foco(r,28,1);
    else plrui_botao_repouso(r,1);
    int cor=foco==-FOCO_MODALIDADE-m?plrui_tinta():245;
    txt(TXT_CAPTION2,i18n(esportes_modalidade(m)->nome),r.x+22,r.y+15,r.w-44,cor);
    float x=fmaxf(xLista,r.x),fim=fminf(xLista+larguraLista,r.x+r.w);
    ponteiro_alvo(x,r.y,fim-x,r.h,pointerModalidade,pointerModalidadeAgir,m,0);
  }
  gfx_sem_recorte();
}
void esportesui_desenhar(Uint32 agora) {
  ESCALA_INI(); medidas();
  txt(TXT_TITULO2,i18n("Esportes"),xLista,68,larguraLista,245);
  const char *sub=pagina==ESP_CONFIG?"ESPN":pagina==ESP_LIGAS?(modalidade>=0?i18n(esportes_modalidade(modalidade)->nome):i18n("Esportes")):pagina==ESP_EVENTOS?liga.nome:evento.nome;
  txt(TXT_CAPTION2,sub,xLista,126,larguraLista,170);
  const char *botoes[]={"Voltar",pagina==ESP_DETALHE?"Encontrar canais":"Buscar","Esportes",pagina==ESP_EVENTOS?(soFavoritos?"Todos":"Favoritos"):"Atualizar"};
  int nBotoes=nTopo();
  float bw=fminf(210,(larguraLista-(nBotoes-1)*12)/nBotoes),by=172;
  for (int i=0;i<nBotoes;i++) {
    GfxRect r={xLista+i*(bw+12),by,bw,60};
    if (foco==-i-1) plrui_pilula_foco(r,1); else plrui_botao_repouso(r,1);
    txt(TXT_CAPTION2,i18n(botoes[i]),r.x+16,r.y+18,r.w-32,foco==-i-1?plrui_tinta():245);
    ponteiro_alvo(r.x,r.y,r.w,r.h,pointerTopo,pointerTopoAgir,i,0);
  }
  if (pagina==ESP_LIGAS) menuModalidades();
  topoLista=pagina==ESP_LIGAS?340:252;alturaLista=NV_TELA_H-topoLista-70;
  if (pagina==ESP_DETALHE) {
    GfxRect r={xLista,topoLista,larguraLista,260};linhaEvento(&evento,r,0);
    topoLista+=280;alturaLista-=280;
  }
  if (alturaLista<60) alturaLista=60;
#ifdef NV_TOUCH_UI
  toquerol_vincular(&toque,(GfxRect){xLista,topoLista,larguraLista,alturaLista},gfx_escala(),0,linhas()*linhaH-alturaLista,1,&rolagem);
  ponteiro_rolagem(rolar);
#endif
  gfx_recorte(xLista-5,topoLista,larguraLista+10,alturaLista);
  float cardW=(larguraLista-(colunas-1)*vao)/colunas;
  for (int i=0;i<nItens;i++) {
    GfxRect r={xLista+(i%colunas)*(cardW+vao),topoLista+(i/colunas)*linhaH-rolagem,cardW,linhaH-16};
    if (r.y+r.h<topoLista || r.y>topoLista+alturaLista) continue;
    if (pagina==ESP_EVENTOS) {
      const EspEvento *e=esportes_evento(indices[i]);if (e) linhaEvento(e,r,foco==i);
    } else if (pagina==ESP_LIGAS) {
      const EspLiga *l=esportes_liga(indices[i]);if (l) cartaoLiga(l,r,foco==i);
    } else {
      if (foco==i) plrui_linha_foco(r,18,1);
      const char *nome="",*subt="";
      if (pagina==ESP_CONFIG) {
        int m=indices[i];nome=i18n(esportes_modalidade(m)->nome);
        subt=esportes_habilitado(m)?i18n("Ligado"):i18n("Desligado");
      } else if (pagina==ESP_DETALHE) {
        nome=i==0?i18n("Encontrar canais"):evento.participantes[i-1].nome;
        subt=i==0?i18n("Buscar este evento no seu guia"):esportes_favorito(&evento,i-1)?i18n("Remover dos favoritos"):i18n("Adicionar aos favoritos");

      }
      txt(TXT_BODY,nome,r.x+22,r.y+14,r.w-44,245);
      txt(TXT_CAPTION2,subt,r.x+22,r.y+51,r.w-44,170);
    }
    ponteiro_alvo_faixa(r.x,r.y,r.w,r.h,topoLista,topoLista+alturaLista,pointerFoco,pointerAtivar,i,0);
    if (pagina==ESP_EVENTOS) ponteiro_alvo_segurar(pointerDetalhe);
  }
  if (!nItens) {
    const char *s=procurando?"Buscando no guia…":esportes_carregando()?"Carregando…":esportes_erro()?"Não foi possível carregar. Use Atualizar para tentar novamente.":"Nenhum resultado";
    txt(TXT_BODY,i18n(s),xLista+22,topoLista+30,larguraLista-44,170);
  }
  gfx_sem_recorte();
  const char *rodape=aviso[0]?aviso:esportes_erro()?i18n("Não foi possível atualizar os eventos"):pagina==ESP_EVENTOS && esportes_eventos_n()==ESP_MAX_EVENTOS?i18n("Mostrando os primeiros 400 eventos"):"ESPN";
  txt(TXT_CAPTION2,rodape,xLista,NV_TELA_H-48,larguraLista,158);
  ESCALA_FIM();
  if (entradaTexto) teclado_desenhar(agora);
}
