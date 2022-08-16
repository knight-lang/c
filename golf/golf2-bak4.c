// this is *almost* but not exactly `golf.c`
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<math.h>
#include<string.h>
#include<time.h>
#define l long long


#define UNMASK(s) ((s) & ~15)
#define S(s) ((char*)UNMASK(s))
#define NEWSTR(x) (2 | (l) (x))
#define NEWBOOL(x) ((x) ? 12 : 4)
#define ARG(x) (((l *) UNMASK(v))[x])

#define die(...) (fprintf(stderr,__VA_ARGS__),fputc('\n',stderr),exit(1))

#define W while
#define I if
#define E else
#define D ;case
#define C(x) D x:R
#define SPN strspn
#define CMP strcmp
#define L strlen
#define NDUP strndup
#define A(x)calloc(1+(x),1)
#define P printf
#define CAT strcat
#define R return
#define A1 ARG(1)
#define A2 ARG(2)

#ifndef MAXVARS
#define MAXVARS 99999
#endif

char*j,*k,*BUF[]={"false","null","true"},*MAPI[MAXVARS],NB[22];
l i,MAPN,MAP[MAXVARS],r(l);
size_t z;

l p(){
	l*f,v=0;char*t=0,u[2]={0},c;

	// strip whitespace
	W(SPN(j,"\t\n\f\r {}[]():#"))I(*j++==35)W(*j-10)++j;

	// check for numbers
	W(isdigit(*j))t="",v=v*10+*j++-48;
	I(t)R 1|v<<4;

	// check for variables
	W(islower(c=*j)||isdigit(c)|c==95)t?++j:(t=j);
	I(t)R(l)NDUP(t,j-t)|4;

	// check for strings
	t=++j;
	I(c==39|c==34){W(*j++!=c);R(l)NDUP(t,j-t-1)|2;}

	// functions
	*u=c;
	W(isupper(c)&&isupper(*j)|*j==95)++j;
	I(SPN(u,"TFN"))R c-84?c-70?8:4:12;

	v=8|(l)(f=A(5*sizeof(l)-1));
	*f++=c;

	I(c-82&&c-80)
	I(*f++=p(),!SPN(u,"EBC`Q!LDOA~"))
	I(*f++=p(),SPN(u,"GIS"))
	I(*f++=p(),c>82)*f=p();
	R v;
}

char*s(l v){R v&2?S(v):v&1?sprintf(NB,"%lld",v>>4),NB:v<13?BUF[v/5]:s(r(v));}
l b(l v){R 0<v&v<13?v==12:v&2?0!=*S(v):v&1||b(r(v));}
l n(l v){R v&1?v>>4:v&2?strtoll(S(v),0,10):v<13?v==12:n(r(v));}

l r(l v) {
	l t,t2;
	FILE* file;	
	size_t tmp,cap,len;

	I(v<13|v&3)R v;
	I(v&4){
		for(t=0;t<MAPN;++t)I(!CMP(S(v),MAPI[t]))R MAP[t];
		die("unknown variable %s",S(v));
	}

	switch(ARG(0)){
	C(82)1|rand()&0xfffff0
	C(80)j=0,getline(&j,&z,stdin),2|(l)strdup(j)

	C('E')j=s(A1),r(p())
	C('B')A1
	C('C')r(r(A1))
	D'`': 
		file=popen(s(A1),"r");
		j=A(cap=2047);
		len=0;
		while ((tmp = fread(j + len, 1, cap - len, file)))
			if ((len += tmp) == cap) j = realloc(j, cap *= 2);
		return 2|(l)strdup(j)
	C('Q')exit(n(A1)),0
	C('!')4+8*!b(A1)
	C('L')1|L(s(A1))<<4
	C('D')
		(1&(t=r(A1))?P("Number(%lld)",t>>4):
			2&t?P("String(%s)",S(t)):
			8-t?t<13?P("Boolean(%s)",t-4?"true":"false"):P("<?>"):P("Null()")),
		P("\n"),
		t
	C('O')z=L(j=s(A1)),P(!z||j[z-1]-92?"%2$s\n":"%.*s",(int)z-1,j),8
	C('A')1&(t=r(A1))?*(j=A(1))=t>>4,2|(l)j:1|*S(t)<<4
	C('~')1|-n(A1)<<4
	C('+')
		2&(t=r(A1))?
			j=S(t),k=s(A2),2|(l)CAT(CAT(A(L(j)+L(k)),j),k):
			t+n(A2)*16

	C('-')1|(n(A1)-n(A2))<<4
	D'*':
		I(1&(t=r(A1)))R 1|(t>>4)*n(A2)*16;
		k=A(L(j=S(t))*(t2=n(A2)));
		W(t2--)CAT(k,j);
		R 2|(l)k
	C('/')1|(n(A1) / n(A2))<<4
	C('%')1|(n(A1) % n(A2))<<4
	C('^')1|(int)pow(n(A1), n(A2))<<4

	D'<':
	C('>')
		(i=1&(t=r(A1))?(t>>4)-n(A2):
			2&t?CMP(S(t),s(A2)):
			(t/12)-b(A2)),NEWBOOL(ARG(0)=='<'?i<0:i>0)

	C('?')NEWBOOL((t=r(A1))==(t2=r(A2))||t&t2&2&&!CMP(S(t),S(t2)))
	C('&')b(t=r(A1))?r(A2):t
	C('|')b(t=r(A1))?t:r(A2)
	C(';')r(A1),r(A2)
	D'=':
		for(t=0,j=S(A1);t<MAPN&&CMP(j,MAPI[t]);++t);
		t-MAPN?0:MAPN++;R MAPI[t]=j,MAP[t]=r(A2)
	D'W':W(b(A1))r(A2);R 8
	C('I')r(ARG(2+b(A1)))
	C('G')2|(l)NDUP(s(A1)+n(A2),n(ARG(3)))
	C('S')
		j=s(A1),
		t=n(A2),
		t2=n(ARG(3)),
		k=s(ARG(4)),
		2|(l)CAT(CAT(strncat(A(L(j)+L(k)+t2),j,t),k),j+t+t2)
	;default:
		die("invalid character '%c'",(int)ARG(0));
	}
}

int main(int argc, char**argv){
	srand(time(0));
	if(!argc)fprintf(stderr,"usage: %s (-e 'program' | -f file)\n",*argv),exit(1);
	if(argv[1][1]-'f')j=argv[2]; // ie if it's `-e`
	else getdelim(&j,&z,'\0',fopen(argv[2],"r"));
	r(p());
}
