void cat_trocar_continuar(const CatItem *items,int count) {
  pthread_mutex_lock(&pubTravaT);
  int row=-1,start=0,old=0;
  for(int k=0;k<nPubFils;k++)if(!strcmp(pubFils[k].chave,"continue_watching")){row=k;start=pubFils[k].ini;old=pubFils[k].n;break;}
  assert(count>=0&&count+nPub-old<4096);
  memmove(pub+start+count,pub+start+old,(size_t)(nPub-start-old)*sizeof(CatItem));
  if(count)memcpy(pub+start,items,(size_t)count*sizeof(CatItem));
  nPub+=count-old;
  for(int k=0;k<nPubFils;k++){
    if(k==row)pubFils[k].n=count;
    else if(pubFils[k].ini>=start+old)pubFils[k].ini+=count-old;
  }
  if(row>=0&&!count){memmove(pubFils+row,pubFils+row+1,(size_t)(nPubFils-row-1)*sizeof(CatFileira));nPubFils--;}
  if(row<0&&count){assert(nPubFils<CAT_FIL_MAX);memmove(pubFils+1,pubFils,(size_t)nPubFils*sizeof(CatFileira));memset(pubFils,0,sizeof(CatFileira));snprintf(pubFils[0].chave,sizeof pubFils[0].chave,"continue_watching");pubFils[0].n=count;nPubFils++;}
  pthread_mutex_unlock(&pubTravaT);
}
