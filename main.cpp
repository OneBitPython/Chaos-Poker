#include <cstdio>
#include <cstring>
#include <cstdint>
#include <algorithm>
using namespace std;

// ---- Card encoding: rank*4+suit, rank 0=2 .. 12=A ----
static inline int cr(int c){return c>>2;}
static inline int cs(int c){return c&3;}
static inline int mkc(int r,int s){return (r<<2)|s;}

static int parse_card(const char* s){
    int r;
    switch(s[0]){
        case '2':r=0;break;case '3':r=1;break;case '4':r=2;break;case '5':r=3;break;
        case '6':r=4;break;case '7':r=5;break;case '8':r=6;break;case '9':r=7;break;
        case 'T':r=8;break;case 'J':r=9;break;case 'Q':r=10;break;case 'K':r=11;break;
        default:r=12;break;
    }
    int su;
    switch(s[1]){
        case 'c':su=0;break;case 'd':su=1;break;case 'h':su=2;break;default:su=3;break;
    }
    return mkc(r,su);
}

// ---- eval7: returns category 0(HC)..8(SF) in top 4 bits, tiebreak in lower 28 ----
static uint32_t eval7(const int* c){
    int freq[13]={};
    int sc[4]={};
    int ranks[7],suits[7];
    for(int i=0;i<7;i++){ranks[i]=cr(c[i]);suits[i]=cs(c[i]);freq[ranks[i]]++;sc[suits[i]]++;}

    // flush?
    int fsuit=-1;
    for(int s=0;s<4;s++) if(sc[s]>=5){fsuit=s;break;}
    int frc[7],nf=0;
    if(fsuit>=0){for(int i=0;i<7;i++) if(suits[i]==fsuit) frc[nf++]=ranks[i];}

    // straight check helper (sorted desc array, returns top card or -1)
    auto str5=[](int* a,int n)->int{
        // sort desc
        for(int i=0;i<n-1;i++) for(int j=i+1;j<n;j++) if(a[j]>a[i]){int t=a[i];a[i]=a[j];a[j]=t;}
        // unique
        int u[14],nu=0;
        for(int i=0;i<n;i++) if(!nu||u[nu-1]!=a[i]) u[nu++]=a[i];
        if(nu&&u[0]==12) u[nu++]=-1;
        for(int i=0;i+4<nu;i++) if(u[i]-u[i+4]==4) return u[i];
        return -1;
    };

    // straight flush?
    if(fsuit>=0){int fc2[7]; memcpy(fc2,frc,nf*4); int sf=str5(fc2,nf); if(sf>=0) return (8u<<28)|(uint32_t)sf;}

    // groups sorted by freq desc, rank desc
    int gr[13],gf[13],ng=0;
    for(int r=12;r>=0;r--) if(freq[r]) {gf[ng]=freq[r];gr[ng]=r;ng++;}
    for(int i=0;i<ng-1;i++) for(int j=i+1;j<ng;j++)
        if(gf[j]>gf[i]||(gf[j]==gf[i]&&gr[j]>gr[i])){int t=gf[i];gf[i]=gf[j];gf[j]=t;t=gr[i];gr[i]=gr[j];gr[j]=t;}

    int g0f=gf[0],g0r=gr[0];
    int g1f=ng>1?gf[1]:0,g1r=ng>1?gr[1]:0;

    // quads
    if(g0f==4){int k=-1;for(int i=0;i<ng;i++) if(gr[i]!=g0r){k=gr[i];break;} return (7u<<28)|((uint32_t)g0r<<4)|(uint32_t)(k&0xF);}
    // full house
    if(g0f==3&&g1f>=2) return (6u<<28)|((uint32_t)g0r<<4)|(uint32_t)g1r;
    // flush
    if(fsuit>=0){uint32_t v=5u<<28;for(int i=0;i<5;i++) v|=(uint32_t)frc[i]<<(16-4*i);return v;}
    // straight
    {int rs[7];memcpy(rs,ranks,28);int st=str5(rs,7);if(st>=0)return(4u<<28)|(uint32_t)st;}
    // trips
    if(g0f==3){uint32_t v=(3u<<28)|((uint32_t)g0r<<8);int cnt=0;for(int i=0;i<ng;i++){if(gr[i]==g0r)continue;v|=(uint32_t)gr[i]<<(4-4*cnt);if(++cnt==2)break;}return v;}
    // two pair
    if(g0f==2&&g1f==2){int hi=g0r>g1r?g0r:g1r,lo=g0r<g1r?g0r:g1r,k=-1;for(int i=0;i<ng;i++) if(gr[i]!=g0r&&gr[i]!=g1r){k=gr[i];break;} return(2u<<28)|((uint32_t)hi<<8)|((uint32_t)lo<<4)|(uint32_t)(k&0xF);}
    // one pair
    if(g0f==2){uint32_t v=(1u<<28)|((uint32_t)g0r<<12);int cnt=0;for(int i=0;i<ng;i++){if(gr[i]==g0r)continue;v|=(uint32_t)gr[i]<<(8-4*cnt);if(++cnt==3)break;}return v;}
    // high card
    {uint32_t v=0;for(int i=0;i<5;i++) v|=(uint32_t)gr[i]<<(16-4*i);return v;}
}

// Category 0..8 -> baseline equity vs N opponents
// Precomputed: roughly p^N where p is single-opponent win rate per category
static const float CAT_EQ1[9]={0.17f,0.44f,0.63f,0.72f,0.78f,0.83f,0.90f,0.95f,0.98f};
static float cat_equity(int cat, int nopps){
    float p=CAT_EQ1[cat];
    float eq=p;
    for(int i=1;i<nopps;i++) eq*=p;
    return eq;
}

// Pre-flop equity lookup by (hi_rank*13+lo_rank, suited)
// Approximation: based on standard pre-flop hand strength tables
static float preflop_equity(int h0, int h1, int nopps){
    int r0=cr(h0),r1=cr(h1);
    if(r0<r1){int t=r0;r0=r1;r1=t;}
    int suited=(cs(h0)==cs(h1))?1:0;
    // pair
    float base;
    if(r0==r1){
        // pair strength: AA=0.85 down to 22=0.50 vs 1 opp
        base=0.50f+(r0/12.0f)*0.35f;
    } else {
        // high card: top rank + gap + suitedness
        float hi=0.30f+(r0/12.0f)*0.30f;
        float lo=(r1/12.0f)*0.10f;
        float gap_pen=(r0-r1-1)*0.02f;
        float suit_b=suited?0.03f:0.0f;
        base=hi+lo-gap_pen+suit_b;
        if(base<0.28f) base=0.28f;
        if(base>0.72f) base=0.72f;
    }
    // scale vs multiple opponents
    float eq=base;
    for(int i=1;i<nopps;i++) eq*=base;
    return eq;
}

// ---- State ----
struct S {
    int seat,npl,chips,pot,sb;
    int hole[2];
    int comm[5]; int ncomm;
    int cc[9]; // chip counts
    int btr[9]; // bet this round
    bool fold[9],dead[52];
    int mults[4]; // preflop,flop,turn,river
    int alive,hand_alive;
    int swap_cnt,last_swap_idx;
} G;

static void reset_hand(){
    G.ncomm=0; G.pot=0; G.swap_cnt=0; G.last_swap_idx=-1;
    memset(G.dead,0,sizeof(G.dead));
    memset(G.fold,0,sizeof(G.fold));
    memset(G.btr,0,sizeof(G.btr));
    G.hand_alive=G.alive;
}
static int opps(){return G.hand_alive>1?G.hand_alive-1:1;}

// Fast eval of my current hand (hole+community padded to 7)
static uint32_t my_score(){
    int c[7];
    c[0]=G.hole[0];c[1]=G.hole[1];
    for(int i=0;i<G.ncomm;i++) c[2+i]=G.comm[i];
    // pad with hole cards again if <7
    for(int i=G.ncomm;i<5;i++) c[2+i]=G.hole[i&1];
    return eval7(c);
}

static float my_equity(){
    if(G.ncomm<3) return preflop_equity(G.hole[0],G.hole[1],opps());
    int cat=(int)(my_score()>>28);
    return cat_equity(cat,opps());
}

// For swap: eval hand without hole[idx], get cat of best 5 from (other_hole + comm)
static float equity_without(int idx){
    // Only one hole card + community - can't really eval properly without 2 hole cards
    // Use community only for rough estimate of "board strength" and hole contribution
    int other=G.hole[1-idx];
    int c[7];
    c[0]=other; c[1]=other; // duplicate
    for(int i=0;i<G.ncomm;i++) c[2+i]=G.comm[i];
    for(int i=G.ncomm;i<5;i++) c[2+i]=other;
    int cat=(int)(eval7(c)>>28);
    return cat_equity(cat,opps())*0.85f; // penalty: we lose a card
}

static float fair_eq(){
    int total=0;
    for(int i=0;i<G.npl;i++) total+=G.cc[i];
    if(!total) return 1.0f/G.alive;
    return (float)G.chips/total;
}

static void do_bet(int chips2,int cur_bet,int my_bet,int min_raise,int pot2){
    int to_call=cur_bet-my_bet;
    float eq=my_equity();
    float fe=fair_eq();
    float pot_odds=pot2>0?(float)to_call/(pot2+to_call):0.0f;
    float R=fe>0?eq/fe:1.0f;

    if(to_call<=0){
        if(R>=1.3f){
            int amt=(int)(pot2*0.75f);
            if(amt<min_raise) amt=min_raise;
            if(amt>chips2+my_bet) amt=chips2+my_bet;
            if(amt>=min_raise) printf("RAISE %d\n",amt);
            else printf("CHECK\n");
        } else if(R>=1.1f){
            int amt=(int)(pot2*0.45f);
            if(amt<min_raise) amt=min_raise;
            if(amt>chips2+my_bet) amt=chips2+my_bet;
            if(amt>=min_raise) printf("RAISE %d\n",amt);
            else printf("CHECK\n");
        } else printf("CHECK\n");
    } else {
        if(eq<=pot_odds+0.04f){ printf("FOLD\n"); return; }
        if(R>=1.35f){
            int amt=(int)(pot2*0.9f);
            if(amt<min_raise) amt=min_raise;
            if(amt>chips2+my_bet) amt=chips2+my_bet;
            if(amt>=min_raise){ printf("RAISE %d\n",amt); return; }
        }
        if(to_call<=chips2) printf("CALL\n");
        else printf("ALLIN\n");
    }
}

static void do_swap(){
    if(G.swap_cnt>=2){ printf("STAY\n"); return; }
    int cost;
    switch(G.ncomm){
        case 0: cost=G.mults[0]*G.sb; break;
        case 3: cost=G.mults[1]*G.sb; break;
        case 4: cost=G.mults[2]*G.sb; break;
        default: cost=G.mults[3]*G.sb; break;
    }
    if(cost>G.chips){ printf("STAY\n"); return; }

    float eq_cur=my_equity();
    float fe=fair_eq();
    if(eq_cur>fe*1.12f){ printf("STAY\n"); return; }

    // Score each hole card: remove it and see if hand gets worse
    float e0=equity_without(0);
    float e1=equity_without(1);
    // Swap the weaker one
    int idx=(e0<e1)?0:1;
    float e_keep=max(e0,e1);
    // Only swap if expected gain justifies cost
    // Rough: swapping gains ~(avg_category_after - current) * pot
    // Heuristic: if current eq is bad AND we can improve, swap
    if(e_keep>eq_cur*1.05f||eq_cur<fe*0.85f){
        G.dead[G.hole[idx]]=true;
        G.swap_cnt++;
        G.last_swap_idx=idx;
        printf("SWAP %d\n",idx);
    } else {
        printf("STAY\n");
    }
}

static void do_vote(int chips2){
    float eq=my_equity();
    float fe=fair_eq();
    if(eq>=fe){
        int w=(int)(chips2*(eq-fe)*0.05f);
        if(w<0) w=0;
        printf("VOTE YES %d\n",w);
    } else {
        int w=(int)(chips2*(fe-eq)*0.05f);
        if(w<0) w=0;
        printf("VOTE NO %d\n",w);
    }
}

int main(){
    // Force unbuffered output
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stdin,NULL,_IONBF,0);

    char line[256];
    while(fgets(line,sizeof(line),stdin)){
        char cmd[32];
        if(sscanf(line,"%31s",cmd)!=1) continue;

        if(!strcmp(cmd,"GAME_START")){
            int np,seat,chips,p,f,t,r;
            sscanf(line,"%*s %d %d %d %d %d %d %d",&np,&seat,&chips,&p,&f,&t,&r);
            G.seat=seat; G.npl=np; G.chips=chips; G.alive=np; G.sb=1;
            G.mults[0]=p; G.mults[1]=f; G.mults[2]=t; G.mults[3]=r;
            for(int i=0;i<np;i++) G.cc[i]=chips;
            memset(G.fold,0,sizeof(G.fold));

        } else if(!strcmp(cmd,"HAND_START")){
            int hn,dealer,sb,bb,sba,bba;
            sscanf(line,"%*s %d %d %d %d %d %d",&hn,&dealer,&sb,&bb,&sba,&bba);
            G.sb=sba;
            reset_hand();
            if(sb>=0&&sb<G.npl){ G.cc[sb]-=sba; G.btr[sb]=sba; G.pot+=sba; }
            if(bb>=0&&bb<G.npl){ int b=sba*2; G.cc[bb]-=b; G.btr[bb]=b; G.pot+=b; }

        } else if(!strcmp(cmd,"CHIPS")){
            int al=0;
            char* p2=line+5;
            for(int i=0;i<G.npl;i++){
                while(*p2==' ') p2++;
                int v=0; while(*p2>='0'&&*p2<='9') v=v*10+(*p2++)-'0';
                G.cc[i]=v; if(v>0) al++;
                if(i==G.seat) G.chips=v;
            }
            G.alive=al; G.hand_alive=al;

        } else if(!strcmp(cmd,"DEAL_HOLE")){
            char c1[4],c2[4];
            sscanf(line,"%*s %3s %3s",c1,c2);
            G.hole[0]=parse_card(c1); G.hole[1]=parse_card(c2);

        } else if(!strcmp(cmd,"DEAL_FLOP")){
            char c1[4],c2[4],c3[4];
            sscanf(line,"%*s %3s %3s %3s",c1,c2,c3);
            G.comm[0]=parse_card(c1); G.comm[1]=parse_card(c2); G.comm[2]=parse_card(c3);
            G.ncomm=3; G.swap_cnt=0; memset(G.btr,0,sizeof(G.btr));

        } else if(!strcmp(cmd,"DEAL_TURN")){
            char c1[4]; sscanf(line,"%*s %3s",c1);
            G.comm[G.ncomm++]=parse_card(c1); G.swap_cnt=0; memset(G.btr,0,sizeof(G.btr));

        } else if(!strcmp(cmd,"DEAL_RIVER")){
            char c1[4]; sscanf(line,"%*s %3s",c1);
            G.comm[G.ncomm++]=parse_card(c1); G.swap_cnt=0; memset(G.btr,0,sizeof(G.btr));

        } else if(!strcmp(cmd,"REDRAW_FLOP")){
            char c1[4],c2[4],c3[4];
            sscanf(line,"%*s %3s %3s %3s",c1,c2,c3);
            G.comm[0]=parse_card(c1); G.comm[1]=parse_card(c2); G.comm[2]=parse_card(c3);
            G.ncomm=3;

        } else if(!strcmp(cmd,"REDRAW_TURN")){
            char c1[4]; sscanf(line,"%*s %3s",c1);
            G.comm[G.ncomm-1]=parse_card(c1);

        } else if(!strcmp(cmd,"REDRAW_RIVER")){
            char c1[4]; sscanf(line,"%*s %3s",c1);
            G.comm[G.ncomm-1]=parse_card(c1);

        } else if(!strcmp(cmd,"SWAP_RESULT")){
            char c1[4]; sscanf(line,"%*s %3s",c1);
            if(G.last_swap_idx>=0) G.hole[G.last_swap_idx]=parse_card(c1);

        } else if(!strcmp(cmd,"ACTION")){
            int seat; char act[16];
            sscanf(line,"%*s %d %15s",&seat,act);
            if(seat<0||seat>=G.npl) continue;
            if(!strcmp(act,"FOLD")){
                if(seat!=G.seat&&!G.fold[seat]){G.fold[seat]=true;G.hand_alive--;}
            } else if(!strcmp(act,"CALL")||!strcmp(act,"RAISE")||!strcmp(act,"ALLIN")){
                int tb; sscanf(line,"%*s %*d %*s %d",&tb);
                int inc=tb-G.btr[seat]; if(inc<0) inc=0;
                G.cc[seat]-=inc; G.btr[seat]=tb; G.pot+=inc;
                if(!strcmp(act,"ALLIN")) G.cc[seat]=0;
                if(seat==G.seat) G.chips=G.cc[seat];
            }

        } else if(!strcmp(cmd,"VOTE_RESULT")){
            int yt,nt; sscanf(line,"%*s %d %d",&yt,&nt);
            G.pot+=yt+nt;

        } else if(!strcmp(cmd,"SWAP_PROMPT")){
            int cost,ch; sscanf(line,"%*s %d %d",&cost,&ch);
            G.chips=ch; G.cc[G.seat]=ch;
            do_swap();

        } else if(!strcmp(cmd,"VOTE_PROMPT")){
            int ch; sscanf(line,"%*s %d",&ch);
            G.chips=ch; G.cc[G.seat]=ch;
            do_vote(ch);

        } else if(!strcmp(cmd,"ACTION_PROMPT")){
            int ch,cb,mb,mr,pt;
            sscanf(line,"%*s %d %d %d %d %d",&ch,&cb,&mb,&mr,&pt);
            G.chips=ch; G.cc[G.seat]=ch; G.pot=pt;
            do_bet(ch,cb,mb,mr,pt);
        }
        // Ignore SWAP_DONE, SHOWDOWN, WINNER, ELIMINATE, GAME_OVER
    }
}