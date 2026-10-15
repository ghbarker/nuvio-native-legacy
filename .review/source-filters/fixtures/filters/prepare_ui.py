#!/usr/bin/env python3
"""Exact Settings handler bodies, with editor/keyboard UI boundaries stubbed."""
import hashlib
import json
from pathlib import Path
import re
import sys

source = Path(sys.argv[1]); output = Path(sys.argv[2])
text = (source / 'src/ajustes.c').read_text()
enum = text[text.index('typedef enum {'):text.index('} OpcaoId;') + len('} OpcaoId;')]
block = text[text.index('static int ffCategoria ='):text.index('static int frFoco;', text.index('static int ffCategoria ='))]
ilha = (source / 'src/ajustes_ux_ilha.inc').read_text()
pointer = ilha[ilha.index('static void ffPont('):ilha.index('static void desenhaFolhaFiltros(')]
def function(name):
    start = re.search(r'^static [^\n]*\b' + name + r'\([^\n]*\)\s*\{', text, re.M).start()
    body = text.index('{', start); depth = 1; p = body + 1
    # These narrow backend handlers contain no brace characters in literals.
    while depth:
        if text[p] == '{': depth += 1
        if text[p] == '}': depth -= 1
        p += 1
    return text[start:p] + '\n'
backend = ''.join(function(n) for n in ['ajFalhaBackend','ajFalhaValor','ajFalhaEspelhar','ajFalhaDefinir'])
prefix = r'''
#include "fontefalha.h"
#include "teclado.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int frAberta, faRetornarFolha, stCampo, keyboard, emptyAllowed, editor;
static char faAddonEditar[40];
static const char *notice;
const char *i18n(const char *s) { return s; }
static void uxCancelar(void) { editor = -1; }
static void uxNotificar(const char *s) { notice = s; }
static void uxAbrirEditor(int op) { editor = op; }
int teclado_aberto(void) { return keyboard; }
void teclado_contexto(const char *s) { assert(s && *s); }
void teclado_abrir_com(const char *t,const char *d,int max,const char *alphabet,const char *initial) {
  assert(t && d && max == TECLADO_LONGO && alphabet && initial); keyboard = 1; emptyAllowed = 0;
}
void teclado_permitir_vazio(int allow) { emptyAllowed = allow; }
'''
main = r'''
int main(int argc, char **argv) {
  assert(argc == 2); fontefalha_dir(argv[1]);
  assert(fontefalha_provedor_mover(1,-1)); assert(fontefalha_provedor_definir_limite(1,7));
  const int rows[] = {0,1,3,4,5,6,7};
  for (unsigned i = 0; i < sizeof rows / sizeof *rows; i++) {
    ffAbrir(); assert(frAberta == 6 && ffCategoria == -1);
    for (int j = 0; j < rows[i]; j++) ffEvento(SDLK_DOWN);
    ffEvento(SDLK_RETURN); int cat = FF_CATEGORIA[rows[i]];
    assert(ffCategoria == cat && ffValorFoco == 0);
    ffEvento(SDLK_RETURN);
    FonteFiltroConfig f; fontefalha_filtro(&f); assert(f.excluir[cat] == fontefiltro_bit(cat,0));
    fontefalha_config_ler(); fontefalha_filtro(&f); assert(f.excluir[cat] == fontefiltro_bit(cat,0));
    ffEvento(SDLK_RETURN); fontefalha_filtro(&f); assert(!f.excluir[cat]);
    ffEvento(SDLK_ESCAPE); assert(frAberta == 6 && ffCategoria == -1);
    // Same production pointer highlight handler and selection action.
    ffPont(rows[i], -1); ffEscolher(); assert(ffCategoria == cat);
    ffPont(1, cat); ffEscolher(); fontefalha_filtro(&f); assert(f.excluir[cat] == fontefiltro_bit(cat,1));
    ffEscolher(); ffVoltar(); ffVoltar(); assert(!frAberta);
  }
  ffAbrir(); ffPont(2,-1); ffEscolher(); assert(!frAberta && editor == AJ_FILTRO_TAM_MAX && faRetornarFolha == 6);
  FonteFiltroConfig before; fontefalha_filtro(&before); assert(!before.maxGB);
  assert(ajFalhaDefinir(AJ_FILTRO_TAM_MAX,45)); fontefalha_filtro(&before); assert(before.maxGB == 45);
  assert(!ajFalhaDefinir(AJ_FILTRO_TAM_MAX,1001)); FonteFiltroConfig now; fontefalha_filtro(&now); assert(now.maxGB == 45);
  ffAbrir(); ffPont(8,-1); ffEscolher(); assert(keyboard && emptyAllowed && stCampo == AJ_FILTRO_GRUPOS);
  keyboard=0; ffGruposDefinir(" yts, NTb, YTS "); fontefalha_filtro(&now); assert(!strcmp(now.grupos,"yts,NTb"));
  ffGruposDefinir("bad/name"); FonteFiltroConfig same; fontefalha_filtro(&same); assert(!memcmp(&now,&same,sizeof now));
  ffGruposDefinir(""); fontefalha_filtro(&now); assert(!now.grupos[0] && now.maxGB == 45);
  assert(fontefalha_provedor_posicao(1)==0 && fontefalha_provedor_limite(1)==7);
  assert(!fontefalha_provedores() && !fontefalha_addons());
  puts("filter_ui_handlers: all nine categories, TV OK/Back, pointer selection, save/reload, max-size confirm/reject and optional release groups preserve order/limits PASS");
}
'''
code = prefix + enum + '\nstatic int valor[AJ_N];\n' + backend + block + pointer + main
output.parent.mkdir(parents=True,exist_ok=True); output.write_text(code)
output.with_suffix('.manifest.json').write_text(json.dumps({'label':'Exact source Settings enum/filter/backend/pointer handler bodies; isolated editor/keyboard boundaries, not a full UI render', 'sources':{str(source/'src/ajustes.c'):hashlib.sha256(text.encode()).hexdigest(),str(source/'src/ajustes_ux_ilha.inc'):hashlib.sha256(ilha.encode()).hexdigest()},'generated_sha256':hashlib.sha256(code.encode()).hexdigest()},indent=2)+'\n')
