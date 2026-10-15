/* Focused no-blank-preview and unchanged-default helper checks only. */
#define main original_reader_matrix_main
#include "reader_probe.c"
#undef main
int main(int argc,char**argv){(void)argc;(void)argv;TxtEstilo styles[]={TXT_ILHA_APOIO,TXT_AJ_ESTADO,TXT_ILHA_SUB,TXT_ILHA_TEXTO,TXT_CAPTION,TXT_HERO_SIN,TXT_DET_META2,TXT_DET_SIN};for(size_t e=0;e<sizeof styles/sizeof styles[0];e++)for(font_size=0;font_size<3;font_size++){
for(int n=1;n<=4;n++){int result=sinopse_linhas(styles[e],30,n);check(result>=1,"bounded nonempty preview retains at least one visible line at every selected size");float h=sinopse_bloco_corta(styles[e],"Visible synopsis preview",255,255,255,0,0,500,30,1,n);check(h>0,"actual block helper does not blank the existing preview");if(!font_size)check(result==n&&h==30*n,"Standard preserves exact existing line count and height");}
if(!font_size){check(sinopse_estilo(styles[e])==styles[e],"Standard preserves exact existing style ID");check(sinopse_entrelinha(styles[e],28.5f)==28.5f,"Standard preserves exact floating-point leading");}
check(sinopse_linhas(styles[e],30,0)==0&&sinopse_linhas(styles[e],30,-1)==-1,"unbounded caller semantics remain unchanged");}
for(font_size=0;font_size<3;font_size++){float h=sinopse_bloco_corta(TXT_DET_SIN,"Title synopsis preview",255,255,255,0,0,500,40,1,3);check(h>0&&h<=120.01f,"all title synopsis sizes stay within existing120px stack budget");}
printf("PASS preview refinement checks=%d typography_styles=8 sizes=3 bounded_line_counts=4\n",checks);return 0;}
