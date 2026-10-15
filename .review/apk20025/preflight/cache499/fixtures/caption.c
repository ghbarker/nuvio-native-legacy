
#include NV_STREAMS_SOURCE
#include <assert.h>
static const char *translations[][3]={{"pt","Em cache","Fora do cache"},
{"idioma_bg","В кеша","Не е в кеша"},
{"idioma_bs","U kešu","Nije u kešu"},
{"idioma_cs","V mezipaměti","Mimo mezipaměť"},
{"idioma_da","I cache","Ikke i cache"},
{"idioma_de","Im Cache","Nicht im Cache"},
{"idioma_el","Στην cache","Εκτός cache"},
{"idioma_es","En caché","Fuera de caché"},
{"idioma_fr","En cache","Hors cache"},
{"idioma_hu","Gyorsítótárban","Nincs gyorsítótárban"},
{"idioma_id","Di cache","Tidak di cache"},
{"idioma_it","In cache","Non in cache"},
{"idioma_ja","キャッシュ済み","キャッシュなし"},
{"idioma_lt","Talpykloje","Ne talpykloje"},
{"idioma_nl","In cache","Niet in cache"},
{"idioma_no","I hurtigbuffer","Ikke i hurtigbuffer"},
{"idioma_pl","W pamięci podręcznej","Poza pamięcią podręczną"},
{"idioma_ptpt","Em cache","Fora da cache"},
{"idioma_ro","În cache","Nu e în cache"},
{"idioma_ru","В кэше","Не в кэше"},
{"idioma_sk","V medzipamäti","Mimo medzipamäte"},
{"idioma_sl","V predpomnilniku","Ni v predpomnilniku"},
{"idioma_sr","U kešu","Nije u kešu"},
{"idioma_sv","I cache","Inte i cache"},
{"idioma_tab","Cached","Not cached"},
{"idioma_tr","Önbellekte","Önbellekte değil"},
{"idioma_uk","У кеші","Не в кеші"},
{"idioma_vi","Có trong cache","Không có trong cache"},
{"idioma_zhcn","已缓存","未缓存"},
{"idioma_zhtw","已快取","未快取"}};
static float uiScale=1; float gfx_escala_ui(void){return uiScale;} int ajustes_idioma(void){return 1;}
static int lang,checks,failures,icons,markSeen;static float iconAlpha,captionWidth;
static char captionText[1024],markText[1024];
#ifdef NV_TOUCH_UI
float nv_layout_w=1080,nv_layout_h=2340;
int layout_modo_mobile(void){return 1;}
#endif
const char*i18n(const char*s){if(!strcmp(s,"Em cache"))return translations[lang][1];if(!strcmp(s,"Fora do cache"))return translations[lang][2];return s;}
static int glyphs(const char*s){int n=0;for(;*s;s++)if(((unsigned char)*s&0xc0)!=0x80)n++;return n;}
TxtLinha txt_linha_corta(TxtEstilo e,const char*s,int r,int g,int b,int a,float maxW){(void)e;(void)r;(void)g;(void)b;(void)a;assert(maxW==340);snprintf(captionText,sizeof captionText,"%s",s);float w=glyphs(s)*11;captionWidth=w>maxW?maxW:w;return (TxtLinha){.w=(int)captionWidth,.h=26};}
float txt_tracking(TxtEstilo e,const char*s,int r,int g,int b,float x,float y,float a,float tracking){(void)e;(void)r;(void)g;(void)b;(void)x;(void)y;(void)a;(void)tracking;snprintf(markText,sizeof markText,"%s",s);markSeen++;return glyphs(s)*11;}
void gfx_icone(GfxRect r,const char*id,float red,float green,float blue,float alpha){assert(!strcmp(id,"aj_zap")&&r.w==22&&r.h==22&&red>.8f&&green>.7f&&blue>.4f);icons++;iconAlpha=alpha;}
static void check(int v,const char*why){checks++;if(!v){if(failures<5)fprintf(stderr,"FAIL %s lang%s caption[%s]\n",why,translations[lang][0],captionText);failures++;}}
static void providerCaption(const Stream*s,int sel){TxtLinha lp;{ char pv[160];
        legendaProvedor(s,pv,sizeof pv);
        lp=txt_linha_corta(TXT_PG_FIM,pv,sel?150:110,sel?150:110,sel?148:108,255,340); }(void)lp;}
static void negativeMark(const Stream*s){float mx=20,cy=30,anim=.7f;if(s->foraCache) mx += caixaAlta("Fora do cache",185,179,171,mx,cy,anim)+18;(void)mx;}
int main(void){
 for(lang=0;lang<(int)(sizeof translations/sizeof*translations);lang++)for(int mask=0;mask<32;mask++)for(int seed=0;seed<2;seed++)for(int selected=0;selected<2;selected++){
  Stream s={0};s.emCache=!!(mask&1);s.foraCache=!!(mask&2);s.urlP2P=!!(mask&4);s.naoVideo=!!(mask&8);
#ifdef NV_CAPTION_NEGATIVE
s.cacheNegado=!!(mask&16);
#endif
s.temSemeadores=seed;s.semeadores=73;strcpy(s.infoHash,"abc");if(!seed)strcpy(s.url,"https://media.invalid/a");
  memset(s.provedor,'X',sizeof s.provedor-1);s.provedor[sizeof s.provedor-1]=0;
  captionText[0]=markText[0]=0;markSeen=0;icons=0;providerCaption(&s,selected);int ready=s.emCache&&!s.foraCache&&!(mask&16)&&!s.urlP2P&&!s.naoVideo;int negativeCaption=(mask&16)&&!s.foraCache;
  int positivePrefix=!strncmp(captionText,translations[lang][1],strlen(translations[lang][1]))&&!strncmp(captionText+strlen(translations[lang][1])," · ",strlen(" · "));
  check(!ready||positivePrefix,"explicit Cached starts caption");
  check(!negativeCaption||!strncmp(captionText,translations[lang][2],strlen(translations[lang][2])),"explicit URL negative is Not cached");
  check(ready||!positivePrefix,"unknown/negative/P2P/nonvideo have no positive status");
  check(captionWidth<=340,"actual provider caption uses bounded column");
  check(!ready||glyphs(translations[lang][1])*11<340,"whole translated status precedes provider clipping");
  negativeMark(&s);check(markSeen==!!s.foraCache,"known negative retains explicit Not cached mark");
  int marker=desenharCache(&s,100,200,selected?1:.4f);check(!!marker==ready&&icons==ready,"lightning agrees with positive status, negative wins");if(ready)check(iconAlpha==(selected?1:.4f),"focus alpha retained");
  // The unchanged Sources geometry reserves the measured right column.
  for(int orientation=0;orientation<2;orientation++)for(int scaling=0;scaling<3;scaling++){uiScale=scaling==0?1:scaling==1?1.3f:1.5f;
#ifdef NV_TOUCH_UI
   nv_layout_w=orientation?2340:1080;nv_layout_h=orientation?1080:2340;
#endif
   float rw=FOLHA_W-FOLHA_PAD_E-FOLHA_PAD_D,tx=FOLHA_TXT,tr=rw-FOLHA_TXT,colW=captionWidth,txtW=tr-tx-colW-28;
   check(txtW>=60,"right-column caption leaves existing title area on phone and TV");
   check(tr-colW>=tx+txtW+28,"provider caption does not overlap left title bounds");
  }
  lista=&s;n=1;canalFolha=0;soMp4=soDub=0;soCache=0;check(passaChips(0),"Cached chip Off retains all operationally eligible display states");soCache=1;check(passaChips(0)==ready,"Cached chip On allows only positive icon/label state");soCache=0;lista=NULL;n=0;
 }
 printf("caption translations=%zu checks=%d failures=%d\n",sizeof translations/sizeof*translations,checks,failures);return failures?42:0;
}
