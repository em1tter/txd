
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include <stdbool.h>

//Parametry
//#define maxVlaku 20
#define minutVJednotceCasu 15
#define maxCas 24*60/minutVJednotceCasu
#define pocetStanic 6
#define pocetJednotekSklonu 3 //Počet jednotek na který se rozdělí sklony
#define maximalniSklon 40 //Maximální sklon tratí v promilích
#define iniCenaZaJednotkuE 0.0005 //Počáteční cena za jednotku energie
#define iniCenaZaJednotkuEEl ((float) iniCenaZaJednotkuE/3)
#define iniCenaZaDostavbuKoleje 35000
#define tihoveZrychleni 9.81
#define maxDelkaCesty 8*pocetStanic
#define pocetVlakuVObchodu 5
#define maxDelkaNazvu 46
#define maxDelkaNazvuStanice 10
#define maxPocetTrati 7
#define maxPocetKoleji 6
#define iniCenaZaElKmKoleje 0.3*iniCenaZaDostavbuKoleje
#define pocetVarTrate 3
#define koefCenyZruseniKeStavbe -0.5
#define koefCenyZruseniElektrifikace 0.2
#define pocetStanicVCesteVNahledu 3
#define poJakeDobeVsichniOdejdou 4 //V jednotkách času - po jaké době čekání odejdou všichni cestující 
#define jakaCastLidiZustane //Jaká část lidí zůstane
#define iniDenniUrok (1+4.0/365)
#define iniVydelekZaPasazerKilometr 2
#define koefOdporu 1.5
#define iniCenaZaUdrzbuKoleje ((float)iniCenaZaDostavbuKoleje/365)
#define iniCenaZaUdrzbuElKoleje 1.7*(iniCenaZaUdrzbuKoleje)
#define iniCenaPracePruvodcich (iniVydelekZaPasazerKilometr*100) //Počáteční cena práce průvodčích na hodinu
#define iniCenaPraceStrojvedoucich (iniVydelekZaPasazerKilometr*150) //Počáteční cena práce strojvedoucích na hodinu

//Pomocné parametry
#define normKoefDoby 2
#define nizsiKoefDoby 1.5
#define vyssiKoefDoby 2.5
#define normNasobekZrychleni 1.4
#define nizsiNasobekZrychleni 1.2
#define vyssiNasobekZrychleni 1.6
#define osa (2*12*60/minutVJednotceCasu)

char jmenaStanic[pocetStanic][maxDelkaNazvuStanice] = {"Poustevna", "Lípa", "Lhota", "Dvory", "Újezd", "Ves"};
float placeneKmZDo[pocetStanic][pocetStanic] = {{0,44,44+15,44+12,44+12+20,44+12+20+10},{44,0,15,12,12+20,12+20+10},{15+44,15,0,15+12,30,40},{12+44,12,12+15,0,20,20+10},{20+12+44,20+12,30,20,0,10},{10+20+12+44,10+20+12,30,10+20,10,0}};


//Typy
typedef struct{
    char nazev[maxDelkaNazvu]; //Název vlaku
    float valivyOdpor; //[kN]
    float hmotnost; //[t]
    float porizovaciCena;
    unsigned char cesta[maxDelkaCesty]; //Postupná cesta vlaku, tj. index stanice kam na x-tém místě míří vlak
    unsigned char stanicePrijezduVlaku[maxCas]; //Souřadnice x jsou časy od 0, 1, ..., maxCas-1 a skládá se z indexů stanic na které v čas x přijede vlak
    unsigned char staniceOdjezduVlaku[maxCas]; //Souřadnice x jsou časy od 0, 1, ..., maxCas-1 a skládá se z indexů stanic ze kterých v čas x odjede vlak
    int pocetPruvodcich;
    unsigned short kapacita; //Kolik cestujících se do vlaku vejde
    float maxVykon; //[kW]
    float maxRychlost; //[km/h]
    char efektivitaRekuperace; //Na kolik procent je vlak schopen rekuperace
    char potrebujeElektrifikaci; //Potřebuje tento vlak elektrifikaci?
}parVlaku;

typedef struct{
    float delka; //Délka tratě v kilometrech
    float stoupani[pocetJednotekSklonu+1];    //Souřadnice x jsou úhly 0, 1/pocetJednotekSklonu, ..., maximalniSklon a skládá se z délek tratě o tomto stoupání
    float klesani[pocetJednotekSklonu+1];     //Souřadnice x jsou úhly od -0, -1/pocetJednotekSklonu, ..., -maximalniSklon a skládá se z délek tratě o tomto klesání
    unsigned char pocetKoleji; //Počet kolejí postavených na této trati
    char elektrifikovano; //Ano nebo ne podle toho zda tato trať byla elektrifikována
    char obsazenost[maxCas]; //Souřadnice x jsou časy od 0, 1, ..., maxCas-1 a skládá se z počtů vlaků, které v čas x zabírají kolej na této trati
    float nasobekZrychleni; //Kolikrát vyšší je celkové zrychlení na trati oproti zrychlení vlaku na jeho maximální rychlost
    float koefDoby; //Kolikrát delší bude průjezd tratě oproti teoretickému minimu
}parTrate;

typedef struct{
    float cenaStavby;
    float delka; //[km]
    float stoupani[pocetJednotekSklonu+1];    //Souřadnice x jsou úhly od 0, 1/pocetJednotekSklonu, ..., maximalniSklon a skládá se z délek těchto stoupání
    float klesani[pocetJednotekSklonu+1];     //Souřadnice x jsou úhly od -0, -1/pocetJednotekSklonu, ..., -maximalniSklon a skládá se z délek těchto klesání
    float koefDoby;
    float nasobekZrychleni; //Kolikrát vyšší je celkové zrychlení na trati oproti zrychlení vlaku na jeho maximální rychlost
}surParTrate;

typedef struct{
    float cenaPracePruvodcich; //Cena práce průvodčích na hodinu
    float cenaPraceStrojvedoucich; //Cena práce strojvedoucích na hodinu
    float cenaZaJednotkuE; //Cena za MJ energie
    float cenaZaJednotkuEEl; //Cena za MJ elektrické energie
    float cenaUdrzbyZaKmNeelTrate; //Cena za km neelektrifikované tratě za den
    float cenaUdrzbyZaKmElTrate; //Cena za km elektrifikované tratě za den
    float cenaZaDostavbuKmKoleje; //Cena za dostavbu kilometru koleje k již postavené trati
    float cenaZaElKmKoleje; //Cena za elektrifikaci kilometru koleje
    float vydelekZaPasazerKilometr; //Kolik korun hráč vydělá za jeden pasažérkilometr
    float denniUrok; //Dení úrok, pokud je hráč v záporu
}hodnotyCen;

typedef struct{
    double hlavniUcet;
}stavPenez;

//Pomůcky
void kontrolaVelikosti(int *vstup, int mez){ //Zkontroluje zda int vstup je větší než mez a pokud ne, tak požádá o znovuzadání hodnoty a přepíše původní. 
    while (*vstup <= mez){
        printf("Toto číslo musí být větší než %d. Prosím zadejte novou hodnotu.\n", mez);
        skenfInt(vstup);
        }
    return;
}



void kontrolaVyberuS0(int *vstup, int maxvstup){ //Zkontroluje zda int vstup je větší nebo roven 0 a zároven menší nebo roven maxvstup a pokud ne, tak požádá o znovuzadání hodnoty a přepíše původní.
    while (*vstup > maxvstup || *vstup < 0){
        printf("Vstupní hodnota nenabývá platné hodnoty. Prosím zadejte novou hodnotu.\n");
        skenfInt(vstup);
    }
    return;
}


const int BUF_SIZE = 80;

int scanfInt(int *cislo) { //Tuto funkci jsem ukradl od inženýra Jiřího Zděnka
    // Stav: odladeno
    // === Bezpecne pro BUF_SIZE zadanych znaku ===
    // Navratova hodnota:
    // TRUE  - zadano cele cislo
    // FALSE - neplatny vstup
    char vstup[BUF_SIZE], smeti[BUF_SIZE];
    fgets(vstup, sizeof (vstup), stdin);
    if (sscanf(vstup, "%i%[^\n]", cislo, smeti) != 1)
        return (0); // Input error
    return (1);
}

int skenfInt(int *cislo){
    char p;
    p = scanfInt(cislo);
    if (p) return;
    while(!p){
    	printf("Vstupní hodnota nenabývá platné hodnoty. Prosím zadejte novou hodnotu.\n");
	p = scanfInt(cislo);
    }
    return;
}

unsigned int posledniNenulovy(int delka,unsigned char pole[delka]){  //Vrátí index posledního nenulového prvku v poli o délce a pokud jsou všechny nulové, tak vrátí 0
    int i;
    for(i=delka-1;i>=0;i--){
        if (pole[i]!=0) return i; 
    }
    return 0;
}

unsigned int prvniNenulovy(int delka,unsigned char pole[delka]){  //Vrátí index prvního nenulového prvku v poli o délce a pokud jsou všechny nulové, tak vrátí 1
    int i;
    for(i=0;i<delka;i++){
        if (pole[i]!=0) return i; 
    }
    return 1;
}


void ukazPole(int vyska, int sirka, int pole[][sirka]){
    int i,j;
    for (i=0; i<vyska; i++){
        for (j=0; j<sirka; j++){
            printf("%d ",pole[i][j]);
        }
        printf("\n");
    }
}

//unsigned char prvniOd(unsigned int delka, unsigned char pole, int pocatecniIndex, char smer){ //Vrátí první nenulovou hodnotu nalevo resp. napravo od pocatecniIndex v pole délky delka pro směr -1 resp. 1 a nebo vrátí 0, pokud nalevo resp. napravo žádná nenulová hodnota není. Do obou směrů se nepočítá i samotný pocatecniIndex
//    while(1){
//        pocatecniIndex+=smer;
//        if (pole[pocatecniIndex]) return pole[pocatecniIndex];
//        if ((pocatecniIndex==0)||(pocatecniIndex==delka-1)) return 0;
//    }
//}

//Funkce
float energieNaPrujezdTrate(parTrate *trat, parVlaku *vlak){ //Realističnost těchto výpočtů je nutné brát s rezervou
    unsigned int i;
    float odporovaSilaVUseku;
    float celkovaE = 0;
    for (i=0;i<=pocetJednotekSklonu;i++){    //Výpočet energie za stoupání
        odporovaSilaVUseku = sin(atan((float)maximalniSklon*(float)i/(float)pocetJednotekSklonu))*(vlak->hmotnost)*tihoveZrychleni+(vlak->valivyOdpor); //[kN]
        celkovaE = celkovaE + odporovaSilaVUseku*(trat->stoupani[i]); //[MJ]
    }
    for (i=0;i<=pocetJednotekSklonu;i++){ //Výpočet energie za klesání
        odporovaSilaVUseku = -sin(atan((float)maximalniSklon*(float)i/(float)pocetJednotekSklonu))*(vlak->hmotnost)*tihoveZrychleni+(vlak->valivyOdpor);
        if (odporovaSilaVUseku<0)odporovaSilaVUseku = odporovaSilaVUseku*(float)(vlak->efektivitaRekuperace)/(float)100;
        celkovaE = celkovaE + odporovaSilaVUseku*(trat->stoupani[i]);
    }
    //Výpočet energie za zrychlení a zpomalení vlaku
    if(vlak->efektivitaRekuperace) celkovaE += trat->nasobekZrychleni*((float)vlak->efektivitaRekuperace/100-1)*vlak->hmotnost*vlak->maxRychlost*vlak->maxRychlost/2;
    else celkovaE += trat->nasobekZrychleni*vlak->hmotnost*vlak->maxRychlost*vlak->maxRychlost/2;
    return celkovaE;
}

float dobaPrujezduTrate(parTrate *trat, parVlaku *vlak){ //[Vrátí v minutách délku průjezdu tratě trat vlakem vlak. Realističnost těchto výpočtů je nutné brát s rezervou
    float rychlostVUseku, odporovaSilaVUseku;
    int i;
    float dobaPrujezdu = 0; //[min]
    for (i=0;i<=pocetJednotekSklonu;i++){    
        rychlostVUseku = 3.6*vlak->maxVykon/(sin(atan((float)maximalniSklon*(float)i/(float)pocetJednotekSklonu/1000))*(vlak->hmotnost)*tihoveZrychleni+(vlak->valivyOdpor)); //[km/h]
        if (rychlostVUseku>vlak->maxRychlost) rychlostVUseku = vlak->maxRychlost;
        dobaPrujezdu += 60*trat->stoupani[i]/(rychlostVUseku); //[min]
    }
    for (i=0;i<=pocetJednotekSklonu;i++){    
        odporovaSilaVUseku = ((vlak->valivyOdpor)-sin(atan((float)maximalniSklon*(float)i/(float)pocetJednotekSklonu)/1000)*(vlak->hmotnost)*tihoveZrychleni); //[kN]
        if (odporovaSilaVUseku>0){
            rychlostVUseku = 3.6*vlak->maxVykon/odporovaSilaVUseku; //[km/h]
            if (rychlostVUseku>vlak->maxRychlost) rychlostVUseku = vlak->maxRychlost;
            dobaPrujezdu += 60*trat->klesani[i]/(rychlostVUseku); //[min]
        }
        else{
            dobaPrujezdu += 60*trat->klesani[i]/(vlak->maxRychlost); //[min]
        }
    }
    return dobaPrujezdu*trat->koefDoby;
}

float cenaZaDenTrate(parTrate *trat, hodnotyCen *ceny){
    if (trat->elektrifikovano){
        return (trat->pocetKoleji)*(trat->delka)*(ceny->cenaUdrzbyZaKmElTrate);
    }
    else{
        return (trat->pocetKoleji)*(trat->delka)*(ceny->cenaUdrzbyZaKmNeelTrate);
    }
}

float cenaZaDenVlaku(parVlaku *vlak, hodnotyCen *ceny, parTrate tratZDo[pocetStanic][pocetStanic]){
    float casPrace = (float)((posledniNenulovy(maxCas,vlak->stanicePrijezduVlaku)-prvniNenulovy(maxCas,vlak->stanicePrijezduVlaku)+1)*minutVJednotceCasu)/(float)60;
    float cenaCelkem = 0, cenaZaMJ;
    unsigned int i, j;
    parTrate *aktTrat;
    if (vlak->potrebujeElektrifikaci){
        cenaZaMJ=ceny->cenaZaJednotkuEEl;
    }
    else{
        cenaZaMJ=ceny->cenaZaJednotkuE;
    }
    for (i=1; i<=posledniNenulovy(maxDelkaCesty,vlak->cesta); i++){
        cenaCelkem += cenaZaMJ*energieNaPrujezdTrate(&tratZDo[vlak->cesta[i-1]-1][vlak->cesta[i]-1],vlak); //[MJ]
    }
    cenaCelkem += (casPrace*(vlak->pocetPruvodcich*ceny->cenaPracePruvodcich+ceny->cenaPraceStrojvedoucich));
    return cenaCelkem;
}

void zobrazProfilTrate(parTrate *trat){ //Pozor také je toto integrované ve funkci postavitTrat
    float maximum = trat->stoupani[0]+trat->klesani[0];
    int i,j;
    float buff;
    for (i=1;i<=pocetJednotekSklonu;i++){
        if (maximum<trat->stoupani[i]){
            maximum = trat->stoupani[i];
        }
        if (maximum<trat->klesani[i]){
            maximum = trat->klesani[i];
        }
    }
    printf("\n");
    printf("         Délka tratě při daném sklonu\n");
    //printf("         Délka tratě při daném sklonu %.1f km\n",maximum);
    printf("Sklon  |                              |\n");
    for (i=pocetJednotekSklonu;i>0;i--){
        buff = ((float)maximalniSklon*(float)i/(float)pocetJednotekSklonu);
        if (buff<10) printf(" -%.1f‰ |",buff);
        else printf("-%.1f‰ |",buff);
        for(j=0;j<(int)(30*(float)trat->klesani[i]/maximum);j++){
            printf("=");
        }
        for(j=0;j<30-(int)(30*(float)trat->klesani[i]/maximum);j++){
            printf(" ");
        }
        printf("|\n");
    }
    printf("  0‰   |");
    for(j=0;j<(int)(30*(float)(trat->klesani[0]+trat->stoupani[0])/maximum);j++){
            printf("=");
        }
    for(j=0;j<30-(int)(30*(float)(trat->klesani[0]+trat->stoupani[0])/maximum);j++){
        printf(" ");
    }
    printf("|\n");
    for (i=1;i<=pocetJednotekSklonu;i++){
        buff = (int)((float)maximalniSklon*(float)i/(float)pocetJednotekSklonu);
        if (buff<10) printf("  %.1f‰ |",buff);
        else printf(" %.1f‰ |",buff);
        for(j=0;j<(int)(30*(float)trat->stoupani[i]/maximum);j++){
            printf("=");
        }
        for(j=0;j<30-(int)(30*(float)trat->stoupani[i]/maximum);j++){
            printf(" ");
        }
        printf("|\n");
    }
    printf("       0 km                           %.1f km\n",maximum);
    printf("\n");
}

void kontrolaVyberu(int *vstup, int maxvstup){ //Zkontroluje zda int vstup je větší než 0 a zároven menší nebo roven maxvstup a pokud ne, tak požádá o znovuzadání hodnoty a přepíše původní.
    if ((maxvstup == 7)&&(*vstup==17)){
        printf("Tuto hru vytvořil Emitter.\nDoufám, že si hraní užíváte a cením si, že Váš životní čas věnujete právě mé hře:)\n");
        printf("\n                                                        \\  /\n                  __                                     \\/\n     _   ---===##===---_________________________--------------  _\n    [ ~~~=================###=###=###=###=###=================~~ ]\n    /  ||  | |~\\  ;;;;            ;;;            ;;;;  /~| |  ||  \\\n   /___||__| |  \\ ;;;;            [_]            ;;;; /  | |__||___\\\n   [\\        |__| ;;;;  ;;;; ;;;; ;;; ;;;; ;;;;  ;;;; |__|        /]\n  (=|    ____[-]_______________________________________[-]____    |=)\n  /  /___/|#(__)=o########o=(__)#||___|#(__)=o#########o=(__)#|\\___\\\n _________-=\\__/=--=\\__/=--=\\__/=-_____-=\\__/=--=\\__/=--=\\__/=-______\n\n");
    }
    while (*vstup > maxvstup || *vstup < 1){
        printf("Vstupní hodnota nenabývá platné hodnoty. Prosím zadejte novou hodnotu.\n");
        skenfInt(vstup);
    }
    return;
}

void doplnSurTrate(surParTrate tratZDoVar[pocetStanic][pocetStanic][pocetVarTrate]){ //Doplní tratě na idexech i < j. Asi by šla udělat i efektivněji
    unsigned char i,j,k,v;
//    for (j=0;j<pocetStanic;j++){ //Pro všechny indexy i<j
//        for (i=0;i<j;i++){
//            for (v=0;v<pocetVarTrate;v++){
//                for (k=0;k<=pocetJednotekSklonu;k++){
//                    tratZDoVar[j][i][v].stoupani[k] = tratZDoVar[i][j][v].klesani[k];
//                    tratZDoVar[j][i][v].klesani[k] = tratZDoVar[i][j][v].stoupani[k];
//                }
//                tratZDoVar[i][j][v].delka = 0;
//                tratZDoVar[j][i][v].delka = 0;
//                for (k=0;k<=pocetJednotekSklonu;k++){
//                    tratZDoVar[i][j][v].delka += tratZDoVar[i][j][v].stoupani[k] + tratZDoVar[i][j][v].klesani[k];
//                    tratZDoVar[j][i][v].delka += tratZDoVar[j][i][v].stoupani[k] + tratZDoVar[j][i][v].klesani[k];
//                }
//                tratZDoVar[j][i][v].koefDoby = tratZDoVar[i][j][v].koefDoby;
//                tratZDoVar[j][i][v].odporZatacek = tratZDoVar[i][j][v].odporZatacek;
//                tratZDoVar[j][i][v].cenaStavby = tratZDoVar[i][j][v].cenaStavby;
//            }
//        }
//    }
    for (i=0;i<pocetStanic;i++){ //Pro indexy s nenulovou cenou
        for (j=0;j<pocetStanic;j++){
            if (i!=j){
                for (v=0;v<pocetVarTrate;v++){
                    if (tratZDoVar[i][j][v].cenaStavby){
                        for (k=0;k<=pocetJednotekSklonu;k++){
                            tratZDoVar[j][i][v].stoupani[k] = tratZDoVar[i][j][v].klesani[k];
                            tratZDoVar[j][i][v].klesani[k] = tratZDoVar[i][j][v].stoupani[k];
                        }
                        tratZDoVar[i][j][v].delka = 0;
                        tratZDoVar[j][i][v].delka = 0;
                        for (k=0;k<=pocetJednotekSklonu;k++){
                            tratZDoVar[i][j][v].delka += tratZDoVar[i][j][v].stoupani[k] + tratZDoVar[i][j][v].klesani[k];
                            tratZDoVar[j][i][v].delka += tratZDoVar[j][i][v].stoupani[k] + tratZDoVar[j][i][v].klesani[k];
                        }
                        tratZDoVar[j][i][v].koefDoby = tratZDoVar[i][j][v].koefDoby;
                        tratZDoVar[j][i][v].nasobekZrychleni = tratZDoVar[i][j][v].nasobekZrychleni;
                        tratZDoVar[j][i][v].cenaStavby = tratZDoVar[i][j][v].cenaStavby;
                    }
                }
            }
        }
    }
    return;
}

void doplnTratZDo(parTrate tratZDo[pocetStanic][pocetStanic], unsigned char i, unsigned char j){ //Podle vyplněné tratě z i do j doplní trať z j do i
    unsigned int k;
    for (k=0;k<=pocetJednotekSklonu;k++){
        tratZDo[j][i].stoupani[k] = tratZDo[i][j].klesani[k];
        tratZDo[j][i].klesani[k] = tratZDo[i][j].stoupani[k];
    }
    tratZDo[j][i].delka = tratZDo[i][j].delka;
    tratZDo[j][i].koefDoby = tratZDo[i][j].koefDoby;
    tratZDo[j][i].pocetKoleji = tratZDo[i][j].pocetKoleji;
    tratZDo[j][i].elektrifikovano = tratZDo[i][j].elektrifikovano;
    tratZDo[j][i].nasobekZrychleni = tratZDo[i][j].nasobekZrychleni;
    return;
}

void zadejCas(unsigned int *ukCas){ //Požádá uživatele o zadání času a uloží jej na adresu ukCas v jednotkách času
    int p;
    *ukCas = 0;
    printf("Zadejte hodinu ve formátu celého čísla od 0 do 23.\n");
    skenfInt(&p);
    kontrolaVyberuS0(&p,23);
    *ukCas +=60*p;
    printf("Zadejte minuty ve formátu celého čísla od 0 do 59. Jednotka času je %d minut a proto jakékoli jiné hodnoty budou zaokrouhleny na násobek %d.\n",minutVJednotceCasu, minutVJednotceCasu);
    skenfInt(&p);
    kontrolaVyberuS0(&p,59);
    p = (int)((((float)p/minutVJednotceCasu))*minutVJednotceCasu);
    *ukCas = (*ukCas + p)/minutVJednotceCasu;
    return;
}
 
 void zobrazCas(unsigned int cas){
     unsigned int i;
     cas *= minutVJednotceCasu;
     i = cas/60;
     if (i<10) printf("0");
     printf("%u",i);
     printf(":");
     i = cas%60;
     if (i<10) printf("0");
     printf("%d",i);
     return;
 }
 
void zobrazStanici(unsigned int index){ //Já vím, že toto je nic moc...
    index++;
    switch(index){
        case(1): printf("<Stanice 1>"); break;
        case(2): printf("<Stanice 1>"); break;
        case(3): printf("<Stanice 1>"); break;
        case(4): printf("<Stanice 1>"); break;
        case(5): printf("<Stanice 1>"); break;
    }
    return;
}

//char jeTratObsazenaOdDo(parTrate tratZDo[pocetStanic][pocetStanic], parVlaku vlaky[maxVlaku], unsigned char zeStanice, unsigned char doStanice,unsigned int odCasu, unsigned int doCasu){
//   unsigned int pocetVlaku = 0, id, cas, i;
//   char b;
//   for (cas=0;cas<=doCasu;cas++){
//       for (i=0;i<maxVlaku;i++){
////           l = prvniOd(maxCas,vlaky[zeStanice][doStanice].staniceOdjezduVlaku,-1);
////           p = prvniOd(maxCas,vlaky[zeStanice][doStanice].stanicePrijezduVlaku,1);
////           s = vlaky[zeStanice][doStanice].staniceOdjezduVlaku+vlaky[zeStanice][doStanice].stanicePrijezduVlaku;
////           if (s){
////               
////           }
////           else{
////               if (l==zeStanice)
////           }
//       }
//   }
//}

void zobrazMapu(parTrate *tratZDo){   //Vím, že tato funkce není úplně ideální...                
    unsigned char i;
    char symboly[maxPocetTrati][3];
    unsigned char prevodZIdTrateNaIdZDo[7] = {0+pocetStanic*1,1+pocetStanic*2,1+pocetStanic*3,2+pocetStanic*4, 3+pocetStanic*4, 2+pocetStanic*5, 4+pocetStanic*5}; 
    for (i=0;i<maxPocetTrati;i++){
        if ((tratZDo+prevodZIdTrateNaIdZDo[i])->pocetKoleji){
            symboly[i][0] = '_';
            symboly[i][1] = '\\';
            symboly[i][2] = '/';
        }
        else{
            symboly[i][0] = '.';
            symboly[i][1] = '\'';
            symboly[i][2] = '.';            
        }
    }
    printf("Legenda:\n ___ - Postavená trať\n ... - Místo, kde lze postavit trať\n\n");
    printf("Poustevna"); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("                   Lhota"); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("%c",symboly[5][0]); printf("\n           "); printf("%c",symboly[0][1]); printf("                 "); printf("%c",symboly[1][2]); printf("     "); printf("%c",symboly[3][1]); printf("%c",symboly[3][0]); printf("%c",symboly[3][0]); printf("%c",symboly[3][0]); printf("%c",symboly[3][0]); printf("%c",symboly[3][0]); printf("            "); printf("%c",symboly[5][1]); printf("\n        "); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][2]); printf("                "); printf("%c",symboly[1][2]); printf("            "); printf("%c",symboly[3][1]); printf("            "); printf("%c",symboly[5][1]); printf("\n       "); printf("%c",symboly[0][2]); printf("                   "); printf("%c",symboly[1][2]); printf("              "); printf("%c",symboly[3][1]); printf("            Ves\n       "); printf("%c",symboly[0][1]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("        "); printf("%c",symboly[1][2]); printf("                "); printf("%c",symboly[3][1]); printf("           "); printf("%c",symboly[6][2]); printf("\n                  "); printf("%c",symboly[0][1]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("%c",symboly[0][0]); printf("Lípa                 "); printf("%c",symboly[3][1]); printf("%c",symboly[3][0]); printf("%c",symboly[3][0]); printf("%c",symboly[3][0]); printf("%c",symboly[3][0]); printf("     "); printf("%c",symboly[6][2]); printf("\n                           "); printf("%c",symboly[2][1]); printf("                     "); printf("%c",symboly[3][1]); printf("   "); printf("%c",symboly[6][2]); printf("\n                            "); printf("%c",symboly[2][1]); printf("%c",symboly[2][0]); printf("%c",symboly[2][0]); printf("%c",symboly[2][0]); printf("%c",symboly[2][0]); printf("Dvory"); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("%c",symboly[4][0]); printf("Újezd\n");
    return;
}

void vytvorJizdniRad(parVlaku *vlak, parTrate tratZDo[pocetStanic][pocetStanic]){
    unsigned int i, j, k, cas, aCas, casPrijezdu;
    int delkaCesty, vyber;
    unsigned char predchoziStanice,dalsiStanice;
    delkaCesty = posledniNenulovy(maxDelkaCesty,vlak->cesta); //Pozor, delkaCesty je o 1 kratší než skutečná délka
    if (delkaCesty){
       printf("Chcete opravdu přepsat jízdní řád vlaku?\n1) Ano\n2) Ne\n");
       skenfInt(&i);
       kontrolaVyberu(&i,2);
       if (i!=1){
           printf("Byla ponechána původní cesta i jízdní řád.\n");
           return;
       }
    }
    j = 0;
    for (i=0;i<delkaCesty;i++){ //Rušení obsazeností tratí
        while(1){
            if (vlak->staniceOdjezduVlaku[j]==vlak->cesta[i]){
                cas = j;
                break;
            }
            j++;
        }
        while(1){
            if (vlak->stanicePrijezduVlaku[j]==vlak->cesta[i+1]){
                casPrijezdu = j;
                break;
            }
            j++;
        }
        for (aCas = cas+1; aCas<=casPrijezdu;aCas++){
            tratZDo[vlak->cesta[i]-1][vlak->cesta[i+1]-1].obsazenost[aCas]--;
            tratZDo[vlak->cesta[i+1]-1][vlak->cesta[i]-1].obsazenost[aCas]--;
        }
    }
    zobrazMapu(tratZDo);
    printf("\n");
    for (i=1;i<pocetStanic;i++){
        printf("%u) %s, ",i,jmenaStanic[i-1]);
    }
    printf("%d) %s\n",pocetStanic,jmenaStanic[pocetStanic-1]);
    delkaCesty = -1; //Pozor, delkaCesty je o 1 kratší než skutečná délka
    while(1){
        if (delkaCesty==maxDelkaCesty){
            printf("Maximální délka cesty byla překročena. Vytvořte jízdní řád znovu.\n");
            for (i=0;i<maxDelkaCesty;i++){
                vlak->cesta[i] = 0;
            }
            return;
        }
        delkaCesty++;
        printf("Zadejte index ");
        switch(delkaCesty+1){
            case 1: printf("první"); break;
            case 2: printf("druhé"); break;
            case 3: printf("třetí"); break;
            default: printf("%d-té", delkaCesty+1); break;
        }
        printf(" stanice ve které má vlak zastavit");
        if (delkaCesty == 0){
            printf(" a nebo zadejte 0 pro odstavení vlaku.\n");
        }
        else{
            if (delkaCesty>2) {
                printf(" a nebo zadejte 0 pro ukončení.\n");
            }
            else{
                printf(".\n");
            }
        }
        skenfInt(&j);
        kontrolaVyberuS0(&j,pocetStanic);
        dalsiStanice = (unsigned char)j;
        if (j==0){
            if (delkaCesty==0){
                printf("Vlak byl odstaven.\n");
                for (i=0;i<=maxDelkaCesty;i++){
                    vlak->cesta[i] = 0;
                }
                return;
                
            }
            delkaCesty--;
            if (delkaCesty>1){
                if (vlak->cesta[0]!=vlak->cesta[delkaCesty]){
                    printf("Vlak musí začínat a končit ve stejné stanici.\n");
                    continue;
                }
                for (i=delkaCesty+2;i<maxDelkaCesty;i++){
                    vlak->cesta[i] = 0;
                }
                break;
            }
            else{
                printf("Nebyl zadán platný index stanice.\n");
            }
        }
        else{
            dalsiStanice--;
            if (delkaCesty>0){ //Kontrola zda je trať postavená
                if (!(tratZDo[predchoziStanice][dalsiStanice].pocetKoleji)){
                    delkaCesty--;
                    if(predchoziStanice==dalsiStanice){
                        printf("Vlak již je ve stanici %s.\n1) Pokračovat v zadávání.\n2) Zrušit cestu.\n",jmenaStanic[predchoziStanice]);
                    }
                    else{
                        printf("Mezi stanicemi %s a %s nevede žádná trať.\n1) Pokračovat v zadávání.\n2) Zrušit cestu.\n",jmenaStanic[predchoziStanice],jmenaStanic[dalsiStanice]);
                    }
                    skenfInt(&vyber);
                    kontrolaVyberu(&vyber,2);
                    switch(vyber){
                        case(1): continue;
                        case(2):
                            for (i=0;i<maxDelkaCesty;i++){
                                vlak->cesta[i] = 0;
                            }
                            return;
                    }
                }
                if ((vlak->potrebujeElektrifikaci)&&(!(tratZDo[predchoziStanice][dalsiStanice].elektrifikovano))){
                    delkaCesty--;
                    printf("Mezi stanicemi %s a %s není trať elektrifikovaná.\n1) Pokračovat v zadávání.\n2) Zrušit cestu.\n",jmenaStanic[predchoziStanice],jmenaStanic[dalsiStanice]);
                    skenfInt(&vyber);
                    kontrolaVyberu(&vyber,2);
                    switch(vyber){
                        case(1): continue;
                        case(2):
                            for (i=0;i<maxDelkaCesty;i++){
                                vlak->cesta[i] = 0;
                            }
                            return;
                    }
                }
            }
            vlak->cesta[delkaCesty]=dalsiStanice+1;
            predchoziStanice = dalsiStanice;
        }
    }
    delkaCesty++; //Teď je délka zas normální
    jizdniRad:
    for (cas=0;cas<maxCas;cas++){ //Vynulování časů stanic do kterých vlak v časech příjezdů a odjezdů přijede/odjede
        vlak->staniceOdjezduVlaku[cas] = 0;
        vlak->stanicePrijezduVlaku[cas] = 0;
    }
    casPrijezdu = 0;
    for (i=0;i<delkaCesty-1;i++){
        divnyCyklus:
            printf("Zadejte čas odjezdu ze stanice %s.\n",jmenaStanic[vlak->cesta[i]-1]);
            zadejCas(&cas);
            if (cas<casPrijezdu){
                printf("Vlak musí odjet až po příjezdu do předchozí stanice.\n");
                goto divnyCyklus; //Vím, že toto by šlo nahradit while a continue, důvod k tomuto netradičnímu řešení je až ve for cyklu níže
            }
            vlak->staniceOdjezduVlaku[cas]=vlak->cesta[i];
            casPrijezdu = cas + (int)(dobaPrujezduTrate(&tratZDo[vlak->cesta[i]-1][vlak->cesta[i+1]-1],vlak)/minutVJednotceCasu)+1; //Je tu +1 kvůli zaokrouhlení nahoru
            if (casPrijezdu>=maxCas){
                printf("Vlak nestihne do půlnoci do zvolené stanice dojet.\n");
                j = 0;
                for (k=0;k<i;k++){ //Rušení obsazeností tratí
                    while(1){
                        if (vlak->staniceOdjezduVlaku[j]==vlak->cesta[k]){
                            cas = j;
                            break;
                        }
                        j++;
                    }
                    while(1){
                        if (vlak->stanicePrijezduVlaku[j]==vlak->cesta[k+1]){
                            casPrijezdu = j;
                            break;
                        }
                        j++;
                    }
                    for (aCas = cas+1; aCas<=casPrijezdu;aCas++){
                        tratZDo[vlak->cesta[k]-1][vlak->cesta[k+1]-1].obsazenost[aCas]--;
                        tratZDo[vlak->cesta[k+1]-1][vlak->cesta[k]-1].obsazenost[aCas]--;
                    }
                }
                printf("Pokud chcete časy zadat znovu, tak zadejte 1 a pokud chcete vlak odstavit a vrátit se do menu, tak zadejte 0.\n");
                skenfInt(&vyber);
                kontrolaVyberuS0(&vyber,1);
                if (!(vyber)){
                    printf("Vlak byl odstaven.\n");
                    for (i=0;i<=maxDelkaCesty;i++){
                        vlak->cesta[i] = 0;
                    }
                    return;
                }
                goto jizdniRad; //Vím, že je to sus, ale zde mě nenapadlo nic lepšího
            }
            for (aCas = cas+1; aCas<=casPrijezdu;aCas++){
                if (tratZDo[vlak->cesta[i]-1][vlak->cesta[i+1]-1].obsazenost[aCas]==tratZDo[vlak->cesta[i]-1][vlak->cesta[i+1]-1].pocetKoleji){
                    printf("V daný čas je trať plně obsazená.\n");
                    goto divnyCyklus; //Vím, že je to sus, ale zde mě nenapadlo nic lepšího
                }
            }
        for (aCas = cas+1; aCas<=casPrijezdu;aCas++){
            tratZDo[vlak->cesta[i]-1][vlak->cesta[i+1]-1].obsazenost[aCas]++;
            tratZDo[vlak->cesta[i+1]-1][vlak->cesta[i]-1].obsazenost[aCas]++;
        }
        vlak->stanicePrijezduVlaku[casPrijezdu]=vlak->cesta[i+1];
        printf("Vlak do stanice %s přijede v ", jmenaStanic[vlak->cesta[i+1]-1]); zobrazCas(casPrijezdu); printf(".\n");
    }
    printf("Jízdní řád vlaku byl dokončen.\n");
    return;
}

void zobrazJizdniRad(parVlaku *vlak){
    unsigned int i = 1;
    printf("Postupná cesta vlaku je:\n");
    printf("%s",jmenaStanic[vlak->cesta[0]-1]);
    for(i=1;i<maxDelkaCesty;i++){
        if (!(vlak->cesta[i])) break;
        printf(" -> %s",jmenaStanic[vlak->cesta[i]-1]);
    }
    printf("\nČasy příjezdů a odjezdů vlaku jsou:\n");
    for (i=0;i<maxCas;i++){
        if (vlak->stanicePrijezduVlaku[i]){
            printf("    "); zobrazCas(i); printf(" - příjezd do stanice %s.\n",jmenaStanic[vlak->stanicePrijezduVlaku[i]-1]);
        }
        if (vlak->staniceOdjezduVlaku[i]){
            printf("    "); zobrazCas(i); printf(" - odjezd ze stanice %s.\n",jmenaStanic[vlak->staniceOdjezduVlaku[i]-1]);
        }
    }
    return;
}

parVlaku* koupitVlak(unsigned int pocetVlaku, parVlaku *ukVlaky, stavPenez *penize, parVlaku *ukObchod){
    parVlaku *ukNoveVlaky, *ukVlak;
    int i, vyber;
    printf("Zvolte vlak k zakoupení a nebo zadejte 0 pro návrat do menu.\n");
    for (i=0;i<pocetVlakuVObchodu;i++){
        ukVlak = ukObchod+i;
        printf("%d) %s\n",i+1,ukVlak->nazev);
        printf("    Maximální rychlost %.1f km/h\n", ukVlak->maxRychlost);
        printf("    Výkon %.1f kW\n", ukVlak->maxVykon);
        printf("    Hmotnost %.1f t\n", ukVlak->hmotnost);
        printf("    Kapacita %hu cestujících\n", ukVlak->kapacita);
        if (ukVlak->potrebujeElektrifikaci) printf("    Vlak je elektrický\n");
        if (ukVlak->efektivitaRekuperace) printf("    Vlak je schopen rekuperace (na %hhi%%)\n", ukVlak->efektivitaRekuperace);
        switch(ukVlak->pocetPruvodcich){
            case(0): printf("    Tento vlak nepotřebuje k provozu žádného průvodčího.\n"); break;
            case(1): printf("    Tento vlak potřebuje k provozu jednoho průvodčího.\n"); break;
            case(2): printf("    Tento vlak potřebuje k provozu dva průvodčí.\n"); break;
            case(3): printf("    Tento vlak potřebuje k provozu tři průvodčí.\n"); break;
            case(4): printf("    Tento vlak potřebuje k provozu čtyři průvodčí.\n"); break;
            default: printf("    Tento vlak potřebuje k provozu %d průvodčích.\n", ukVlak->pocetPruvodcich); break;
        }
        printf("    Cena %.1f0 korun\n", ukVlak->porizovaciCena);
    }
    skenfInt(&vyber);
    kontrolaVyberuS0(&vyber,pocetVlakuVObchodu);
    if (!(vyber)) return ukVlaky;
    vyber--;
    parVlaku *ukKoupenyVlak = (ukObchod+vyber);
    penize->hlavniUcet -= ukKoupenyVlak->porizovaciCena;
    ukNoveVlaky = calloc(sizeof(parVlaku),pocetVlaku+1);
    for (i=0;i<pocetVlaku;i++){
        *(ukNoveVlaky+i) = *(ukVlaky+i);
    }
    ukVlak = (ukNoveVlaky+pocetVlaku);
    strcpy(ukVlak->nazev, ukKoupenyVlak->nazev);
    ukVlak->valivyOdpor = ukKoupenyVlak->valivyOdpor;
    ukVlak->hmotnost = ukKoupenyVlak->hmotnost;
    ukVlak->porizovaciCena = ukKoupenyVlak->porizovaciCena;
    for (i=0;i<maxDelkaCesty;i++){
        ukVlak->cesta[i] = 0;
    }
    ukVlak->pocetPruvodcich = ukKoupenyVlak->pocetPruvodcich;
    ukVlak->maxVykon = ukKoupenyVlak->maxVykon;
    ukVlak->maxRychlost = ukKoupenyVlak->maxRychlost;
    ukVlak->kapacita = ukKoupenyVlak->kapacita;
    ukVlak->efektivitaRekuperace = ukKoupenyVlak->efektivitaRekuperace;
    ukVlak->potrebujeElektrifikaci = ukKoupenyVlak->potrebujeElektrifikaci;
    if (pocetVlaku) free(ukVlaky);
    printf("Vlak byl zakoupen.\n");
    return(ukNoveVlaky);
}

void postavitTrat(parTrate tratZDo[pocetStanic][pocetStanic], surParTrate postavitelneZDoVar[pocetStanic][pocetStanic][pocetVarTrate], stavPenez *penize, hodnotyCen *ceny){
    unsigned int i, j;
    int vyber1, vyber2, vyber3;
    unsigned char pomPole[pocetStanic];
    printf("Vyberte, ze které stanice má nová trať vést a nebo zadejte 0 pro návrat do menu.\n");
    for (i=1;i<=pocetStanic;i++){
        printf("%u) %s\n", i, jmenaStanic[i-1]);
    }
    skenfInt(&vyber1);
    kontrolaVyberuS0(&vyber1, pocetStanic);
    if (!(vyber1)) return;
    vyber1--;
    printf("Vyberte, do které stanice má nová trať vést a nebo zadejte 0 pro návrat do menu.\n");
    j=0;
    for (i=1;i<=pocetStanic;i++){
        if (postavitelneZDoVar[vyber1][i-1][0].cenaStavby){
            printf("%u) %s\n", j+1, jmenaStanic[i-1]);
            pomPole[j] = i-1;
            j++;
        }
    }
    skenfInt(&vyber2);
    kontrolaVyberuS0(&vyber2, j);
    if (!(vyber2)) return;
    vyber2 = pomPole[vyber2-1];
    if(tratZDo[vyber1][vyber2].pocetKoleji){
        if (tratZDo[vyber1][vyber2].pocetKoleji == maxPocetKoleji){
            printf("Na této trati již bylo postavené maximální množství kolejí.\n");
            return;
        }
        penize->hlavniUcet -= ceny->cenaZaDostavbuKmKoleje*tratZDo[vyber1][vyber2].delka;
        tratZDo[vyber1][vyber2].pocetKoleji++;
        tratZDo[vyber2][vyber1].pocetKoleji++;
        printf("K trati ze stanice %s do stanice %s byla postavena dodatečná kolej. Nyní na této trati ved", jmenaStanic[vyber1], jmenaStanic[vyber2]);
        switch(tratZDo[vyber1][vyber2].pocetKoleji){
            case(2): printf("ou dvě koleje.\n"); return;
            case(3): printf("ou tři koleje.\n"); return;
            case(4): printf("ou čtyři koleje.\n"); return;
            default: printf("e %hhu kolejí.\n", tratZDo[vyber1][vyber2].pocetKoleji); return;
        }
    }
    else{
        printf("Zvote, kterou variantu chcete postavit a nebo zadejte 0 pro návrat do menu.\n");
        unsigned char pocet = 0;
        float maximum;
        int k,j;
        float buff;
        for (i=0;i<pocetVarTrate;i++){
            if(postavitelneZDoVar[vyber1][vyber2][i].cenaStavby){
                pocet++;
                printf("%u)\n", i+1);
                printf("    Cena %.1f0 korun\n   Délka %.1f Km\n K průjezdu je potřeba %.1f-násobek teoretického minimálního zrychlení\n Průjezd zabere %.1f-krát delší dobu nežli teoretické minimum\n",postavitelneZDoVar[vyber1][vyber2][i].cenaStavby, postavitelneZDoVar[vyber1][vyber2][i].delka, postavitelneZDoVar[vyber1][vyber2][i].nasobekZrychleni, postavitelneZDoVar[vyber1][vyber2][i].koefDoby );
                //Níže je prakticky funkce zobrazProfilTrate
                maximum = postavitelneZDoVar[vyber1][vyber2][i].stoupani[0]+postavitelneZDoVar[vyber1][vyber2][i].klesani[0];
                for (k=1;k<=pocetJednotekSklonu;k++){
                    if (maximum<postavitelneZDoVar[vyber1][vyber2][i].stoupani[k]){
                        maximum = postavitelneZDoVar[vyber1][vyber2][i].stoupani[k];
                    }
                    if (maximum<postavitelneZDoVar[vyber1][vyber2][i].klesani[k]){
                        maximum = postavitelneZDoVar[vyber1][vyber2][i].klesani[k];
                    }
                }
                printf("\n");
                printf("         Délka tratě při daném sklonu\n");
                printf("Sklon  |                              |\n");
                for (k=pocetJednotekSklonu;k>0;k--){
                    buff = ((float)maximalniSklon*(float)k/(float)pocetJednotekSklonu);
                    if (buff<10) printf(" -%.1f‰ |",buff);
                    else printf("-%.1f‰ |",buff);
                    for(j=0;j<(int)(30*(float)postavitelneZDoVar[vyber1][vyber2][i].klesani[k]/maximum);j++){
                        printf("=");
                    }
                    for(j=0;j<30-(int)(30*(float)postavitelneZDoVar[vyber1][vyber2][i].klesani[k]/maximum);j++){
                        printf(" ");
                    }
                    printf("|\n");
                }
                printf("  0‰   |");
                for(j=0;j<(int)(30*(float)(postavitelneZDoVar[vyber1][vyber2][i].klesani[0]+postavitelneZDoVar[vyber1][vyber2][i].stoupani[0])/maximum);j++){
                        printf("=");
                    }
                for(j=0;j<30-(int)(30*(float)(postavitelneZDoVar[vyber1][vyber2][i].klesani[0]+postavitelneZDoVar[vyber1][vyber2][i].stoupani[0])/maximum);j++){
                    printf(" ");
                }
                printf("|\n");
                for (k=1;k<=pocetJednotekSklonu;k++){
                    buff = (int)((float)maximalniSklon*(float)k/(float)pocetJednotekSklonu);
                    if (buff<10) printf("  %.1f‰ |",buff);
                    else printf(" %.1f‰ |",buff);
                    for(j=0;j<(int)(30*(float)postavitelneZDoVar[vyber1][vyber2][i].stoupani[k]/maximum);j++){
                        printf("=");
                    }
                    for(j=0;j<30-(int)(30*(float)postavitelneZDoVar[vyber1][vyber2][i].stoupani[k]/maximum);j++){
                        printf(" ");
                    }
                    printf("|\n");
                }
                printf("       0 km                           %.1f km\n",maximum);
                printf("\n");

            }
        }
        skenfInt(&vyber3);
        kontrolaVyberuS0(&vyber3, pocet);
        if (!(vyber3)) return;
        vyber3--;
        penize->hlavniUcet -= postavitelneZDoVar[vyber1][vyber2][vyber3].cenaStavby;
        for (k=0;k<=pocetJednotekSklonu;k++){
            tratZDo[vyber1][vyber2].stoupani[k] = postavitelneZDoVar[vyber1][vyber2][vyber3].stoupani[k];
            tratZDo[vyber1][vyber2].klesani[k] = postavitelneZDoVar[vyber1][vyber2][vyber3].klesani[k];
        }
        tratZDo[vyber1][vyber2].delka = postavitelneZDoVar[vyber1][vyber2][vyber3].delka;
        tratZDo[vyber1][vyber2].koefDoby = postavitelneZDoVar[vyber1][vyber2][vyber3].koefDoby;
        tratZDo[vyber1][vyber2].pocetKoleji = 1;
        tratZDo[vyber1][vyber2].elektrifikovano = 0;
        tratZDo[vyber1][vyber2].nasobekZrychleni = postavitelneZDoVar[vyber1][vyber2][vyber3].nasobekZrychleni;
        doplnTratZDo(tratZDo,vyber1,vyber2);
        printf("Trať ze stanice %s do stanice %s byla postavena.\n", jmenaStanic[vyber1], jmenaStanic[vyber2]);
    }
}

void elektrifikujTrat(parTrate tratZDo[pocetStanic][pocetStanic],unsigned char zeS, unsigned char doS, stavPenez *penize, hodnotyCen *ceny){
    //Pozor, není hlídáno, zda je trať skutečne neelektrifikovaná
    int vyber;
    float cena;
    cena = ceny->cenaZaElKmKoleje*tratZDo[zeS][doS].delka*tratZDo[zeS][doS].pocetKoleji;
    printf("Náklady na elektrifikaci jsou %.1f0 korun, pokud si přejete pokračovat, tak zadejte 1 a nebo pro návrat do menu zadejte 0.\n",cena);
    skenfInt(&vyber);
    kontrolaVyberuS0(&vyber,1);
    if (!(vyber)) return;
    penize->hlavniUcet -= cena;
    tratZDo[zeS][doS].elektrifikovano = !tratZDo[zeS][doS].elektrifikovano;
    tratZDo[doS][zeS].elektrifikovano = !tratZDo[doS][zeS].elektrifikovano;
    printf("Trať byla elektrifikována.\n");
    return;
}

void zrusElektrifikaciTrate(parTrate tratZDo[pocetStanic][pocetStanic],unsigned char zeS, unsigned char doS, stavPenez *penize, hodnotyCen *ceny){
    int vyber; //Pozor, není hlídáno, zda je trať skutečne elektrifikovaná
    float cena; //Pozor, pozitivní cena znamená příjem a negativní znamená náklady
    cena = ceny->cenaZaElKmKoleje*tratZDo[zeS][doS].delka*tratZDo[zeS][doS].pocetKoleji*koefCenyZruseniElektrifikace;
    if (cena<0){
        printf("Náklady na zrušení elektrifikace tratě jsou %.1f0 korun, pokud si přejete pokračovat, tak zadejte 1 a nebo pro návrat do menu zadejte 0.\n", -cena);
    }
    else{
        printf("Za zrušení elektrifikace tratě získáte %.1f0 korun, pokud si přejete pokračovat, tak zadejte 1 a nebo pro návrat do menu zadejte 0.\n", cena);
    }
    skenfInt(&vyber);
    kontrolaVyberuS0(&vyber,1);
    if (!(vyber)) return;
    penize->hlavniUcet += cena;
    tratZDo[zeS][doS].elektrifikovano = !tratZDo[zeS][doS].elektrifikovano;
    tratZDo[doS][zeS].elektrifikovano = !tratZDo[doS][zeS].elektrifikovano;
    printf("Elektrifikace tratě byla zrušena.\n");
    return;
}

void zrusTrat(parTrate tratZDo[pocetStanic][pocetStanic],unsigned char zeS, unsigned char doS, stavPenez *penize, hodnotyCen *ceny){
    int vyber;
    float cena; //Pozor, pozitivní cena znamená příjem a negativní znamená náklady
    cena = (ceny->cenaZaDostavbuKmKoleje*koefCenyZruseniKeStavbe+tratZDo[zeS][doS].elektrifikovano*ceny->cenaZaElKmKoleje*koefCenyZruseniElektrifikace)*tratZDo[zeS][doS].delka*tratZDo[zeS][doS].pocetKoleji;
    if (cena<0){
        printf("Náklady na zrušení tratě jsou %.1f0 korun, pokud si přejete pokračovat, tak zadejte 1 a nebo pro návrat do menu zadejte 0.\n", -cena);
    }
    else{
        printf("Za zrušení tratě získáte %.1f0 korun, pokud si přejete pokračovat, tak zadejte 1 a nebo pro návrat do menu zadejte 0.\n", cena);
    }
    skenfInt(&vyber);
    kontrolaVyberuS0(&vyber,1);
    if (!(vyber)) return;
    for (vyber=0;vyber<maxCas;vyber++){
        if(tratZDo[zeS][doS].obsazenost[vyber]){
            printf("Tato trať je obsazená v "); zobrazCas(vyber); printf(" a proto ji nelze zrušit.\n");
            return;
        }
    }
    penize->hlavniUcet += cena;
    tratZDo[zeS][doS].pocetKoleji = 0;
    tratZDo[doS][zeS].pocetKoleji = 0;
    printf("Trať byla zrušena.\n");
    return;
}

void zobrazInfoOVlaku(parVlaku *ukVlak){
    printf("%s\n",ukVlak->nazev);
        printf("Maximální rychlost %.1f km/h\n", ukVlak->maxRychlost);
        printf("Výkon %.1f kW\n", ukVlak->maxVykon);
        printf("Hmotnost %.1f t\n", ukVlak->hmotnost);
        printf("Kapacita %d cestujících\n", ukVlak->kapacita);
        if (ukVlak->potrebujeElektrifikaci) printf("Vlak je elektrický\n");
        if (ukVlak->efektivitaRekuperace) printf("Vlak je schopen rekuperace (na %hhi%%)\n", ukVlak->efektivitaRekuperace);
        switch(ukVlak->pocetPruvodcich){
            case(0): printf("Tento vlak nepotřebuje k provozu žádného průvodčího.\n"); break;
            case(1): printf("Tento vlak potřebuje k provozu jednoho průvodčího.\n"); break;
            case(2): printf("Tento vlak potřebuje k provozu dva průvodčí.\n"); break;
            case(3): printf("Tento vlak potřebuje k provozu tři průvodčí.\n"); break;
            case(4): printf("Tento vlak potřebuje k provozu čtyři průvodčí.\n"); break;
            default: printf("Tento vlak potřebuje k provozu %d průvodčích.\n", ukVlak->pocetPruvodcich); break;
        }
        if (ukVlak->cesta[0]){
            zobrazJizdniRad(ukVlak);
        }
        else{
            printf("Vlak je odstaven v depu.\n");
        }
}

void zobrazInfoOTrati(parTrate *trat){
    unsigned char posledniObs;
    unsigned int i;
    printf("Délka tratě je %.1f Km a ",trat->delka);
    switch(trat->pocetKoleji){
        case(1): printf("je jednokolejná.\n"); break;
        case(2): printf("je dvojkolejná.\n"); break;
        case(3): printf("je trojkolejná.\n"); break;
        default: printf("skládá se z %hhu kolejí.\n",trat->pocetKoleji); break;
    }
    printf("Na této trati je potřeba %.1f-krát větší zrychlení než je teoretické minimum a vlak proto pojede %.1f-krát déle.\n",trat->nasobekZrychleni,trat->koefDoby);
    if (trat->elektrifikovano){
        printf("Tato trať je elektrifikovaná.\n");
    }
    else{
        printf("Tato trať není elektrifikovaná.\n");
    }
    printf("\n");
    for (i=0;i<maxCas;i++){
        if (trat->obsazenost[i]){
            posledniObs = trat->obsazenost[i];
            printf("Tato trať je obsazená od "); zobrazCas(i);
            for (;i<maxCas;i++){
                if (trat->obsazenost[i]!=posledniObs){
                    break;
                }
            }
            i--;
            printf(" do "); zobrazCas(i); printf(" a to ");
            switch(posledniObs){
                case(1): printf("jedním vlakem.\n"); break;
                case(2): printf("dvěma vlaky.\n"); break;
                case(3): printf("třemi vlaky.\n"); break;
                default: printf("%hhu vlaky.\n",posledniObs); break;
            }
        }
    }
    zobrazProfilTrate(trat);
    return;
}

void posunVCase(unsigned int oDni,unsigned char pocetVlaku,parVlaku vlaky[pocetVlaku],parTrate tratZDo[pocetStanic][pocetStanic],unsigned int krivkaZDo[pocetStanic][pocetStanic][maxCas],hodnotyCen *ceny, stavPenez *penize){
    if (!(oDni)) return;
    unsigned int den,cas,ubytek,cestujicichPrepraveno = 0,cestujicichNeprepraveno = 0;
    unsigned char i, j, k, sPr;
    unsigned short mistVeVlaku;
    char l;
    unsigned int staniceBezUsp[poJakeDobeVsichniOdejdou][pocetStanic][pocetStanic] = {{{}}};
    unsigned int (*stanice[poJakeDobeVsichniOdejdou])[pocetStanic][pocetStanic];
    unsigned int (*sPom)[pocetStanic][pocetStanic];
    unsigned short lidiDoStanice[pocetVlaku][pocetStanic]; //Aktuální počet lidí ve vlaku x směřujících do stanice y
    unsigned int pasKmDoStanice[pocetVlaku][pocetStanic]; //Aktuální počet pasažérkilometrů ve vlaku x směřujících do stanice y
    unsigned short pocetLidiVeVlaku[pocetVlaku]; //Aktuální počet lidí ve vlaku x
    float vydelek = 0,naklady = 0, zaUroky = 0;
    for (i=0;i<pocetVlaku;i++){
        for (j=0;j<pocetStanic;j++){
            lidiDoStanice[i][j] = 0;
            pasKmDoStanice[i][j] = 0;
        }
        pocetLidiVeVlaku[i] = 0;
    }
    for (i=0;i<poJakeDobeVsichniOdejdou;i++){
        stanice[i] = &staniceBezUsp[i];
    }
    for (cas=0;cas<maxCas;cas++){
        for (i=0;i<pocetStanic;i++){
            //Přidávání cestujících
            for (j=0;j<pocetStanic;j++){
                if (i!=j){
                    (*stanice[0])[i][j] += krivkaZDo[i][j][cas];
                    (*stanice[0])[i][j] += krivkaZDo[j][i][osa-cas];
                }
            }
        }
        //Příjezdy a odjezdy vlaků
        #define vlak (vlaky[i])
        for (i=0;i<pocetVlaku;i++){
            if (vlak.cesta[0]&&vlak.stanicePrijezduVlaku[cas]){
                vydelek += ceny->vydelekZaPasazerKilometr*pasKmDoStanice[i][vlak.stanicePrijezduVlaku[cas]-1];
                pasKmDoStanice[i][vlak.stanicePrijezduVlaku[cas]-1]=0;
                pocetLidiVeVlaku[i] -= lidiDoStanice[i][vlak.stanicePrijezduVlaku[cas]-1];
                cestujicichPrepraveno += lidiDoStanice[i][vlak.stanicePrijezduVlaku[cas]-1];
                lidiDoStanice[i][vlak.stanicePrijezduVlaku[cas]-1] = 0;
            }
        }
        for (i=0;i<pocetVlaku;i++){ //Přes všechny vlaky
            if (vlak.cesta[0]&&vlak.staniceOdjezduVlaku[cas]){
                mistVeVlaku = vlak.kapacita - pocetLidiVeVlaku[i];
                j = vlak.staniceOdjezduVlaku[cas]-1; //Index stanice, ze které vlak i odjíždí
                for(k=cas+1;k<maxCas;k++){  //Přes všechny pozdější časy
                    //Přistoupí všichni cestující, kteří jedou do stanic kam jede tento vlak, ale pokud se vlak ještě mezitím do této stanice předtím vrátí, tak nepřistoupí.
                    if (vlak.stanicePrijezduVlaku[k]){ //Pokud vlak i přijede do nějaké stanice v čas k
                        if (vlak.stanicePrijezduVlaku[k]==j+1) break; //Pokud se vlak v tento čas vrátí do stanice, ze které teď odjíždí, tak konec
                        sPr = vlak.stanicePrijezduVlaku[k]-1; //Index stanice, do které vlak i přijíždí
                        for (l=poJakeDobeVsichniOdejdou-1;l>=0;l--){ //Postupně nastupují cestující od těch co zde čekají nejdéle
                            if (mistVeVlaku>(*stanice[l])[j][sPr]){
                                mistVeVlaku -= (*stanice[l])[j][sPr];
                                lidiDoStanice[i][sPr] += (*stanice[l])[j][sPr];
                                pasKmDoStanice[i][sPr] += (*stanice[l])[j][sPr]*placeneKmZDo[j][sPr];
                                pocetLidiVeVlaku[i] += (*stanice[l])[j][sPr];
                                (*stanice[l])[j][sPr] = 0;
                            }
                            else{
                                lidiDoStanice[i][sPr] += mistVeVlaku;
                                pasKmDoStanice[i][sPr] += mistVeVlaku*placeneKmZDo[j][sPr];
                                pocetLidiVeVlaku[i] = vlak.kapacita;
                                (*stanice[l])[j][sPr] -= mistVeVlaku;
                                goto vlakPlny; //"velký" break přes tři for cykly
                            }
                        }
                    }
                }
            }
            vlakPlny: ;
        }
        #undef vlak
        //Odebírání cestujících, co čekali příliš dlouho
        for (i=0;i<pocetStanic;i++){
            for (j=0;j<pocetStanic;j++){
                if ((*stanice[poJakeDobeVsichniOdejdou-1])[i][j]){
                    cestujicichNeprepraveno += (*stanice[poJakeDobeVsichniOdejdou-1])[i][j];
                    (*stanice[poJakeDobeVsichniOdejdou-1])[i][j] = 0;
                }
            }
        }
        sPom = stanice[poJakeDobeVsichniOdejdou-1];
        for (i=poJakeDobeVsichniOdejdou-1;i>0;i--){
            stanice[i]=stanice[i-1];
        }
        stanice[0] = sPom;
    }
    //Výpočet nákladů
    for (i=0;i<pocetVlaku;i++){
        naklady += cenaZaDenVlaku(&vlaky[i],ceny,tratZDo);
    }
    for (i=0;i<pocetStanic-1;i++){
        for (j=i+1;j<pocetStanic;j++){
            if (tratZDo[i][j].pocetKoleji){
                naklady += cenaZaDenTrate(&tratZDo[i][j],ceny);
            }
        }
    }
    for (den=0;den<oDni;den++){
        penize->hlavniUcet -= naklady;
        penize->hlavniUcet += vydelek;
        if (penize->hlavniUcet<0){
            zaUroky -= (ceny->denniUrok-1)*penize->hlavniUcet;
            penize->hlavniUcet *= ceny->denniUrok;
        }
    }
    printf("Za toto období bylo\nPříjem byl %.1f0 korun\nNáklady byly %.1f0 korun\n",vydelek*(float)oDni,naklady*(float)oDni);
    if (zaUroky) printf("Na úrocích bylo zaplaceno %.1f\n",zaUroky);
    printf("A celková bilance je %.1f0 korun\nBylo přepraveno %.1f%% cestujících (celkem bylo vašimi vlaky přepraveno %u cestujících)\n",(vydelek-naklady)*(float)oDni-zaUroky,100*(float)cestujicichPrepraveno/(float)(cestujicichNeprepraveno+cestujicichPrepraveno),cestujicichPrepraveno*oDni);
    return;
}


void pruzkumZDo(unsigned char zeS,unsigned char doS ,unsigned char pocetVlaku,parVlaku vlaky[pocetVlaku],parTrate tratZDo[pocetStanic][pocetStanic],unsigned int krivkaZDo[pocetStanic][pocetStanic][maxCas]){
    unsigned int den,cas,ubytek,pomocna,cestujicichPrepraveno = 0,cestujicichNeprepraveno = 0;
    unsigned char i, j, k, sPr;
    unsigned short mistVeVlaku;
    char l;
    unsigned int staniceBezUsp[poJakeDobeVsichniOdejdou][pocetStanic][pocetStanic] = {{{0}}};
    unsigned int (*stanice[poJakeDobeVsichniOdejdou])[pocetStanic][pocetStanic];
    unsigned int (*sPom)[pocetStanic][pocetStanic];
    unsigned short lidiDoStanice[pocetVlaku][pocetStanic]; //Aktuální počet lidí ve vlaku x směřujících do stanice y
    unsigned short pocetLidiVeVlaku[pocetVlaku]; //Aktuální počet lidí ve vlaku x
    for (i=0;i<poJakeDobeVsichniOdejdou;i++){
        stanice[i] = &staniceBezUsp[i];
    }
    for (i=0;i<pocetVlaku;i++){
        for (j=0;j<pocetStanic;j++){
            lidiDoStanice[i][j] = 0;
        }
        pocetLidiVeVlaku[i] = 0;
    }
    printf("Cestujících    ve stanici čekalo    z nich bylo přepraveno    nechtělo déle čekat a naštvaně odešlo\n");
    for (cas=0;cas<maxCas;cas++){
        zobrazCas(cas); printf(" | ");
        for (i=0;i<pocetStanic;i++){
            //Přidávání cestujících
            for (j=0;j<pocetStanic;j++){
                if (i!=j){
                    (*stanice[0])[i][j] += krivkaZDo[i][j][cas];
                    (*stanice[0])[i][j] += krivkaZDo[j][i][osa-cas];
                }
            }
        }
        pomocna = 0;
        for (i=0;i<poJakeDobeVsichniOdejdou;i++){
            pomocna += (*stanice[i])[zeS][doS];
        }
        if (pomocna){
            printf("       %u",pomocna);
            pomocna = 19 - (int)log10((float)pomocna);
            for (i=0;i<=pomocna;i++) printf(" ");
            pomocna = 0;
        }
        else{
            printf("                     ");
        }
        //Příjezdy a odjezdy vlaků
        #define vlak (vlaky[i])
        for (i=0;i<pocetVlaku;i++){
            if (vlak.cesta[0]&&vlak.stanicePrijezduVlaku[cas]){
                pocetLidiVeVlaku[i] -= lidiDoStanice[i][vlak.stanicePrijezduVlaku[cas]-1];
                lidiDoStanice[i][vlak.stanicePrijezduVlaku[cas]-1] = 0;
            }
        }
        for (i=0;i<pocetVlaku;i++){ //Přes všechny vlaky
            if (vlak.cesta[0]&&vlak.staniceOdjezduVlaku[cas]){
                mistVeVlaku = vlak.kapacita - pocetLidiVeVlaku[i];
                j = vlak.staniceOdjezduVlaku[cas]-1; //Index stanice, ze které vlak i odjíždí
                for(k=cas+1;k<maxCas;k++){  //Přes všechny pozdější časy
                    //Přistoupí všichni cestující, kteří jedou do stanic kam jede tento vlak, ale pokud se vlak ještě mezitím do této stanice předtím vrátí, tak nepřistoupí.
                    if (vlak.stanicePrijezduVlaku[k]){ //Pokud vlak i přijede do nějaké stanice v čas k
                        if (vlak.stanicePrijezduVlaku[k]==j+1) break; //Pokud se vlak v tento čas vrátí do stanice, ze které teď odjíždí, tak konec
                        sPr = vlak.stanicePrijezduVlaku[k]-1; //Index stanice, do které vlak i přijíždí
                        for (l=poJakeDobeVsichniOdejdou-1;l>=0;l--){ //Postupně nastupují cestující od těch co zde čekají nejdéle
                            if (mistVeVlaku>(*stanice[l])[j][sPr]){
                                mistVeVlaku -= (*stanice[l])[j][sPr];
                                lidiDoStanice[i][sPr] += (*stanice[l])[j][sPr];
                                pocetLidiVeVlaku[i] += (*stanice[l])[j][sPr];
                                if ((j==zeS)&&(sPr==doS)){
                                    cestujicichPrepraveno += (*stanice[l])[j][sPr];
                                    pomocna += (*stanice[l])[j][sPr];
                                }
                                (*stanice[l])[j][sPr] = 0;
                            }
                            else{
                                lidiDoStanice[i][sPr] += mistVeVlaku;
                                pocetLidiVeVlaku[i] = vlak.kapacita;
                                (*stanice[l])[j][sPr] -= mistVeVlaku;
                                if ((j==zeS)&&(sPr==doS)){
                                    cestujicichPrepraveno += mistVeVlaku;
                                    pomocna += mistVeVlaku;
                                }
                                goto vlakPlny; //"velký" break přes tři for cykly
                            }
                        }
                    }
                }
            }
            vlakPlny: ;
        }
        if (pomocna){
            printf("%u",pomocna);
            pomocna = 24 - (int)log10((float)pomocna);
            for (i=0;i<=pomocna;i++) printf(" ");
            pomocna = 0;
        }
        else{
            printf("                          ");
        }
        #undef vlak
        //Odebírání cestujících, co čekali příliš dlouho
        for (i=0;i<pocetStanic;i++){
            for (j=0;j<pocetStanic;j++){
                if ((*stanice[poJakeDobeVsichniOdejdou-1])[i][j]){
                    if ((i==zeS)&&(j==doS)){
                        cestujicichNeprepraveno += (*stanice[poJakeDobeVsichniOdejdou-1])[i][j];
                        pomocna += (*stanice[poJakeDobeVsichniOdejdou-1])[i][j];
                    }
                    (*stanice[poJakeDobeVsichniOdejdou-1])[i][j] = 0;
                }
            }
        }
        if (pomocna) printf("%u",pomocna);
        printf("\n");
        sPom = stanice[poJakeDobeVsichniOdejdou-1];
        for (i=poJakeDobeVsichniOdejdou-1;i>0;i--){
            stanice[i]=stanice[i-1];
        }
        stanice[0] = sPom;
    }
    printf("\nCelkově bývá ze stanice %s do stanice %s přepravováno %.1f%% cestujících.\n",jmenaStanic[zeS],jmenaStanic[doS],100*(float)cestujicichPrepraveno/(float)(cestujicichPrepraveno+cestujicichNeprepraveno));
    return;
}

int main()
{
    //Proměnné
    FILE *ukSoubor;
    char nazevSouboru[] = "txd0.sav";
    char idSlotu = 3;
    unsigned short pocetVlaku = 0; //Počet vlaků
    int aktCas = 0; //Aktuální čas
    unsigned int i,j,k,l; //Pomocné
    int vyber; //Pomocné
    unsigned char pomPole[pocetStanic-1];
    hodnotyCen ceny;
        ceny.cenaPracePruvodcich = iniCenaPracePruvodcich;
        ceny.cenaPraceStrojvedoucich = iniCenaPraceStrojvedoucich;
        ceny.cenaZaJednotkuE = iniCenaZaJednotkuE;
        ceny.cenaZaJednotkuEEl = iniCenaZaJednotkuEEl;
        ceny.cenaZaDostavbuKmKoleje = iniCenaZaDostavbuKoleje;
        ceny.cenaZaElKmKoleje = iniCenaZaElKmKoleje;
        ceny.vydelekZaPasazerKilometr = iniVydelekZaPasazerKilometr;
        ceny.denniUrok = iniDenniUrok;
        ceny.cenaUdrzbyZaKmNeelTrate = iniCenaZaUdrzbuKoleje;
        ceny.cenaUdrzbyZaKmElTrate = iniCenaZaUdrzbuElKoleje;
        
    
    parTrate tratZDo[pocetStanic][pocetStanic];
    for (i=0;i<pocetStanic;i++){ //Vynulování obsazenosti
        for (j=0;j<pocetStanic;j++){
            for (k=0;k<maxCas;k++){
                tratZDo[i][j].obsazenost[k]=0;
            }
        }
    }
    //Vím, že toto je celkem sus...
    unsigned int krivkaZDo[pocetStanic][pocetStanic][maxCas] = {{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0}, {1, 0, 1, 0, 2, 1, 2, 1, 0, 1, 1, 0, 1, 1,
    2, 1, 1, 2, 1, 0, 0, 2, 2, 8, 19, 38, 64, 93, 105, 94, 67, 39, 17,
    6, 4, 6, 7, 7, 2, 6, 4, 7, 2, 7, 3, 7, 0, 2, 1, 2, 2, 6, 3, 3, 6, 
   6, 5, 2, 6, 3, 2, 7, 4, 5, 4, 2, 4, 6, 2, 3, 3, 3, 2, 2, 3, 7, 2, 
   4, 3, 2, 5, 0, 3, 5, 1, 3, 4, 1, 1, 1, 1, 1, 2, 1, 1, 1}, {0, 1, 1,
    1, 0, 1, 1, 1, 2, 2, 1, 1, 0, 1, 1, 1, 1, 0, 2, 1, 1, 2, 5, 13, 
   25, 37, 42, 36, 26, 14, 7, 4, 2, 2, 3, 2, 2, 1, 2, 2, 2, 0, 2, 1, 
   1, 2, 1, 2, 2, 3, 2, 2, 1, 0, 1, 1, 3, 2, 1, 1, 3, 3, 1, 3, 3, 2, 
   2, 2, 3, 1, 1, 0, 2, 4, 2, 2, 3, 2, 3, 2, 2, 2, 3, 2, 3, 3, 3, 2, 
   0, 2, 1, 1, 0, 2, 1, 1}, {1, 2, 2, 1, 1, 1, 2, 2, 1, 2, 1, 0, 1, 0,
    0, 2, 1, 1, 1, 1, 1, 1, 2, 5, 6, 12, 26, 35, 40, 37, 28, 15, 6, 3,
    2, 2, 2, 1, 2, 1, 2, 1, 1, 3, 1, 1, 1, 2, 1, 2, 1, 1, 2, 3, 2, 2, 
   1, 3, 3, 2, 2, 2, 3, 3, 1, 1, 2, 2, 3, 1, 0, 0, 3, 1, 3, 3, 1, 3, 
   2, 3, 2, 0, 3, 0, 4, 3, 2, 1, 1, 1, 1, 1, 1, 0, 2, 1}, {1, 1, 1, 1,
    0, 1, 0, 1, 1, 2, 2, 1, 1, 2, 1, 1, 0, 2, 2, 1, 2, 2, 5, 10, 20, 
   27, 31, 26, 18, 12, 6, 3, 2, 4, 2, 2, 2, 3, 2, 1, 0, 3, 2, 2, 0, 2,
    3, 2, 2, 1, 1, 3, 0, 2, 2, 1, 3, 1, 4, 3, 1, 1, 2, 2, 3, 2, 4, 1, 
   2, 2, 3, 2, 2, 1, 1, 2, 1, 1, 2, 4, 0, 2, 3, 2, 3, 2, 3, 3, 1, 1, 
   1, 1, 1, 1, 1, 2}, {1, 1, 1, 0, 1, 0, 1, 1, 2, 2, 2, 0, 1, 0, 1, 2,
    1, 1, 1, 2, 1, 1, 0, 0, 2, 3, 7, 9, 11, 9, 8, 3, 2, 1, 2, 1, 0, 1,
    1, 2, 1, 2, 2, 2, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 2, 
   1, 0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 2, 1, 1, 0, 2, 1, 0, 1, 1, 1, 
   1, 0, 2, 2, 1, 1, 0, 1, 0, 2, 1, 1, 1, 0}}, {{0, 0, 2, 0, 1, 0, 1, 
   1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 2, 1, 1, 0, 3, 9, 18, 32, 48, 
   51, 48, 34, 16, 9, 4, 3, 4, 3, 3, 3, 4, 2, 0, 1, 2, 4, 1, 5, 5, 1, 
   1, 4, 5, 3, 2, 3, 3, 2, 4, 3, 3, 5, 4, 1, 3, 1, 2, 1, 4, 4, 4, 3, 
   3, 4, 1, 0, 1, 1, 2, 2, 0, 2, 3, 4, 3, 1, 3, 4, 1, 1, 2, 1, 1, 2, 
   1, 1, 1}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 2, 0, 0, 1, 2, 0, 1, 0,
    1, 2, 0, 1, 2, 2, 1, 2, 0, 1, 4, 8, 18, 33, 49, 51, 49, 34, 17, 8,
    2, 2, 4, 4, 2, 0, 0, 2, 0, 2, 1, 4, 1, 4, 2, 5, 0, 4, 4, 4, 2, 2, 
   0, 5, 0, 3, 3, 1, 2, 1, 3, 2, 1, 4, 2, 3, 4, 5, 3, 2, 2, 3, 0, 2, 
   4, 0, 1, 5, 4, 0, 5, 2, 3, 2, 0, 4, 5, 1, 1, 2, 1, 1, 1, 1, 1}, {1,
    2, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1, 
   5, 7, 18, 31, 49, 53, 45, 33, 19, 9, 4, 2, 3, 2, 1, 2, 4, 3, 2, 2, 
   3, 4, 5, 2, 2, 2, 3, 4, 1, 3, 3, 4, 2, 3, 2, 1, 1, 3, 2, 2, 3, 3, 
   0, 2, 4, 4, 4, 0, 1, 0, 3, 0, 4, 1, 2, 4, 3, 2, 4, 2, 2, 4, 2, 2, 
   2, 0, 0, 2, 1, 1, 1, 1, 1}, {2, 2, 1, 2, 0, 2, 1, 2, 1, 1, 1, 1, 1,
    0, 1, 2, 0, 2, 1, 1, 2, 0, 1, 3, 7, 13, 28, 36, 43, 35, 26, 14, 6,
    4, 2, 2, 3, 0, 1, 1, 3, 2, 1, 3, 3, 1, 1, 4, 4, 3, 2, 3, 1, 1, 3, 
   3, 1, 3, 1, 0, 2, 3, 3, 2, 0, 2, 2, 2, 4, 3, 2, 1, 2, 1, 2, 2, 1, 
   3, 3, 3, 3, 3, 1, 0, 1, 1, 2, 0, 0, 0, 0, 1, 2, 2, 0, 1}, {2, 0, 1,
    2, 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 1, 1, 1, 0, 2, 2, 1, 2, 0, 10, 
   13, 59, 104, 194, 273, 309, 279, 192, 105, 49, 16, 21, 5, 5, 17, 9,
    20, 21, 5, 14, 19, 17, 1, 7, 10, 0, 4, 13, 11, 18, 1, 7, 6, 11, 1,
    10, 7, 8, 19, 17, 6, 17, 9, 8, 21, 15, 7, 12, 12, 20, 12, 8, 8, 
   14, 3, 14, 17, 4, 5, 2, 9, 13, 10, 14, 1, 0, 2, 2, 0, 1, 0, 
   2}}, {{1, 0, 1, 2, 1, 2, 2, 2, 0, 1, 2, 0, 2, 1, 0, 0, 0, 0, 1, 1, 
   1, 2, 7, 14, 20, 21, 18, 14, 9, 4, 1, 1, 0, 3, 2, 2, 1, 2, 2, 1, 1,
    3, 2, 0, 1, 0, 2, 1, 2, 2, 3, 1, 1, 1, 1, 3, 3, 2, 1, 2, 3, 0, 1, 
   1, 0, 1, 2, 2, 2, 3, 2, 0, 1, 2, 2, 1, 1, 2, 3, 3, 2, 2, 2, 2, 3, 
   1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1}, {1, 0, 1, 0, 0, 2, 1, 1, 1, 2, 1,
    1, 1, 0, 0, 1, 1, 2, 2, 1, 3, 7, 21, 54, 97, 134, 160, 142, 96, 
   50, 28, 8, 4, 5, 2, 2, 2, 3, 6, 5, 11, 1, 10, 12, 8, 7, 7, 8, 5, 2,
    10, 5, 2, 2, 12, 5, 1, 9, 10, 8, 10, 9, 8, 3, 6, 10, 4, 2, 10, 3, 
   5, 5, 12, 10, 7, 8, 3, 2, 8, 7, 6, 1, 9, 4, 1, 4, 4, 9, 2, 1, 0, 2,
    1, 0, 1, 1}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {2, 0, 1, 1, 0, 1, 1, 1, 1,
    0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 4, 5, 15, 27, 38, 41, 
   37, 25, 13, 7, 4, 3, 2, 1, 3, 2, 1, 3, 2, 2, 2, 2, 1, 3, 1, 4, 4, 
   0, 4, 1, 1, 1, 2, 0, 1, 3, 3, 3, 3, 2, 3, 2, 3, 2, 2, 3, 4, 2, 1, 
   1, 1, 1, 1, 4, 2, 1, 1, 3, 2, 1, 0, 2, 3, 3, 2, 1, 1, 1, 2, 2, 0, 
   0, 1}, {0, 1, 0, 1, 2, 1, 2, 0, 0, 1, 1, 0, 0, 1, 2, 1, 1, 2, 1, 1,
    1, 2, 0, 3, 2, 4, 5, 14, 27, 37, 43, 37, 25, 12, 6, 2, 3, 2, 2, 3,
    1, 3, 1, 2, 2, 3, 3, 2, 0, 2, 3, 2, 3, 2, 2, 2, 1, 2, 3, 3, 4, 1, 
   4, 2, 0, 0, 4, 2, 4, 3, 1, 3, 4, 2, 3, 2, 2, 3, 3, 1, 0, 3, 1, 2, 
   1, 1, 1, 4, 0, 1, 1, 1, 0, 0, 0, 2}, {2, 1, 2, 0, 0, 1, 1, 2, 1, 1,
    2, 2, 1, 1, 0, 2, 1, 2, 2, 0, 1, 1, 0, 12, 10, 10, 23, 59, 95, 
   142, 151, 141, 98, 52, 29, 7, 4, 8, 8, 4, 8, 0, 6, 10, 5, 5, 11, 8,
    11, 3, 8, 10, 7, 4, 4, 6, 3, 3, 9, 6, 3, 3, 12, 6, 9, 10, 0, 6, 3,
    4, 1, 1, 2, 1, 9, 3, 1, 7, 1, 6, 10, 6, 6, 3, 6, 4, 5, 10, 1, 2, 
   1, 0, 2, 2, 1, 0}}, {{0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 2, 0, 
   2, 1, 1, 1, 1, 1, 1, 4, 12, 20, 27, 32, 29, 21, 11, 7, 2, 1, 1, 2, 
   1, 1, 0, 3, 0, 1, 2, 4, 2, 1, 3, 4, 1, 3, 1, 2, 1, 2, 2, 1, 2, 1, 
   2, 1, 2, 3, 2, 3, 3, 2, 0, 2, 2, 2, 2, 3, 1, 3, 3, 2, 2, 4, 3, 3, 
   1, 1, 1, 1, 2, 3, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 1}, {1, 1, 1, 0, 2,
    1, 1, 1, 1, 0, 2, 0, 2, 1, 0, 1, 1, 1, 0, 1, 2, 1, 5, 35, 67, 138,
    251, 365, 418, 369, 255, 140, 64, 31, 29, 15, 6, 15, 3, 21, 19, 6,
    25, 25, 6, 4, 25, 13, 14, 12, 7, 11, 6, 4, 7, 25, 17, 21, 7, 28, 
   21, 17, 0, 23, 15, 12, 14, 25, 16, 21, 1, 14, 9, 1, 7, 17, 24, 13, 
   4, 21, 2, 19, 22, 14, 27, 25, 4, 8, 1, 1, 0, 0, 0, 2, 0, 2}, {1, 1,
    0, 0, 1, 2, 1, 0, 2, 0, 2, 1, 1, 1, 1, 2, 1, 0, 1, 0, 0, 1, 0, 2, 
   5, 12, 22, 28, 31, 29, 21, 12, 7, 3, 2, 3, 3, 2, 4, 1, 2, 3, 3, 2, 
   1, 2, 3, 2, 2, 3, 3, 4, 1, 3, 2, 2, 0, 1, 2, 4, 1, 1, 0, 2, 3, 3, 
   4, 3, 2, 1, 1, 4, 1, 3, 2, 2, 3, 1, 1, 1, 1, 4, 2, 2, 2, 3, 3, 3, 
   2, 1, 1, 1, 1, 1, 1, 1}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 0, 0,
    0, 2, 1, 1, 1, 1, 2, 0, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 5, 4, 9, 
   17, 33, 45, 52, 44, 31, 16, 8, 6, 5, 0, 1, 2, 4, 3, 3, 2, 2, 2, 3, 
   4, 1, 2, 2, 3, 2, 4, 1, 2, 1, 1, 1, 3, 1, 3, 5, 1, 2, 1, 3, 4, 2, 
   4, 3, 3, 3, 3, 3, 3, 1, 3, 2, 4, 2, 1, 5, 3, 4, 4, 3, 1, 1, 0, 0, 
   1, 2, 1, 1}, {0, 2, 0, 0, 1, 0, 2, 1, 2, 1, 0, 2, 1, 0, 0, 2, 2, 0,
    1, 1, 2, 2, 0, 2, 1, 1, 6, 9, 19, 34, 46, 52, 45, 32, 19, 8, 4, 3,
    1, 1, 4, 3, 3, 5, 4, 3, 4, 2, 0, 3, 4, 2, 1, 2, 5, 4, 4, 1, 2, 0, 
   5, 3, 2, 2, 3, 3, 4, 4, 3, 2, 3, 2, 2, 3, 4, 3, 4, 1, 2, 2, 3, 3, 
   3, 2, 1, 4, 1, 3, 2, 2, 2, 1, 1, 1, 1, 2}}, {{0, 1, 1, 1, 1, 1, 1, 
   1, 1, 0, 1, 1, 0, 2, 2, 1, 2, 0, 1, 0, 3, 4, 6, 9, 10, 10, 6, 4, 1,
    2, 2, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 0, 1, 1, 1, 2, 1, 1, 2, 
   1, 1, 2, 1, 2, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 2, 2, 
   1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 2, 1, 1, 1, 2, 2, 1, 0, 2, 1, 1, 
   1}, {1, 1, 1, 0, 2, 1, 0, 0, 1, 1, 2, 2, 1, 2, 2, 1, 2, 1, 1, 2, 7,
    20, 49, 98, 135, 155, 140, 97, 56, 28, 17, 4, 9, 6, 2, 9, 8, 1, 
   10, 1, 2, 1, 8, 4, 1, 8, 5, 4, 6, 10, 3, 3, 3, 6, 8, 10, 2, 8, 5, 
   8, 0, 5, 8, 7, 10, 7, 3, 4, 11, 1, 8, 9, 5, 8, 6, 7, 4, 7, 4, 2, 6,
    7, 10, 10, 2, 2, 7, 4, 1, 0, 1, 0, 0, 0, 0, 1}, {2, 0, 0, 1, 1, 1,
    1, 2, 2, 1, 1, 0, 2, 2, 1, 2, 1, 1, 1, 1, 0, 1, 2, 7, 3, 11, 19, 
   39, 62, 94, 103, 88, 68, 35, 15, 6, 2, 5, 2, 6, 5, 4, 5, 4, 8, 4, 
   2, 3, 6, 5, 5, 5, 3, 5, 1, 2, 3, 1, 8, 0, 0, 7, 6, 6, 6, 3, 4, 1, 
   4, 6, 7, 5, 3, 0, 7, 3, 1, 2, 4, 4, 1, 5, 5, 8, 4, 2, 2, 5, 1, 1, 
   2, 0, 2, 1, 1, 0}, {1, 1, 2, 0, 1, 0, 2, 0, 0, 1, 1, 0, 1, 2, 1, 0,
    1, 0, 2, 1, 2, 1, 2, 2, 1, 0, 3, 8, 20, 34, 47, 53, 48, 31, 18, 9,
    5, 2, 3, 1, 4, 3, 1, 1, 2, 4, 4, 2, 1, 3, 4, 1, 2, 4, 4, 0, 1, 4, 
   3, 1, 0, 1, 3, 2, 2, 2, 2, 4, 3, 3, 2, 3, 2, 2, 3, 4, 3, 1, 2, 3, 
   1, 3, 3, 1, 2, 3, 2, 4, 1, 1, 0, 1, 1, 1, 1, 1}, {0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0}, {2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 0, 0, 1, 0, 1, 1, 0, 1,
    1, 1, 2, 10, 2, 10, 9, 16, 31, 67, 136, 183, 213, 184, 126, 75, 
   29, 12, 5, 8, 1, 7, 5, 9, 7, 6, 2, 6, 11, 11, 1, 7, 2, 7, 8, 8, 6, 
   6, 5, 3, 2, 4, 13, 3, 0, 3, 11, 14, 9, 10, 13, 4, 7, 4, 1, 2, 9, 4,
    5, 8, 10, 7, 4, 6, 13, 4, 13, 3, 0, 2, 1, 2, 0, 0, 1, 2}}, {{1, 1,
    0, 1, 2, 0, 2, 2, 2, 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 2, 5, 5, 7, 5, 
   5, 3, 2, 1, 1, 1, 1, 1, 2, 1, 0, 2, 1, 1, 1, 1, 2, 2, 0, 0, 1, 0, 
   0, 1, 0, 0, 1, 0, 2, 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 2, 1, 1, 0, 
   2, 1, 1, 0, 1, 1, 2, 1, 1, 0, 0, 2, 0, 1, 1, 1, 1, 2, 1, 1, 2, 1, 
   1, 2, 1, 1, 0, 1}, {2, 2, 0, 0, 2, 1, 0, 2, 1, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 4, 11, 35, 82, 152, 234, 260, 230, 160, 82, 39, 27, 8, 8, 
   17, 4, 10, 2, 9, 1, 17, 12, 1, 12, 14, 9, 10, 17, 2, 12, 2, 4, 16, 
   9, 1, 6, 8, 6, 7, 16, 2, 9, 6, 10, 15, 4, 12, 14, 16, 12, 11, 16, 
   6, 1, 15, 5, 14, 6, 6, 4, 5, 15, 13, 15, 7, 5, 17, 18, 3, 14, 0, 1,
    1, 2, 1, 0, 1, 0}, {1, 2, 0, 1, 1, 0, 2, 1, 1, 2, 1, 1, 0, 1, 0, 
   1, 0, 1, 1, 2, 1, 0, 1, 0, 2, 0, 4, 7, 15, 18, 22, 17, 15, 8, 4, 1,
    2, 2, 0, 1, 0, 1, 2, 3, 2, 2, 1, 2, 1, 3, 0, 1, 0, 1, 1, 2, 0, 0, 
   2, 2, 0, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 2, 3, 2, 1, 2, 1, 1, 
   2, 2, 3, 2, 0, 2, 1, 1, 0, 1, 1, 1, 0, 2, 1, 1}, {1, 1, 0, 2, 2, 0,
    2, 0, 2, 1, 1, 1, 2, 2, 1, 0, 1, 1, 2, 1, 2, 0, 2, 3, 1, 2, 4, 7, 
   13, 20, 21, 19, 14, 6, 4, 0, 3, 2, 2, 0, 3, 0, 3, 2, 1, 2, 0, 1, 0,
    0, 2, 2, 2, 0, 2, 2, 2, 3, 2, 2, 3, 1, 1, 2, 0, 1, 0, 0, 2, 1, 3, 
   3, 1, 2, 1, 2, 2, 1, 1, 1, 1, 2, 2, 2, 1, 0, 2, 1, 1, 1, 0, 1, 1, 
   1, 1, 1}, {1, 1, 2, 1, 2, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 2, 2, 0,
    1, 1, 1, 1, 3, 2, 3, 1, 3, 7, 14, 25, 36, 43, 36, 25, 14, 7, 3, 3,
    4, 2, 4, 1, 2, 2, 3, 2, 0, 3, 2, 3, 3, 3, 1, 2, 1, 3, 3, 4, 3, 1, 
   0, 2, 3, 3, 2, 1, 1, 1, 4, 2, 1, 4, 4, 4, 3, 2, 2, 2, 2, 0, 1, 3, 
   2, 3, 0, 4, 3, 1, 0, 1, 1, 1, 1, 2, 2}, {0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};
    
    // parVlaku vlaky[maxVlaku];
    //int vlaky[maxVlaku][pocetStanic]; //Souřadnice x jsou indexy vlaků, souřadnice y je postupná cestu vlaku a skládá se z indexu stanice do které míří vlak x na y-tém místě
    // int stanice[pocetStanic][pocetStanic]; //Souřadnice x jsou indexy stanic, souřadnice y jsou indexy stanic a skládá se z počtů cestujících ze stanice x směřujících do stanice y
    //int casyPrijezduVlaku[maxCas][maxVlaku];
    //int stanicePrijezduVlaku[maxCas][maxVlaku]; //Souřadnice x jsou časy od 0, 1, ..., maxCas a souřadnice y jsou indexy vlaků a skládá se z indexů stanic na které v čas x přijede vlak y
    //int staniceDoKterychJedeVlak[maxVlaku][pocetStanic]; //Souřadnice x jsou indexy vlaků, souřadnice y jsou indexy stanic a skládá se z logických hodnot zda vlak x jede do stanice y
    //infoOVlaku[maxVlaku];
    parVlaku obchod[pocetVlakuVObchodu];
    i = 0;
        strcpy(obchod[i].nazev,"Motorový vůz řady 810");
        obchod[i].valivyOdpor = 20.0/1000.0;
        obchod[i].hmotnost = 20;
        obchod[i].porizovaciCena = 50000;
        obchod[i].pocetPruvodcich = 0;
        obchod[i].maxVykon = 155;
        obchod[i].maxRychlost = 80;
        obchod[i].efektivitaRekuperace = 0;
        obchod[i].potrebujeElektrifikaci = 0;
        obchod[i].kapacita = 55;
    i = 1;
        strcpy(obchod[i].nazev,"RegioSprinter");
        obchod[i].valivyOdpor = 50.0/1000.0;
        obchod[i].hmotnost = 50;
        obchod[i].porizovaciCena = 100000;
        obchod[i].pocetPruvodcich = 1;
        obchod[i].maxVykon = 396;
        obchod[i].maxRychlost = 100;
        obchod[i].efektivitaRekuperace = 0;
        obchod[i].potrebujeElektrifikaci = 0;
        obchod[i].kapacita = 75;
    i = 2;
        strcpy(obchod[i].nazev,"Lokomotiva řady 749 se čtyřmi vozy Bmto");
        obchod[i].valivyOdpor = (float)(75+4*50)/1000.0;
        obchod[i].hmotnost = 75+4*50;
        obchod[i].porizovaciCena = 200000;
        obchod[i].pocetPruvodcich = 1;
        obchod[i].maxVykon = 1103;
        obchod[i].maxRychlost = 100;
        obchod[i].efektivitaRekuperace = 0;
        obchod[i].potrebujeElektrifikaci = 0;
        obchod[i].kapacita = 126*4;
    i = 3;
        strcpy(obchod[i].nazev,"RegioPanter");
        obchod[i].valivyOdpor = 107.0/1000.0;
        obchod[i].hmotnost = 107;
        obchod[i].porizovaciCena = 300000;
        obchod[i].pocetPruvodcich = 1;
        obchod[i].maxVykon = 4*340;
        obchod[i].maxRychlost = 160;
        obchod[i].efektivitaRekuperace = 80;
        obchod[i].potrebujeElektrifikaci = 1;
        obchod[i].kapacita = 160;
    i = 4;
        strcpy(obchod[i].nazev,"Lokomotiva řady 362 s pěti vozy Bee²⁴³");
        obchod[i].valivyOdpor = (float)(86+5*42)/1000.0;
        obchod[i].hmotnost = 86+5*42;
        obchod[i].porizovaciCena = 300000;
        obchod[i].pocetPruvodcich = 1;
        obchod[i].maxVykon = 3480;
        obchod[i].maxRychlost = 140;
        obchod[i].efektivitaRekuperace = 0;
        obchod[i].potrebujeElektrifikaci = 1;
        obchod[i].kapacita = 5*80;
        
    stavPenez penize;
        penize.hlavniUcet=0;
    //Zde níže nastavení parametrů postavitelných tratí
    surParTrate postavitelneZDo[pocetStanic][pocetStanic][pocetVarTrate];
    parVlaku *ukVlaky;
    
    char oddelovac[] = "\n\n";
    
    for (i=0;i<pocetStanic;i++){
        for (j=0;j<pocetStanic;j++){
            for (k=0;k<pocetVarTrate;k++){
                postavitelneZDo[i][j][k].cenaStavby=0;
            }
        }
    }
   
    //Trať Poustevna-Lípa
        postavitelneZDo[0][1][0].delka = 44;
        postavitelneZDo[0][1][0].cenaStavby = postavitelneZDo[0][1][0].delka*1.5*ceny.cenaZaDostavbuKmKoleje;
        i = 0; j = 1; k = 0;
            postavitelneZDo[i][j][k].stoupani[0]=0;
            postavitelneZDo[i][j][k].stoupani[1]=0;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=5;
            postavitelneZDo[i][j][k].klesani[1]=15;
            postavitelneZDo[i][j][k].klesani[2]=20;
            postavitelneZDo[i][j][k].klesani[3]=4;
            postavitelneZDo[i][j][k].koefDoby=normKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=normNasobekZrychleni;
            
        postavitelneZDo[0][1][1].delka = 60;
        postavitelneZDo[0][1][1].cenaStavby = postavitelneZDo[0][1][1].delka*1.3*ceny.cenaZaDostavbuKmKoleje;
        i = 0; j = 1; k = 1;
            postavitelneZDo[i][j][k].stoupani[0]=0;
            postavitelneZDo[i][j][k].stoupani[1]=0;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=4;
            postavitelneZDo[i][j][k].klesani[1]=45;
            postavitelneZDo[i][j][k].klesani[2]=11;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=nizsiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=nizsiNasobekZrychleni;
        
        postavitelneZDo[0][1][2].cenaStavby = 0;
    //Trať Lípa-Lhota
        postavitelneZDo[1][2][0].delka = 20;
        postavitelneZDo[1][2][0].cenaStavby = postavitelneZDo[1][2][0].delka*1.3*ceny.cenaZaDostavbuKmKoleje;
        i = 1; j = 2; k = 0;
            postavitelneZDo[i][j][k].stoupani[0]=15;
            postavitelneZDo[i][j][k].stoupani[1]=2.5;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=2.5;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=vyssiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=vyssiNasobekZrychleni;
        
        postavitelneZDo[1][2][1].delka = 15;
        postavitelneZDo[1][2][1].cenaStavby = postavitelneZDo[1][2][1].delka*2.5*ceny.cenaZaDostavbuKmKoleje;
        i = 1; j = 2; k = 1;
            postavitelneZDo[i][j][k].stoupani[0]=9;
            postavitelneZDo[i][j][k].stoupani[1]=3;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=3;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=nizsiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=nizsiNasobekZrychleni;
        
        postavitelneZDo[1][2][2].cenaStavby = 0;
    //Trať Lhota-Ves
        postavitelneZDo[2][5][0].delka = 30;
        postavitelneZDo[2][5][0].cenaStavby = postavitelneZDo[2][5][0].delka*1.3*ceny.cenaZaDostavbuKmKoleje;
        i = 2; j = 5; k = 0;
            postavitelneZDo[i][j][k].stoupani[0]=20;
            postavitelneZDo[i][j][k].stoupani[1]=5;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=5;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=vyssiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=vyssiNasobekZrychleni;
        
        postavitelneZDo[2][5][1].delka = 25;
        postavitelneZDo[2][5][1].cenaStavby = postavitelneZDo[2][5][1].delka*2.5*ceny.cenaZaDostavbuKmKoleje;
        i = 2; j = 5; k = 1;
            postavitelneZDo[i][j][k].stoupani[0]=20;
            postavitelneZDo[i][j][k].stoupani[1]=2.5;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=2.5;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=nizsiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=nizsiNasobekZrychleni;
        
        postavitelneZDo[2][5][2].cenaStavby = 0;
    //Trať Ves-Újezd
        postavitelneZDo[5][4][0].delka = 10;
        postavitelneZDo[5][4][0].cenaStavby = postavitelneZDo[5][4][0].delka*1.5*ceny.cenaZaDostavbuKmKoleje;
        i = 5; j = 4; k = 0;
            postavitelneZDo[i][j][k].stoupani[0]=5;
            postavitelneZDo[i][j][k].stoupani[1]=4;
            postavitelneZDo[i][j][k].stoupani[2]=1;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=0;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=normKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=normNasobekZrychleni;
        
        postavitelneZDo[5][4][1].delka = 15;
        postavitelneZDo[5][4][1].cenaStavby = postavitelneZDo[5][4][1].delka*1.3*ceny.cenaZaDostavbuKmKoleje;
        i = 5; j = 4; k = 1;
            postavitelneZDo[i][j][k].stoupani[0]=3;
            postavitelneZDo[i][j][k].stoupani[1]=12;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=0;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=vyssiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=vyssiNasobekZrychleni;
        
        postavitelneZDo[5][4][2].cenaStavby = 0;
    //Trať Újezd-Dvory
        postavitelneZDo[4][3][0].delka = 20;
        postavitelneZDo[4][3][0].cenaStavby = postavitelneZDo[4][3][0].delka*1.5*ceny.cenaZaDostavbuKmKoleje;
        i = 4; j = 3; k = 0;
            postavitelneZDo[i][j][k].stoupani[0]=0;
            postavitelneZDo[i][j][k].stoupani[1]=0;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=10;
            postavitelneZDo[i][j][k].klesani[1]=9;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=1;
            
            postavitelneZDo[i][j][k].koefDoby=normKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=normNasobekZrychleni;
        
        postavitelneZDo[4][3][1].delka = 30;
        postavitelneZDo[4][3][1].cenaStavby = postavitelneZDo[4][3][1].delka*1.3*ceny.cenaZaDostavbuKmKoleje;
        i = 4; j = 3; k = 1;
            postavitelneZDo[i][j][k].stoupani[0]=0;
            postavitelneZDo[i][j][k].stoupani[1]=0;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=18;
            postavitelneZDo[i][j][k].klesani[1]=12;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=vyssiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=vyssiNasobekZrychleni;
        
        postavitelneZDo[4][3][2].cenaStavby = 0;
    //Trať Dvory-Lípa
        postavitelneZDo[3][1][0].delka = 12;
        postavitelneZDo[3][1][0].cenaStavby = postavitelneZDo[3][1][0].delka*1.3*ceny.cenaZaDostavbuKmKoleje;
        i = 3; j = 1; k = 0;
            postavitelneZDo[i][j][k].stoupani[0]=11;
            postavitelneZDo[i][j][k].stoupani[1]=1;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=1;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=vyssiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=vyssiNasobekZrychleni;
        
        postavitelneZDo[3][1][1].cenaStavby = 0;
        
        postavitelneZDo[3][1][2].cenaStavby = 0;
    //Trať Lhota-Újezd
        postavitelneZDo[2][4][0].delka = 30;
        postavitelneZDo[2][4][0].cenaStavby = postavitelneZDo[2][4][0].delka*1.5*ceny.cenaZaDostavbuKmKoleje;
        i = 2; j = 4; k = 0;
            postavitelneZDo[i][j][k].stoupani[0]=1;
            postavitelneZDo[i][j][k].stoupani[1]=15;
            postavitelneZDo[i][j][k].stoupani[2]=3;
            postavitelneZDo[i][j][k].stoupani[3]=1;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=9;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=1;
            
            postavitelneZDo[i][j][k].koefDoby=normKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=normNasobekZrychleni;
        
        postavitelneZDo[2][4][1].delka = 25;
        postavitelneZDo[2][4][1].cenaStavby = postavitelneZDo[2][4][1].delka*2.5*ceny.cenaZaDostavbuKmKoleje;
        i = 2; j = 4; k = 1;
            postavitelneZDo[i][j][k].stoupani[0]=10;
            postavitelneZDo[i][j][k].stoupani[1]=13.5;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=1.5;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=nizsiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=nizsiNasobekZrychleni;
        
        postavitelneZDo[2][4][2].delka = 40;
        postavitelneZDo[2][4][2].cenaStavby = postavitelneZDo[2][4][2].delka*1.3*ceny.cenaZaDostavbuKmKoleje;
        i = 2; j = 4; k = 2;
            postavitelneZDo[i][j][k].stoupani[0]=14;
            postavitelneZDo[i][j][k].stoupani[1]=14;
            postavitelneZDo[i][j][k].stoupani[2]=0;
            postavitelneZDo[i][j][k].stoupani[3]=0;
            
            postavitelneZDo[i][j][k].klesani[0]=0;
            postavitelneZDo[i][j][k].klesani[1]=12;
            postavitelneZDo[i][j][k].klesani[2]=0;
            postavitelneZDo[i][j][k].klesani[3]=0;
            
            postavitelneZDo[i][j][k].koefDoby=vyssiKoefDoby;
            postavitelneZDo[i][j][k].nasobekZrychleni=vyssiNasobekZrychleni;
        
        for (i=0;i<pocetStanic;i++){
            for (j=0;j<pocetStanic;j++){
                tratZDo[i][j].pocetKoleji = 0;
            }
        }
    
    //Kontrola konstant
   // if(maxVlaku < 1) {printf("Maximální počet vlaků musí být větší než 0."); return EXIT_FAILURE;}
    if(24*60%minutVJednotceCasu!=0) {printf("Počet minut ve dni musí být dělitelný počtem minut v jednotce času."); return EXIT_FAILURE;}
    
    //Zkušební hodnoty
//    parVlaku zkVlak;
//    zkVlak.cesta[0] = 1;
//        zkVlak.cesta[1] = 2;
//        zkVlak.cesta[2] = 3;
//        zkVlak.cesta[3] = 2;
//        zkVlak.cesta[4] = 1;
//        zkVlak.cesta[5] = 0;
//        zkVlak.cesta[6] = 0;
//        zkVlak.cesta[7] = 0;
//        zkVlak.cesta[8] = 0;
//        zkVlak.cesta[9] = 0;    
//    zkVlak.efektivitaRekuperace = 20;
//    zkVlak.valivyOdpor = 50;
//    zkVlak.hmotnost = 70;
//    for (int i=0;i<maxCas;i++){
//        zkVlak.stanicePrijezduVlaku[i] = 0;
//    }
//    strcpy(zkVlak.nazev,"Zkušební vlak");
//    zkVlak.stanicePrijezduVlaku[0] = 1;
//    zkVlak.stanicePrijezduVlaku[10] = 2;
//    zkVlak.stanicePrijezduVlaku[25] = 3;
//    zkVlak.stanicePrijezduVlaku[40] = 2;
//    zkVlak.stanicePrijezduVlaku[50] = 50;
//    
//    parVlaku zkVlak2;
//    zkVlak2.cesta[0] = 1;
//        zkVlak2.cesta[1] = 2;
//        zkVlak2.cesta[2] = 3;
//        zkVlak2.cesta[3] = 2;
//        zkVlak2.cesta[4] = 1;
//        zkVlak2.cesta[5] = 0;
//        zkVlak2.cesta[6] = 0;
//        zkVlak2.cesta[7] = 0;
//        zkVlak2.cesta[8] = 0;
//        zkVlak2.cesta[9] = 0;    
//    zkVlak2.efektivitaRekuperace = 20;
//    zkVlak2.valivyOdpor = 50;
//    zkVlak2.hmotnost = 70;
//    for (int i=0;i<maxCas;i++){
//        zkVlak2.stanicePrijezduVlaku[i] = 0;
//    }
//    zkVlak2.stanicePrijezduVlaku[0] = 1;
//    zkVlak2.stanicePrijezduVlaku[10] = 2;
//    zkVlak2.stanicePrijezduVlaku[25] = 3;
//    zkVlak2.stanicePrijezduVlaku[40] = 2;
//    zkVlak2.stanicePrijezduVlaku[50] = 50;
    
//    tratZDo[1-1][2-1].stoupani[0] = 10;
//        tratZDo[1-1][2-1].stoupani[1] = 5;
//        tratZDo[1-1][2-1].stoupani[2] = 1;
//        tratZDo[1-1][2-1].stoupani[3] = 0;
//    tratZDo[1-1][2-1].klesani[0] = 0;
//        tratZDo[1-1][2-1].klesani[1] = 3;
//        tratZDo[1-1][2-1].klesani[2] = 2;
//        tratZDo[1-1][2-1].klesani[3] = 0;
//    tratZDo[1-1][2-1].koefDoby = 2;
//    tratZDo[2-1][3-1].stoupani[0] = 10;
//        tratZDo[2-1][3-1].stoupani[1] = 10;
//        tratZDo[2-1][3-1].stoupani[2] = 0;
//        tratZDo[2-1][3-1].stoupani[3] = 0;
//    tratZDo[2-1][3-1].klesani[0] = 0;
//        tratZDo[2-1][3-1].klesani[1] = 0;
//        tratZDo[2-1][3-1].klesani[2] = 3;
//        tratZDo[2-1][3-1].klesani[3] = 2;
//    tratZDo[2-1][3-1].koefDoby = 2;
//    tratZDo[1-1][2-1].pocetKoleji = 1;
//    tratZDo[2-1][3-1].pocetKoleji = 2;
        
        doplnSurTrate(postavitelneZDo);
        
        
        
//        tratZDo[0][1].pocetKoleji = 0;
//        zobrazMapu(tratZDo);
//        zobrazProfilTrate(&tratZDo[1-1][2-1]);
//        zobrazProfilTrate(&tratZDo[2-1][1-1]);
//        int a = posledniNenulovy(maxCas,zkVlak.stanicePrijezduVlaku);
//        int b = prvniNenulovy(maxCas,zkVlak.stanicePrijezduVlaku);
//        float c = cenaZaDen(&zkVlak,&ceny,tratZDo);
//        vytvorJizdniRad(&zkVlak, tratZDo);
//        vytvorJizdniRad(&zkVlak2, tratZDo);
        
        
        printf("        ,----,                           \n      ,/   .`|                           \n    ,`   .'  :,--,     ,--,    ,---,     \n  ;    ;     /|'. \\   / .`|  .'  .' `\\   \n.'___,/    ,' ; \\ `\\ /' / ;,---.'     \\  \n|    :     |  `. \\  /  / .'|   |  .`\\  | \n;    |.';  ;   \\  \\/  / ./ :   : |  '  | \n`----'  |  |    \\  \\.'  /  |   ' '  ;  : \n    '   :  ;     \\  ;  ;   '   | ;  .  | \n    |   |  '    / \\  \\  \\  |   | :  |  ' \n    '   :  |   ;  /\\  \\  \\ '   : | /  ;  \n    ;   |.'  ./__;  \\  ;  \\|   | '` ,/   \n    '---'    |   : / \\  \\  ;   :  .'     \n             ;   |/   \\  ' |   ,.'       \n             `---'     `--`'---'         \n-----------------------------------------\n        T E X T O V É   D R Á H Y");
        while(1){    
        printf("%s\nZvolte akci\n1) Pokročit v čase.\n2) Stavby tratí.\n3) Změnit jízdní řád\n4) Koupit vlak.\n5) Zobrazit informace a nebo udělat průzkum.\n6) Uložit nebo načíst hru.\n7) Odejít ze hry.\n",oddelovac);
        skenfInt(&vyber);
        kontrolaVyberu(&vyber, 7);
        switch(vyber){
            case(1):
                printf("└─» Zadejte, o kolik let chcete v čase postoupit.\n    ");
                skenfInt(&vyber);
                while(vyber<0){
                    printf("    Toto číslo musí být kladné.\n    ");
                    skenfInt(&vyber);
                }
                i = 365*vyber;
                printf("    └─» Zadejte, o kolik dní chcete v čase postoupit.\n        ");
                skenfInt(&vyber);
                while(vyber<0){
                    printf("        Toto číslo musí být kladné.\n        ");
                    skenfInt(&vyber);
                }
                i += vyber;
                printf("%s",oddelovac);
                posunVCase((unsigned int)i,pocetVlaku,ukVlaky,tratZDo,krivkaZDo,&ceny,&penize);
                break;
            case(2):
                printf("└─» Zvolte akci a nebo zadejte 0 pro návrat do menu.\n    1) Postavit novou trať.\n    2) Elektrifikovat trať.\n    3) Zrušit trať.\n    4) Zrušit elektrifikaci tratě.\n    ");
                skenfInt(&vyber);
                kontrolaVyberuS0(&vyber,4);
                switch(vyber){
                    case(1):
                        printf("%s",oddelovac);
                        postavitTrat(tratZDo,postavitelneZDo, &penize, &ceny);
                        break;
                    case(2):
                        printf("    └─» Zvolte trať, kterou chcete elektrifikovat a nebo zadejte 0 pro návrat do menu.\n");
                        k=0;
                        l=0;
                        for (i=0;i<pocetStanic-1;i++){
                            for (j=i+1;j<pocetStanic;j++){
                                
                                if (tratZDo[i][j].pocetKoleji&&(!(tratZDo[i][j].elektrifikovano))){
                                    printf("        %u) Trať ze stanice %s do stanice %s.\n",k+1, jmenaStanic[i], jmenaStanic[j]);
                                    pomPole[k] = l;
                                    k++;
                                }
                                else{
                                    l++;
                                }
                            }
                        }
                        printf("        ");
                        skenfInt(&j);
                        kontrolaVyberuS0(&j,k);
                        if (!(j)) break;
                        j = j-1+pomPole[j-1];
                        for (i=pocetStanic-1;i>0;i--){
                            if (j<i){
                                j = pocetStanic-i+j;
                                i = pocetStanic-i-1;
                                break;
                            }
                            j -= i;
                        }
                        printf("%s",oddelovac);
                        elektrifikujTrat(tratZDo,i,j,&penize,&ceny);
                        break;
                    case(3):
                        printf("    └─» Zvolte trať, kterou chcete zrušit a nebo zadejte 0 pro návrat do menu.\n");
                        k=0;
                        l=0;
                        for (i=0;i<pocetStanic-1;i++){
                            for (j=i+1;j<pocetStanic;j++){
                                
                                if (tratZDo[i][j].pocetKoleji){
                                    printf("        %u) Trať ze stanice %s do stanice %s.\n",k+1, jmenaStanic[i], jmenaStanic[j]);
                                    pomPole[k] = l;
                                    k++;
                                }
                                else{
                                    l++;
                                }
                            }
                        }
                        printf("        ");
                        skenfInt(&j);
                        kontrolaVyberuS0(&j,k);
                        if (!(j)) break;
                        j = j-1+pomPole[j-1];
                        for (i=pocetStanic-1;i>0;i--){
                            if (j<i){
                                j = pocetStanic-i+j;
                                i = pocetStanic-i-1;
                                break;
                            }
                            j -= i;
                        }
                        printf("%s",oddelovac);
                        zrusTrat(tratZDo,i,j,&penize,&ceny);
                        break;
                    case(4):
                        printf("    └─» Zvolte trať, kterou chcete elektrifikovat a nebo zadejte 0 pro návrat do menu.\n");
                        k=0;
                        l=0;
                        for (i=0;i<pocetStanic-1;i++){
                            for (j=i+1;j<pocetStanic;j++){
                                
                                if (tratZDo[i][j].pocetKoleji&&(tratZDo[i][j].elektrifikovano)){
                                    printf("        %u) Trať ze stanice %s do stanice %s.\n",k+1, jmenaStanic[i], jmenaStanic[j]);
                                    pomPole[k] = l;
                                    k++;
                                }
                                else{
                                    l++;
                                }
                            }
                        }
                        printf("        ");
                        skenfInt(&j);
                        kontrolaVyberuS0(&j,k);
                        if (!(j)) break;
                        j = j-1+pomPole[j-1];
                        for (i=pocetStanic-1;i>0;i--){
                            if (j<i){
                                j = pocetStanic-i+j;
                                i = pocetStanic-i-1;
                                break;
                            }
                            j -= i;
                        }
                        printf("%s",oddelovac);
                        zrusElektrifikaciTrate(tratZDo,i,j,&penize,&ceny);
                        break;
                }
                break;
            case(3):
                printf("└─» Zvolte, kterému vlaku chcete změnit jízdní řád a nebo zadejte 0 pro návrat do menu.\n");
                for (i=0;i<pocetVlaku;i++){
                    printf("    %u) Vlak č. %u %s",i+1,i+1,ukVlaky[i].nazev);
                    if (ukVlaky[i].cesta[0]){
                        printf(" s cestou %s",jmenaStanic[ukVlaky[i].cesta[0]-1]);
                        for(j=1;;j++){
                            if(ukVlaky[i].cesta[j]){
                                if(j>pocetStanicVCesteVNahledu-1){
                                    printf(", ...");
                                    break;
                                }
                                printf(", %s",jmenaStanic[ukVlaky[i].cesta[j]-1]);
                            }
                            else{
                                break;
                            }
                        }
                        printf("\n");
                    }
                    else{
                        printf(", který je odstavený\n");
                    }
                }
                printf("    ");
                skenfInt(&vyber);
                kontrolaVyberuS0(&vyber,pocetVlaku);
                if (!(vyber)) break;
                vyber--;
                printf("%s",oddelovac);
                vytvorJizdniRad(ukVlaky+vyber,tratZDo);
                break;
            case(4):
                printf("%s",oddelovac);
                if (pocetVlaku==USHRT_MAX){
                    printf("    Už vlastníte maximální počet vlaků.\n");
                    break;
                }
                ukVlaky = koupitVlak(pocetVlaku++,ukVlaky,&penize, obchod);
                break;
            case(5):
                printf("└─» Zvolte, jaké informace chcete zobrazit a nebo zadejte 0 pro návrat do menu.\n    1) Zobrazit mapu.\n    2) Zobrazit informace o trati.\n    3) Zobrazit informace o vlaku.\n    4) Udělat průzkum počtu cestujících.\n    5) Zobrazit stav účtu\n    ");
                skenfInt(&vyber);
                kontrolaVyberuS0(&vyber,5);
                switch(vyber){
                    case(1):
                        printf("%s",oddelovac);
                        zobrazMapu(tratZDo);
                        break;
                    case(2):
                    printf("    └─» Zvolte trať, o které chcete zobrazit informace a nebo zadejte 0 pro návrat do menu.\n");
                        k=0;
                        l=0;
                        for (i=0;i<pocetStanic-1;i++){
                            for (j=i+1;j<pocetStanic;j++){
                                
                                if (tratZDo[i][j].pocetKoleji){
                                    printf("        %u) Trať ze stanice %s do stanice %s.\n",k+1, jmenaStanic[i], jmenaStanic[j]);
                                    pomPole[k] = l;
                                    k++;
                                }
                                else{
                                    l++;
                                }
                            }
                        }
                        printf("        ");
                        skenfInt(&j);
                        kontrolaVyberuS0(&j,k);
                        if (!(j)) break;
                        j = j-1+pomPole[j-1];
                        for (i=pocetStanic-1;i>0;i--){
                            if (j<i){
                                j = pocetStanic-i+j;
                                i = pocetStanic-i-1;
                                break;
                            }
                            j -= i;
                        }
                        printf("%s",oddelovac);
                        zobrazInfoOTrati(&tratZDo[i][j]);
                        break;
                    case(3):
                        printf("    └─» Zvolte, kterému vlaku chcete změnit jízdní řád a nebo zadejte 0 pro návrat do menu.\n");
                        for (i=0;i<pocetVlaku;i++){
                            printf("        %u) Vlak č. %u %s",i+1,i+1,ukVlaky[i].nazev);
                            if (ukVlaky[i].cesta[0]){
                                printf(" s cestou %s",jmenaStanic[ukVlaky[i].cesta[0]-1]);
                                for(j=1;;j++){
                                    if(ukVlaky[i].cesta[j]){
                                        if(j>pocetStanicVCesteVNahledu-1){
                                            printf(", ...");
                                            break;
                                        }
                                        printf(", %s",jmenaStanic[ukVlaky[i].cesta[j]-1]);
                                    }
                                    else{
                                        break;
                                    }
                                }
                                printf("\n");
                            }
                            else{
                                printf(", který je odstavený\n");
                            }
                        }
                        printf("        ");
                        skenfInt(&vyber);
                        kontrolaVyberuS0(&vyber,pocetVlaku);
                        if (!(vyber)) break;
                        vyber--;
                        printf("%s",oddelovac);
                        zobrazInfoOVlaku(ukVlaky+vyber);
                        break;
                    case(4):
                        printf("        └─» Zadejte, ze které stanice chcete počet cestujících sledovat a nebo zadejte 0 pro návrat do menu.\n");
                        for (i=0;i<pocetStanic;i++){
                            printf("            %u) %s\n",i+1,jmenaStanic[i]);
                        }
                        printf("            ");
                        skenfInt(&vyber);
                        kontrolaVyberuS0(&vyber,pocetStanic);
                        if (!(vyber)) break;
                        j = --vyber;
                        printf("            └─» Zadejte, do které stanice ze stanice %s chcete počet cestujících sledovat a nebo zadejte 0 pro návrat do menu.\n",jmenaStanic[j]);
                        for (i=0;i<pocetStanic;i++){
                            printf("                %u) %s\n",i+1,jmenaStanic[i]);
                        }
                        printf("                ");
                        skenfInt(&vyber);
                        kontrolaVyberuS0(&vyber,pocetStanic);
                        if (!(vyber)) break;
                        vyber--;
                        printf("%s",oddelovac);
                        pruzkumZDo(j,vyber,pocetVlaku,ukVlaky,tratZDo,krivkaZDo);
                        break;
                    case(5):
                        printf("%sAktuální stav účtu je %lf korun.\n",oddelovac,penize.hlavniUcet);
                        break;
                    case(0):
                        break;
                }
                break;
            case(6):
                printf("└─» Zvolte akci a nebo zadejte 0 pro návrat do menu.\n    1) Uložit hru.\n    2) Načíst hru.\n    ");
                skenfInt(&vyber);
                kontrolaVyberuS0(&vyber,2);
                switch(vyber){
                    case(0): break;
                    case(1):
                        printf("    └─» Zvolte, do kterého slotu (od 0 do 9) se hra uloží.\n        ");
                        skenfInt(&vyber);
                        kontrolaVyberuS0(&vyber,9);
                        nazevSouboru[idSlotu] = (char)(vyber+48);
                        printf("%s",oddelovac);
                        printf("Probíhá zápis, nevypínejte program...\n");
                        ukSoubor = fopen(nazevSouboru,"wb");
                        if (ukSoubor == NULL){
                            perror("Chyba");
                            break;
                        }
                        fwrite(&tratZDo,sizeof(tratZDo),1,ukSoubor);
                        fwrite(&ceny,sizeof(ceny),1,ukSoubor);
                        fwrite(&krivkaZDo,sizeof(krivkaZDo),1,ukSoubor);
                        fwrite(&penize,sizeof(penize),1,ukSoubor);
                        fwrite(&pocetVlaku,sizeof(pocetVlaku),1,ukSoubor);
                        for (i=0;i<pocetVlaku;i++){
                            fwrite(ukVlaky+i,sizeof(parVlaku),1,ukSoubor);
                        }
                        fclose(ukSoubor);
                        printf("Hra byla uložena.\n");
                        break;
                    case(2):
                        printf("    └─» Zvolte, ze kterého slotu (od 0 do 9) chcete hru načíst.\n        ");
                        skenfInt(&vyber);
                        kontrolaVyberuS0(&vyber,9);
                        nazevSouboru[idSlotu] = (char)(vyber+48);
                        printf("%s",oddelovac);
                        printf("Probíhá načítání...\n");
                        ukSoubor = fopen(nazevSouboru,"rb");
                        if (ukSoubor == NULL){
                            printf("V tomto slotu není žádná hra uložená.\n");
                            break;
                        }
                        fread(&tratZDo,sizeof(tratZDo),1,ukSoubor);
                        fread(&ceny,sizeof(ceny),1,ukSoubor); //Vím, že zatím jsou ceny neměnné, ale časem to může být implementováno
                        fread(&krivkaZDo,sizeof(krivkaZDo),1,ukSoubor);
                        fread(&penize,sizeof(penize),1,ukSoubor);
                        if (pocetVlaku){
                            free(ukVlaky);
                        }
                        fread(&pocetVlaku,sizeof(pocetVlaku),1,ukSoubor);
                        ukVlaky = calloc(sizeof(parVlaku),pocetVlaku);
                        for (i=0;i<pocetVlaku;i++){
                            fread(ukVlaky+i,sizeof(parVlaku),1,ukSoubor);
                        }
                        fclose(ukSoubor);
                        printf("Načítání hry bylo dokončeno.\n");
                        break;
                }
                break;
            case(7):
                return EXIT_SUCCESS;
        }
    }
}
