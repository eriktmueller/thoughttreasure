/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#ifdef MACOS
#include <console.h>
#include <unix.h>
int getpid()
{
  return 0;
}
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef SUNOS
#include <varargs.h>
#include <values.h>
double drand48();
#else
#include <stdarg.h>
#include <float.h>
#endif

#ifdef SOLARIS
#include <dirent.h>
#endif

#define maxchecking
/*
#define INTEGSYNSEM
#define INTEGSYNSEMANA
 */

/* ThoughtTreasure's registered port number, listed by the IANA.
 * See <URL:http://www.isi.edu/in-notes/iana/assignments/port-numbers>
 */
#define PORT		1832

#define DBGOBJ		0x1
#define DBGDB		0x2
#define DBGPLAN		0x4
#define DBGTREE		0x8
#define DBGUA		0x10
#define DBGSPACE	0x20
#define DBGGS		0x40
#define DBGGEN		0x80
#define DBGGRID		0x100
#define DBGCLOTHING	0x200
#define DBGLEX		0x400
#define DBGGENER	0x800
#define DBGTENSE	0x1000
#define DBGSYNPAR	0x2000
#define DBGSEMPAR	0x4000
#define DBGUNIFY	0x8000

#define DBGALL		0x1fff
#define DBGALLSYN	0x3fff
#define DBGALLSEM	0x5fff
#define DBGALLSYNSEM	0x7fff

#define DBGOFF		0
#define DBGBAD		1
#define DBGOK		2
#define DBGDETAIL	3
#define DBGHYPER	4

#define INTNA		-22222
#define INTNAF		-22222.0
#define INTERR		-22221
#define INTNEGINF	INT_MIN
#define INTPOSINF	INT_MAX

#define LONGNA		-22222L
#define LONGNEGINF	LONG_MIN
#define LONGPOSINF	LONG_MAX
#define SIZENEGINF	       0	/* REALLY: for size_t */
#define SIZEPOSINF	LONG_MAX
#define SIZENA		LONG_MAX

#define FLOATNA		-876543210.0
#define FLOATERR	-123456789.0
#ifdef SUNOS
#define FLOATPOSINF	MAXDOUBLE
#define FLOATNEGINF	(-MAXDOUBLE)
#else
#define FLOATPOSINF	DBL_MAX
#define FLOATNEGINF	(-DBL_MAX)
#endif
typedef double Float;

typedef short Bool;

typedef unsigned char uc;

#define WORDLEN		32
#define DWORDLEN	64
#define FEATLEN		32
#define OBJNAMELEN	128
#define PHRASELEN	128
#define FILENAMELEN	128
#define LINELEN		256
#define SENTLEN		2048
#define TREEITEMLEN	2048
#define PARAGRAPHLEN	4096
#define MAXPARTDEPTH	5
#define MAXISADEPTH	30
#define MAXINTERGRID	10
#define MAXLISTLEN	10
#define MAXTHETAFILLED	MAXLISTLEN
#define DSPLINELEN	79

#define TERM 		'\0'
#define BS		8
#define FF              12
#define SPACE		' '
#define NEWLINE		'\n'
#define CR		'\r'
#define TAB		'\t'
#define SQUOTE		'\''
#define SQUOTE2		180
#define DQUOTE		'"'
#define LPAREN		'('
#define RPAREN		')'
#define LBRACE		'{'
#define RBRACE		'}'
#define LBRACKET	'['
#define RBRACKET	']'
#define LABRACKET	'<'
#define RABRACKET	'>'
#define UNDERLINE	'_'
#define LGUILLEMETS	((uc)'«')
#define RGUILLEMETS	((uc)'»')
#define DASH1		'-'
#define DASH2		((uc)'­')
#define DASH3		((uc)'¦')
#define GROUPPUNC	",-­¦"

#define PUNCT_ENGLISH	"\"\""
#define PUNCT_FRENCH	"«»"
#define TABSIZE_UNIX	8

#define EARTHRADIUS	6366713.0
#define GRAMSPERAPOUND	453.59
#define METERSPERSMILE	1609.3
#define METERSPERFOOT	0.3048
#define METERSPERINCH	(METERSPERFOOT/12.0)
#define TTPI 3.1415926535897932384626433832795028841971
#define SPEEDOFLIGHT	2.99792458e8

#define DAYSPERYEAR	365.2425

#define SECONDSPERMIN	60
#define SECONDSPERHOUR	(SECONDSPERMIN*60)
#define SECONDSPERDAY	(SECONDSPERHOUR*24)

#define SECONDSPERMINL	60L
#define SECONDSPERHOURL	(SECONDSPERMINL*60L)
#define SECONDSPERDAYL	(SECONDSPERHOURL*24L)

#define SECONDSPERMINF	60.0
#define SECONDSPERHOURF	(SECONDSPERMINF*60.0)
#define SECONDSPERDAYF	(SECONDSPERHOURF*24.0)
#define SECONDSPERYEARF	(SECONDSPERDAYF*DAYSPERYEAR)

#define NUMPOS		19
  /* REALLY: 1 + the largest pos/constit index in POSGI/IPOSG. */

#define PHONE_WHITESPACE	" -+."
#define WHITESPACE		" \t\n\r"
#define IsWhitespace(c)		(StringIn((c), WHITESPACE))

#define FT_LANG			"yz"
#define FT_POS			"NHVA9BDKRUx0«»"
#define FT_INITIAL_SOUND	"ÈÌ"
#define FT_LE_MORE		"Qj·"

#define FT_TENSE	"fpisucde"
#define FT_GENDER	"MFC"
#define FT_NUMBER	"SP"
#define FT_PERSON	"123"
#define FT_MOOD		"GJI"
#define FT_DEGREE	"678"
#define FT_ALTER	"ÍÁÄ"
#define FT_MODALITY	"µ"

#define FT_STYLE	"tTa"	/* cf StyleToInt, StyleCongruence */
#define FT_DIALECT	"Àgçîo"	/* cf DialectScore */
#define FT_FREQ		"q¹"
#define FT_FREQ_DICT	"¹?q"
#define FT_CONNOTE	"hñ"
#define FT_ADDRESS	"²×"

#define FT_EPITH_PRED	"ÓÚ" 
#define FT_GR_ADJ	"b" FT_EPITH_PRED
#define FT_GR_ADV_BLK	"°¿ÞðÆ"
#define FT_GR_CONJ_BLK	"w"
#define FT_GR_CONJ_ENB	"©"
#define FT_ARTICLE	"kÉ"
#define FT_GR_N		"m" FT_ARTICLE
#define FT_GR_V		"lÔô"
#define FT_FILTER	"ï" FT_GR_ADJ FT_GR_ADV_BLK FT_GR_CONJ_BLK \
			FT_GR_CONJ_ENB FT_GR_N FT_GR_V
#define FT_GRAMMAR	"®" FT_FILTER
#define FT_PARUNIV	"Î"
#define FT_TASK_RESTRICT	"¾"

#define FT_SUBJLOC	"ùúü"
#define FT_OBJLOC	"èéë"
#define FT_IOBJLOC	"öõ"
#define FT_SUBCAT	"O÷Ï±"
#define FT_SUBCATALL	FT_SUBJLOC FT_OBJLOC FT_IOBJLOC FT_SUBCAT "_"

#define FT_INSTRUCT	"º§ß¶ÿ"
#define FT_NULL		"?"
#define FT_OTHER	FT_NULL "@%v¸+_"

#define FT_LEXENTRY	FT_GENDER FT_POS FT_LANG FT_INITIAL_SOUND FT_LE_MORE
#define FT_LEXENTRY_ALL	FT_POS FT_LANG FT_INITIAL_SOUND FT_LE_MORE
#define FT_LE_MINUS	FT_LE_MORE
#define FT_ADJLEXENTRY	FT_POS FT_LANG FT_INITIAL_SOUND FT_LE_MORE
#define FT_INFL		FT_TENSE FT_GENDER FT_NUMBER FT_PERSON FT_MOOD \
			FT_ALTER FT_DEGREE FT_MODALITY
#define FT_INFL_FILE	FT_INFL FT_STYLE FT_DIALECT FT_FREQ
#define FT_USAGE	FT_STYLE FT_DIALECT FT_FREQ FT_CONNOTE FT_ADDRESS \
			FT_GRAMMAR FT_PARUNIV FT_TASK_RESTRICT
#define FT_ALL		FT_LEXENTRY_ALL FT_INFL FT_USAGE FT_SUBCATALL \
			FT_INSTRUCT FT_OTHER
#define FT_GENDERPLUS	FT_GENDER "§"
#define FT_LINK		FT_USAGE
#define FT_INFL_LINK	FT_INFL FT_LINK

#define FT_CONSTIT	"ELWXYZ"

#define FT_UNUSED	"àáäâêþª"

#define F_NULL			'?'
#define FS_NULL			"?"
#define F_TODO			'@'
#define F_ENTER			'%'
#define F_SLANG			'a'
#define F_PREPOSED_ADJ		'b'
#define F_CONDITIONAL		'c'
#define F_PAST_PARTICIPLE	'd'
#define F_PRESENT_PARTICIPLE	'e'
#define F_INFINITIVE		'f'
#define F_BRITISH		'g'
#define F_PEJORATIVE   		'h'
#define F_IMPERFECT		'i'
#define F_BORROWING		'j'
#define F_DEFINITE_ART		'k'
#define F_ETRE			'l'
#define F_MASS_NOUN		'm'
#define F_OLD			'o'
#define F_PRESENT		'p'
#define F_INFREQUENT		'q'
#define F_PASSE_SIMPLE		's'
#define F_LITERARY		't'
#define F_FUTURE		'u'
#define F_PROVERB		'v'
#define F_CLAUSE2_ONLY		'w'
#define F_S_POS			'x'
#define F_FRENCH		'y'
#define FS_FRENCH		"y"
#define F_ENGLISH		'z'
#define FS_ENGLISH		"z"
#define F_ADJECTIVE		'A'
#define FS_ADJECTIVE		"A"
#define F_ADVERB		'B'
#define FS_ADVERB		"B"
#define F_NEUTER		'C'
#define F_DETERMINER		'D'
#define F_ADJP			'E'
#define F_FEMININE		'F'
#define F_INDICATIVE		'G'
#define F_PRONOUN		'H'
#define F_IMPERATIVE		'I'
#define F_SUBJUNCTIVE		'J'
#define F_CONJUNCTION   	'K'
#define FS_CONJUNCTION		"K"
#define F_ADVP			'L'
#define F_MASCULINE		'M'
#define F_NOUN			'N'
#define FS_NOUN			"N"
#define F_SUBCAT_SUBJUNCTIVE	'O'
#define F_PLURAL		'P'
#define F_QUESTION		'Q'
#define F_PREPOSITION		'R'
#define FS_PREPOSITION		"R"
#define F_SINGULAR		'S'
#define F_INFORMAL      	'T'
#define F_INTERJECTION		'U'
#define F_VERB			'V'
#define F_VP			'W'
#define F_NP			'X'
#define F_PP			'Y'
#define F_S			'Z'
#define F_EXPLETIVE		'0'
#define F_FIRST_PERSON		'1'
#define F_SECOND_PERSON		'2'
#define F_THIRD_PERSON		'3'
#define F_POSITIVE		'6'
#define F_COMPARATIVE		'7'
#define F_SUPERLATIVE		'8'
#define F_ELEMENT		'9'
#define F_TRADEMARK		((uc)'®')
#define F_MODAL			((uc)'µ')
#define F_AMERICAN		((uc)'À')
#define F_CANADIAN		((uc)'ç')
#define F_EMPTY_ART		((uc)'É')
#define F_OTHER_DIALECT		((uc)'î')
#define F_COMMON_INFL		((uc)'ï')
#define F_FREQUENT		((uc)'¹')
#define FS_FREQUENT		"¹"
#define F_NAME			((uc)'º')
#define F_IRONIC		((uc)'ñ')
#define F_ISM			((uc)'Î')
#define F_TUTOIEMENT		((uc)'²')
#define F_VOUVOIEMENT		((uc)'×')
#define F_NO_PROGRESSIVE	((uc)'Ô')
#define F_EPITHETE_ADJ		((uc)'Ó')
#define F_PREDICATIVE_ADJ	((uc)'Ú')
#define F_INFL_CHECKED		((uc)'¸')
#define FS_INFL_CHECKED		"¸"
#define F_REALLY		((uc)'·')
#define FS_REALLY		"·"
#define F_SUBCAT_INDICATIVE	((uc)'÷')
#define F_SUBCAT_INFINITIVE	((uc)'Ï')
#define F_SUBCAT_PRESENT_PARTICIPLE	((uc)'±')
#define F_CONTRACTION		((uc)'Í')
#define F_ELISION		((uc)'Á')
#define F_PREVOWEL		((uc)'Ä')
#define F_ASPIRE		((uc)'È')
#define F_VOCALIC		((uc)'Ì')
#define F_OBJ1			((uc)'è')
#define F_OBJ2			((uc)'é')
#define F_OBJ3			((uc)'ë')
#define FS_OBJS			"èéë"
#define F_SUBJ12		((uc)'ù')
#define F_SUBJ2			((uc)'ú')
#define F_SUBJ3			((uc)'ü')
#define F_IOBJ3			((uc)'ö')
#define F_IOBJ4			((uc)'õ')
#define F_DO_NOT_REORDER	((uc)'ô')
#define F_TRANS_ONLY		((uc)'¾')
#define F_NO_INFLECT		((uc)'¶')
#define F_MASC_FEM		((uc)'§')
#define F_FUSED			((uc)'ÿ')
#define F_INVARIANT		((uc)'ß')
#define F_PREFIX		((uc)'«')
#define F_SUFFIX		((uc)'»')
#define F_OPTIONAL		'_'
#define F_NO_BZ_Z		((uc)'°')
#define F_NO_ZB_Z		((uc)'¿')
#define F_NO_BW_W		((uc)'Þ')
#define F_NO_WB_W		((uc)'ð')
#define F_NO_BE_E		((uc)'Æ')
#define FS_NO_BE_E		"Æ"
#define F_COORDINATOR		((uc)'©')

#define FS_COMPTENSE		"BHRVW"
#define FS_CHECKING		"¸·"
#define FS_ADJDET		"AD"
#define FS_SAOBJ		"xU"
#define FS_HEADNOUN		"HN"
#define FS_NP_PP		"XY"
#define FS_VP_PP		"WY"
#define FS_FINITE_TENSE		"pisuc"

#define TREE_LEVEL		'='
#define TREE_LEVEL_CAPITAL	'*'
#define TREE_LEVEL_CONCRETE	'*'
#define TREE_LEVEL_MULTI	'-'
#define TREE_LEVEL_CONTRAST	'-'
#define TREE_COMMENT		';'
#define TREE_ESCAPE		92
#define TREE_RANGE		((uc) '¯')
#define TREE_SLOT_SEP		((uc)'|')
#define TREE_TS_SLOT		'@'
#define TREE_RETRACT		'Ð'
#define TREE_SLOTVALUE_SEP1	'='
#define TREE_SLOTVALUE_SEP2	((uc)'¤')
#define TREE_VALUE_SEP		','
#define TREE_MICRO_SEP		':'
#define TREE_NANO_SEP		'+'
#define TREE_INSTANTIATE	'#'

#define LE_SEP			'/'
#define LE_FEATURE_SEP		'.'
#define LE_PHRASE_PREP		'+'
#define LE_PHRASE_OPT_PREP	'_'
#define LE_PHRASE_INFLECT	'*'
#define	LE_PHRASE_NO_INFLECT	'#'
#define LE_NULLNOUN		"½"
#define LE_PHRASE_OBJECT	"ø"
#define LE_EXPL_SEP		((uc)'ø')
#define LE_WORD_SRCPHRASE	((uc)'Ý')
#define LE_WHITESPACE		" -\"',;:/«»()"
#define LE_NONWHITESPACE	"$%.@"
#define LE_PHRASE_FEATSEP	"*#+_"
#define LE_ESCAPE_CHARS		"."

/* ctype.h gives different results depending on the locale
 * and whether this is compiled -Bstatic or -Bdynamic.
 * The below is more dependable.
 */
#define Char_isdigit(c)		((c) >= '0' && (c) <= '9')
#define Char_islower(c)		((c) >= 'a' && (c) <= 'z')
#define Char_isupper(c)		((c) >= 'A' && (c) <= 'Z')
#define Char_isalpha(c)		(Char_islower(c) || Char_isupper(c))
#define CharSentenceStart(c)	(CharIsUpper((uc)(c)) || (c) == LPAREN)

#define LexEntryWhitespace(c)	(StringIn((c), LE_WHITESPACE))
#define LexEntryNonwhite(c)	(CharIsLower(c) || CharIsUpper(c) || \
                                 Char_isdigit(c) || \
                                 StringIn((c), LE_NONWHITESPACE))

#define ALTERN_SEP		" /"
#define UNTRANS			'#'

#define UT_NA			0
#define UT_QUESTION		1
#define UT_STATEMENT		2
#define UT_COMMAND		3

#define FLIP12(n)		((n) == 1 ? 2 : 1)
#define CREATE(type)		((type *) MemAlloc(sizeof(type),#type))
#define CREAT(type, force_malloc) \
		((type *) MemAlloc1(sizeof(type),#type,force_malloc))
#define streq(a,b) \
	(((a)[0] == (b)[0]) && (strcmp((a),(b)) == 0))
#define HASHSIG		6
#define DHASHSIG	12

typedef struct HashEntry_s {
  char	*symbol;
  char	*value;
  struct HashEntry_s *next;
} HashEntry;

typedef struct {
  long		size;
  HashEntry	*hashentries[5];
} HashTable;

#define UNIXTSNA	0
#define UNIXTSNEGINF	1

#ifdef MACOS
#define UNIXTSPOSINF	ULONG_MAX
#else
#define UNIXTSPOSINF	LONG_MAX
#endif

#define TSFLAG_TOD		0x01
#define TSFLAG_DUR		0x02
#define TSFLAG_APPROX		0x04

#define TSFLAG_STORY_TIME	0x08
#define TSFLAG_NOW		0x10

typedef struct Ts_s {
  time_t	unixts;
  char		flag;
  struct Context_s	*cx;
} Ts;

typedef struct Directory_s {
  char			fn[FILENAMELEN];
  char			basename[FILENAMELEN];
  Ts			ts;
  struct Directory_s	*next;
} Directory;

typedef short Day;
typedef short Days;
#define DAYNA			-1
#define DAYSNA			0x80

#define TODNA			-1L
#define TODERR			-2L
#define TODNOON			(12L*60L*60L)
typedef long Tod;

#define DURNA			-1L
#define DURERR			-2L
#define DURPOSINF		LONG_MAX
typedef long Dur;

typedef struct {
  Ts		startts;
  Ts		stopts;
  Days		days;
  Tod		tod;
  Dur		dur;
  struct Context_s	*cx;
} TsRange;

typedef struct StringArray_s {
  long	maxlen;
  long	len;
  char	**strings;
} StringArray;

#define StringArrayLen(sa)	((sa)->len)

#define OBJ_NO_CREATE		0
#define OBJ_CREATE_A		1
#define OBJ_CREATE_C		2
#define OBJ_CREATE_AC		3

#define OBJTYPEANY		0
#define OBJTYPEDESTROYED	15
#define OBJTYPEMIN		60
#define OBJTYPEASYMBOL		60
#define OBJTYPEACSYMBOL		61
#define OBJTYPECSYMBOL		62
#define OBJTYPESTRING		63
#define OBJTYPENUMBER		64
#define OBJTYPELIST		65
#define OBJTYPEGRID		66
#define OBJTYPEGRIDSUBSPACE	67
#define OBJTYPETRIPLEG		68
#define OBJTYPETSR		69
#define OBJTYPENAME		70
#define OBJTYPEMAX		70

#define ObjTypeOK(type)		(((type) >= OBJTYPEMIN) && \
                                 ((type) <= OBJTYPEMAX))
#define ObjIsList(obj)		((obj)->type == OBJTYPELIST)
#define ObjIsHumanName(obj)	((obj)->type == OBJTYPENAME)
#define ObjIsString(obj)	((obj)->type == OBJTYPESTRING)
#define ObjIsSymbol(obj)	(((obj)->type == OBJTYPEASYMBOL) || \
				 ((obj)->type == OBJTYPECSYMBOL) || \
				 ((obj)->type == OBJTYPEACSYMBOL))
#define ObjIsAbstract(obj)	((obj)->type != OBJTYPECSYMBOL)
#define ObjIsConcrete(obj)	((obj)->type == OBJTYPECSYMBOL)
#define ObjIsConcreteNC(obj)	(((obj) != NULL) && \
                                 ((obj)->type == OBJTYPECSYMBOL))
#define ObjIsContrast(obj)	((obj)->type == OBJTYPEACSYMBOL)
#define ObjIsNumber(obj)	((obj)->type == OBJTYPENUMBER)
#define ObjIsTsRange(obj)	((obj)->type == OBJTYPETSR)
#define ObjContainsTsRange(obj)	(((obj)->type == OBJTYPELIST) || \
                                 ((obj)->type == OBJTYPETSR))
#define ObjIsTsRangeNC(obj)	(((obj) != NULL) && \
                                 ((obj)->type == OBJTYPETSR))
#define ObjIsGridSubspace(obj)	((obj)->type == OBJTYPEGRIDSUBSPACE)
#define ObjIsVar(obj)		(((obj)->type == OBJTYPELIST) ? \
                                 0 : ((obj)->u1.nlst.name[0] == '?'))
#define ObjIsNotVar(obj)	(((obj)->type == OBJTYPELIST) ? \
                                 1 : ((obj)->u1.nlst.name[0] != '?'))
#define ObjIsNotVarNC(obj)	(((obj) != NULL) && ObjIsNotVar(obj))
#define ObjLen(obj)		(((obj)->type == OBJTYPELIST) ? \
                                 ((obj)->u1.lst.len) : 0)
#define ObjIsNa(obj)		(((obj) == NULL) || ((obj) == ObjNA))
#define ObjAsserted(obj)	(((obj)->type == OBJTYPELIST) ? \
                                 ((obj)->u1.lst.asserted) : 0)

typedef struct Obj_s {
  short			type;
  union {
    struct {		/* not OBJTYPELIST */
      char		*name;
      short		numparents, numchildren;
      short		maxparents, maxchildren;
      struct Obj_s	**parents;
      struct Obj_s	**children;
    } nlst;
    struct {		/* OBJTYPELIST */
      short		len;
      struct Obj_s	**list;
      struct PNode_s	**pn_list;
      struct ObjList_s	*justification;	/* response to "Why?" */
      struct ObjList_s	*superseded_by;
      char		asserted;
    } lst;
  } u1;
  union {
    void			*any;
    char			*s;		/* OBJTYPESTRING */
    double			number;		/* OBJTYPENUMBER */
    struct Grid_s		*grid;		/* OBJTYPEGRID */
    struct GridSubspace_s	*gridsubspace;	/* OBJTYPEGRIDSUBSPACE */
    struct TripLeg_s		*tripleg;	/* OBJTYPETRIPLEG */
    TsRange			tsr;		/* OBJTYPETSR, OBJTYPELIST */
    struct Name_s		*nm;		/* OBJTYPENAME */
  } u2;
  struct ObjToLexEntry_s	*ole;
  struct Obj_s	*next;	/* linked list of all objects */
  struct Obj_s	*prev;	/* linked list of all objects */
} Obj;

typedef struct SP_s {
  Float				score;
  struct PNode_s		*pn;
  struct Anaphor_s		*anaphors;
  struct LexEntryToObj_s	*leo;
} SP;

typedef struct ObjList_s {
  Obj			*obj;
  struct ObjList_s	*next;	/* This must be after <obj>. cf ObjListShort. */
  union {
    SP sp;		/* Sem_Parse */
    struct {		/* Discourse ds_speakers/ds_listeners */
      int		gender;
      int		number;
    } sl;
    long		n;		/* ObjListIncrement, ObjListISACount */
    struct ObjList_s	*objs;		/* ObjListList, Style */
    Obj			*tgt_obj;	/* UA_Analogy */
    char                *s;
  } u;
} ObjList;

typedef ObjList ObjListList;

#define ObjListN(objs)	((objs)->u.n)

typedef struct ObjListShort_s {
  Obj			*obj;
  struct ObjList_s	*next;
} ObjListShort;

typedef struct Word_s {
  char		*word;
  char		*features;
  struct Word_s	*next;
} Word;

typedef struct LexEntry_s {
	char	*srcphrase;
	char	*features;
	Word	*infl;
	char	*phrase_seps;
	struct LexEntryToObj_s	*leo;
	struct LexEntry_s	*next;	/* Global debugging chain. */
} LexEntry;

typedef struct IndexEntry_s {
	LexEntry		*lexentry;
	char			*features;
	struct IndexEntry_s	*next;
} IndexEntry;

typedef struct IndexEntrySpell_s {
	LexEntry		*lexentry;
	char			*word;
	char			*features;
	struct IndexEntrySpell_s	*next;
} IndexEntrySpell;

typedef struct CaseFrame_s {
  struct Obj_s			*cas;
  struct Obj_s			*concept;
  SP				sp;
  Bool				theta_marked;
  struct CaseFrame_s		*next;
} CaseFrame;

#define TRPOS_NA			0
#define TRPOS_PRE_VERB			1
#define TRPOS_POST_VERB_PRE_OBJ		2
#define TRPOS_POST_VERB_POST_OBJ	3	/* Unless obj heavy. */

typedef struct ThetaRole_s {
  char			isoptional;
  struct Obj_s		*cas;
  struct LexEntry_s	*le;
  int			subcat;
  char			position;
  struct ThetaRole_s	*next;
} ThetaRole;

#define WEIGHT_DEFAULT	0.55	/* Average of neutral (0.1) and 1.0. */

typedef struct LexEntryToObj_s {
  char			*features;
  struct Obj_s		*obj;
  ThetaRole		*theta_roles;
  struct LexEntryToObj_s	*next;
} LexEntryToObj;

typedef struct ObjToLexEntry_s {
  char			*features;
  LexEntry		*le;
  ThetaRole		*theta_roles;
  time_t		lastused;
  struct ObjToLexEntry_s	*next;
} ObjToLexEntry;

typedef struct Lexitem_s {
  char			*word;
  char			*features;
  LexEntry		*le;
} Lexitem;

typedef struct LexitemList_s {
  Lexitem		*lexitem;
  struct LexitemList_s	*next;
} LexitemList;

typedef struct {
  char		*filename;
  size_t	startpos;
  size_t	stoppos;
  Ts		ts;
  ObjList	*speakers;
  ObjList	*listeners;
  char		*text;
  char		*emmn;
} Article;

typedef struct {
  Obj		*header_type;
  Obj		*message_id;
  Ts		send_ts;
  Ts		receive_ts;
  char		return_path[PHRASELEN];
  Obj		*from_obj;
  char		from_address[PHRASELEN];
  Obj		*reply_obj;
  char		reply_address[PHRASELEN];
  ObjList	*to_objs;
  ObjList	*cc_objs;
  Ts		resent_ts;
  Obj		*resent_from_obj;
  char		resent_from_address[PHRASELEN];
  ObjList	*resent_to_objs;
  ObjList	*resent_cc_objs;
  char		subject[PHRASELEN];
  Obj		*newsgroup;
  ObjList	*newsgroups;
  long		article_number;
  char		mime_version[PHRASELEN];
} EmailHeader;

typedef struct Name_s {
  char		fullname[PHRASELEN];
  char		pretitle[PHRASELEN];
  ObjList	*pretitles;
  char		givenname1[PHRASELEN];
  ObjList	*givennames1;
  short		giveninitial1;
  short		givenname12_sep;
  char		givenname2[PHRASELEN];
  ObjList	*givennames2;
  short		giveninitial2;
  char		givenname3[PHRASELEN];
  ObjList	*givennames3;
  short		giveninitial3;
  char		surname1[PHRASELEN];
  ObjList   	*surnames1;
  short		surinitial1;
  short		surname12_sep;
  char		surname2[PHRASELEN];
  ObjList   	*surnames2;
  short		surinitial2;
  char		posttitle[PHRASELEN];
  ObjList	*posttitles;
} Name;

#define EMMN_LEN	8

typedef struct Attribution_s {
  Obj	*speaker;
  Ts	ts;
  char	emmn[EMMN_LEN];
} Attribution;

#define MAXFIELDLEN	40
#define MINLINES	5
#define MINFIELDS	2
#define MAXFIELDS	5
#define FLD_FILL	'*'

typedef struct Field_s {
  short			startpos;
  short			stoppos;
  Obj			*class;
  short			lang;
  StringArray		*sa;
  ObjList		*objs;
  Obj			*rels[MAXFIELDS];
} Field;

typedef struct Table_s {
  Field		*flds;
  short		num_flds;
} Table;

typedef unsigned short PNNumber;
#define PNUMNA		0
#define PNUMSTART	1

#define EOSNA		' '

/* Numbers below must be contiguous. */
#define PNTYPE_NA				0
#define PNTYPE_MIN				1

#define PNTYPE_MIN_SYNPARSE		1
#define		PNTYPE_LEXITEM		1	/* word-level */
#define		PNTYPE_NAME		2
#define		PNTYPE_ORG_NAME		3
#define		PNTYPE_POLITY		4
#define		PNTYPE_TSRANGE		5
#define		PNTYPE_TELNO		6
#define		PNTYPE_MEDIA_OBJ	7
#define		PNTYPE_PRODUCT		8
#define		PNTYPE_NUMBER		9
#define PNTYPE_MAX_SYNPARSE		9

#define PNTYPE_END_OF_SENT		10

#define PNTYPE_CONSTITUENT		11	/* sentence-level */

#define PNTYPE_MIN_BLOCK		12
#define		PNTYPE_COMMUNICON	12
#define		PNTYPE_ATTRIBUTION	13
#define		PNTYPE_EMAILHEADER	14	/* paragraph-level */
#define		PNTYPE_TABLE		15
#define PNTYPE_MAX_BLOCK		15

#define PNTYPE_ARTICLE			16	/* raw text */
#define PNTYPE_STRING			17

#define PNTYPE_MAX			17
#define PNTYPE_DESTROYED		123

#define PUNCLEN	5

#define SCORE_NA		-1.0
#define SCORE_MIN		0.0
#define SCORE_MAX		1.0

typedef struct PNode_s {
  short			type;
  Float			score;
  ObjList		*concepts;	/* Results of Sem_Parse. */
  struct PNode_s	*concepts_parent;
  ObjList		*allcons;	/* PNTYPE_LEXITEM: Res. of Sem_Parse. */
  short			eos;
  short			gender, number, person;
  PNNumber		num;
  char			feature;	/* POS or constituent. */
  Lexitem		*lexitem;	/* PNTYPE_LEXITEM */
  Obj			*obj;		/* PNTYPE_LEXITEM */
  ObjToLexEntry		*ole;		/* PNTYPE_LEXITEM */
  char			punc[PUNCLEN];	/* Not PNTYPE_CONSTITUENT. */
  Bool			is_coord;	/* PNTYPE_LEXITEM */
  struct PNode_s	*pn1;		/* PNTYPE_CONSTITUENT */
  struct PNode_s	*pn2;		/* PNTYPE_CONSTITUENT */
  union {
    void		*any;
    char		*na;		/* PNTYPE_NA */
    Name		*name;		/* PNTYPE_NAME */
    ObjList		*polities;	/* PNTYPE_POLITY */
    TsRange		*tsr;		/* PNTYPE_TSRANGE */
    Obj			*telno;		/* PNTYPE_TELNO */
    ObjList		*media_obj;	/* PNTYPE_MEDIA_OBJ */
    ObjList		*product;	/* PNTYPE_PRODUCT */
    Obj			*number;	/* PNTYPE_NUMBER */
    ObjList		*communicons;	/* PNTYPE_COMMUNICON */
    Attribution		*attribution;	/* PNTYPE_ATTRIBUTION */
    EmailHeader		*emh;		/* PNTYPE_EMAILHEADER */
    Table		*table;		/* PNTYPE_TABLE */
    Article		*article;	/* PNTYPE_ARTICLE */
    char		*s;		/* PNTYPE_STRING */
  } u;
  size_t		lowerb;
  size_t		upperb;
  size_t		lowerb_subj;
  size_t		upperb_subj;
  long			versuslen;
  long			versusmaxlen;
  PNNumber		*versus;
  char			appliedsingletons;
  struct PNode_s	*next;		/* Next PNodeList node for sentence. */
  struct PNode_s	*next_altern;	/* Next translation alternative. */
} PNode;

typedef struct {
  PNode		*first;
  PNode		*last;
  PNode		*cache;
} PNodeList;

typedef struct Demon_s {
  Ts			ts;
  Obj			*ptn;
  short			tostate;
  Dur			waitidle;
  struct Demon_s	*next;
} Demon;

#define SubgoalStateIsFailure(a) (((a) == STFAILURE)||((a) == STFAILURENOPLAN))
#define SubgoalStateIsStopped(a) (((a) == STSUCCESS) || \
                                  ((a) == STFAILURE) || \
                                  ((a) == STFAILURENOPLAN))
#define SubgoalStateIsOutcome(a) (((a) == STSUCCESS) || \
                                  ((a) == STFAILURE) || \
                                  ((a) == STFAILURENOPLAN))

#define STNA			0
#define STBEGIN			-1
#define STWAITING		-2
#define STFAILURE		-3
#define STFAILURENOPLAN		-4
#define STSUCCESS		-5
#define STPOP			-6
/* Values of spin_to_state and not valid subgoal states: */
#define STSPIN			-7
#define STNOSPIN		-8

typedef short SubgoalState;

typedef struct Subgoal_s {
  SubgoalState		state;
  struct Subgoal_s	*supergoal;
  struct Actor_s	*ac;
  Obj			*obj;		/* [sleep John] */
  Obj			*cur_goal;	/* [active-goal [sleep John]] */
  Ts			startts;
  Ts			lastts;
  Ts			ts;
  short			onsuccess;
  short			onfailure;
  struct Subgoal_s	*next;
/* Temporary within ContextSprout: */
  struct Subgoal_s	*child_copy;
/* For debugging: */
  SubgoalState		last_state;
/* Temporary within PA_MainLoop: */
  Obj			*spin_emotion;
  short			spin_to_state;
  ObjList		*success_causes;
  ObjList		*failure_causes;
  Demon			*demons;
  Obj			*trigger;	/* Assertion causing demon to fire. */
/* Temporary within PA_MainLoop, to replace with FINDO: */
  Obj			*hand1, *hand2;
  Obj			*obj1;
  struct Trip_s		*trip;
  struct TripLeg_s	*leg;
  short			i;
  Obj			*last_sg;		/* HandleRequest */
  short			retries;
  short			return_to_state;	/* Chatterbot */
  union {
    struct {
      Obj	*tvssp;
      Ts	tvshowend;
    } WaTV;
    struct {
      Obj	*shelf, *hand1;
    } TaOf;
    struct {
      Obj	*hand;
    } HaTo;
    struct {
      Obj	*hand;
    } Inside;
    struct {
      Obj	*performance;
    } AtPe;
    struct {
      Obj	*boxofficeperson, *ticket, *perfts, *price;
    } PuTi;
    struct {
      ObjList	*cash, *nextcash;
      Float	change;
    } PaCa;
    struct {
      Obj	*creditcard;
    } PBD;
    struct {
      Obj	*check;
    } PBC;
    struct {
      Obj	*cashregister, *ticketbox, *customer, *priceofticket;
      ObjList	*tickets;
      Float	cashreceived;
      short	retries;
    } WBO;
    struct {
      Obj	*holding;
      Float	cashreceived, amount;
    } CoCa;
    struct {
      short	retries;
    } CoPa;
    struct {
      ObjList	*nextobjs;
      ObjList	*objs;
      ObjList	*wnextobjs;
      ObjList	*wobjs;
    } Dr;
  } w;
} Subgoal;

#define RELEVANCE_NONE		0.0
#define RELEVANCE_HALF		0.5
#define RELEVANCE_TOTAL		1.0
#define SENSE_NONE		0.0
#define SENSE_LITTLE		0.1
#define SENSE_SOME		0.25
#define SENSE_HALF		0.5
#define SENSE_MOSTLY		0.75
#define SENSE_TOTAL		1.0
#define NOVELTY_NONE		0.0
#define NOVELTY_EXPECTED	0.25
#define NOVELTY_HALF		0.5
#define NOVELTY_MOSTLY		0.75
#define NOVELTY_TOTAL		1.0

typedef struct Friend_s {
  Obj				*actor;
  ObjList			*assertions;
  Ts				ts_last_seen;
  Dur				see_interval;
  Subgoal			*subgoal_maintain;
  struct Friend_s		*next;
} Friend;

#define SENTENCE_ID_NA	-1

#define SYNTACTIC_NA		0
#define SYNTACTIC_TOP_S		1
#define SYNTACTIC_TOP_VP	2
#define SYNTACTIC_SUBJ		10
#define SYNTACTIC_OBJ		11
#define SYNTACTIC_IOBJ		12

#define SALIENCE(an) (((an)->sigweight_major)+((an)->sigweight_subject)+\
                      ((an)->sigweight_nested)+((an)->sigweight_list))
#define SALIENCE_MINIMAL	1

typedef struct Anaphor_s {
  Obj		*con;		/* N("Mary") */
  Obj		*source_con;	/* N("subject-pronoun") */
  PNode		*pn;
  int		salience;	/* Used only in ???. */
  short		gender;
  short		number;
  short		person;
  struct Anaphor_s	*next;
} Anaphor;

typedef long SentenceId;

typedef struct Antecedent_s {
  int		sigweight_major;
  int		sigweight_subject;
  int		sigweight_nested;
  int		sigweight_list;
  short		gender;
  short		number;
  short		person;
} Antecedent;

#define ACTOR_ANTECEDENT(ac, cx)  (((ac)->antecedent)[(cx)->dc->curchannel])

#define DCMAX           6

typedef struct Actor_s {
  Obj		*actor;
  Subgoal	*subgoals;
  ObjList	*emotions;
  Friend	*friends;
  Obj		*appointment_cur;
  Obj		*rest_level;
  Obj		*energy_level;
  Antecedent    antecedent[DCMAX];      /* Element DCIN (= 0) unused. */
  struct Context_s	*cx;
  struct Actor_s	*next;
} Actor;

#define MODE_NA				0
#define MODE_STOPPED			1
#define MODE_SPINNING			2
#define MODE_DAYDREAMING		3
#define MODE_PERFORMANCE		4

typedef int TenseStep;

#define TENSESTEP_PAST           -4
#define TENSESTEP_PAST_RECENT    -1
#define TENSESTEP_PRESENT        0
#define TENSESTEP_FUTURE_NEAR    1
#define TENSESTEP_FUTURE         3

#define TENSEIMPERFECTABS       60L     /* seconds */
#define TENSEIMPERFECTREL       .2      /* ratio of durations */
#define TENSEEQUALREL           .02     /* ratio of separation versus spread */
#define TENSENEARREL            .1      /* ratio of separation versus spread */
#define TENSEMINSPREAD          60.0    /* seconds */
#define TENSEPERSISTENT         86400.0 /* seconds */

/* Defaults for parsing. */
#define TENSEDEFNEARABS         (14*SECONDSPERMINL)
#define TENSEDEFSTEPDIST        (9*SECONDSPERDAYL)

typedef struct {
  Float		relevance;
  Float		sense;
  Float		novelty;
} RSN;

typedef struct Context_s {
  struct Context_s	*parent;
  Float			sense;
  TsRange		story_time;	/* = "then"; cf ds_now */
  TenseStep		story_tensestep;
  Actor			*actors;
  struct Question_s	*last_question;
  struct Discourse_s	*dc;
  struct Context_s	*next;
/* For convenience: */
  Ts			TsNA;
/* Specific to last UA_Understand pass: */
  Obj			*sproutcon;
  PNode			*sproutpn;
  struct Answer_s	*answer;
  struct Answer_s	*commentary;
  ObjList		*makes_sense_reasons;
  ObjList		*not_make_sense_reasons;
  Obj			*reinterp;
  Anaphor		*anaphors;
  struct Question_s	*questions;
/* Used within UA_Understand: */
  short			mode;
  RSN			rsn;
/* For debugging: */
  long			id;
  short			numchildren;
} Context;

typedef struct TsrGenAdvice_s {
  Bool		attempt_relative;
  Bool		attempt_day_and_part_of_the_day;
  Bool		include_time_of_day;
  Bool		include_day_of_the_week;
  Bool		include_day;
  Bool		include_month;
  Bool		include_year;
  Bool		include_dur;
  Bool		prep_ok;
  Bool		exactly;
} TsrGenAdvice;

/* In addition to natural language, TT can also output its representations
 * at the following levels:
 */
#define AGENCY_SYNTAX		0x0001
#define AGENCY_SEMANTICS	0x0002
#define AGENCY_ANAPHORA		0x0004
#define AGENCY_UNDERSTANDING	0x0008
#define AGENCY_ALL		0x000f

#define CH_REALTIME_CHAR_BASED	1
#define CH_REALTIME_LINE_BASED	2

typedef struct Channel_s {
  FILE		*stream;	/* NULL if this Channel is empty. */
  int		linelen;
  Bool		realtime;
  Ts		last_activity;
  Bool		activity;
  char		fn[FILENAMELEN];
  char		mode[WORDLEN];
  short		output_reps;
  short		lang;
  short		dialect;
  short		style;
  Bool		infrequent_ok;
  HashTable	*ht;
  TsRange	last_tsr;
  /* For input channels: */
  unsigned char	*buf;
  size_t	len, maxlen, pos;
  /* For output channels: */
  short		paraphrase_input;
  short		echo_input;
  char		*echo_input_last;
  /* For parsing: */
  PNodeList	*pnf;			/* All PNodes for this Channel. */
  PNodeList	*pnf_holding_area;	/* Used in Corpus processing. */
  PNode		*synparse_pns;		/* PNodes for current sentence. */
  PNNumber	synparse_pnnnext;
  size_t	synparse_lowerb;
  size_t	synparse_upperb;
  short		synparse_sentences;
  /* For translation: */
  PNode		*translations;
  /* For output, learning, debugging: */
  char		*input_text;
} Channel;

#define DC(dc)		((dc)->channel[(dc)->curchannel])
#define DCI(dc, i)	((dc)->channel[i])
#define DCIN		0
#define DCOUT1		1
#define DCOUT2		2
#define DCOTH		3

typedef struct {
  Obj	*process;
  Ts	startts;
  Ts	timeoutts;
  Dur	timelimit;
  short	recursion_count;
  short	recursion_limit;
} ABrainTask;

#define DEICTICSTACKMAX		5

#define DCLISTENERS(dc)		(((dc)->ds_listeners)[(dc)->ds_cur])
#define DCSPEAKERS(dc)		(((dc)->ds_speakers)[(dc)->ds_cur])
#define DCNOW(dc)		(&(((dc)->ds_now)[(dc)->ds_cur]))
#define DCREALTIME(dc)		(((dc)->ds_realtime)[(dc)->ds_cur])
#define DCCLASS(dc)		(((dc)->ds_class)[(dc)->ds_cur])
#define DCSTACKFULL(dc)		(((dc)->ds_cur) >= 0)
#define DCSTACKLEVEL(dc)	((dc)->ds_cur)

#define DiscourseGetCurrentChannel(dc)	((dc)->curchannel)

typedef struct CompTense_s {
  PNode	*mainverb_r;	/* Return value: "local" main verb of recursive call. */
  Obj	*tense_r;	/* Return value: tense of recursive call. */
  Obj	*mood_r;	/* Return value: mood of recursive call. */
  PNode *mainverb;	/* Variable containing found "global" main verb. */
  Obj	*maintense_a;	/* Argument to recursive call. */
  short	leftmost;	/* Argument to recursive call. */
  short	rightmost;	/* Argument to recursive call. */
} CompTenseHolder;

#define DC_MODE_TURING		0x0001
  /* TT should pretend to be a human. */
#define DC_MODE_PROGRAMMER	0x0002
  /* User is a TT programmer. */
#define DC_MODE_CONV		0x0004
  /* Conversational mode. Questions from TT OK. */
#define DC_MODE_THOUGHTSTREAM	0x0008
  /* Generate assertions. */
#define DC_MODE_COMPOUND_NOUN	0x0010
  /* Compound noun understanding. */

/* todo: Move comptense into the below. */
typedef struct GenAdvice_s {
  Obj			*aspect;
  Bool			consistent;
  Bool			gen_tsr;
  TsrGenAdvice		tga;
  LexEntry		*gen_advle;
  LexEntry		*gen_negle1;
  LexEntry		*gen_negle2;
  short			qwq;	/* Question word question generation flag. */
  Anaphor		*anaphors;
  Bool			flip_lang_ok;
} GenAdvice;

#define DCMAX		6

typedef struct Discourse_s {
/* Multiple communications channels: different languages for in and out. */
  short			curchannel;
  Channel		channel[DCMAX];
  ABrainTask		*abt;
/* Language-insensitive discourse context: */
  Context		*cx_alterns;
  Context		*cx_best;
  Context		*cx_cur;
  Context		*cx_children;
  short			mode;
  struct Answer_s	*last_answers;
  short			run_agencies;
  Obj			*task;
/* Deictic stack: */
  short			ds_cur;
  Obj			*ds_class[DEICTICSTACKMAX];
  ObjList		*ds_speakers[DEICTICSTACKMAX];
  ObjList		*ds_listeners[DEICTICSTACKMAX];
  Ts			ds_now[DEICTICSTACKMAX];	/* cf story_time */
  short			ds_realtime[DEICTICSTACKMAX];
/* Temporary within Sem_Parse: */
  short			relax;
  short			defer_ok;
  Obj			*tense;
  PNode			*pn_root;
  Dur			rel_dur;
    /* DURNA ("3 minutes ago", "after 3 minutes", "then") */
  Dur			abs_dur;
    /* ("for 3 minutes", defaulted from action) */
  CompTenseHolder	cth;
  short			exactly;
/* Temporary within Syn_Gen: */
  GenAdvice		ga;
/* Global program options (perhaps move out of Discourse): */
  char			langs[FEATLEN];
  char			dialects[FEATLEN];
} Discourse;

#define GR_IS_VAR(c)	(CharIsUpper((uc)(c)) || CharIsLower((uc)(c)) || \
                         Char_isdigit(c))
#define GR_VARSEP	':'
#define GR_FILLALG_WALL	'.'
#define GR_FILLALG_NA	TERM
#define GR_OBJNAME	'&'
  /* Concrete object/existing object. Do not instantiate. */
#define GR_SECTION_SEP	".\n"
#define GR_NOT		'~'
#define GR_FILL		'*'
#define GR_EMPTY	' '
#define GR_WALLCHARS	"~ "

typedef short GridCoord;

typedef struct Grid_s {
  GridCoord	rows, cols;
  char		*m;
  Obj		*grid;					
} Grid;

typedef struct GridSubspace_s {
  Grid			*grid;
  long			maxlen;
  long			len;
  GridCoord		*rows;
  GridCoord		*cols;
/* Only used during grid parsing: */
  struct GridSubspace_s *next;
} GridSubspace;

typedef struct IntergridPath_s {
  long		maxlen;
  long		len;
  Obj		**grids;
  Obj		**wormholes;
} IntergridPath;

#define SPACEFAILURE		0
#define SPACEALREADYNEAR	1
#define SPACEGRIDPATH		2
#define SPACEINTERGRIDPATH	3
#define SPACENONCE		4

#define DBFILETYPE_ISA		0
#define	DBFILETYPE_POLITY	1

typedef struct Answer_s {
/* Debugging: */
  Obj			*ua;
/* Question: */
  Obj			*question;
  Obj			*qclass;
/* Answers: */
  Float			sense;
  ObjList		*answer;		/* First response. */
  ObjList		*rephrased_answer;	/* Response to "What?". */
  short			eos;
  struct Text_s		*answer_text[DCMAX];
/* Generation directions: */
  short			sort_by_ts;
  short			gen_pairs;
  short			gen_tsr;
  TsrGenAdvice		tga;
  struct Answer_s	*next;
} Answer;

typedef struct Text_s {
  char		*s;
  long		maxlen;
  long		len;
  short		lang;
  long		pos;
  int		linelen;
  short		attach_next_word;
  Discourse	*dc;
} Text;

#define TextLen(text) ((text)->len)

typedef struct BdElem_s {
  Obj		*var;
  Obj		*val;
  struct BdElem_s	*next;
} BdElem;

typedef struct Bd_s {
  BdElem	*elems;
} Bd;

typedef struct TripLeg_s {
  Bool		start;		/* Beginning of a segment. */
  Obj		*action;	/* grid-walk, grid-drive-car, warp, ... */
  Obj		*driver;	/* &driver85, &pilot62, ... */
  Obj		*transporter;	/* &motor-vehicle23, &airplane42, ... */
  Obj		*script;	/* &trip89, ... */
  Obj		*grid;
  Obj		*from;
  Obj		*to;
  Obj		*wormhole;
  GridSubspace	*path;
  Ts		depart;
  Ts		arrive;
  Obj		*cost;
  struct TripLeg_s	*next;
} TripLeg;

typedef struct Trip_s {
  TripLeg	*legs;
  Obj		*cost;
  Ts		depart;
  Ts		arrive;
  struct Trip_s	*next;
} Trip;

typedef struct ProofReason_s {
  struct Proof_s	*proof;
  struct ProofReason_s	*next;
} ProofReason;

typedef struct Proof_s {
  Float			score;
  Obj			*fact;
  Bd			*bd;
  Obj			*rule;
  struct ProofReason_s	*reasons;
  struct Proof_s	*next;
} Proof;

typedef struct Stat_s {
  long		sum;
  long		total;
  long		min;
  long		max;
} Stat;

typedef struct AnaMorphClass_s {
  short		count;
  short		lang;
  short		pos;
  short		gender;
  LexEntry	*example;
  Word		*infls;
/* For debugging and studies: */
  struct AnaMorphClass_s	*next;
} AnaMorphClass;

typedef struct AnaMorphClassList_s {
  AnaMorphClass			*mc;
  struct AnaMorphClassList_s	*next;
} AnaMorphClassList;

#define RPT_JUST_LEFT	'l'
#define RPT_JUST_RIGHT	'r'
#define RPT_JUST_CENTER	'c'
#define RPT_MAXFIELDS	20

typedef struct RptField_s {
  char			*s;
  short			just;
  struct RptField_s	*next;
} RptField;

typedef struct RptLine_s {
  RptField		*fields;
  struct RptLine_s	*next;
} RptLine;

typedef struct Rpt {
  short		curfield;
  short		numfields;
  short		fieldlens[RPT_MAXFIELDS];
  RptLine	*lines;
} Rpt;

typedef struct WordFormRule_s {
  short		count;
  char		*suffix1;
  char		*lefeat1;
  char		*linkfeat1;
  char		*suffix2;
  char		*lefeat2;
  char		*linkfeat2;
  LexEntry	*example1;
  LexEntry	*example2;
  struct WordFormRule_s	*next;
} WordFormRule;

typedef struct AnagramClass_s {
  char			*word;
  char			*features;
  struct AnagramClass_s	*next;
} AnagramClass;

typedef struct {
  HashTable		*ht;
} Corpus;

typedef struct CorpusWordList_s {
  long			offset;
  Article		*article;
  struct CorpusWordList_s	*next;
} CorpusWordList;

#define LEARN_INTXT_LEN	75L

typedef struct LearnFile_s {
  Obj		*class;
  FILE		*stream;
  Obj   	*last_parent;
  Obj		*last_obj;
  TsRange	last_tsr;
  Bool		close_assertion;
  char 	 	last_input_text1[LEARN_INTXT_LEN];
  char		last_input_text2[LEARN_INTXT_LEN];
  struct LearnFile_s	*next;
} LearnFile;

typedef struct {
  short	subjnp_theta_role_i;
  short	objnp1_theta_role_i;
  short	objnp2_theta_role_i;
  short	iobjnp1_theta_role_i;
  short	iobjnp2_theta_role_i;
  short	iobjnp3_theta_role_i;
  PNode	*subjnp_src;
  PNode	*objnp1_src;
  PNode	*objnp2_src;
  PNode	*iobjpp1_src;
  PNode	*iobjpp2_src;
  PNode	*iobjpp3_src;
  PNode	*adjp_src;
  PNode	*adv1;
  PNode	*adv2;
  PNode *adv3;
  PNode	*preadv1;
  PNode *postadv1;
  Bool	subjnp_theta_marked;
  Bool	objnp1_theta_marked;
  Bool	objnp2_theta_marked;
  Bool	iobjpp1_theta_marked;
  Bool	iobjpp2_theta_marked;
  Bool	iobjpp3_theta_marked;
} Arguments;

typedef struct Affix_s {
  char	*word;
  char	*features;
  short	lang;
  Obj	*obj;
  struct Affix_s	*next;
} Affix;

/* All these fields are for convenience. If desired, you can use only
 * props, only attributes (or any combination).
 */
typedef struct Intension_s {
  Ts		*ts;
  TsRange	*tsr;
  Obj		*class;
    /* N("car") */
  ObjList	*props;
    /* {AND: [N("somerel") N("someobj") <class>]
     *       [N("someattr") <class>]}
     */
  ObjListList	*isas;
    /* {AND: {OR: N("Fiat-124-Spider") N("Alfa-Romeo-Spyder")}
     *       {OR: N("Convertible")}}
     */
  ObjList	*skip_this_isa;	/* optimization */
  ObjListList	*attributes;
    /* {AND: {OR: N("red") N("white")}
     *       {OR: N("expensive")}}
     */
  Ts		*ts_created;
  Context	*cx;
  int		cx_only;
} Intension;

typedef struct Question_s {
  Obj		*qclass;
  Float		importance;
  Obj		*asking_ua;
  Obj		*answer_class;
  ObjList	*preface;		/* Preface to first question. */
  Obj		*elided_question;	/* First question. */
  Obj		*full_question;		/* Response to "What?". */
  Obj		*full_question_var;
  Intension	*itn;
  struct Question_s	*next;
} Question;

typedef struct {
  int   output_reps;
  int   lang;
  int   dialect;
  int   style;
  int   translate;
  int   parain;
  int   echoin;
  int   port;
  char  new[PHRASELEN];
  char  old[PHRASELEN];
  char  file[PHRASELEN];
  char  dir[PHRASELEN];
  char  word[PHRASELEN];
  Obj   *obj;
  Obj   *speaker;
  Obj   *listener;
  int   dbgflags;
  int   dbglevel;
} ShellArgs;

typedef struct {
  long		maxlen;
  long		len;
  size_t	*positions;
  char		*firstchars;	/* Used only as a check. */
} TaggerWords;

typedef struct {
  size_t        size;
  char          *typ;
} MemCheck;

typedef struct Buffer_s {
  char *buf;
  struct Buffer_s *next;
} Buffer;

typedef struct Fifo_s {
  int    bufsize;
  Buffer *first;
  int    first_removepos;
  Buffer *last;
  int    last_addpos;
} Fifo;

typedef struct Socket_s {
  int       fd;
  char      *host;
  Fifo      *fifo_read;
  Fifo      *fifo_write;
  Discourse *dc;
} Socket;

typedef struct CaseElementLink_s {
  Obj                      *pred;
  char                     *label;
  struct CaseElement_s     *ce;
  struct CaseElementLink_s *next;
} CaseElementLink;

typedef struct CaseElement_s {
  char            *line;
  char            *label;
  Obj             *con;
  CaseElementLink *cels;
  struct CaseElement_s *next;
} CaseElement;

typedef struct Case_s {
  char          *email_address;
  int           stored;
  CaseElement   *ces;
  struct Case_s *next; /* linked list of all stored cases */
} Case;

typedef struct CaseList_s {
  int               count;
  Answer            *answer;
  Case              *cas;
  struct CaseList_s *next;
} CaseList;

#define E		NULL
#define N(name)		NameToObj((name), OBJ_CREATE_A)
#define I(obj, i)	ObjIth((obj), (i))
#define PNI(obj, i)	ObjIthPNode((obj), (i))
#define L		ObjCreateList
#define LTS		ObjTsAdd
#define LTSS		ObjTsStartStop
#define LTSR		ObjTsRangeAdd
#define M		ObjToName
#define D		NumberToObj
#define DI(x)		NumberToObj((double)(x))

#define DbRetrieveExact(ts,tsr,ptn,f) \
  DbRetrieval((ts),(tsr),(ptn),NULL,NULL,f)
#define DbRetrieveDesc(ts,tsr,ptn,i,l,f) \
  DbRetrievalDesc((ts),(tsr),(ptn),(i),NULL,NULL,l,5,f)
#define DbRetrieveAnc(ts,tsr,ptn,i,l) \
  DbRetrievalAnc((ts),(tsr),(ptn),(i),NULL,NULL,l,5,1)
#define DbRetrieveRelaxed(ts,tsr,ptn,l,f) \
  DbRetrievalRelaxed((ts),(tsr),(ptn),NULL,NULL,l,5,f)
#define DbRetractExact(ts,tsr,ptn) \
  DbRetrieval((ts),(tsr),(ptn),NULL,(ts),1)
#define DbRetractDesc(ts,tsr,ptn,i) \
  DbRetrievalDesc((ts),(tsr),(ptn),(i),NULL,(ts),1,5,1)
#define DbRetractAnc(ts,tsr,ptn,i) \
  DbRetrievalAnc((ts),(tsr),(ptn),(i),NULL,(ts),1,5,1)
#define DbRetrieveAncDesc(ts,tsr,ptn,i,j,f) \
  DbRetrievalAncDesc((ts),(tsr),(ptn),(i),(j),NULL,NULL,5,5,f)

#define AA			DbAssertActionDur
#define AR			DbAssertActionRange
#define AS			DbAssertState

#define TE(ts,ptn)		DbRetractExact((ts), NULL, (ptn))
#define TD(ts,ptn,j)		DbRetractDesc((ts), NULL, (ptn), (j))

#define RE(ts,ptn)		DbRetrieveExact((ts), NULL, (ptn), 1)
#define RER(tsr,ptn)		DbRetrieveExact(NULL, (tsr), (ptn), 1)
#define REB(ts,tsr,ptn)		DbRetrieveExact((ts), (tsr), (ptn), 1)
#define REI(i,ts,ptn)		DbRetrieveExactIth((i), (ts), NULL, (ptn), 1)
#define REIR(i,tsr,ptn)		DbRetrieveExactIth((i), NULL, (tsr), (ptn), 1)
#define R1E(ts,ptn) \
  ObjListRandomElemFree(DbRetrieveExact((ts), NULL, (ptn), 1))
#define R1EI(i,ts,ptn) \
  ObjIth(ObjListRandomElemFree(DbRetrieveExact((ts), NULL, (ptn), 1)),(i))
#define R1EIR(i,tsr,ptn) \
  ObjIth(ObjListRandomElemFree(DbRetrieveExact(NULL, (tsr), (ptn), 1)),(i))
#define R1EIB(i,ts,tsr,ptn) \
  ObjIth(ObjListRandomElemFree(DbRetrieveExact((ts), (tsr), (ptn), 1)),(i))

/* Z = don't free ptn */

#define ZRE(ts,ptn)		DbRetrieveExact((ts), NULL, (ptn), 0)
#define ZRER(tsr,ptn)		DbRetrieveExact(NULL, (tsr), (ptn), 0)
#define ZR1E(ts,ptn) \
  ObjListRandomElemFree(DbRetrieveExact((ts), NULL, (ptn), 0))
#define ZR1EI(i,ts,ptn) \
  ObjIth(ObjListRandomElemFree(DbRetrieveExact((ts), NULL, (ptn), 0)),(i))

#define RD(ts,ptn,j)		DbRetrieveDesc((ts), NULL, (ptn), (j), 1, 1)
#define RDR(tsr,ptn,j)		DbRetrieveDesc(NULL, (tsr), (ptn), (j), 1, 1)
#define RDB(ts,tsr,ptn,j)	DbRetrieveDesc((ts), (tsr), (ptn), (j), 1, 1)
#define RDI(i,ts,ptn,j) \
  DbRetrieveDescIth((i), (ts), NULL, (ptn), (j), 1)
#define RDIB(i,ts,tsr,ptn,j) \
  DbRetrieveDescIth((i), (ts), (tsr), (ptn), (j), 1)
#define R1D(ts,ptn,j) \
  ObjListRandomElemFree(DbRetrieveDesc((ts), NULL, (ptn), (j), 1, 1))
#define R1DI(i,ts,ptn,j) \
  ObjIth(ObjListRandomElemFree(DbRetrieveDesc((ts),NULL,(ptn),(j),1,1)),(i))
#define R1DBI(i,ts,tsr,ptn,j) \
  ObjIth(ObjListRandomElemFree(DbRetrieveDesc((ts),(tsr),(ptn),(j),1,1)),(i))

#define ZRD(ts,ptn,j)		DbRetrieveDesc((ts), NULL, (ptn), (j), 1, 0)

#define RN(ts,ptn,j)		DbRetrieveAnc((ts),NULL,(ptn),(j),1)
#define RNB(ts,tsr,ptn,j)	DbRetrieveAnc((ts),(tsr),(ptn),(j),1)
#define R1N(ts,ptn,j) \
  ObjListRandomElemFree(DbRetrieveAnc((ts),NULL,(ptn),(j),1))
#define R1NI(i,ts,ptn,j) \
  ObjIth(ObjListRandomElemFree(DbRetrieveAnc((ts), NULL,(ptn),(j),1)),(i))

#define RAD(ts,ptn,i,j)		DbRetrieveAncDesc((ts),NULL,(ptn),(i),(j),1)
#define RADNI(i,ts,ptn,j,k) \
ObjIth(ObjListRandomElemFree(DbRetrieveAncDesc((ts),NULL,(ptn),(j),(k),1)),(i))

#define YES(p)				ObjListNonEmpty(p)

#include "ttglobal.h"

/* End of file. */
