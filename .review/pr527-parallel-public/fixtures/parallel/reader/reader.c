#include <assert.h>
#include <stdatomic.h>
#include <errno.h>
#include <unistd.h>
#include DISCOVERY_SOURCE
static const char *mode;
static int hideWatched, hideFuture, disabled, watchedAll;
static pthread_mutex_t netLock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t netCond=PTHREAD_COND_INITIALIZER;
static int netN,netEntered,netRelease,netBlocked,enrichCalls;
static char seenUrl[16][NV_ADDON_PEDIDO_MAX];
static char *addonBody(int start,int count,int same,int future){
 size_t cap=(size_t)count*240+64,used=0;char *b=malloc(cap);assert(b);
 used+=(size_t)snprintf(b+used,cap-used,"{\"metas\":[");
 for(int i=0;i<count;i++)used+=(size_t)snprintf(b+used,cap-used,"%s{\"id\":\"tt%d\",\"type\":\"movie\",\"name\":\"Title %d\",\"poster\":\"https://img.test/p%d.jpg\",\"releaseInfo\":\"%d\"}",i?",":"",same?start:start+i,start+i,start+i,future?2099:2020);
 snprintf(b+used,cap-used,"]}");return b;
}
static int skipOf(const char *u){const char*p=strstr(u,"skip=");return p?atoi(p+5):0;}
static int pageOf(const char *u){const char*p=strstr(u,"page=");return p?atoi(p+5):1;}
static char *respond(const char *u){
 if(!strncmp(mode,"manual-addon",12)){int sk=skipOf(u);if(!strcmp(mode,"manual-addon-error")&&sk)return NULL;return addonBody(sk+1,31,0,0);}
 if(!strcmp(mode,"manual-watched"))return addonBody(skipOf(u)+1,20,0,0);
 if(!strcmp(mode,"manual-tmdb")){int page=pageOf(u);char b[2048];snprintf(b,sizeof b,"{\"page\":%d,\"total_pages\":3,\"results\":[{\"id\":%d,\"title\":\"Movie A\",\"poster_path\":\"/a.jpg\",\"release_date\":\"2020-01-01\"},{\"id\":%d,\"title\":\"Movie B\",\"poster_path\":\"/b.jpg\",\"release_date\":\"2020-01-01\"}]}",page,page*100+1,page*100+2);return strdup(b);}
 if(!strcmp(mode,"manual-trakt")){int page=pageOf(u),count=page==1?40:3;size_t used=0,cap=12000;char*b=malloc(cap);assert(b);used+=(size_t)snprintf(b+used,cap-used,"[");for(int i=1;i<=count;i++)used+=(size_t)snprintf(b+used,cap-used,"%s{\"movie\":{\"title\":\"Movie%d\",\"year\":2020,\"ids\":{\"imdb\":\"tt%d\"}}}",i>1?",":"",(page-1)*40+i,(page-1)*40+i);snprintf(b+used,cap-used,"]");return b;}

 if(!strcmp(mode,"failure"))return NULL;
 if(!strcmp(mode,"empty-spaces"))return strdup("{\"metas\":[                  \n\t\r                   ]}");
 if(!strcmp(mode,"trakt-empty-spaces"))return strdup("[                  \n\t\r                   ]");
 if(!strcmp(mode,"null-array"))return strdup("{\"metas\":null}");
 if(!strcmp(mode,"object-array"))return strdup("{\"metas\":{}}");
 if(!strcmp(mode,"string-array"))return strdup("{\"metas\":\"[]\"}");
 if(!strcmp(mode,"number-array"))return strdup("{\"metas\":42}");
 if(!strcmp(mode,"truncated-array"))return strdup("{\"metas\":[{\"id\":\"tt1\",\"name\":\"One\",\"poster\":\"p.jpg\"}");
 if(!strcmp(mode,"trakt-truncated-array"))return strdup("[{\"movie\":{\"title\":\"One\",\"ids\":{\"imdb\":\"tt1\"}}}");
 if(!strcmp(mode,"trakt-compact-both")){size_t used=0,cap=12000;char*b=malloc(cap);assert(b);used+=(size_t)snprintf(b+used,cap-used,"[");for(int i=1;i<=40;i++)used+=(size_t)snprintf(b+used,cap-used,"%s{\"movie\":{\"title\":\"Movie%d\",\"year\":2020,\"ids\":{\"imdb\":\"tt%d\"}}}",i>1?",":"",i,i);snprintf(b+used,cap-used,"]");return b;}

 if(!strcmp(mode,"invalid"))return strdup("{\"error\":\"no catalog\"}");
 if(!strcmp(mode,"empty"))return strdup("{\"metas\":[]}");
 if(strstr(u,"api.themoviedb.org")){
  const char *key=strstr(u,"/collection/")?"parts":strstr(u,"/list/")?"items":strstr(u,"/person/")?(!strcmp(mode,"tmdb-director")?"crew":"cast"):"results";
  char b[2048];snprintf(b,sizeof b,"{\"page\":1,\"total_pages\":1,\"%s\":[{\"id\":101,\"title\":\"A Movie\",\"poster_path\":\"/a.jpg\",\"release_date\":\"2020-01-01\",\"media_type\":\"movie\",\"job\":\"Director\"},{\"id\":102,\"name\":\"A Series\",\"poster_path\":\"/b.jpg\",\"first_air_date\":\"2021-01-01\",\"media_type\":\"tv\",\"job\":\"Actor\"},{\"id\":101,\"title\":\"Duplicate\",\"poster_path\":\"/a.jpg\",\"job\":\"Director\"},{\"id\":103,\"title\":\"Future\",\"poster_path\":\"/c.jpg\",\"release_date\":\"2099-01-01\",\"job\":\"Director\"}]}",key);return strdup(b);
 }
 if(strstr(u,"api.trakt.tv"))return strdup("[{\"type\":\"movie\",\"movie\":{\"title\":\"Movie\",\"year\":2020,\"ids\":{\"imdb\":\"tt201\"}}},{\"type\":\"show\",\"show\":{\"title\":\"Series\",\"year\":2021,\"ids\":{\"imdb\":\"tt202\"}}},{\"type\":\"movie\",\"movie\":{\"title\":\"Duplicate\",\"year\":2020,\"ids\":{\"imdb\":\"tt201\"}}},{\"movie\":{\"title\":\"No ID\",\"ids\":{}}},{\"movie\":{\"title\":\"Future\",\"year\":2099,\"ids\":{\"imdb\":\"tt203\"}}}]");
 if(!strcmp(mode,"addon")||!strcmp(mode,"genre"))return strdup("{\"metas\":[{\"id\":\"tt1\",\"name\":\"One\",\"poster\":\"p.jpg\",\"releaseInfo\":\"2020\"},{\"id\":\"tt1\",\"name\":\"Duplicate\",\"poster\":\"p.jpg\"},{\"id\":\"tt2\",\"name\":\"Future\",\"poster\":\"p.jpg\",\"releaseInfo\":\"2099\"},{\"id\":\"tt3\",\"poster\":\"p.jpg\"},{\"id\":\"tt4\",\"name\":\"Four\",\"poster\":\"p.jpg\"},{\"id\":\"tt5\",\"name\":\"No Poster\"}]}");
 int sk=skipOf(u);
 if(!strcmp(mode,"received-supplement")||!strcmp(mode,"late-supplement")||!strcmp(mode,"reserve-hidden-off"))return addonBody(1,100,0,0);
 if(!strcmp(mode,"watched-pages")||!strcmp(mode,"watched-bounded"))return addonBody(sk+1,20,0,0);
 if(!strcmp(mode,"raw-skip"))return addonBody(sk+1,31,0,0);
 if(!strcmp(mode,"duplicate-pages"))return addonBody(1,20,0,0);
 if(!strcmp(mode,"top100")||!strcmp(mode,"top250"))return addonBody(sk+1,100,0,0);
 if(!strcmp(mode,"overlap"))return addonBody(strstr(u,"/second.")?201:1,3,0,0);
 return addonBody(1,3,0,0);
}
char *rede_baixar(const char *u,int timeout){
 assert(timeout>0&&timeout<=12);pthread_mutex_lock(&netLock);assert(netN<16);snprintf(seenUrl[netN++],sizeof seenUrl[0],"%s",u);netEntered++;pthread_cond_broadcast(&netCond);while(netBlocked&&!netRelease)pthread_cond_wait(&netCond,&netLock);pthread_mutex_unlock(&netLock);return respond(u);
}
char *rede_baixar_com(const char *u,int timeout,const char *const *headers){assert(headers&&headers[0]&&headers[1]);assert(!strcmp(headers[0],"trakt-api-version: 2"));assert(!strcmp(headers[1],"trakt-api-key: fixture-client"));return rede_baixar(u,timeout);}
const char *nuvem_trakt_cliente(void){return "fixture-client";}
int ajustes_ocultar_filmes_vistos(void){return hideWatched;}
int ajustes_ocultar_nao_lancados(void){return hideFuture;}
int ajustes_tmdb_ligado(void){return 1;}
const char *ajustes_tmdb_idioma(void){return "en-US";}
int ajustes_idioma_ingles(void){return 1;}
const char *i18n(const char *s){return s;}
int addons_n(void){return 1;}
const char *addons_base(int i){(void)i;return "https://addon.test/token";}
const char *addons_id_manifesto(int i){(void)i;return "fixture.addon";}
const char *addons_nome(int i){(void)i;return "Fixture addon";}
int addons_base_desligada(const char *base){(void)base;return disabled;}
unsigned cat_revisao_itens(void){return 0;}
int cat_visto(const CatItem *it){int n=!strncmp(it->imdb,"tt",2)?atoi(it->imdb+2):0;return (!strcmp(mode,"manual-watched")&&n>0&&n<=60)||watchedAll||((hideWatched||!strcmp(mode,"reserve-hidden-off"))&&n>0&&n<=(!strcmp(mode,"late-supplement")?80:!strcmp(mode,"raw-skip")?31:!strcmp(mode,"watched-pages")?40:20));}
int cat_copiar_por_id(const char *id,const char *type,CatItem *out){(void)id;(void)type;(void)out;return 0;}
int trakt_enfeitar_lote(CatItem *v,int n){enrichCalls++;for(int i=0;i<n;i++)snprintf(v[i].poster,sizeof v[i].poster,"https://img.test/%s.jpg",v[i].imdb);if(!strcmp(mode,"trakt-compact-first")){assert(n==2);v[0]=v[1];return 1;}if(!strcmp(mode,"trakt-compact-last")){assert(n==2);return 1;}if(!strcmp(mode,"trakt-compact-both")){assert(n==20);if(enrichCalls==1)memmove(v,v+1,(size_t)(n-1)*sizeof *v);return n-1;}return n;}
int trakt_enfeitar_lote_sessao(CatItem*v,int n,uint64_t session){assert(nv_perfil_sessao_atual(session));int r=trakt_enfeitar_lote(v,n);if(!strcmp(mode,"trakt-stale"))perfis_invalidar_sessao();return r;}
static ColSource addon(const char *id){ColSource s={0};snprintf(s.base,sizeof s.base,"https://addon.test/token");snprintf(s.type,sizeof s.type,"movie");snprintf(s.catId,sizeof s.catId,"%s",id);return s;}
static void singletonSeed(void){vtN=1;vtPagina=777;vtGeracao=888;vtFioVivo=0;vtFim=1;vtErro=3;snprintf(vtItens[0].imdb,sizeof vtItens[0].imdb,"tt-singleton-sentinel");snprintf(vtBase,sizeof vtBase,"https://singleton.test");}
static void singletonCheck(void){assert(vtN==1&&vtPagina==777&&vtGeracao==888&&!vtFioVivo&&vtFim==1&&vtErro==3);assert(!strcmp(vtItens[0].imdb,"tt-singleton-sentinel"));assert(!strcmp(vtBase,"https://singleton.test"));}
typedef struct {ColSource source;NvPerfilSessao session;CatItem items[40];DescColecaoResultado result;int ok;} Job;
static void *readJob(void *p){Job*j=p;j->ok=desc_colecao_inicial(&j->source,j->source.base,j->session,j->items,20,&j->result);return NULL;}
static void waitEntered(int n){struct timespec until;clock_gettime(CLOCK_REALTIME,&until);until.tv_sec+=5;pthread_mutex_lock(&netLock);while(netEntered<n)assert(pthread_cond_timedwait(&netCond,&netLock,&until)!=ETIMEDOUT);pthread_mutex_unlock(&netLock);}
static void unblock(void){pthread_mutex_lock(&netLock);netRelease=1;pthread_cond_broadcast(&netCond);pthread_mutex_unlock(&netLock);}
static void ids(CatItem*v,int offset,int n,int first){for(int i=0;i<n;i++){char id[32];snprintf(id,sizeof id,"tt%d",first+i);assert(!strcmp(v[offset+i].imdb,id));}}
static void metadataCheck(const Job*j){
 if(!strcmp(mode,"received-supplement")||!strcmp(mode,"late-supplement")||!strcmp(mode,"reserve-hidden-off"))assert(j->result.pagina==100&&j->result.mais);
 if(!strcmp(mode,"watched-pages")||!strcmp(mode,"watched-bounded"))assert(j->result.pagina==60&&j->result.mais);
 if(!strcmp(mode,"raw-skip"))assert(j->result.pagina==62&&j->result.mais);
 if(!strcmp(mode,"duplicate-pages"))assert(j->result.pagina==40&&!j->result.mais);
 if(!strcmp(mode,"top100"))assert(j->result.pagina==100&&!j->result.mais);
 if(!strcmp(mode,"top250"))assert(j->result.pagina==300&&!j->result.mais);
 if(!strcmp(mode,"trakt-compact-both"))assert(j->result.pagina==2&&j->result.mais);
 if(!strncmp(mode,"tmdb-",5)||(!strncmp(mode,"trakt",5)&&strcmp(mode,"trakt-stale")&&strcmp(mode,"trakt-compact-both")))assert(j->result.pagina==1&&!j->result.mais);
 if(!strcmp(mode,"failure")||!strcmp(mode,"invalid")||!strcmp(mode,"disabled")||strstr(mode,"array")||!strcmp(mode,"empty")||!strcmp(mode,"empty-spaces"))assert(!j->result.mais);
}
static void manualCheck(Job*j){
 int watched=!strcmp(mode,"manual-watched"),tmdb=!strcmp(mode,"manual-tmdb"),trakt=!strcmp(mode,"manual-trakt"),error=!strcmp(mode,"manual-addon-error");
 int cursor=watched?60:tmdb||trakt?2:31,initialRequests=watched?3:1;
 assert(j->ok&&!j->result.erro&&j->result.pagina==cursor&&j->result.mais&&netN==initialRequests);
 if(tmdb)assert(j->result.n==2&&!strcmp(j->items[0].imdb,"tmdb:101"));
 else if(trakt)assert(j->result.n==20&&!strcmp(j->items[0].imdb,"tt1"));
 else assert(j->result.n==20&&!strcmp(j->items[0].imdb,"tt1"));
 if(j->source.prov[0])desc_vertudo_fonte_desde(&j->source,j->result.pagina);
 else desc_vertudo_filtro_desde(j->source.base,j->source.type,j->source.catId,j->source.genre,j->result.pagina);
 for(int i=0;i<500&&desc_vertudo_carregando();i++){struct timespec ts={0,10000000};nanosleep(&ts,NULL);}
 assert(!desc_vertudo_carregando()&&netN==initialRequests+1);
 if(error){assert(desc_vertudo_erro()&&desc_vertudo_pagina()==31&&!desc_vertudo_n_bruto());}
 else{
  CatItem first;assert(!desc_vertudo_erro()&&desc_vertudo_item(0,&first));
  if(tmdb)assert(desc_vertudo_n()==2&&desc_vertudo_pagina()==3&&!desc_vertudo_fim()&&!strcmp(first.imdb,"tmdb:201")&&pageOf(seenUrl[initialRequests])==2);
  else if(trakt)assert(desc_vertudo_n()==3&&desc_vertudo_pagina()==2&&desc_vertudo_fim()&&!strcmp(first.imdb,"tt41")&&pageOf(seenUrl[initialRequests])==2);
  else if(watched)assert(desc_vertudo_n()==20&&desc_vertudo_pagina()==80&&!desc_vertudo_fim()&&!strcmp(first.imdb,"tt61")&&skipOf(seenUrl[initialRequests])==60);
  else assert(desc_vertudo_n()==31&&desc_vertudo_pagina()==62&&!desc_vertudo_fim()&&!strcmp(first.imdb,"tt32")&&skipOf(seenUrl[initialRequests])==31);
  if(!strcmp(mode,"manual-addon-genre"))assert(strstr(seenUrl[initialRequests],"genre=Action%20%26%20Adventure&skip=31.json"));
 }
 printf("PASS %s actual-reader-to-legacy-continuation initial_cursor=%d final_cursor=%d requests=%d\n",mode,j->result.pagina,desc_vertudo_pagina(),netN);
}
int main(int argc,char**argv){assert(argc==2);mode=argv[1];hideFuture=1;singletonSeed();desc_tmdb_definir("0123456789abcdef0123456789abcdef");Job *j=calloc(1,sizeof* j);assert(j);j->source=addon("catalog");j->session=nv_perfil_sessao_ler();
 if(!strcmp(mode,"manual-addon-genre"))snprintf(j->source.genre,sizeof j->source.genre,"Action & Adventure");
 if(!strcmp(mode,"manual-tmdb")||!strcmp(mode,"manual-trakt")){snprintf(j->source.prov,sizeof j->source.prov,"%s",!strcmp(mode,"manual-tmdb")?"tmdb":"trakt");snprintf(j->source.midia,sizeof j->source.midia,"MOVIE");j->source.traktLista=42;}
 if(!strcmp(mode,"manual-watched"))hideWatched=1;
 if(!strcmp(mode,"genre"))snprintf(j->source.genre,sizeof j->source.genre,"Action & Adventure/日本");
 if(!strncmp(mode,"tmdb-",5)){snprintf(j->source.prov,sizeof j->source.prov,"tmdb");snprintf(j->source.midia,sizeof j->source.midia,"MOVIE");const char*k=mode+5;if(strcmp(k,"discover")){snprintf(j->source.tmdbTipo,sizeof j->source.tmdbTipo,"%s",k);j->source.tmdbId=42;}snprintf(j->source.ordenar,sizeof j->source.ordenar,"vote_average.desc");snprintf(j->source.filtros,sizeof j->source.filtros,"{\"withGenres\":\"18\",\"withOriginalLanguage\":\"ja\",\"year\":2020}");}
 if(!strncmp(mode,"trakt",5)){snprintf(j->source.prov,sizeof j->source.prov,"trakt");snprintf(j->source.midia,sizeof j->source.midia,"MOVIE");j->source.traktLista=42;snprintf(j->source.ordenar,sizeof j->source.ordenar,"added");snprintf(j->source.ordem,sizeof j->source.ordem,"desc");}
 if(!strcmp(mode,"received-supplement")||!strcmp(mode,"late-supplement")||!strcmp(mode,"trakt-compact-both")||!strncmp(mode,"watched-",8)||!strcmp(mode,"raw-skip")||!strcmp(mode,"duplicate-pages")||!strncmp(mode,"top",3))hideWatched=1;
 watchedAll=!strcmp(mode,"watched-bounded")||!strcmp(mode,"duplicate-pages")||!strncmp(mode,"top",3);
 if(!strncmp(mode,"top",3))snprintf(j->source.catId,sizeof j->source.catId,"fixture.%s",mode);
 if(!strcmp(mode,"disabled"))disabled=1;
 if(!strcmp(mode,"stale-before"))perfis_invalidar_sessao();
 if(!strcmp(mode,"stale-during")||!strcmp(mode,"overlap")||!strcmp(mode,"overlap-mixed"))netBlocked=1;
 pthread_t a;assert(!pthread_create(&a,NULL,readJob,j));
 if(!strcmp(mode,"stale-during")){waitEntered(1);perfis_invalidar_sessao();perfis_invalidar_sessao();unblock();}
 if(!strcmp(mode,"overlap")){Job*k=calloc(1,sizeof*k);assert(k);k->source=addon("second");k->session=j->session;pthread_t b;assert(!pthread_create(&b,NULL,readJob,k));waitEntered(2);assert(netN==2);singletonCheck();unblock();assert(!pthread_join(b,NULL));assert(k->ok&&!k->result.erro&&k->result.n==3);ids(k->items,0,3,201);free(k);}
 if(!strcmp(mode,"overlap-mixed")){Job*k=calloc(1,sizeof*k),*l=calloc(1,sizeof*l);assert(k&&l);k->source=addon("tmdb-unused");l->source=addon("trakt-unused");snprintf(k->source.prov,sizeof k->source.prov,"tmdb");snprintf(k->source.midia,sizeof k->source.midia,"MOVIE");snprintf(l->source.prov,sizeof l->source.prov,"trakt");snprintf(l->source.midia,sizeof l->source.midia,"MOVIE");l->source.traktLista=42;k->session=l->session=j->session;pthread_t b,c;assert(!pthread_create(&b,NULL,readJob,k));assert(!pthread_create(&c,NULL,readJob,l));waitEntered(3);assert(netN==3);singletonCheck();unblock();assert(!pthread_join(b,NULL));assert(!pthread_join(c,NULL));assert(k->ok&&l->ok&&!k->result.erro&&!l->result.erro&&k->result.n==2&&l->result.n==2);assert(!strcmp(k->items[0].imdb,"tmdb:101")&&!strcmp(k->items[1].imdb,"tmdb:102"));ids(l->items,0,2,201);assert(enrichCalls==1);free(k);free(l);}
 assert(!pthread_join(a,NULL));singletonCheck();
 if(!strncmp(mode,"manual-",7)){manualCheck(j);free(j);return 0;}
 metadataCheck(j);
 if(!strcmp(mode,"addon")||!strcmp(mode,"genre")){assert(j->ok&&!j->result.erro&&j->result.n==2&&!j->result.extraN&&netN==1);assert(!strcmp(j->items[0].imdb,"tt1")&&!strcmp(j->items[1].imdb,"tt4"));assert(!strcmp(j->items[0].origem,"fixture.addon"));if(!strcmp(mode,"genre"))assert(!strcmp(seenUrl[0],"https://addon.test/token/catalog/movie/catalog/genre=Action%20%26%20Adventure%2F%E6%97%A5%E6%9C%AC&skip=0.json"));}
 else if(!strcmp(mode,"late-supplement")){assert(j->ok&&!j->result.erro&&j->result.n==20&&j->result.extraN==20&&netN==1);ids(j->items,0,20,1);ids(j->items,20,20,81);}
 else if(!strcmp(mode,"reserve-hidden-off")){assert(j->ok&&!j->result.erro&&j->result.n==20&&j->result.extraN==20&&netN==1);ids(j->items,0,20,1);ids(j->items,20,20,21);}
 else if(!strcmp(mode,"received-supplement")){assert(j->ok&&!j->result.erro&&j->result.n==20&&j->result.extraN==20&&netN==1);ids(j->items,0,20,1);ids(j->items,20,20,21);}
 else if(!strcmp(mode,"watched-pages")){assert(j->ok&&!j->result.erro&&j->result.n==20&&j->result.extraN==20&&netN==3);ids(j->items,0,20,1);ids(j->items,20,20,41);assert(skipOf(seenUrl[1])==20&&skipOf(seenUrl[2])==40);}
 else if(!strcmp(mode,"watched-bounded")){assert(j->ok&&!j->result.erro&&j->result.n==20&&!j->result.extraN&&netN==3);ids(j->items,0,20,1);}
 else if(!strcmp(mode,"raw-skip")){assert(j->ok&&!j->result.erro&&j->result.n==20&&j->result.extraN==20&&netN==2);assert(skipOf(seenUrl[1])==31);ids(j->items,20,20,32);}
 else if(!strcmp(mode,"duplicate-pages")){assert(j->ok&&!j->result.erro&&j->result.n==20&&!j->result.extraN&&netN==2);}
 else if(!strcmp(mode,"top100")){assert(j->ok&&!j->result.erro&&j->result.n==20&&!j->result.extraN&&netN==1);}
 else if(!strcmp(mode,"top250")){assert(j->ok&&!j->result.erro&&j->result.n==20&&!j->result.extraN&&netN==3);assert(skipOf(seenUrl[1])==100&&skipOf(seenUrl[2])==200);}
 else if(!strncmp(mode,"tmdb-",5)){assert(j->ok&&!j->result.erro&&!j->result.extraN&&netN==1);int director=!strcmp(mode,"tmdb-director");assert(j->result.n==(director?1:2));assert(!strcmp(j->items[0].imdb,"tmdb:101"));assert(!strcmp(j->items[0].poster,"https://image.tmdb.org/t/p/w342/a.jpg"));if(!director)assert(!strcmp(j->items[1].imdb,"tmdb:102")&&!strcmp(j->items[1].tipo,"series"));assert(strstr(seenUrl[0],"api_key=0123456789abcdef0123456789abcdef&language=en-US"));if(!strcmp(mode,"tmdb-discover"))assert(strstr(seenUrl[0],"/discover/movie?")&&strstr(seenUrl[0],"&with_genres=18")&&strstr(seenUrl[0],"&with_original_language=ja")&&strstr(seenUrl[0],"&year=2020"));if(!strcmp(mode,"tmdb-list"))assert(strstr(seenUrl[0],"/list/42?"));if(!strcmp(mode,"tmdb-collection"))assert(strstr(seenUrl[0],"/collection/42?"));if(!strcmp(mode,"tmdb-person")||director)assert(strstr(seenUrl[0],"/person/42/combined_credits?"));if(!strcmp(mode,"tmdb-company"))assert(strstr(seenUrl[0],"&with_companies=42"));if(!strcmp(mode,"tmdb-network"))assert(strstr(seenUrl[0],"/discover/tv?")&&strstr(seenUrl[0],"&with_networks=42")&&strstr(seenUrl[0],"&first_air_date.lte="));}
 else if(!strcmp(mode,"trakt")){assert(j->ok&&!j->result.erro&&j->result.n==2&&!j->result.extraN&&netN==1&&enrichCalls==1);assert(!strcmp(j->items[0].imdb,"tt201")&&!strcmp(j->items[1].imdb,"tt202")&&!strcmp(j->items[1].tipo,"series"));assert(j->items[0].poster[0]&&j->items[1].poster[0]);assert(!strcmp(seenUrl[0],"https://api.trakt.tv/lists/42/items/movie?page=1&limit=40&sort_by=added&sort_how=desc"));}
 else if(!strcmp(mode,"trakt-compact-first")||!strcmp(mode,"trakt-compact-last")){assert(j->ok&&!j->result.erro&&j->result.n==1&&!j->result.extraN&&netN==1&&enrichCalls==1);assert(!strcmp(j->items[0].imdb,!strcmp(mode,"trakt-compact-first")?"tt202":"tt201"));}
 else if(!strcmp(mode,"trakt-compact-both")){assert(j->ok&&!j->result.erro&&j->result.n==19&&j->result.extraN==19&&netN==1&&enrichCalls==2);ids(j->items,0,19,2);ids(j->items,20,19,21);}
 else if(!strcmp(mode,"empty-spaces")||!strcmp(mode,"trakt-empty-spaces")){assert(j->ok&&!j->result.erro&&!j->result.n&&!j->result.extraN&&netN==1&&!enrichCalls);}
 else if(!strcmp(mode,"empty")){assert(j->ok&&!j->result.erro&&!j->result.n&&!j->result.extraN&&netN==1);}
 else if(!strcmp(mode,"failure")||!strcmp(mode,"invalid")||!strcmp(mode,"disabled")||!strcmp(mode,"null-array")||!strcmp(mode,"object-array")||!strcmp(mode,"string-array")||!strcmp(mode,"number-array")||!strcmp(mode,"truncated-array")||!strcmp(mode,"trakt-truncated-array")){assert(j->ok&&j->result.erro&&!j->result.n&&!j->result.extraN);assert(netN==(!strcmp(mode,"disabled")?0:1));}
 else if(!strcmp(mode,"trakt-stale")){assert(!j->ok&&!j->result.n&&!j->result.extraN&&netN==1&&enrichCalls==1);}
 else if(!strcmp(mode,"stale-before")||!strcmp(mode,"stale-during")){assert(!j->ok&&!j->result.n&&!j->result.extraN);assert(netN==(!strcmp(mode,"stale-before")?0:1));}
 else if(!strcmp(mode,"overlap-mixed")){assert(j->ok&&!j->result.erro&&j->result.n==3&&!j->result.extraN&&netN==3);ids(j->items,0,3,1);}
 else if(!strcmp(mode,"overlap")){assert(j->ok&&!j->result.erro&&j->result.n==3&&!j->result.extraN&&netN==2);ids(j->items,0,3,1);}
 else assert(!"unknown case");
 printf("PASS %s requests=%d prefix=%d supplement=%d error=%d singleton=unchanged\n",mode,netN,j->result.n,j->result.extraN,j->result.erro);free(j);return 0;
}
