#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<math.h>
#include<string.h>
#include<time.h>
#ifndef X
#define X 99999
#endif
#define S(s)((char*)((s)&~15))
#define G(x)(((l*)((v)&~15))[x])
#define die(...)(fprintf(stderr,__VA_ARGS__),fputc(10,stderr),exit(1))
#define l long long
#define W while
#define I if
#define C ;case
#define K(x)C x:R
#define Z strspn
#define M strcmp
#define L strlen
#define D strndup
#define A(x)calloc(1+(x),1)
#define P printf
#define J strcat
#define R return
#define O G(1)
#define T G(2)
char*j,*k,*B[]={"false","true","null"},*U[X],E[22];l i,V[X],r(l);size_t x,y,z;l
p(){l*f,v=0;char*t=0,u[2]={0},c;W(Z(j,"\t\f\r\n{}[]():# "))I(*j++==35)W(*j-10)j
++;W(isdigit(*j))t=j,v=v*10+*j++-48;I(t)R 1|v<<4;W(islower(c=*j)||isdigit(c)|95
==c)t?++j:(t=j);I(t){for(v=j-t,i=0;(k=U[i])&&v!=(l)L(k)|strncmp(k,t,v);++i);I(!
U[i])U[i]=D(t,v);R(l)&V[i]|4;}t=++j;I(c==39|c==34){W(*j++!=c);R(l)D(t,j-t-1)|2;
}*u=c;W(isupper(c)&&isupper(*j)|*j==95)++j;I(Z(u,"TFN"))R+ c-84?c-70?12:0:8;v=8
|(l)(f=A(5*sizeof(l)-1));*f++=c;I(c-82&&c-80)I(*f++=p(),!Z(u,"EBC`Q!LDOA~"))I(*
f++=p(),Z(u,"GIS"))I(*f++=p(),c>82)*f=p();R v;}l b(l v){R 0<=v&v<13?v==8:v&2?*S
(v)!=0:v&1||b(r(v));}char*s(l v){R v&2?S(v):v&1?sprintf(E,"%lld",v>>4),E:v<13?B
[v/5]:s(r(v));}l n(l v){R v&1?v>>4:v&2?strtoll(S(v),0,10):v<13?v==8:n(r(v));} l
r(l v){l t,u; I(v<13|v&3)R v;I(v&4)R*(l*)(v-4);switch(G(0)){K(80)j=0,getline(&j
,&z,stdin),2|(l)D(j,L(j))K(82)1|0xfffff0&rand()K(69)j=s(O),r(p())K(66)O K(67)r(
r(O))C 96:j=A(x=2047);y=0;FILE*f=popen(s(O),"r");W(z=fread(j+y,1,x-y,f))I(x==(y
+=z))j=realloc(j,x*=2);R(l)D(j,L(j))|2 K(81)exit(n(O)),0 K(33)8*!b(O)K(76)1|16*
L(s(O))K(68)(1&(t=r(O))?P("Number(%lld)",t>>4):2&t?P("String(%s)",S(t)):t<12?P(
"Boolean(%s)",t?"true":"false"):P(t-12?"<?>":"Null()")),P("\n"),t K(79)z=L(j=s(
O)),P(!z||j[z-1]-92?"%2$s\n":"%.*s",(int)z-1,j),12 K(65)1&(t=r(O))?*(j=A(1))=t/
16,2|(l)j:1|16**S(t)K('~')1|16*-n(O)K(43)2&(t=r(O))?j=S(t),k=s(T),2|(l)J(J(A(L(
j)+L(k)),j),k):t+16*n(T)K(45)1|16*(n(O)-n(T))C 42:I(1&(t=r(O)))R 1|(t>>4)*n(T)*
16;k=A(L(j=S(t))*(u=n(T)));W(u--)J(k,j);R 2|(l)k K(47)1|16*(n(O)/n(T))K(37)1|16
*(n(O)%n(T))K(94)1|(int)pow(n(O),n(T))<<4 C 60:K(62)(i=1&(t=r(O))?(t>>4)-n(T):2
&t?M(S(t),s(T)):(t/8)-b(T)),8*(G(0)-60?i>0:i<0)K(63)8*((t=r(O))==(u=r(T))||t&u&
2&&!M(S(t),S(u)))K(38)b(t=r(O))?r(T):t K('|')b(t=r(O))?t:r(T)K(59)r(O),r(T)K(61
)*(l*)(O-4)=r(T)C 87:W(b(O))r(T);R 12 K(73)r(G(3-b(O)))K(71)2|(l)D(s(O)+n(T),n(
G(3)))K(83)j=s(O),t=n(T),u=n(G(3)),k=s(G(4)),2|(l)J(J(strncat(A(L(j)+L(k)+u),j,
t),k),j+t+u);default:die("invalid char '%c'",(int)G(0));}}int main(int c,char**
v){srand(time(0));I(c-3)die("usage: %s (-e 'program' | -f file)\n",*v);v[1][1]-
'f'?j=v[2],0:getdelim(&j,&z,0,fopen(v[2],"r"));r(p());}
