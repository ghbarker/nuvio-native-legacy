#include NV_STREAMS_SOURCE
#include <assert.h>
static int calls, nativeService, nativeResult = 1, negative, icons,budgetMode,budgetCalls;static int budgetService[8];static char budgetHash[8][80];
static uint64_t epoch = 44;
uint64_t perfis_sessao_geracao(void) { return epoch; }
int perfis_sessao_atual(uint64_t e) { return e == epoch; }
int perfis_sessao_travar(uint64_t e) { return e == epoch; }
void perfis_sessao_destravar(void) {}
int debrid_ativo(void) { return 1; }
int p2p_ativo(void) { return 0; }
int p2pmotor_e_url(const char *u) { (void)u; return 0; }
int p2p_ultimo_erro(void) { return 0; }
int p2p_resolver(const char *h, int f, const char *s, char *u, unsigned n) {
  (void)h; (void)f; (void)s; (void)u; (void)n; assert(!"P2P must not run"); return 0;
}
int debrid_servico_por_nome(const char *s) { (void)s; return nativeService; }
const char *debrid_servico_nome(int s) { (void)s; return "Native test provider"; }
int debrid_servico_disponivel(int s) { assert(s == nativeService); return 1; }
void debrid_contexto_entrar(const DebridContexto *c) { assert(c); }
void debrid_contexto_sair(void) {}
int debrid_resolver_cache(const char *h, int f, char *u, unsigned n, int *fora) {
  assert(h && *h && f == -1); calls++; *fora = negative;
  if (nativeResult == 1) snprintf(u, n, "https://native.invalid/%d.mp4", nativeService);
  return nativeResult == 1;
}
int debrid_resolver_servico_cache(int s, const char *h, int f, char *u, unsigned n, int *fora) {
  if(budgetMode){assert(budgetCalls<8);budgetService[budgetCalls]=s;snprintf(budgetHash[budgetCalls++],80,"%s",h);nativeService=s;nativeResult=s==2?1:0;}assert(s == nativeService); return debrid_resolver_cache(h, f, u, n, fora);
}
int debrid_resolver_escolhido(const char *h, int f, char *u, unsigned n, char *s, unsigned ns, int *pct) {
  assert(h && *h && f == -1); calls++; if (pct) *pct = 50;
  if (s && ns) snprintf(s, ns, "Native test provider");
  if (nativeResult == 1) snprintf(u, n, "https://native.invalid/%d.mp4", nativeService);
  return nativeResult;
}
int rede_url_final_tipo(const char *u, int s, const char *const *c, char *d, unsigned n, int *st, char *m, unsigned nm, long *b) {
  (void)u;(void)s;(void)c;(void)d;(void)n;(void)st;(void)m;(void)nm;(void)b; assert(!"No playback URL probe"); return 0;
}
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) {
  (void)u;(void)s;(void)c;(void)st; assert(!"No media download"); return NULL;
}
int vazao_url_aviso(const char *u) { (void)u; return 0; }
long fontesmortas_idade(const Stream *s) { (void)s; return -1; }
void gfx_icone(GfxRect r, const char *s, float a, float b, float c, float d) {
  (void)r;(void)a;(void)b;(void)c;(void)d; assert(!strcmp(s,"aj_zap")); icons++;
}
static void ready(const Stream *s, int expected) {
#ifdef NV_CACHE_FIX
  assert(s->emCache == expected);
#ifdef NV_CAPTION_NEGATIVE
assert(s->cacheNegado==!expected);
#endif
  int old = icons; assert(desenharCache(s, 0, 0, 1) == (expected && !s->foraCache ? 30 : 0));
  assert(icons == old + (expected && !s->foraCache));
#else
  (void)s;(void)expected;
#endif
}
static void reset(Stream *s) {
  memset(s, 0, sizeof *s); snprintf(s->infoHash, sizeof s->infoHash, "0123"); s->fileIdx = -1;
  nativeResult = 1; negative = 0;
#ifdef NV_CAPTION_NEGATIVE
s->cacheNegado=1;
#endif
}

static int checks,failures,marks;static char markText[512];
static int interfaceLanguage=IDIOMA_EN;
float gfx_escala_ui(void){return 1;}int ajustes_idioma(void){return interfaceLanguage;}
#ifdef NV_TOUCH_UI
float nv_layout_w=1080,nv_layout_h=2340;int layout_modo_mobile(void){return 1;}
#endif
const char*i18n(const char*s){if(!strcmp(s,"Em cache"))return "Cached";if(!strcmp(s,"Fora do cache"))return "Not cached";return s;}
float txt_tracking(TxtEstilo e,const char*s,int a,int b,int c,float x,float y,float alpha,float tracking){(void)e;(void)a;(void)b;(void)c;(void)x;(void)y;(void)alpha;(void)tracking;marks++;snprintf(markText,sizeof markText,"%s",s);return strlen(s)*10;}
static void check(int v,const char*why){checks++;if(!v){failures++;fprintf(stderr,"FAIL %s provider%d\n",why,nativeService);}}
static void negativeMark(const Stream*s){float mx=0,cy=0,anim=1;if(s->foraCache && !cacheConfirmado(s)) mx += caixaAlta("Fora do cache",185,179,171,mx,cy,anim)+18;;(void)mx;}
static void display(const Stream*s,int expected){char caption[160];legendaProvedor(s,caption,sizeof caption);check(cacheConfirmado(s)==expected,"latest observed readiness");int priorIcons=icons;check(desenharCache(s,0,0,1)==(expected?30:0)&&icons==priorIcons+expected,"lightning follows latest observation");marks=0;negativeMark(s);check(marks==(!expected&&s->foraCache),"obsolete Not cached mark suppressed");if(expected)check(!strncmp(caption,"Cached · ",strlen("Cached · ")),"native success Cached label");
 lista=(Stream*)s;n=1;canalFolha=0;soMp4=soDub=0;soCache=1;check(passaChips(0)==expected,"strict view follows latest native truth");soCache=0;check(passaChips(0),"filter Off preserves list eligibility");}

// Rebase regression: cache and upstream preferred-audio filters must BOTH apply.
static int combinedChecks;
static void combinedCheck(int expected,const char *why){combinedChecks++;if(passaChips(0)!=expected){failures++;fprintf(stderr,"FAIL combined cache/audio: %s\n",why);}}
static void combinedCacheAudio(void){
 Stream v;memset(&v,0,sizeof v);v.emCache=1;v.mp4=1;strcpy(v.audioIdiomas,"en");lista=&v;n=1;soCache=soDub=1;soMp4=0;canalFolha=0;
 ling_conta_audio("");ling_local_audio("en");
 combinedCheck(1,"confirmed cached English");
 strcpy(v.audioIdiomas,"es");combinedCheck(0,"cached wrong audio");
 v.audioIdiomas[0]=0;combinedCheck(0,"cached unknown audio");
 strcpy(v.legendaIdiomas,"en");combinedCheck(0,"English subtitles are not audio");
 strcpy(v.arquivo,"English dubbed dual");combinedCheck(0,"filename is not declared audio");
 strcpy(v.audioIdiomas,"eng");combinedCheck(1,"English ISO639-2 accepted");
 strcpy(v.audioIdiomas,"en-US");combinedCheck(1,"English regional tag accepted");
 strcpy(v.audioIdiomas,"es, en");combinedCheck(1,"declared multilingual includes English");
 v.emCache=0;combinedCheck(0,"unknown cache stays excluded");
 v.emCache=1;v.cacheNegado=1;combinedCheck(0,"explicit cache negative excluded");
 v.cacheNegado=0;v.foraCache=1;combinedCheck(0,"old uncached operational state excluded");
 v.cacheNativo=1;combinedCheck(1,"newer native proof overrides old negative");
 v.urlP2P=1;combinedCheck(0,"P2P not confirmed cache");v.urlP2P=0;
 v.naoVideo=1;combinedCheck(0,"nonvideo not confirmed cache");v.naoVideo=0;
 soMp4=1;v.mp4=0;combinedCheck(0,"MP4 composes with cache and audio");
 soMp4=0;combinedCheck(1,"MP4 off retains other filters");
 v.emCache=0;soCache=0;strcpy(v.audioIdiomas,"en");combinedCheck(1,"cache off retains English");
 strcpy(v.audioIdiomas,"es");combinedCheck(0,"cache off still rejects wrong audio");
 soDub=0;combinedCheck(1,"both chips off restore eligibility");
 soCache=soDub=1;v.emCache=1;ling_local_audio("*");combinedCheck(1,"All audio retains Spanish cached source");
 v.emCache=0;combinedCheck(0,"All audio cannot bypass strict cache");
 v.emCache=1;v.cacheNegado=1;combinedCheck(0,"All audio cannot bypass cache negative");
 v.cacheNegado=0;ling_local_audio("");interfaceLanguage=IDIOMA_EN;strcpy(v.audioIdiomas,"en");combinedCheck(1,"device English fallback matches English");
 strcpy(v.audioIdiomas,"es");combinedCheck(0,"device English rejects Spanish");
 interfaceLanguage=IDIOMA_ES;combinedCheck(1,"device Spanish fallback matches Spanish");
 strcpy(v.audioIdiomas,"en");combinedCheck(0,"device Spanish rejects English");
 ling_local_audio("en");combinedCheck(1,"explicit English overrides device Spanish");
 strcpy(v.audioIdiomas,"es");combinedCheck(0,"explicit English rejects device Spanish source");
 ling_definir_original("en");ling_local_audio("~");strcpy(v.audioIdiomas,"en");combinedCheck(1,"Original resolves declared title language");
 ling_definir_original("");strcpy(v.audioIdiomas,"es");combinedCheck(1,"unresolved Original does not invent language");
 ling_local_audio("");ling_conta_audio("");interfaceLanguage=IDIOMA_EN;soCache=soDub=soMp4=0;lista=NULL;n=0;
 assert(combinedChecks==30&&calls==13);
 printf("combined cache/audio checks=%d failures=%d existingResolverCalls=%d\n",combinedChecks,failures,calls);
}

int main(void){
 Stream s;lista=&s;n=1;listaGeracao=7;
 for(nativeService=0;nativeService<DEBRID_N;nativeService++){
  reset(&s);s.foraCache=1;snprintf(s.provedor,sizeof s.provedor,"Prior uncached source");display(&s,0);
  int priorCalls=calls;char url[4096],provider[80];int pct;
  check(stream_resolver_escolhida(0,7,url,sizeof url,provider,sizeof provider,&pct)==1,"actual manual native resolution succeeds");check(calls==priorCalls+1&&url[0],"only existing resolver call");check(s.foraCache==1,"operational old-negative contract retained");display(&s,1);
#ifdef NV_NATIVE_REFRESH
  check(s.cacheNativo==1,"native provenance published");
#endif
  // The sole later cache-negative writer must invalidate newer display proof.
  s.url[0]=0;s.foraCache=0;nativeResult=0;negative=1;Conferencia c={.geracao=7,.sessao=44};
  check(!verificarUma(0,&c)&&s.foraCache==1,"actual newer native cache miss retained");display(&s,0);
#ifdef NV_NATIVE_REFRESH
  check(s.cacheNativo==0,"later negative invalidates native provenance");
#endif
  reset(&s);s.foraCache=1;nativeResult=DEBRID_BAIXANDO;priorCalls=calls;
  check(stream_resolver_escolhida(0,7,url,sizeof url,provider,sizeof provider,&pct)==DEBRID_BAIXANDO&&calls==priorCalls+1,"downloading remains nonready");display(&s,0);
 }
 Stream*parsed=NULL;const char*conflict="{\"streams\":[{\"infoHash\":\"0123\",\"name\":\"uncached lightning ⚡\",\"clientResolve\":{\"isCached\":true}}]}";
 check(stream_extrair(conflict,"AIOStreams",&parsed)==1,"parse contradictory metadata");if(parsed){s=parsed[0];free(parsed);display(&s,0);check(s.foraCache&&!s.emCache,"negative metadata still wins over positive marker");
#ifdef NV_NATIVE_REFRESH
 check(!s.cacheNativo,"parser cannot invent native confirmation");
#endif
 nativeService=1;nativeResult=1;negative=0;char url[4096],provider[80];int pct;check(stream_resolver_escolhida(0,7,url,sizeof url,provider,sizeof provider,&pct)==1,"genuine later native confirmation refreshes parsed negative");check(s.foraCache==1,"parsed operational negative unchanged");display(&s,1);}
 parsed=NULL;const char*direct="{\"streams\":[{\"url\":\"https://media.invalid/a\",\"name\":\"⚡\",\"clientResolve\":{\"isCached\":false}}]}";
 check(stream_extrair(direct,"AIOStreams",&parsed)==1,"parse direct explicit false");if(parsed){s=parsed[0];free(parsed);check(!s.foraCache&&s.cacheNegado&&!s.emCache,"direct false stays display-only");display(&s,0);}
 lista=NULL;n=0;assert(checks==135&&calls==13);printf("refresh checks=%d failures=%d existingResolverCalls=%d\n",checks,failures,calls);combinedCacheAudio();return failures?42:0;
}
