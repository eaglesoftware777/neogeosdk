#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "cx_tournament.h"
int main(void){
 unsigned seed;for(seed=1;seed<=1000;seed++){
  CxTournament t,copy;int n;unsigned seen[CX_SQUADS]={0},matches=0,i;
  cx_tournament_init(&t,(uint8_t)(seed%CX_SQUADS),seed);copy=t;
  for(i=0;i<32;i++)assert(t.groups[i]<CX_SQUADS&&++seen[t.groups[i]]==1);
  assert(seen[seed%CX_SQUADS]==1);    /* the player's side is always there */
  while((n=cx_tournament_next(&t))>=0){
   CxFixture *f=&t.fixture[n];uint8_t a=(uint8_t)((seed+n)%3),b=(uint8_t)((seed+n*3)%3);
   uint8_t winner=a==b?(n<48?255:t.human):a>b?f->home:f->away;
   assert(cx_tournament_result(&t,n,a,b,winner));assert(!cx_tournament_result(&t,n,a,b,winner));matches++;
  }
  assert(t.completed==64&&t.generated==64&&t.champion<CX_SQUADS&&seen[t.champion]&&matches>=3);
  for(i=0;i<32;i++){assert(t.table[i].played==3);assert(t.table[i].points==3*t.table[i].wins+t.table[i].draws);}
  for(i=0;i<64;i++)assert(t.fixture[i].played&&t.fixture[i].home!=t.fixture[i].away);
  /* Replay exactly the same inputs through the same random seed. */
  while((n=cx_tournament_next(&copy))>=0){CxFixture *f=&t.fixture[n];assert(cx_tournament_result(&copy,n,f->goals[0],f->goals[1],f->winner));}
  assert(!memcmp(&copy,&t,sizeof(t)));
 }
 /* scorers: accumulated per man, the leaders in order; CPU results credit somebody */
 {
  CxTournament t;CxScorer top[3];unsigned total=0,i;uint8_t xi[11];
  cx_tournament_init(&t,5,77);
  cx_tournament_scorer(&t,5,9);cx_tournament_scorer(&t,5,9);cx_tournament_scorer(&t,7,10);
  assert(cx_tournament_top(&t,top,3)>=2&&top[0].team==5&&top[0].squad==9&&top[0].goals==2);
  while(cx_tournament_next(&t)>=0){int n=cx_tournament_next(&t);CxFixture *f=&t.fixture[n];
   uint8_t w=(n<48)?255:f->home;assert(cx_tournament_result(&t,(uint8_t)n,(uint8_t)(n<48?1:2),1,w));}
  for(i=0;i<CX_SCORERS;i++)total+=t.scorers[i].goals;
  assert(total>=CX_SCORERS&&cx_tournament_top(&t,top,1)==1&&top[0].goals>=3);   /* the table is full, and somebody leads */
  /* a line-up without a man: the bench fills his place, the reserve keeper the keeper's */
  cx_lineup((1u<<9)|(1u<<0),xi);
  assert(xi[9]==12&&xi[0]==11&&xi[8]==8);
  cx_lineup(0,xi);for(i=0;i<11;i++)assert(xi[i]==i);
 }
 printf("1000 complete tournaments: 64 fixtures, unique entrants, standings, bracket and replay passed; state %zu bytes\n",sizeof(CxTournament));return 0;
}
