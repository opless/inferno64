#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"

#define DOT			((uintptr)code)

#define	RESCHED 1	/* check for interpreter reschedule */

// this is needed ?
void	(*comvec)(void);


#ifdef NOPE
enum
{
	NOPE	= 0,

	NMACRO
};

static	uchar*	code;
static	uchar*	base;
static	uintptr*	patch;
static	int	pass;
static	Module*	mod;
static	uchar*	tinit;
static	uintptr*	litpool;
static	int	nlit;
static	void	macfrp(void);
static	void	macret(void);
static	void	maccase(void);
static	void	maccolr(void);
static	void	macmcal(void);
static	void	macfram(void);
static	void	macmfra(void);
static	void	macrelq(void);
static	uintptr	macro[NMACRO];
void	(*comvec)(void);
extern	void	das(uchar*, int);

#define T(r)	*((void**)(R.r))

struct
{
	int	idx;
	void	(*gen)(void);
} mactab[] =
{
	MacFRP,		macfrp,		/* decrement and free pointer */
	MacRET,		macret,		/* return instruction */
	MacCASE,	maccase,	/* case instruction */
	MacCOLR,	maccolr,	/* increment and color pointer */
	MacMCAL,	macmcal,	/* mcall bottom half */
	MacFRAM,	macfram,	/* frame instruction */
	MacMFRA,	macmfra,	/* punt mframe because t->initialize==0 */
	MacRELQ,		macrelq,	/* reschedule */
};

static void
bounds(void)
{
	error(exBounds);
}

static void
rdestroy(void)
{
	destroy(R.s);
}

static void
rmcall(void)
{
	error(exCompile);
}

static void
rmfram(void)
{
	error(exCompile);
}

static int
bc(int o)
{
	if(o < 127 && o > -128)
		return 1;
	return 0;
}

static void
urk(void)
{
	error(exCompile);
}

static void
genb(uchar o)
{
}

static void
gen2(uchar o1, uchar o2)
{
}

static void
genw(uintptr o)
{

}

static void
modrm(int inst, uintptr disp, int rm, int r)
{

}

static void
con(uintptr o, int r)
{

}

static void
opwld(Inst *i, int mi, int r)
{

}

static void
opwst(Inst *i, int mi, int r)
{

}

static void
bra(uintptr dst, int op)
{

}

static void
rbra(uintptr dst, int op)
{

}

static void
literal(uintptr imm, int roff)
{

}

static void
punt(Inst *i, int m, void (*fn)(void))
{


}

static void
mid(Inst *i, uchar mi, int r)
{

}

static void
arith(Inst *i, int op2, int rm)
{

}

static void
arithb(Inst *i, int op2)
{

}

static void
shift(Inst *i, int ld, int st, int op, int r)
{

}

static void
arithf(Inst *i, int op)
{

}

static void
cmpl(int r, uintptr v)
{

}

static int
swapbraop(int b)
{

}

static void
schedcheck(Inst *i)
{

}

static void
cbra(Inst *i, int jmp)
{

}

static void
cbral(Inst *i, int jmsw, int jlsw, int mode)
{

}

static void
cbrab(Inst *i, int jmp)
{

}

static void
cbraf(Inst *i, int jmp)
{

}

static void
comcase(Inst *i, int w)
{

}

/* TODO this is buggy */
static void
comcasel(Inst *i)
{
	int l;
	WORD *t, *e;

	t = (WORD*)(mod->origmp+i->d.ind+sizeof(LONG)/*8*/);
	l = t[-2];
	if(pass == 0) {
		if(l >= 0)
			t[-2] = -l-1;	/* Mark it not done */
		return;
	}
	if(l >= 0)			/* Check pass 2 done */
		return;
	t[-2] = -l-1;			/* Set real count */
	e = t + t[-2]*6;
	while(t < e) {
		t[4] = (uintptr)base + patch[t[4]];
		t += 6;
	}
	t[0] = (uintptr)base + patch[t[0]];
}

static void
commframe(Inst *i)
{
	int o;
	uchar *punt, *mlnil;

	opwld(i, Oldw, RAX);
	cmpl(RAX, (ulong)H);
	gen2(Ojeqb, 0);
	mlnil = code - 1;
	if((i->add&ARM) == AXIMM) {
		o = OA(Modlink, links)+i->reg*sizeof(Modl)+O(Modl, frame);
		modrm(Oldw, o, RAX, RTA);
	} else {
		gen2(Oldw, (3<<6)|(RTMP<<3)|RAX);	// MOVL	AX, RTMP
		mid(i, Oldw, RCX);			// index
		gen2(Olea, (0<<6)|(0<<3)|4);		// lea	(AX)(RCX*8)
		genb((3<<6)|(RCX<<3)|RAX);		// assumes sizeof(Modl) == 8 hence 3
		o = OA(Modlink, links)+O(Modl, frame);
		modrm(Oldw, o, RAX, RTA);		// frame
		genb(OxchgAX+RTMP);			// get old AX back
	}
	modrm(0x83, O(Type, initialize), RTA, 7);
	genb(0);
	gen2(Ojneb, 0);
	punt = code - 1;
	genb(OxchgAX+RTA);
	opwst(i, Olea, RTA);
	*mlnil = code-mlnil-1;
	rbra(macro[MacMFRA], Ocall);
	rbra(patch[i-mod->prog+1], Ojmp);

	*punt = code-punt-1;
	rbra(macro[MacFRAM], Ocall);
	opwst(i, Ostw, RCX);
}

static void
commcall(Inst *i)
{
	uchar *mlnil;

	con((uintptr)&R, RTMP);			// MOVL	$R, RTMP
	opwld(i, Oldw, RCX);
	modrm(Omov, O(Frame, lr), RCX, 0);	// MOVL $.+1, lr(CX)	f->lr = R.PC
	genw((uintptr)base+patch[i-mod->prog+1]);
	modrm(Ostw, O(Frame, fp), RCX, RFP); 	// MOVL RFP, fp(CX)	f->fp = R.FP
	modrm(Oldw, O(REG, M), RTMP, RTA);	// MOVL R.M, RTA
	modrm(Ostw, O(Frame, mr), RCX, RTA);	// MOVL RTA, mr(CX) 	f->mr = R.M
	opwst(i, Oldw, RTA);			// MOVL ml, RTA
	cmpl(RTA, (ulong)H);
	gen2(Ojeqb, 0);
	mlnil = code - 1;
	if((i->add&ARM) == AXIMM)
		modrm(Oldw, OA(Modlink, links)+i->reg*sizeof(Modl)+O(Modl, u.pc), RTA, RAX);
	else {
		genb(Opushl+RCX);
		mid(i, Oldw, RCX);		// index
		gen2(Olea, (0<<6)|(0<<3)|4);	// lea	(RTA)(RCX*8)
		genb((3<<6)|(RCX<<3)|RTA);	// assumes sizeof(Modl) == 8 hence 3
		modrm(Oldw, OA(Modlink, links)+O(Modl, u.pc), RAX, RAX);
		genb(Opopl+RCX);
	}
	*mlnil = code-mlnil-1;
	rbra(macro[MacMCAL], Ocall);
}

static void
larith(Inst *i, int op, int opc)
{
	opwld(i, Olea, RTMP);
	mid(i, Olea, RTA);
	modrm(Oldw, 0, RTA, RAX);	// MOVL	0(RTA), AX
	modrm(op, 0, RTMP, RAX);	// ADDL 0(RTMP), AX
	modrm(Oldw, 4, RTA, RCX);	// MOVL 4(RTA), CX
	modrm(opc, 4, RTMP, RCX);	// ADCL 4(RTMP), CX
	if((i->add&ARM) != AXNON)
		opwst(i, Olea, RTA);
	modrm(Ostw, 0, RTA, RAX);
	modrm(Ostw, 4, RTA, RCX);
}

static void
shll(Inst *i)
{
	uchar *label, *label1;

	opwld(i, Oldw, RCX);
	mid(i, Olea, RTA);
	gen2(Otestib, (3<<6)|(0<<3)|RCX);
	genb(0x20);
	gen2(Ojneb, 0);
	label = code-1;
	modrm(Oldw, 0, RTA, RAX);
	modrm(Oldw, 4, RTA, RBX);
	genb(0x0f);
	gen2(Oshld, (3<<6)|(RAX<<3)|RBX);
	gen2(Oshl, (3<<6)|(4<<3)|RAX);
	gen2(Ojmpb, 0);
	label1 = code-1;
	*label = code-label-1;
	modrm(Oldw, 0, RTA, RBX);
	con(0, RAX);
	gen2(Oshl, (3<<6)|(4<<3)|RBX);
	*label1 = code-label1-1;
	opwst(i, Olea, RTA);
	modrm(Ostw, 0, RTA, RAX);
	modrm(Ostw, 4, RTA, RBX);
}

static void
shrl(Inst *i)
{
	uchar *label, *label1;

	opwld(i, Oldw, RCX);
	mid(i, Olea, RTA);
	gen2(Otestib, (3<<6)|(0<<3)|RCX);
	genb(0x20);
	gen2(Ojneb, 0);
	label = code-1;
	modrm(Oldw, 0, RTA, RAX);
	modrm(Oldw, 4, RTA, RBX);
	genb(0x0f);
	gen2(Oshrd, (3<<6)|(RBX<<3)|RAX);
	gen2(Osar, (3<<6)|(7<<3)|RBX);
	gen2(Ojmpb, 0);
	label1 = code-1;
	*label = code-label-1;
	modrm(Oldw, 4, RTA, RBX);
	gen2(Oldw, (3<<6)|(RAX<<3)|RBX);
	gen2(Osarimm, (3<<6)|(7<<3)|RBX);
	genb(0x1f);
	gen2(Osar, (3<<6)|(7<<3)|RAX);
	*label1 = code-label1-1;
	opwst(i, Olea, RTA);
	modrm(Ostw, 0, RTA, RAX);
	modrm(Ostw, 4, RTA, RBX);
}

static
void
compdbg(void)
{
	print("%s:%zd@%.8zx\n", R.M->m->name, *(uintptr*)R.m, *(uintptr*)R.s);
}

static void
comp(Inst *i)
{
	int r;
	WORD *t, *e;
	char buf[64];

	if(0) {
		Inst xx;
		xx.add = AXIMM|SRC(AIMM);
		xx.s.imm = (uintptr)code;
		xx.reg = i-mod->prog;
		punt(&xx, SRCOP, compdbg);
	}

	switch(i->op) {
	default:
		snprint(buf, sizeof buf, "%s compile, no '%D'", mod->name, i);
		error(buf);
		break;
	case IMCALL:
		if((i->add&ARM) == AXIMM)
			commcall(i);
		else
			punt(i, SRCOP|DSTOP|THREOP|WRTPC|NEWPC, optab[i->op]);
		break;
	case ISEND:
	case IRECV:
	case IALT:
		punt(i, SRCOP|DSTOP|TCHECK|WRTPC, optab[i->op]);
		break;
	case ISPAWN:
		punt(i, SRCOP|DBRAN, optab[i->op]);
		break;
	case IBNEC:
	case IBEQC:
	case IBLTC:
	case IBLEC:
	case IBGTC:
	case IBGEC:
		punt(i, SRCOP|DBRAN|NEWPC|WRTPC, optab[i->op]);
		break;
	case ICASEC:
		comcase(i, 0);
		punt(i, SRCOP|DSTOP|NEWPC, optab[i->op]);
		break;
	case ICASEL:
		comcasel(i);
		punt(i, SRCOP|DSTOP|NEWPC, optab[i->op]);
		break;
	case IADDC:
	case IMULL:
	case IDIVL:
	case IMODL:
	case IMNEWZ:
	case ILSRW:
	case ILSRL:
		punt(i, SRCOP|DSTOP|THREOP, optab[i->op]);
		break;
	case ILOAD:
	case INEWA:
	case INEWAZ:
	case INEW:
	case INEWZ:
	case ISLICEA:
	case ISLICELA:
	case ICONSB:
	case ICONSW:
	case ICONSL:
	case ICONSF:
	case ICONSM:
	case ICONSMP:
	case ICONSP:
	case IMOVMP:
	case IHEADMP:
	case IHEADL:
	case IINSC:
	case ICVTAC:
	case ICVTCW:
	case ICVTWC:
	case ICVTLC:
	case ICVTCL:
	case ICVTFC:
	case ICVTCF:
	case ICVTRF:
	case ICVTFR:
	case ICVTWS:
	case ICVTSW:
	case IMSPAWN:
	case ICVTCA:
	case ISLICEC:
	case INBALT:
		punt(i, SRCOP|DSTOP, optab[i->op]);
		break;
	case INEWCM:
	case INEWCMP:
		punt(i, SRCOP|DSTOP|THREOP, optab[i->op]);
		break;
	case IMFRAME:
		if((i->add&ARM) == AXIMM)
			commframe(i);
		else
			punt(i, SRCOP|DSTOP|THREOP, optab[i->op]);
		break;
	case INEWCB:
	case INEWCW:
	case INEWCF:
	case INEWCP:
	case INEWCL:
		punt(i, DSTOP|THREOP, optab[i->op]);
		break;
	case IEXIT:
		punt(i, 0, optab[i->op]);
		break;
	case ICVTBW:
		opwld(i, Oldb, RAX);
		genb(0x0f);
		gen2(0xb6, (3<<6)|(RAX<<3)|RAX);
		opwst(i, Ostw, RAX);
		break;
	case ICVTWB:
		opwld(i, Oldw, RAX);
		opwst(i, Ostb, RAX);
		break;
	case ICVTFW:
		if(1){
			punt(i, SRCOP|DSTOP, optab[i->op]);
			break;
		}
		opwld(i, Omovf, 0);
		opwst(i, 0xdb, 3);
		break;
	case ICVTWF:
		if(1){
			punt(i, SRCOP|DSTOP, optab[i->op]);
			break;
		}
		opwld(i, 0xdb, 0);
		opwst(i, Omovf, 3);
		break;
	case ICVTLF:
		if(1){
			punt(i, SRCOP|DSTOP, optab[i->op]);
			break;
		}
		opwld(i, 0xdf, 5);
		opwst(i, Omovf, 3);
		break;
	case ICVTFL:
		if(1){
			punt(i, SRCOP|DSTOP, optab[i->op]);
			break;
		}
		opwld(i, Omovf, 0);
		opwst(i, 0xdf, 7);
		break;
	case IHEADM:
		opwld(i, Oldw, RAX);
		modrm(Olea, OA(List, data), RAX, RAX);
		goto movm;
	case IMOVM:
		opwld(i, Olea, RAX);
	movm:
		opwst(i, Olea, RBX);
		mid(i, Oldw, RCX);
		genb(OxchgAX+RSI);
		gen2(Oxchg, (3<<6)|(RDI<<3)|RBX);
		genb(Ocld);
		gen2(Orep, Omovsb);
		genb(OxchgAX+RSI);
		gen2(Oxchg, (3<<6)|(RDI<<3)|RBX);
		break;
	case IRET:
		rbra(macro[MacRET], Ojmp);
		break;
	case IFRAME:
		if(UXSRC(i->add) != SRC(AIMM)) {
			punt(i, SRCOP|DSTOP, optab[i->op]);
			break;
		}
		tinit[i->s.imm] = 1;
		con((uintptr)mod->type[i->s.imm], RTA);
		rbra(macro[MacFRAM], Ocall);
		opwst(i, Ostw, RCX);
		break;
	case ILEA:
		if(UXSRC(i->add) == SRC(AIMM)) {
			gen2(Ojmpb, IBY2WD/*4*/);
			genw(i->s.imm);
			con((uintptr)(code-IBY2WD/*4*/), RAX);
		}
		else
			opwld(i, Olea, RAX);
		opwst(i, Ostw, RAX);
		break;
	case IHEADW:
		opwld(i, Oldw, RAX);
		modrm(Oldw, OA(List, data), RAX, RAX);
		opwst(i, Ostw, RAX);
		break;
	case IHEADF:
		opwld(i, Oldw, RAX);
		modrm(Omovf, OA(List, data), RAX, 0);
		opwst(i, Omovf, 3);
		break;
	case IHEADB:
		opwld(i, Oldw, RAX);
		modrm(Oldb, OA(List, data), RAX, RAX);
		opwst(i, Ostb, RAX);
		break;
	case ITAIL:
		opwld(i, Oldw, RAX);
		modrm(Oldw, O(List, tail), RAX, RBX);
		goto movp;
	case IMOVP:
	case IHEADP:
		opwld(i, Oldw, RBX);
		if(i->op == IHEADP)
			modrm(Oldw, OA(List, data), RBX, RBX);
	movp:
		cmpl(RBX, (ulong)H);
		gen2(Ojeqb, 0x05);
		rbra(macro[MacCOLR], Ocall);
		opwst(i, Oldw, RAX);
		opwst(i, Ostw, RBX);
		rbra(macro[MacFRP], Ocall);
		break;
	case ILENA:
		opwld(i, Oldw, RBX);
		con(0, RAX);
		cmpl(RBX, (ulong)H);
		gen2(Ojeqb, 0x02);
		modrm(Oldw, O(Array, len), RBX, RAX);
		opwst(i, Ostw, RAX);
		break;
	case ILENC:
		opwld(i, Oldw, RBX);
		con(0, RAX);
		cmpl(RBX, (ulong)H);
		gen2(Ojeqb, 0x09);
		modrm(Oldw, O(String, len), RBX, RAX);
		cmpl(RAX, 0);
		gen2(Ojgeb, 0x02);
		gen2(Oneg, (3<<6)|(3<<3)|RAX);
		opwst(i, Ostw, RAX);
		break;
	case ILENL:
		con(0, RAX);
		opwld(i, Oldw, RBX);
		cmpl(RBX, (ulong)H);
		gen2(Ojeqb, 0x05);
		modrm(Oldw, O(List, tail), RBX, RBX);
		genb(Oincr+RAX);
		gen2(Ojmpb, 0xf6);
		opwst(i, Ostw, RAX);
		break;
	case IBEQF:
		cbraf(i, Ojeql);
		break;
	case IBNEF:
		cbraf(i, Ojnel);
		break;
	case IBLEF:
		cbraf(i, Ojlsl);
		break;
	case IBLTF:
		cbraf(i, Ojcsl);
		break;
	case IBGEF:
		cbraf(i, Ojccl);
		break;
	case IBGTF:
		cbraf(i, Ojhil);
		break;
	case IBEQW:
		cbra(i, Ojeql);
		break;
	case IBLEW:
		cbra(i, Ojlel);
		break;
	case IBNEW:
		cbra(i, Ojnel);
		break;
	case IBGTW:
		cbra(i, Ojgtl);
		break;
	case IBLTW:
		cbra(i, Ojltl);
		break;
	case IBGEW:
		cbra(i, Ojgel);
		break;
	case IBEQB:
		cbrab(i, Ojeql);
		break;
	case IBLEB:
		cbrab(i, Ojlsl);
		break;
	case IBNEB:
		cbrab(i, Ojnel);
		break;
	case IBGTB:
		cbrab(i, Ojhil);
		break;
	case IBLTB:
		cbrab(i, Ojbl);
		break;
	case IBGEB:
		cbrab(i, Ojael);
		break;
	case ISUBW:
		arith(i, 0x29, 5);
		break;
	case ISUBB:
		arithb(i, 0x28);
		break;
	case ISUBF:
		arithf(i, 5);
		break;
	case IADDW:
		arith(i, 0x01, 0);
		break;
	case IADDB:
		arithb(i, 0x00);
		break;
	case IADDF:
		arithf(i, 0);
		break;
	case IORW:
		arith(i, 0x09, 1);
		break;
	case IORB:
		arithb(i, 0x08);
		break;
	case IANDW:
		arith(i, 0x21, 4);
		break;
	case IANDB:
		arithb(i, 0x20);
		break;
	case IXORW:
		arith(i, Oxor, 6);
		break;
	case IXORB:
		arithb(i, 0x30);
		break;
	case ISHLW:
		shift(i, Oldw, Ostw, 0xd3, 4);
		break;
	case ISHLB:
		shift(i, Oldb, Ostb, 0xd2, 4);
		break;
	case ISHRW:
		shift(i, Oldw, Ostw, 0xd3, 7);
		break;
	case ISHRB:
		shift(i, Oldb, Ostb, 0xd2, 5);
		break;
	case IMOVF:
		opwld(i, Omovf, 0);
		opwst(i, Omovf, 3);
		break;
	case INEGF:
		opwld(i, Omovf, 0);
		genb(0xd9);
		genb(0xe0);
		opwst(i, Omovf, 3);
		break;
	case IMOVB:
		opwld(i, Oldb, RAX);
		opwst(i, Ostb, RAX);
		break;
	case IMOVW:
	case ICVTLW:			// Little endian
		if(UXSRC(i->add) == SRC(AIMM)) {
			opwst(i, Omov, RAX);
			genw(i->s.imm);
			break;
		}
		opwld(i, Oldw, RAX);
		opwst(i, Ostw, RAX);
		break;
	case ICVTWL:
		opwst(i, Olea, RTMP);
		opwld(i, Oldw, RAX);
		modrm(Ostw, 0, RTMP, RAX);
		genb(0x99);
		modrm(Ostw, 4, RTMP, RDX);
		break;
	case ICALL:
		if(UXDST(i->add) != DST(AIMM))
			opwst(i, Oldw, RTA);
		opwld(i, Oldw, RAX);
		modrm(Omov, O(Frame, lr), RAX, 0);	// MOVL $.+1, lr(AX)
		genw((uintptr)base+patch[i-mod->prog+1]);
		modrm(Ostw, O(Frame, fp), RAX, RFP); 	// MOVL RFP, fp(AX)
		gen2(Oldw, (3<<6)|(RFP<<3)|RAX);	// MOVL AX,RFP
		if(UXDST(i->add) != DST(AIMM)){
			gen2(Ojmprm, (3<<6)|(4<<3)|RTA);
			break;
		}
		/* no break */
	case IJMP:
		if(RESCHED)
			schedcheck(i);
		rbra(patch[i->d.ins-mod->prog], Ojmp);
		break;
	case IMOVPC:
		opwst(i, Omov, RAX);
		genw(patch[i->s.imm]+(uintptr)base);
		break;
	case IGOTO:
		opwst(i, Olea, RBX);
		opwld(i, Oldw, RAX);
		gen2(Ojmprm, (0<<6)|(4<<3)|4);
		genb((2<<6)|(RAX<<3)|RBX);

		if(pass == 0)
			break;

		t = (WORD*)(mod->origmp+i->d.ind);
		e = t + t[-1];
		t[-1] = 0;
		while(t < e) {
			t[0] = (uintptr)base + patch[t[0]];
			t++;
		}
		break;
	case IMULF:
		arithf(i, 1);
		break;
	case IDIVF:
		arithf(i, 7);
		break;
	case IMODW:
	case IDIVW:
	case IMULW:
		mid(i, Oldw, RAX);
		opwld(i, Oldw, RTMP);
		if(i->op == IMULW)
			gen2(0xf7, (3<<6)|(4<<3)|RTMP);
		else {
			genb(Ocdq);
			gen2(0xf7, (3<<6)|(7<<3)|RTMP);	// IDIV AX, RTMP
			if(i->op == IMODW)
				genb(0x90+RDX);		// XCHG	AX, DX
		}
		opwst(i, Ostw, RAX);
		break;
	case IMODB:
	case IDIVB:
	case IMULB:
		mid(i, Oldb, RAX);
		opwld(i, Oldb, RTMP);
		if(i->op == IMULB)
			gen2(0xf6, (3<<6)|(4<<3)|RTMP);
		else {
			genb(Ocdq);
			gen2(0xf6, (3<<6)|(7<<3)|RTMP);	// IDIV AX, RTMP
			if(i->op == IMODB)
				genb(0x90+RDX);		// XCHG	AX, DX
		}
		opwst(i, Ostb, RAX);
		break;
	case IINDX:
		opwld(i, Oldw, RTMP);			// MOVW	xx(s), BX

		if(bflag){
			opwst(i, Oldw, RAX);
			modrm(0x3b, O(Array, len), RTMP, RAX);	/* CMP index, len */
			gen2(0x72, 5);		/* JB */
			bra((uintptr)bounds, Ocall);
			modrm(Oldw, O(Array, t), RTMP, RTA);
			modrm(0xf7, O(Type, size), RTA, 5);		/* IMULL AX, xx(t) */
		}
		else{
			modrm(Oldw, O(Array, t), RTMP, RAX);	// MOVW	t(BX), AX
			modrm(Oldw, O(Type, size), RAX, RAX);	// MOVW size(AX), AX
			if(UXDST(i->add) == DST(AIMM)) {
				gen2(0x69, (3<<6)|(RAX<<3)|0);
				genw(i->d.imm);
			}
			else
				opwst(i, 0xf7, 5);		// IMULL AX,xx(d)
		}

		modrm(0x03, O(Array, data), RBX, RAX);	// ADDL data(BX), AX
		r = RMP;
		if((i->add&ARM) == AXINF)
			r = RFP;
		modrm(Ostw, i->reg, r, RAX);
		break;
	case IINDB:
		r = 0;
		goto idx;
	case IINDF:
	case IINDL:
		r = 3;
		goto idx;
	case IINDW:
		r = 2;
	idx:
		opwld(i, Oldw, RAX);
		opwst(i, Oldw, RTMP);
		if(bflag){
			modrm(0x3b, O(Array, len), RAX, RTMP);	/* CMP index, len */
			gen2(0x72, 5);		/* JB */
			bra((uintptr)bounds, Ocall);
		}
		modrm(Oldw, O(Array, data), RAX, RAX);
		gen2(Olea, (0<<6)|(0<<3)|4);		/* lea	(AX)(RTMP*r) */
		genb((r<<6)|(RTMP<<3)|RAX);
		r = RMP;
		if((i->add&ARM) == AXINF)
			r = RFP;
		modrm(Ostw, i->reg, r, RAX);
		break;
	case IINDC:
		opwld(i, Oldw, RAX);			// string
		mid(i, Oldw, RBX);			// index
		if(bflag){
			modrm(Oldw, O(String, len), RAX, RTA);
			cmpl(RTA, 0);
			gen2(Ojltb, 16);
			gen2(0x3b, (3<<6)|(RBX<<3)|RTA);	/* cmp index, len */
			gen2(0x72, 5);		/* JB */
			bra((uintptr)bounds, Ocall);
			genb(0x0f);
			gen2(Omovzxb, (1<<6)|(0<<3)|4);
			gen2((0<<6)|(RBX<<3)|RAX, O(String, data));
			gen2(Ojmpb, sizeof(Rune)==4? 10: 11);
			gen2(Oneg, (3<<6)|(3<<3)|RTA);
			gen2(0x3b, (3<<6)|(RBX<<3)|RTA);	/* cmp index, len */
			gen2(0x73, 0xee);		/* JNB */
			if(sizeof(Rune) == 4){
				gen2(Oldw, (1<<6)|(0<<3)|4);
				gen2((2<<6)|(RBX<<3)|RAX, O(String, data));
			}else{
				genb(0x0f);
				gen2(Omovzxw, (1<<6)|(0<<3)|4);
				gen2((1<<6)|(RBX<<3)|RAX, O(String, data));
			}
			opwst(i, Ostw, RAX);
			break;
		}
		modrm(Ocmpi, O(String, len), RAX, 7);
		genb(0);
		gen2(Ojltb, 7);
		genb(0x0f);
		gen2(Omovzxb, (1<<6)|(0<<3)|4);		/* movzbx 12(AX)(RBX*1), RAX */
		gen2((0<<6)|(RBX<<3)|RAX, O(String, data));
		if(sizeof(Rune) == 4){
			gen2(Ojmpb, 4);
			gen2(Oldw, (1<<6)|(0<<3)|4);		/* movl 12(AX)(RBX*4), RAX */
			gen2((2<<6)|(RBX<<3)|RAX, O(String, data));
		}else{
			gen2(Ojmpb, 5);
			genb(0x0f);
			gen2(Omovzxw, (1<<6)|(0<<3)|4);		/* movzwx 12(AX)(RBX*2), RAX */
			gen2((1<<6)|(RBX<<3)|RAX, O(String, data));
		}
		opwst(i, Ostw, RAX);
		break;
	case ICASE:
		comcase(i, 1);
		break;
	case IMOVL:
		opwld(i, Olea, RTA);
		opwst(i, Olea, RTMP);
		modrm(Oldw, 0, RTA, RAX);
		modrm(Ostw, 0, RTMP, RAX);
		modrm(Oldw, 4, RTA, RAX);
		modrm(Ostw, 4, RTMP, RAX);
		break;
	case IADDL:
		larith(i, 0x03, 0x13);
		break;
	case ISUBL:
		larith(i, 0x2b, 0x1b);
		break;
	case IORL:
		larith(i, 0x0b, 0x0b);
		break;
	case IANDL:
		larith(i, 0x23, 0x23);
		break;
	case IXORL:
		larith(i, 0x33, 0x33);
		break;
	case IBEQL:
		cbral(i, Ojnel, Ojeql, ANDAND);
		break;
	case IBNEL:
		cbral(i, Ojnel, Ojnel, OROR);
		break;
	case IBLEL:
		cbral(i, Ojltl, Ojbel, EQAND);
		break;
	case IBGTL:
		cbral(i, Ojgtl, Ojal, EQAND);
		break;
	case IBLTL:
		cbral(i, Ojltl, Ojbl, EQAND);
		break;
	case IBGEL:
		cbral(i, Ojgtl, Ojael, EQAND);
		break;
	case ISHLL:
		shll(i);
		break;
	case ISHRL:
		shrl(i);
		break;
	case IRAISE:
		punt(i, SRCOP|WRTPC|NEWPC, optab[i->op]);
		break;
	case IMULX:
	case IDIVX:
	case ICVTXX:
	case IMULX0:
	case IDIVX0:
	case ICVTXX0:
	case IMULX1:
	case IDIVX1:
	case ICVTXX1:
	case ICVTFX:
	case ICVTXF:
	case IEXPW:
	case IEXPL:
	case IEXPF:
		punt(i, SRCOP|DSTOP|THREOP, optab[i->op]);
		break;
	case ISELF:
		punt(i, DSTOP, optab[i->op]);
		break;
	}
}

static void
preamble(void)
{
	if(comvec)
		return;

	comvec = malloc(32);
	if(comvec == nil)
		error(exNomem);
	code = (uchar*)comvec;

	genb(Opushl+RBX);
	genb(Opushl+RCX);
	genb(Opushl+RDX);
	genb(Opushl+RSI);
	genb(Opushl+RDI);
	con((uintptr)&R, RTMP);
	modrm(Oldw, O(REG, FP), RTMP, RFP);
	modrm(Oldw, O(REG, MP), RTMP, RMP);
	modrm(Ojmprm, O(REG, PC), RTMP, 4);

	segflush(comvec, 32);
}

static void
maccase(void)
{
	uchar *loop, *def, *lab1;

	modrm(Oldw, 0, RSI, RDX);		// n = t[0]
	modrm(Olea, 4, RSI, RSI);		// t = &t[1]
	gen2(Oldw, (3<<6)|(RBX<<3)|RDX);	// MOVL	DX, BX
	gen2(Oshr, (3<<6)|(4<<3)|RBX);		// SHL	BX,1
	gen2(0x01, (3<<6)|(RDX<<3)|RBX);	// ADDL	DX, BX	BX = n*3
	gen2(Opushrm, (0<<6)|(6<<3)|4);
	genb((2<<6)|(RBX<<3)|RSI);		// PUSHL 0(SI)(BX*4)
	loop = code;
	cmpl(RDX, 0);
	gen2(Ojleb, 0);
	def = code-1;
	gen2(Oldw, (3<<6)|(RCX<<3)|RDX);	// MOVL	DX, CX	n2 = n
	gen2(Oshr, (3<<6)|(5<<3)|RCX);		// SHR	CX,1	n2 = n2>>1
	gen2(Oldw, (3<<6)|(RBX<<3)|RCX);	// MOVL	CX, BX
	gen2(Oshr, (3<<6)|(4<<3)|RBX);		// SHL	BX,1
	gen2(0x01, (3<<6)|(RCX<<3)|RBX);	// ADDL	CX, BX	BX = n2*3
	gen2(0x3b, (0<<6)|(RAX<<3)|4);
	genb((2<<6)|(RBX<<3)|RSI);		// CMPL AX, 0(SI)(BX*4)
	gen2(Ojgeb, 0);				// JGE	lab1
	lab1 = code-1;
	gen2(Oldw, (3<<6)|(RDX<<3)|RCX);
	gen2(Ojmpb, loop-code-2);
	*lab1 = code-lab1-1;			// lab1:
	gen2(0x3b, (1<<6)|(RAX<<3)|4);
	gen2((2<<6)|(RBX<<3)|RSI, 4);		// CMPL AX, 4(SI)(BX*4)
	gen2(Ojltb, 0);
	lab1 = code-1;
	gen2(Olea, (1<<6)|(RSI<<3)|4);
	gen2((2<<6)|(RBX<<3)|RSI, 12);		// LEA	12(SI)(RBX*4), RSI
	gen2(0x2b, (3<<6)|(RDX<<3)|RCX);	// SUBL	CX, DX		n -= n2
	gen2(Odecrm, (3<<6)|(1<<3)|RDX);	// DECL	DX		n -= 1
	gen2(Ojmpb, loop-code-2);
	*lab1 = code-lab1-1;			// lab1:
	gen2(Oldw, (1<<6)|(RAX<<3)|4);
	gen2((2<<6)|(RBX<<3)|RSI, 8);		// MOVL 8(SI)(BX*4), AX
	genb(Opopl+RSI);			// ditch default
	genb(Opopl+RSI);
	gen2(Ojmprm, (3<<6)|(4<<3)|RAX);	// JMP*L AX
	*def = code-def-1;			// def:
	genb(Opopl+RAX);			// ditch default
	genb(Opopl+RSI);
	gen2(Ojmprm, (3<<6)|(4<<3)|RAX);
}

static void
macfrp(void)
{
	cmpl(RAX, (ulong)H);			// CMPL AX, $H
	gen2(Ojneb, 0x01);			// JNE	.+1
	genb(Oret);				// RET
	modrm(0x83, O(Heap, ref)-sizeof(Heap), RAX, 7);
	genb(0x01);				// CMP	AX.ref, $1
	gen2(Ojeqb, 0x04);			// JNE	.+4
	modrm(Odecrm, O(Heap, ref)-sizeof(Heap), RAX, 1);
	genb(Oret);				// DEC	AX.ref
						// RET
	con((uintptr)&R, RTMP);			// MOV  $R, RTMP
	modrm(Ostw, O(REG, FP), RTMP, RFP);	// MOVL	RFP, R.FP
	modrm(Ostw, O(REG, s), RTMP, RAX);	// MOVL	RAX, R.s
	bra((uintptr)rdestroy, Ocall);		// CALL rdestroy
	con((uintptr)&R, RTMP);			// MOVL	$R, RTMP
	modrm(Oldw, O(REG, FP), RTMP, RFP);	// MOVL	R.MP, RMP
	modrm(Oldw, O(REG, MP), RTMP, RMP);	// MOVL R.FP, RFP
	genb(Oret);
}

static void
macret(void)
{
	Inst i;
	uchar *s;
	static uintptr lpunt, lnomr, lfrmr, linterp;

	s = code;

	lpunt -= 2;
	lnomr -= 2;
	lfrmr -= 2;
	linterp -= 2;

	con(0, RBX);				// MOVL  $0, RBX
	modrm(Oldw, O(Frame, t), RFP, RAX);	// MOVL  t(FP), RAX
	gen2(Ocmpw, (3<<6)|(RAX<<3)|RBX);	// CMPL  RAX, RBX
	gen2(Ojeqb, lpunt-(code-s));		// JEQ	 lpunt
	modrm(Oldw, O(Type, destroy), RAX, RAX);// MOVL  destroy(RAX), RAX
	gen2(Ocmpw, (3<<6)|(RAX<<3)|RBX);	// CMPL	 RAX, RBX
	gen2(Ojeqb, lpunt-(code-s));		// JEQ	 lpunt
	modrm(Ocmpw, O(Frame, fp), RFP, RBX);	// CMPL	 fp(FP), RBX
	gen2(Ojeqb, lpunt-(code-s));		// JEQ	 lpunt
	modrm(Ocmpw, O(Frame, mr), RFP, RBX);	// CMPL	 mr(FP), RBX
	gen2(Ojeqb, lnomr-(code-s));		// JEQ	 lnomr
	con((uintptr)&R, RTMP);			// MOVL	 $R, RTMP
	modrm(Oldw, O(REG, M), RTMP, RTA);	// MOVL	 R.M, RTA
	modrm(Odecrm, O(Heap, ref)-sizeof(Heap), RTA, 1);
	gen2(Ojneb, lfrmr-(code-s));		// JNE	 lfrmr
	modrm(Oincrm, O(Heap, ref)-sizeof(Heap), RTA, 0);
	gen2(Ojmpb, lpunt-(code-s));		// JMP	 lpunt
	lfrmr = code - s;
	modrm(Oldw, O(Frame, mr), RFP, RTA);	// MOVL	 mr(FP), RTA
	modrm(Ostw, O(REG, M), RTMP, RTA);	// MOVL	 RTA, R.M
	modrm(Oldw, O(Modlink, MP), RTA, RMP);	// MOVL	 MP(RTA), RMP
	modrm(Ostw, O(REG, MP), RTMP, RMP);	// MOVL	 RMP, R.MP
	modrm(Ocmpi, O(Modlink, compiled), RTA, 7);// CMPL $0, M.compiled
	genb(0x00);
	gen2(Ojeqb, linterp-(code-s));		// JEQ	linterp
	lnomr = code - s;
	gen2(Ocallrm, (3<<6)|(2<<3)|RAX);	// CALL* AX
	con((uintptr)&R, RTMP);			// MOVL	 $R, RTMP
	modrm(Ostw, O(REG, SP), RTMP, RFP);	// MOVL  RFP, R.SP
	modrm(Oldw, O(Frame, lr), RFP, RAX);	// MOVL  lr(RFP), RAX
	modrm(Oldw, O(Frame, fp), RFP, RFP);	// MOVL  fp(RFP), RFP
	modrm(Ostw, O(REG, FP), RTMP, RFP);	// MOVL  RFP, R.FP
	gen2(Ojmprm, (3<<6)|(4<<3)|RAX);	// JMP*L AX

	linterp = code - s;			// return to uncompiled code
	gen2(Ocallrm, (3<<6)|(2<<3)|RAX);	// CALL* AX
	con((uintptr)&R, RTMP);			// MOVL	 $R, RTMP
	modrm(Ostw, O(REG, SP), RTMP, RFP);	// MOVL  RFP, R.SP
	modrm(Oldw, O(Frame, lr), RFP, RAX);	// MOVL  lr(RFP), RAX
	modrm(Ostw, O(REG, PC), RTMP, RAX);	// MOVL  RAX, R.PC
	modrm(Oldw, O(Frame, fp), RFP, RFP);	// MOVL  fp(RFP), RFP
	modrm(Ostw, O(REG, FP), RTMP, RFP);	// MOVL  RFP, R.FP
	genb(Opopl+RDI);			// return to uncompiled code
	genb(Opopl+RSI);
	genb(Opopl+RDX);
	genb(Opopl+RCX);
	genb(Opopl+RBX);
	genb(Oret);
						// label:
	lpunt = code - s;

	i.add = AXNON;
	punt(&i, TCHECK|NEWPC, optab[IRET]);
}

static void
maccolr(void)
{
	modrm(Oincrm, O(Heap, ref)-sizeof(Heap), RBX, 0);
	gen2(Oldw, (0<<6)|(RAX<<3)|5);		// INCL	ref(BX)
	genw((uintptr)&mutator);			// MOVL	mutator, RAX
	modrm(Ocmpw, O(Heap, color)-sizeof(Heap), RBX, RAX);
	gen2(Ojneb, 0x01);			// CMPL	color(BX), RAX
	genb(Oret);				// MOVL $propagator,RTMP
	con(propagator, RAX);			// MOVL	RTMP, color(BX)
	modrm(Ostw, O(Heap, color)-sizeof(Heap), RBX, RAX);
	gen2(Ostw, (0<<6)|(RAX<<3)|5);		// can be any !0 value
	genw((uintptr)&nprop);			// MOVL	RBX, nprop
	genb(Oret);
}

static void
macmcal(void)
{
	uchar *label, *mlnil, *interp;

	cmpl(RAX, (ulong)H);
	gen2(Ojeqb, 0);
	mlnil = code - 1;
	modrm(0x83, O(Modlink, prog), RTA, 7);	// CMPL $0, ml->prog
	genb(0x00);
	gen2(Ojneb, 0);				// JNE	patch
	label = code-1;
	*mlnil = code-mlnil-1;
	modrm(Ostw, O(REG, FP), RTMP, RCX);
	modrm(Ostw, O(REG, dt), RTMP, RAX);
	bra((uintptr)rmcall, Ocall);		// CALL rmcall
	con((uintptr)&R, RTMP);			// MOVL	$R, RTMP
	modrm(Oldw, O(REG, FP), RTMP, RFP);
	modrm(Oldw, O(REG, MP), RTMP, RMP);
	genb(Oret);				// RET
	*label = code-label-1;			// patch:
	gen2(Oldw, (3<<6)|(RFP<<3)|RCX);	// MOVL CX, RFP		R.FP = f
	modrm(Ostw, O(REG, M), RTMP, RTA);	// MOVL RTA, R.M
	modrm(Oincrm, O(Heap, ref)-sizeof(Heap), RTA, 0);
	modrm(Oldw, O(Modlink, MP), RTA, RMP);	// MOVL R.M->mp, RMP
	modrm(Ostw, O(REG, MP), RTMP, RMP);	// MOVL RMP, R.MP	R.MP = ml->MP
	modrm(Ocmpi, O(Modlink, compiled), RTA, 7);// CMPL $0, M.compiled
	genb(0x00);
	genb(Opopl+RTA);			// balance call
	gen2(Ojeqb, 0);				// JEQ	interp
	interp = code-1;
	gen2(Ojmprm, (3<<6)|(4<<3)|RAX);	// JMP*L AX
	*interp = code-interp-1;		// interp:
	modrm(Ostw, O(REG, FP), RTMP, RFP);	// MOVL FP, R.FP
	modrm(Ostw, O(REG, PC), RTMP, RAX);	// MOVL PC, R.PC
	genb(Opopl+RDI);			// call to uncompiled code
	genb(Opopl+RSI);
	genb(Opopl+RDX);
	genb(Opopl+RCX);
	genb(Opopl+RBX);
	genb(Oret);
}

static void
macfram(void)
{
	uchar *label;

	con((uintptr)&R, RTMP);			// MOVL	$R, RTMP
	modrm(Oldw, O(REG, SP), RTMP, RAX);	// MOVL	R.SP, AX
	modrm(0x03, O(Type, size), RTA, RAX);	// ADDL size(RCX), RAX
	modrm(0x3b, O(REG, TS), RTMP, RAX);	// CMPL	AX, R.TS
	gen2(0x7c, 0x00);			// JL	.+(patch)
	label = code-1;

	modrm(Ostw, O(REG, s), RTMP, RTA);
	modrm(Ostw, O(REG, FP), RTMP, RFP);	// MOVL	RFP, R.FP
	bra((uintptr)extend, Ocall);		// CALL	extend
	con((uintptr)&R, RTMP);
	modrm(Oldw, O(REG, FP), RTMP, RFP);	// MOVL	R.MP, RMP
	modrm(Oldw, O(REG, MP), RTMP, RMP);	// MOVL R.FP, RFP
	modrm(Oldw, O(REG, s), RTMP, RCX);	// MOVL	R.s, *R.d
	genb(Oret);				// RET
	*label = code-label-1;
	modrm(Oldw, O(REG, SP), RTMP, RCX);	// MOVL	R.SP, CX
	modrm(Ostw, O(REG, SP), RTMP, RAX);	// MOVL	AX, R.SP

	modrm(Ostw, O(Frame, t), RCX, RTA);	// MOVL	RTA, t(CX) f->t = t
	modrm(Omov, REGMOD*4, RCX, 0);     	// MOVL $0, mr(CX) f->mr
	genw(0);
	modrm(Oldw, O(Type, initialize), RTA, RTA);
	gen2(Ojmprm, (3<<6)|(4<<3)|RTA);	// JMP*L RTA
	genb(Oret);				// RET
}

static void
macmfra(void)
{
	con((uintptr)&R, RTMP);			// MOVL	$R, RTMP
	modrm(Ostw, O(REG, FP), RTMP, RFP);
	modrm(Ostw, O(REG, s), RTMP, RAX);	// Save type
	modrm(Ostw, O(REG, d), RTMP, RTA);	// Save destination
	bra((uintptr)rmfram, Ocall);		// CALL rmfram
	con((uintptr)&R, RTMP);			// MOVL	$R, RTMP
	modrm(Oldw, O(REG, FP), RTMP, RFP);
	modrm(Oldw, O(REG, MP), RTMP, RMP);
	genb(Oret);				// RET
}

static void
macrelq(void)
{
	modrm(Ostw, O(REG, FP), RTMP, RFP);	// MOVL FP, R.FP
	genb(Opopl+RAX);
	modrm(Ostw, O(REG, PC), RTMP, RAX);	// MOVL PC, R.PC
	genb(Opopl+RDI);
	genb(Opopl+RSI);
	genb(Opopl+RDX);
	genb(Opopl+RCX);
	genb(Opopl+RBX);
	genb(Oret);
}

void
comd(Type *t)
{
	int i, j, m, c;

	for(i = 0; i < t->np; i++) {
		c = t->map[i];
		j = i<<5;
		for(m = 0x80; m != 0; m >>= 1) {
			if(c & m) {
				modrm(Oldw, j, RFP, RAX);
				rbra(macro[MacFRP], Ocall);
			}
			j += sizeof(WORD*);
		}
	}
	genb(Oret);
}

void
comi(Type *t)
{
	int i, j, m, c;

	con((ulong)H, RAX);
	for(i = 0; i < t->np; i++) {
		c = t->map[i];
		j = i<<5;
		for(m = 0x80; m != 0; m >>= 1) {
			if(c & m)
				modrm(Ostw, j, RCX, RAX);
			j += sizeof(WORD*);
		}
	}
	genb(Oret);
}

void
typecom(Type *t)
{
	int n;
	uchar *tmp;

	if(t == nil || t->initialize != 0)
		return;

	tmp = mallocz(4096*sizeof(uchar), 0);
	if(tmp == nil)
		error(exNomem);

	code = tmp;
	comi(t);
	n = code - tmp;
	code = tmp;
	comd(t);
	n += code - tmp;
	free(tmp);

	code = mallocz(n, 0);
	if(code == nil)
		return;

	t->initialize = code;
	comi(t);
	t->destroy = code;
	comd(t);

	if(cflag > 3)
		print("typ= %.8zx %4d i %.8zx d %.8zx asm=%d\n",
			(uintptr)t, t->size, (uintptr)t->initialize, (uintptr)t->destroy, n);

	segflush(t->initialize, n);
}

static void
patchex(Module *m, uintptr *p)
{
	Handler *h;
	Except *e;

	if((h = m->htab) == nil)
		return;
	for( ; h->etab != nil; h++){
		h->pc1 = p[h->pc1];
		h->pc2 = p[h->pc2];
		for(e = h->etab; e->s != nil; e++)
			e->pc = p[e->pc];
		if(e->pc != -1)
			e->pc = p[e->pc];
	}
}
#endif

int
compile(Module *m, int size, Modlink *ml)
{
	return 0;
}

