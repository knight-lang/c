// this is *almost* but not exactly `golf.c`
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<math.h>
#include<string.h>
#include<time.h>
#define l long long

#define TNUM 1
#define TSTR 2
#define TVAR 4
#define TFNC 8
#define TRU 12
#define FLS 4
#define NUL 8
#define UNMASK(s) ((s) & ~15)
#define ASSTR(s) ((char *) UNMASK(s))
#define ASNUM(s) ((s) >> 4)
#define NEWNUM(n) (TNUM | (n) << 4)
#define NEWSTR(n) (TSTR | (l) (n))
#define NEWBOOL(n) ((n) ? TRU : FLS)
#define ARG(n) (((l *) UNMASK(v))[n])

#define die(...) (fprintf(stderr,__VA_ARGS__),fputc('\n',stderr),exit(1))

#define W while
#define I if
#define E else
#define C case
#define SPN strspn
#define CMP strcmp
#define LEN strlen
#define NDUP strndup
#define MALLOC malloc
#define CALLOC calloc
#define PRINTF printf
#define CAT strcat
#define R return
#define A1 ARG(1)
#define A2 ARG(2)
#define CAST_(t) (t)

char*s,*s2,*BUF[]={0,"false","null","true","<err>"};
char*MAPI[1000];
l i,MAPN,MAP[1000];
size_t SIZE;

l p(){
	l v,*f;
	char c,*t,n[2]={0,0};

	v=(l)(t=0);

	// strip whitespace
	W(SPN(s,"\t\n\f\r {}[]():#"))I(*s++==35)W(*s-10)++s;

	// check for numbers
	W(isdigit(*s))t=CAST_(char*)1,v=v*10+*s++-48;
	I(t)R TNUM|v<<4;

	// check for variables
	W(islower(c=*s)||isdigit(c)|c==95)t?++s:(t=s);
	I(t)R TVAR|(l)NDUP(t,s-t);

	// check for strings
	t=++s;
	I(c==39|c==34){W(*s++!=c);R TSTR|(l)NDUP(t,s-t-1);}

	// functions
	*n=c;
	W(isupper(c)&&isupper(*s)|*s==95)++s;
	I(SPN(n,"TFN"))R c-84?c-70?NUL:FLS:TRU;

	v=TFNC|(l)(f=CALLOC(sizeof(l),5));
	*f++=c;

	I(c!=82&&c!=80)
	I(*f++=p(),!SPN(n,"EBC`Q!LDOA~"))
	I(*f++=p(),SPN(n,"GIS"))
	I(*f++=p(),c==83)*f=p();
	R v;
}

l r(l);
char*tos(l v){
	R v&TSTR?ASSTR(v):
	  v&TNUM?s=MALLOC(24),sprintf(s,"%lld",v>>4),s:
	  v<TRU+1?BUF[v/4]:
	  tos(r(v));
}
_Bool tob(l v){
	R v<TRU+1?v==TRU:
	  v&TSTR?*ASSTR(v):
	  v&TNUM||tob(r(v));
}
l ton(l v) {
	R v&TNUM?v>>4:
	  v&TSTR?strtoll(ASSTR(v),0,10):
	  v<TRU+1?v==TRU:
	  ton(r(v));
}

l r(l v) {
	l t,t2;
	FILE* file;	
	static size_t tmp,cap,len;

	I(v<TRU+1|v&(TNUM|TSTR))R v;
	if(v&TVAR) {
		for(i=0;i<MAPN;++i)I(!CMP(ASSTR(v),MAPI[i]))R MAP[i];
		die("unknown variable %s",ASSTR(v));
	}

	switch(ARG(0)) {
	C'R':R TNUM|rand()&0xfffff0;
	C'P':R s=0,getline(&s,&SIZE,stdin),NEWSTR(strdup(s));

	C'E':R s=tos(A1),r(p());
	C'B':R A1;
	C'C':R r(r(A1));
	case '`': 
		file=popen(tos(A1),"r");
		s=MALLOC(cap=2048);
		while ((tmp = fread(s + len, 1, cap - len, file)))
			if ((len += tmp) == cap) s = realloc(s, cap *= 2);
		return NEWSTR(s);
	C'Q':exit(ton(A1));
	case'!':R NEWBOOL(!tob(A1));
	C'L':R TNUM|LEN(tos(A1))<<4;
	case 'D':
		if ((t=r(A1)) & TNUM) PRINTF("Number(%lld)", t>>4);
		else if (t & TSTR) PRINTF("String(%s)", ASSTR(t));
		else if (t == NUL) PRINTF("Null()");
		else PRINTF("Boolean(%s)", t == TRU ? "true" : "false");
		return t;
	C'O':
		SIZE=LEN(s=tos(A1));
		PRINTF(SIZE&&s[SIZE-1]-92?"%2$s\n":"%.*s",CAST_(int)SIZE-1,s);
		R NUL;
	C'A':
		R TNUM&(t=r(A1))?
			s=CALLOC(2,1),*s=ASNUM(t),NEWSTR(s):
			TNUM|*ASSTR(t)<<4;
	C'~':R TNUM|-ton(A1)<<4;
	C'+':
		R TSTR&(t=r(A1))?
			s=ASSTR(t),
			s2=tos(A2),
			NEWSTR(CAT(strcpy(MALLOC(1+LEN(s)+LEN(s2)),s),s2)):
			t+ton(A2)*16;
	C'-':R TNUM|(ton(A1)-ton(A2))<<4;
	C'*':
		I(TNUM&(t=r(A1)))R TNUM|t*ton(A2);
		s2=CALLOC(1+LEN(s=ASSTR(t))*(t2=ton(A2)),1);
		W(t2--)CAT(s2,s);
		R NEWSTR(s2);
	C'/':R TNUM|(ton(A1) / ton(A2))<<4;
	C'%':R TNUM|(ton(A1) % ton(A2))<<4;
	C'^':R TNUM|(int)pow(ton(A1), ton(A2))<<4;

	C'<':
	C'>':
		i = TNUM&(t=r(A1))?(t>>4)-ton(A2):
			TSTR&t?CMP(ASSTR(t),tos(A2)):
			(t==FLS)-tob(A2);
		R NEWBOOL(ARG(0)=='<'?i<0:i>0);

	C'?':R NEWBOOL((t=r(A1))==(t2=r(A2))||t&t2&TSTR&&!CMP(ASSTR(t),ASSTR(t2)));
	C'&':R tob(t=r(A1))?r(A2):t;
	C'|':R tob(t=r(A1))?t:r(A2);
	C';':R r(A1),r(A2);
	C'=':
		for(i=0,s=ASSTR(A1);i<MAPN&&CMP(s,MAPI[i]);++i);
		i-MAPN?0:MAPN++;
		R MAPI[i]=s,MAP[i]=r(A2);
	C'W':W(tob(A1))r(A2);R NUL;
	C'I':R r(ARG(tob(A1)?2:3));
	C'G':R NEWSTR(NDUP(tos(A1)+ton(A2),ton(ARG(3))));
	case'S':
		s=tos(A1);
		t=ton(A2);
		t2=ton(ARG(3));
		s2=tos(ARG(4));
		R NEWSTR(CAT(CAT(strncat(
			CALLOC(LEN(s)+LEN(s2)+t2+1,1),s,t),s2),s+t+t2));
	default:
		die("invalid character '%c'",CAST_(char)ARG(0));
	}
}

int main(int argc, char**argv){
	srand(time(0));
	if(!argc)fprintf(stderr,"usage: %s (-e 'program' | -f file)\n",*argv),exit(1);
	if(argv[1][1]-'f')s=argv[2]; // ie if it's `-e`
	else getdelim(&s,&SIZE,'\0',fopen(argv[2],"r"));
	r(p());
}
