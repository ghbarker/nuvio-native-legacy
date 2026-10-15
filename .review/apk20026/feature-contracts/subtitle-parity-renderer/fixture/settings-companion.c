#include SETTINGS_SOURCE

void fixture_secondary(int size, int color, int bg, int border, int family,
                       int opacity, int outline, int bold, int position) {
  valor[AJ_LEG2_TAMANHO] = size;
  valor[AJ_LEG2_COR] = color;
  valor[AJ_LEG2_FUNDO] = bg;
  valor[AJ_LEG2_BORDA] = border;
  valor[AJ_LEG2_FONTE] = family;
  valor[AJ_LEG2_OPACIDADE] = opacity;
  valor[AJ_LEG2_COR_BORDA] = outline;
  valor[AJ_LEG2_NEGRITO] = bold;
  valor[AJ_LEG2_POSICAO] = position;
  valor[AJ_LEG2_ATRASO] = 0;
}
