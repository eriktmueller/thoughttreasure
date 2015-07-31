/*
 * TT
 *
 * Copyright 1998, 1999, 2015 Erik Thomas Mueller
 *
 * 19981108T091100: begun
 * 19981116T164656: PNodes
 * 19981117T200147: objectToPrettyString
 */

package net.erikmueller.tt;

import java.io.*;
import java.lang.*;
import java.text.*;
import java.util.*;

/**
 * The <code>TT</code> class provides utilities and constants
 * for the ThoughtTreasure natural language/commonsense platform.
 *
 * @author Erik T. Mueller
 * @version 0.00023
 */

public class TT {

public static final char    F_NULL              = '?';
public static final char    F_TODO              = '@';
public static final char    F_ENTER             = '%';
public static final char    F_SLANG             = 'a';
public static final char    F_PREPOSED_ADJ      = 'b';
public static final char    F_CONDITIONAL       = 'c';
public static final char    F_PAST_PARTICIPLE   = 'd';
public static final char    F_PRESENT_PARTICIPLE = 'e';
public static final char    F_INFINITIVE        = 'f';
public static final char    F_BRITISH           = 'g';
public static final char    F_PEJORATIVE        = 'h';
public static final char    F_IMPERFECT         = 'i';
public static final char    F_BORROWING         = 'j';
public static final char    F_DEFINITE_ART      = 'k';
public static final char    F_ETRE              = 'l';
public static final char    F_MASS_NOUN         = 'm';
public static final char    F_OLD               = 'o';
public static final char    F_PRESENT           = 'p';
public static final char    F_INFREQUENT        = 'q';
public static final char    F_PASSE_SIMPLE      = 's';
public static final char    F_LITERARY          = 't';
public static final char    F_FUTURE            = 'u';
public static final char    F_PROVERB           = 'v';
public static final char    F_CLAUSE2_ONLY      = 'w';
public static final char    F_S_POS             = 'x';
public static final char    F_FRENCH            = 'y';
public static final char    F_ENGLISH           = 'z';
public static final char    F_ADJECTIVE         = 'A';
public static final char    F_ADVERB            = 'B';
public static final char    F_NEUTER            = 'C';
public static final char    F_DETERMINER        = 'D';
public static final char    F_ADJP              = 'E';
public static final char    F_FEMININE          = 'F';
public static final char    F_INDICATIVE        = 'G';
public static final char    F_PRONOUN           = 'H';
public static final char    F_IMPERATIVE        = 'I';
public static final char    F_SUBJUNCTIVE       = 'J';
public static final char    F_CONJUNCTION       = 'K';
public static final char    F_ADVP              = 'L';
public static final char    F_MASCULINE         = 'M';
public static final char    F_NOUN              = 'N';
public static final char    F_SUBCAT_SUBJUNCTIVE = 'O';
public static final char    F_PLURAL            = 'P';
public static final char    F_QUESTION          = 'Q';
public static final char    F_PREPOSITION       = 'R';
public static final char    F_SINGULAR          = 'S';
public static final char    F_INFORMAL          = 'T';
public static final char    F_INTERJECTION      = 'U';
public static final char    F_VERB              = 'V';
public static final char    F_VP                = 'W';
public static final char    F_NP                = 'X';
public static final char    F_PP                = 'Y';
public static final char    F_S                 = 'Z';
public static final char    F_EXPLETIVE         = '0';
public static final char    F_FIRST_PERSON      = '1';
public static final char    F_SECOND_PERSON     = '2';
public static final char    F_THIRD_PERSON      = '3';
public static final char    F_POSITIVE          = '6';
public static final char    F_COMPARATIVE       = '7';
public static final char    F_SUPERLATIVE       = '8';
public static final char    F_ELEMENT           = '9';
public static final char    F_TRADEMARK         = '®';
public static final char    F_MODAL             = 'µ';
public static final char    F_AMERICAN          = 'À';
public static final char    F_CANADIAN          = 'ç';
public static final char    F_EMPTY_ART         = 'É';
public static final char    F_OTHER_DIALECT     = 'î';
public static final char    F_COMMON_INFL       = 'ï';
public static final char    F_FREQUENT          = '¹';
public static final char    F_NAME              = 'º';
public static final char    F_IRONIC            = 'ñ';
public static final char    F_ISM               = 'Î';
public static final char    F_ATTACHMENT        = 'þ';
public static final char    F_TUTOIEMENT        = '²';
public static final char    F_VOUVOIEMENT       = '×';
public static final char    F_NO_PROGRESSIVE    = 'Ô';
public static final char    F_EPITHETE_ADJ      = 'Ó';
public static final char    F_PREDICATIVE_ADJ   = 'Ú';
public static final char    F_INFL_CHECKED      = '¸';
public static final char    F_REALLY            = '·';
public static final char    F_SUBCAT_INDICATIVE = '÷';
public static final char    F_SUBCAT_INFINITIVE = 'Ï';
public static final char    F_SUBCAT_PRESENT_PARTICIPLE = '±';
public static final char    F_CONTRACTION       = 'Í';
public static final char    F_ELISION           = 'Á';
public static final char    F_PREVOWEL          = 'Ä';
public static final char    F_ASPIRE            = 'È';
public static final char    F_VOCALIC           = 'Ì';
public static final char    F_BECOME            = 'ê';
public static final char    F_OBJ1              = 'è';
public static final char    F_OBJ2              = 'é';
public static final char    F_OBJ3              = 'ë';
public static final char    F_SUBJ12            = 'ù';
public static final char    F_SUBJ2             = 'ú';
public static final char    F_SUBJ3             = 'ü';
public static final char    F_IOBJ3             = 'ö';
public static final char    F_IOBJ4             = 'õ';
public static final char    F_DO_NOT_REORDER    = 'ô';
public static final char    F_GEN_ONLY          = 'ª';
public static final char    F_TRANS_ONLY        = '¾';
public static final char    F_NO_INFLECT        = '¶';
public static final char    F_MASC_FEM          = '§';
public static final char    F_FUSED             = 'ÿ';
public static final char    F_INVARIANT         = 'ß';
public static final char    F_ROLE1             = 'à';
public static final char    F_ROLE2             = 'á';
public static final char    F_ROLE3             = 'ä';
public static final char    F_PREFIX            = '«';
public static final char    F_SUFFIX            = '»';
public static final char    F_OPTIONAL          = '_';
public static final char    F_NO_BZ_Z           = '°';
public static final char    F_NO_ZB_Z           = '¿';
public static final char    F_NO_BW_W           = 'Þ';
public static final char    F_NO_WB_W           = 'ð';
public static final char    F_NO_BE_E           = 'Æ';
public static final char    F_COORDINATOR       = '©';

public static final String  FT_LANG             = "yz";
public static final String  FT_POS              = "NHVA9BDKRUx0«»";
public static final String  FT_INITIAL_SOUND    = "ÈÌ";
public static final String  FT_LE_MORE          = "Qj·";

public static final String  FT_TENSE            = "fpisucde";
public static final String  FT_GENDER           = "MFC";
public static final String  FT_NUMBER           = "SP";
public static final String  FT_PERSON           = "123";
public static final String  FT_MOOD             = "GJI";
public static final String  FT_DEGREE           = "678";
public static final String  FT_ALTER            = "ÍÁÄ";
public static final String  FT_MODALITY         = "µ";

public static final String  FT_STYLE            = "tTa";
public static final String  FT_DIALECT          = "Àgçîo";
public static final String  FT_FREQ             = "q¹";
public static final String  FT_FREQ_DICT        = "¹?q";
public static final String  FT_CONNOTE          = "hñ";
public static final String  FT_ADDRESS          = "²×";

public static final String  FT_EPITH_PRED       = "ÓÚ";
public static final String  FT_GR_ADJ = "b"+FT_EPITH_PRED;
public static final String  FT_GR_ADV_BLK       = "°¿ÞðÆ";
public static final String  FT_GR_CONJ_BLK      = "w";
public static final String  FT_GR_CONJ_ENB      = "©";
public static final String  FT_ARTICLE          = "kÉ";
public static final String  FT_GR_N = "m"+FT_ARTICLE;
public static final String  FT_GR_V             = "lÔô";
public static final String  FT_ROLE             = "àáä";
public static final String  FT_FILTER = "ï"+FT_GR_ADJ+FT_GR_ADV_BLK+
                                        FT_GR_CONJ_BLK+FT_GR_CONJ_ENB+
                                        FT_GR_N+FT_GR_V;
public static final String  FT_GRAMMAR = "®"+FT_FILTER+FT_ROLE;
public static final String  FT_PARUNIV          = "Îþ";
public static final String  FT_TASK_RESTRICT    = "ª¾";

public static final String  FT_SUBJLOC          = "ùúü";
public static final String  FT_OBJLOC           = "èéë";
public static final String  FT_IOBJLOC          = "öõ";
public static final String  FT_SUBCAT           = "O÷Ï±";
public static final String  FT_SUBCATALL = FT_SUBJLOC+FT_OBJLOC+FT_IOBJLOC+
                                           FT_SUBCAT+"_ê";

public static final String  FT_INSTRUCT         = "º§ß¶ÿ";
public static final String  FT_NULL             = "?";
public static final String  FT_OTHER = FT_NULL+"@%v¸+_";

public static final String  FT_LEXENTRY = FT_GENDER+FT_POS+FT_LANG+
                                          FT_INITIAL_SOUND+FT_LE_MORE;
public static final String  FT_LEXENTRY_ALL = FT_POS+FT_LANG+FT_INITIAL_SOUND+
                                              FT_LE_MORE;
public static final String  FT_LE_MINUS = FT_LE_MORE;
public static final String  FT_ADJLEXENTRY = FT_POS+FT_LANG+FT_INITIAL_SOUND+
                                             FT_LE_MORE;
public static final String  FT_INFL = FT_TENSE+FT_GENDER+FT_NUMBER+FT_PERSON+
                                      FT_MOOD+FT_ALTER+FT_DEGREE+FT_MODALITY;
public static final String  FT_INFL_FILE = FT_INFL+FT_STYLE+FT_DIALECT+FT_FREQ;
public static final String  FT_USAGE = FT_STYLE+FT_DIALECT+FT_FREQ+FT_CONNOTE+
                                       FT_ADDRESS+FT_GRAMMAR+FT_PARUNIV+
                                       FT_TASK_RESTRICT;
public static final String  FT_ALL = FT_LEXENTRY_ALL+FT_INFL+FT_USAGE+
                                     FT_SUBCATALL+FT_INSTRUCT+FT_OTHER;
public static final String  FT_GENDERPLUS = FT_GENDER+"§";
public static final String  FT_LINK = FT_USAGE;
public static final String  FT_INFL_LINK = FT_INFL+FT_LINK;
public static final String  FT_CONSTIT = "ELWXYZ";

public static final char    TREE_LEVEL          = '=';
public static final char    TREE_LEVEL_CAPITAL  = '*';
public static final char    TREE_LEVEL_CONCRETE = '*';
public static final char    TREE_LEVEL_MULTI    = '-';
public static final char    TREE_LEVEL_CONTRAST = '-';
public static final char    TREE_ESCAPE         = '\\';
public static final String  TREES_ESCAPE        = "\\";
public static final char    TREE_SLOT_SEP       = '|';
public static final String  TREES_SLOT_SEP      = "|";

public static final char    LE_PHRASE_INFLECT = '*';
public static final char    LE_PHRASE_NO_INFLECT = '#';
public static final char    LE_SEP           = '/';
public static final String  LES_SEP          = "/";
public static final char    LE_FEATURE_SEP   = '.';
public static final String  LES_FEATURE_SEP  = ".";
public static final String  LE_WHITESPACE0   = "-\"',;:/()";
public static final String  LE_WHITESPACE    = " "+LE_WHITESPACE0;
public static final String  LE_NONWHITESPACE = "$%.@";

public static final double  RANGE_DEFAULT_FROM = 0.1;
public static final double  RANGE_DEFAULT_TO   = 1.0;

/**
 * Converts the specified String representing a ThoughtTreasure Obj or
 * parse tree into a Java Object. The result may contain Double, String,
 * Date, Vector, and TTPNode.
 *
 * @param s the string to parse
 * @return <code>null</code> on failure
 * @see TT#objectToString
 * @see TT#objectToPrettyString
 */
public static Object stringToObject(String s)
{
  StreamTokenizer st = stringToStreamTokenizer(s);
  if (st == null) return null;
  setupObjSt(st);
  return stringToObject(st);
}

protected static Object createString(String s, String parent)
{
  Vector v = new Vector();
  v.addElement("STRING");
  v.addElement(s);
  v.addElement(parent);
  return v;
}

protected static Object createName(String s)
{
  Vector v = new Vector();
  v.addElement("NAME");
  v.addElement(s);
  return v;
}

protected static Object createNumber(double d, String parent)
{
  Vector v = new Vector();
  v.addElement("NUMBER");
  v.addElement(new Double(d));
  v.addElement(parent);
  return v;
}

protected static Object stringToObject(StreamTokenizer st)
{
  int t = nextToken(st);

  if (t == StreamTokenizer.TT_EOF) return null;

  if (t == '@') {
  /* todo: implement timestamp range parsing */
    while (true) {
      t = nextToken(st);
      if (t == StreamTokenizer.TT_EOF) return null;
      if (t == '[') break;
    }
  }

  if (t == '[') {
  /* TYPELIST */
    Object o;
    Vector v = new Vector();
    t = nextToken(st);
    while (t != ']') {
      st.pushBack();
      if (null == (o = stringToObject(st))) break;
      v.addElement(o);
      t = nextToken(st);
    }
    return v;
  }

  if (t == '"' || t == '\'' || t == '/') {
  /* TYPESTRING */
    return createString(st.sval, "string");
  }

  if (t != StreamTokenizer.TT_WORD) {
    dbg("TT.stringToObject: unexpected character <"+((char)t)+">");
    return null;
  }

  if ((st.sval).equals("STRING")) {
  /* TYPESTRING */
    String parent = parseColonParentColon(st);
    if (parent == null) return null;
    String s = getNextString(st);
    if (s == null) {
      dbg("TT.stringToObject: missing \"string\" after STRING:class:");
      return null;
    }
    return createString(s, parent);
  }

  if ((st.sval).equals("NAME")) {
    if (!skipColon(st)) {
      dbg("TT.stringToObject: missing : after NAME"); return null; }
    String name = getNextString(st);
    if (name == null) {
      dbg("TT.stringToObject: missing \"string\" after NAME:");
      return null;
    }
    return createName(name);
  }

  if ((st.sval).equals("NUMBER")) {
  /* TYPENUMBER */
    String parent = parseColonParentColon(st);
    if (parent == null) return null;
    t = nextToken(st);
    if (t != StreamTokenizer.TT_WORD) {
      dbg("TT.stringToObject: missing number after NUMBER:class:");
      return null;
    }
    double d = doubleParse(st.sval, true);
    return createNumber(d, parent);
  }

  if ((st.sval).equals("PNODE")) {
    return parsePNode(st);
  }

  return parseToken(st.sval, st);
}

protected static String parseColonParentColon(StreamTokenizer st)
{
  int t;
  t = nextToken(st);
  if (t != ':') { dbg("TT.parseColonParentColon: missing :"); return null; }
  t = nextToken(st);
  if (t != StreamTokenizer.TT_WORD) {
    dbg("TT.parseColonParentColon: missing class");
    return null;
  }
  String parent = st.sval;
  t = nextToken(st);
  if (t !=':') {
    dbg("TT.parseColonParentColon: missing : after class");
    return null;
  }
  return parent;
}

protected static Object parseToken(String s, StreamTokenizer st)
{
  if (s.endsWith("u")) {
    double d = doubleParse(s.substring(0, s.length()-1), false);
    return createNumber(d, "u");
  }

  /* TYPETS */
  Date date = stringToDate(s);
  if (date != null) return date;
  
  /* TYPESYMBOL */
  if (!validObjName(s)) {
    dbg("TT.parseToken: objname <"+s+"> contains invalid character");
    return null;
  }

  return s;
}

protected static TTPNode parsePNode(StreamTokenizer st)
{
  if (!skipColon(st)) {
    dbg("TT.parsePNode: missing : after PNODE");
    return null;
  }
  String feat = getNextToken(st);
  if (feat == null) {
    dbg("TT.parsePNode: missing feature after PNODE:");
    return null;
  }

  int t = nextToken(st);
  if (t != ':') {
  /* CONSTITUENT */
    st.pushBack();
    TTPNode pn = new TTPNode();
    pn.feature = feat.charAt(0);
    pn.leo = null;
    pn.startpos = -1L;
    pn.endpos = -1L;
    return pn;
  }
  /* PNODE:A:L:"bright":"Az¸":"bright":"Az¸"
   * feat = "A"
   */
  String pnType = getNextToken(st);
  if (pnType == null) {
    dbg("TT.parsePNode: missing PNode type");
    return null;
  }
  if (pnType.equals("L")) {
  /* LEXITEM */
    return parseLexitem(feat, st);
  } else {
    return parsePNodeOther(feat, pnType, st);
  }
}

protected static TTPNode parseLexitem(String feat, StreamTokenizer st)
{
  if (!skipColon(st)) {
    dbg("TT.parseLexitem: missing : after PNode type");
    return null;
  }

  /* Get citationForm. */
  String citationForm = getNextString(st);
  if (citationForm == null) {
    dbg("TT.parseLexitem: missing citationForm");
    return null;
  }
  if (!skipColon(st)) {
    dbg("TT.parseLexitem: missing : after citationForm");
    return null;
  }

  /* Get features. */
  String features = getNextString(st);
  if (features == null) {
    dbg("TT.parseLexitem: missing features");
    return null;
  }
  if (!skipColon(st)) {
    dbg("TT.parseLexitem: missing : after features");
    return null;
  }

  /* Get inflection. */
  String inflection = getNextString(st);
  if (inflection == null) {
    dbg("TT.parseLexitem: missing inflection");
    return null;
  }
  if (!skipColon(st)) {
    dbg("TT.parseLexitem: missing : after inflection");
    return null;
  }

  /* Get inflFeatures. */
  String inflFeatures = getNextString(st);
  if (inflFeatures == null) {
    dbg("TT.parseLexitem: missing inflFeatures");
    return null;
  }

  TTLexEntry le = new TTLexEntry();
  le.citationForm = citationForm;
  le.features = features;
  le.inflection = inflection;
  le.inflFeatures = inflFeatures;

  TTLexEntryToObj leo = new TTLexEntryToObj();
  leo.lexentry = le;
  leo.objname = null;
  leo.features = "";

  TTPNode pn = new TTPNode();
  pn.feature = feat.charAt(0);
  pn.leo = leo;
  pn.startpos = -1L;
  pn.endpos = -1L;
  return pn;
}

protected static TTPNode parsePNodeOther(String feat, String pnType,
                                         StreamTokenizer st)
{
  /* todo: elaborate the various types */
  String s;
  if (!skipColon(st)) {
    s = "";
  } else {
    s = getNextString(st);
    if (s == null) {
      dbg("TT.parsePNodeOther: missing PNode string for pnType <"+pnType+">");
      return null;
    }
  }
  TTLexEntry le = new TTLexEntry();
  if (s.length() == 0) {
    le.citationForm = pnType;
  } else {
    le.citationForm = pnType+":"+s;
  }
  le.features = feat;
  le.inflection = le.citationForm;
  le.inflFeatures = feat;

  TTLexEntryToObj leo = new TTLexEntryToObj();
  leo.lexentry = le;
  leo.objname = null;
  leo.features = "";

  TTPNode pn = new TTPNode();
  pn.feature = feat.charAt(0);
  pn.leo = leo;
  pn.startpos = -1L;
  pn.endpos = -1L;
  return pn;
}

protected static boolean validObjName(String s)
{
  int i, len, c;
  for (i = 0, len = s.length(); i < len; i++) {
    c = s.charAt(i);
    if (!validObjNameChar(c)) return false;
  }
  return true;
}

protected static boolean validObjNameChar(int c)
{
  return Character.isLetterOrDigit((char)c) ||
         c == '-' || c == '?' ||
         c == '&' || c == '!';
}

protected static void setupObjSt(StreamTokenizer st)
{
  st.resetSyntax();
  st.commentChar(';');
  st.quoteChar('"');
  st.quoteChar('\'');
  st.quoteChar('/');
  st.wordChars('.','.');
  st.wordChars('0','9');
  st.wordChars('a','z');
  st.wordChars('A','Z');
  st.wordChars('-','-');
  st.wordChars('+','+');
  st.wordChars('?','?');
  st.wordChars('&','&');
  st.wordChars('!','!');
  st.whitespaceChars(' ',' ');
  st.whitespaceChars('\n','\n');
  st.whitespaceChars('\r','\r');
  st.eolIsSignificant(false);
  st.slashStarComments(false);
  st.slashSlashComments(false);
}

protected static boolean skipColon(StreamTokenizer st)
{
  int t = nextToken(st);
  if (t !=':') return false;
  return true;
}

protected static String getNextToken(StreamTokenizer st)
{
  int t = nextToken(st);
  if (t != StreamTokenizer.TT_WORD) return null;
  return st.sval;
}

protected static String getNextString(StreamTokenizer st)
{
  int t = nextToken(st);
  if (t != '"') return null;
  return st.sval;
}

protected static int nextToken(StreamTokenizer st)
{
  try {
    return st.nextToken();
  } catch (Exception e) {
    return StreamTokenizer.TT_EOF;
  }
}

protected static StreamTokenizer stringToStreamTokenizer(String s)
{
  try {
    Reader reader = new StringReader(s);
    return new StreamTokenizer(reader);
  } catch (Exception e) {
    return null;
  }
}

/**
 * Converts the specified Java Object into a String.
 * The String looks like a ThoughtTreasure Obj.
 *
 * @param o the Java Object to convert into a String
 * @see TT#objectToPrettyString
 * @see TT#stringToObject
 * @return String
 */
public static String objectToString(Object o)
{
  if (o instanceof Integer) return objectToString((Integer)o);
  else if (o instanceof Double) return objectToString((Double)o);
  else if (o instanceof Date) return objectToString((Date)o);
  else if (o instanceof Vector) return objectToString((Vector)o);
  else return o.toString();
}

protected static String objectToString(Integer i)
{
  return i+"u";
}

protected static String objectToString(Double d)
{
  return d+"u";
}

protected static String objectToString(Date date)
{
  return dateToString(date);
}

protected static String objectToString(Vector v)
{
  StringBuffer r = new StringBuffer();
  boolean first = true;
  r.append("[");
  for (Enumeration e = v.elements(); e.hasMoreElements(); ) {
    if (first) {
      first = false;
    } else {
      r.append(" ");
    }
    r.append(objectToString(e.nextElement()));
  }
  r.append("]");
  return r.toString();
}

/**
 * Converts the specified Java Object into a pretty printed String.
 * The String looks like a ThoughtTreasure Obj.
 *
 * @param obj the Java Object to convert into a String
 * @see TT#objectToString
 * @see TT#stringToObject
 * @return String
 */
public static String objectToPrettyString(Object obj)
{
  return objectToPrettyString(obj, 0);
}

protected static String objectToPrettyString(Object o, int indent)
{
  if (o instanceof Vector) return objectToPrettyString((Vector)o, indent);
  else return objectToString(o);
}

protected static String objectToPrettyString(Vector v, int indent)
{
  int len = v.size();
  if (len == 0) return "[]";
  StringBuffer r = new StringBuffer();
  r.append('[');
  int i = 0;
  for (Enumeration e = v.elements(); e.hasMoreElements(); i++) {
    r.append(objectToPrettyString(e.nextElement(), indent+1));
    if (i == (len-1)) {
      r.append(']');
      break;
    } else {
      r.append("\n");
      r.append(nSpaces(indent+1));
    }
  }
  return r.toString();
}

/**
 * Gets the specified feature type from the specified String.
 *
 * @param features the feature string
 * @param ft the feature type such as <code>FT_POS</code> (part
 *           of speech) or <code>FT_LANG</code> (language).
 * @param def the feature to return if a feature of the specified
 *            type is not found in the the feature string
 * @return feature type character
 */
public static char featureGet(String features, String ft, char def)
{
  int len = features.length();
  for (int i = 0; i < len; i++) {
    if (-1 != ft.indexOf(features.charAt(i))) return features.charAt(i);    
  }
  return def;
}

/**
 * Parses the specified ThoughtTreasure date/timestamp String into a Date.
 * A ThoughtTreasure date/timestamp is the following subset of ISO 8601:
 * <pre>
 * YYYYMMDD"T"HHMMSS"Z" (in GMT) or
 * YYYYMMDD"T"HHMMSS (in some unspecified local time) or
 * YYYYMMDD"T"HHMMSS"-"HHMM or
 * YYYYMMDD"T"HHMMSS"+"HHMM
 * </pre>
 * Examples are:
 * <pre>
 * 20001213T160000
 * 20001213T160000Z
 * 20001213T160000-0500
 * 20001213T160000+0500
 * </pre>
 *
 * @param s the string to be parsed
 * @return <code>null</code> if the string is not a valid ThoughtTreasure
 *         date/timestamp
 * @see TT#dateToString
 */
public static Date stringToDate(String s)
{
  if (s.length() < 15) return null;

  if (s.charAt(8) != 'T') return null;

  if (s.length() == 15) {
  /* 19970130T160000 */
    return stringToDate(s, TimeZone.getDefault());
  }

  if (s.length() == 16 && s.charAt(15) == 'Z') {
  /* 19970130T160000Z */
    return stringToDate(s, TimeZone.getTimeZone("GMT"));
  }

  if (s.length() == 20 && (s.charAt(15) == '-' || s.charAt(15) == '+')) {
  /* 19970130T160000-0500
   * 19970130T160000+0500
   */
    return stringToDateTZ(s);
  }

  return null;
}

protected static Date stringToDateTZ(String s)
{
  int hrs = intParse(s.substring(16, 18), true);
  int mins = intParse(s.substring(18, 20), true);
  int offsetMillis = ((hrs*60)+mins)*60*1000;
  String[] ids = TimeZone.getAvailableIDs(offsetMillis);
  for (int i = 0; i < ids.length; i++) {
    TimeZone tz = TimeZone.getTimeZone(ids[i]);
    if (tz != null) return stringToDate(s, tz);
  }
  return null;
}

protected static Date stringToDate(String s0, TimeZone tz)
{
  try {
    String s = s0.substring(0, 4)+"-"+
               s0.substring(4, 6)+"-"+
               s0.substring(6, 8)+"-"+
               s0.substring(9, 11)+"-"+
               s0.substring(11, 13)+"-"+
               s0.substring(13, 15);
    SimpleDateFormat f = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
    f.setTimeZone(tz);
    Date date = f.parse(s, new ParsePosition(0));
    return date;
  } catch (Exception e) {
    return null;
  }
}

/**
 * Converts the specified Date into a ThoughtTreasure date/timestamp
 * String.
 *
 * @param date the Date to be converted
 * @see TT#stringToDate
 * @return String
 */
public static String dateToString(Date date)
{
  SimpleDateFormat fmt = new SimpleDateFormat ("yyyyMMdd'T'HHmmss'Z'");
  fmt.setTimeZone(TimeZone.getTimeZone("GMT"));
  return fmt.format(date);
}

protected static String nSpaces(int n)
{
  if (n < 0) return "";
  StringBuffer r = new StringBuffer();
  for (int i = 0; i < n; i++) r.append(' ');
  return r.toString();
}

protected static int intParse(String s, boolean reportError)
{
  try {
    return Integer.parseInt(s);
  } catch (Exception e) {
    if (reportError) dbg("<"+s+"> is not valid int");
    return 0;
  }
}

protected static long longParse(String s, boolean reportError)
{
  try {
    return Long.parseLong(s);
  } catch (Exception e) {
    if (reportError) dbg("<"+s+"> is not valid long");
    return 0L;
  }
}

protected static double doubleParse(String s, boolean reportError)
{
  try {
    return Double.valueOf(s).doubleValue();
  } catch (Exception e) {
    if (reportError) dbg("<"+s+"> is not valid double");
    return 0.0;
  }
}

protected static void dbg(String s)
{
  System.out.print(s+"\n");
  System.out.flush();
}

}

/* End of file. */
