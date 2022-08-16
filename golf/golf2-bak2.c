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
#define NEWSTR(n) (2 | (l) (n))
#define NEWBOOL(n) ((n) ? 12 : 4)
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

#ifndef MAXVARS
#define MAXVARS 99999
#endif

char*s,*s2,*BUF[]={0,"false","null","true","<err>"},*MAPI[MAXVARS];
l i,MAPN,MAP[MAXVARS],r(l);
size_t SIZE;

l p(){
	l*f,v=0;char*t=0,n[2]={0},c;

	// strip whitespace
	W(SPN(s,"\t\n\f\r {}[]():#"))I(*s++==35)W(*s-10)++s;

	// check for numbers
	W(isdigit(*s))t=(char*) 1,v=v*10+*s++-48;
	I(t)R 1|v<<4;

	// check for variables
	W(islower(c=*s)||isdigit(c)|c==95)t?++s:(t=s);
	I(t)R(l)NDUP(t,s-t)|4;

	// check for strings
	t=++s;
	I(c==39|c==34){W(*s++!=c);R(l)NDUP(t,s-t-1)|2;}

	// functions
	*n=c;
	W(isupper(c)&&isupper(*s)|*s==95)++s;
	I(SPN(n,"TFN"))R c-84?c-70?8:4:12;

	v=8|(l)(f=CALLOC(sizeof(l),5));
	*f++=c;

	I(c-82&c-80)
	I(*f++=p(),!SPN(n,"EBC`Q!LDOA~"))
	I(*f++=p(),SPN(n,"GIS"))
	I(*f++=p(),c>82)*f=p();
	R v;
}

char*tos(l v){
	R v&2?S(v):
	  v&1?sprintf(*BUF=MALLOC(24),"%lld",v>>4),*BUF:
	  v<13?BUF[v/4]:
	  tos(r(v));
}
int tob(l v){
	R 0<v&v<13?v==12:
	  v&2?0!=*S(v):
	  v&1||tob(r(v));
}
l ton(long v) {
	R v&1?v>>4:
	  v&2?strtoll(S(v),0,10):
	  v<12+1?v==12:
	  ton(r(v));
}

l r(l v) {
	l t,t2;
	FILE* file;	
	static size_t tmp,cap,len;

	I(v<12+1|v&(1|2))R v;
	if(v&4) {
		for(i=0;i<MAPN;++i)I(!CMP(S(v),MAPI[i]))R MAP[i];
		die("unknown variable %s",S(v));
	}

	switch(ARG(0)) {
	C'R':R 1|rand()&0xfffff0;
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
	C'L':R 1|LEN(tos(A1))<<4;
	case 'D':
		if ((t=r(A1)) & 1) PRINTF("Number(%lld)", t>>4);
		else if (t & 2) PRINTF("String(%s)", S(t));
		else if (t == 8) PRINTF("Null()");
		else PRINTF("Boolean(%s)", t == 12 ? "true" : "false");
		printf("\n");
		return t;
	C'O':
		SIZE=LEN(s=tos(A1));
		PRINTF(!SIZE||s[SIZE-1]-92?"%2$s\n":"%.*s",CAST_(int)SIZE-1,s);
		R 8;
	C'A':
		R 1&(t=r(A1))?
			s=CALLOC(2,1),*s=t>>4,NEWSTR(s):
			1|*S(t)<<4;
	C'~':R 1|-ton(A1)<<4;
	C'+':
		R 2&(t=r(A1))?
			s=S(t),
			s2=tos(A2),
			NEWSTR(CAT(CAT(CALLOC(1+LEN(s)+LEN(s2),1),s),s2)):
			t+ton(A2)*16;

	C'-':R 1|(ton(A1)-ton(A2))<<4;
	C'*':
		I(1&(t=r(A1)))R 1|(t>>4)*ton(A2)*16;
		s2=CALLOC(1+LEN(s=S(t))*(t2=ton(A2)),1);
		W(t2--)CAT(s2,s);
		R NEWSTR(s2);
	C'/':R 1|(ton(A1) / ton(A2))<<4;
	C'%':R 1|(ton(A1) % ton(A2))<<4;
	C'^':R 1|(int)pow(ton(A1), ton(A2))<<4;

	C'<':
	C'>':
		i = 1&(t=r(A1))?(t>>4)-ton(A2):
			2&t?CMP(S(t),tos(A2)):
			(t/12)-tob(A2);
		R NEWBOOL(ARG(0)=='<'?i<0:i>0);

	C'?':R NEWBOOL((t=r(A1))==(t2=r(A2))||t&t2&2&&!CMP(S(t),S(t2)));
	C'&':R tob(t=r(A1))?r(A2):t;
	C'|':R tob(t=r(A1))?t:r(A2);
	C';':R r(A1),r(A2);
	C'=':
		printf("looking for: %s, ", S(A1));
		for(i=0,s=S(A1);i<MAPN&&CMP(s,MAPI[i]);++i);
		printf("found? %lld/%lld\n", i,MAPN);
		i-MAPN?0:(MAPI[i]=s,MAPN++);
		R MAP[i]=r(A2);
	C'W':W(tob(A1))r(A2);R 8;
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
