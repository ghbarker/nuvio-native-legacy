/* Offline graphics/text/metadata boundary stubs. No watched/projection/source logic is replaced. */
void ponteiro_rolagem(PonteiroRolagemFn fn) { (void)fn; }
void ajustes_acento(float *r,float *g,float *b) { *r=*g=*b=.8f; }
int ajustes_borda_foco(void) { return 0; }
int ajustes_idioma(void) { return 0; }
float ajustes_raio_poster_px(void) { return 0; }
int ajustes_tinta_foco(void) { return 0; }
int ajustes_tinta_foco2(void) { return 0; }
int ajustes_vidro(void) { return 0; }
float badges_desenhar(uint64_t mask,float x,float y,float maxW,float height,float alpha) { return 0; }
uint64_t badges_provedor(const char *name) { return 0; }
const char * col_banner(const ColFolder *f) { return NULL; }
void col_cor(const ColFolder *f,float *r,float *g,float *b) { (void)f; *r=*g=*b=.8f; }
const char * diretor_foto(const char *nome) { return NULL; }
void diretor_pedir(const char *nome) {  }
void fundo_desenhar_modo(int modo, GfxRect area, float raioPx, const char *arteUrl, float a) {  }
int fundo_modo(void) { return 0; }
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) {  }
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco,
              float parx, float pary, float raio,
              float cr, float cg, float cb, float ca) {  }
float gfx_tex_aspect_atual;
void gfx_veu_base(GfxRect card, float raio, float fracao, float alfa) {  }
void gfx_vidro_cartao(GfxRect r, float raio, float foco, float a) {  }
int home_vitrine_arte(const CatItem *ci, float alfa) { return 0; }
void home_vitrine_cartao(const CatItem *ci, GfxRect base, GfxRect card, float alfa) {  }
void home_vitrine_texto(const CatItem *ci, float alfa) {  }
int imdbnota_obter(const char *identity, int catalogRating, int series) { return 0; }
float nv_cor_fundo_viva[3] = {.05f,.05f,.05f};
void ponteiro_alvo_faixa(float x, float y, float w, float h, float y0, float y1,
                         PonteiroFn focar, PonteiroFn ativar, int a, int b) {  }
float tex_aspecto(const char *caminho) { return 0; }
int tex_marca_escura(const char *caminho) { return 0; }
GLuint tex_obter_hero(const char *caminho) { return 0; }
GLuint tex_obter_larg(const char *caminho, float largLayout) { return 0; }
GLuint tex_obter_logo_larg(const char *caminho, float largLayout) { return 0; }
float txt_bloco(TxtEstilo estilo, const char *s, int r, int g, int b,
                float x, float y, float larg, float leading, float alpha, int maxLinhas) { return 0; }
