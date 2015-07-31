/*
 * TTConnection
 *
 * Copyright 1998, 1999, 2015 Erik Thomas Mueller
 *
 * 19981107T074105: begun
 * 19981108T115035: more work
 * 19981109T074057: more work
 * 19981116T145557: syntactic parse, generate, chatterbot
 * 19981210T103200: added registered port number
 */

package net.erikmueller.tt;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;
import net.erikmueller.tt.*;

/**
 * The <code>TTConnection</code> class provides an interface
 * to the ThoughtTreasure natural language/commonsense server.
 * It implements the ThoughtTreasure Server Protocol (TTSP).
 * <p>
 * Here is a simple program which asks the ThoughtTreasure
 * server whether Evian is a beverage:
 * <pre>
 * import net.erikmueller.tt.*;
 * 
 * public class Example {
 * 
 * public static void main(String args[])
 * {
 *   try {
 *     TTConnection tt = new TTConnection("somehost");
 *     System.out.println(tt.ISA("beverage", "Evian"));
 *     tt.close();
 *   } catch (Exception e) {
 *   }
 * }
 * 
 * }
 * </pre>
 * <p>
 * To start the ThoughtTreasure server, start ThoughtTreasure
 * and then issue the command:
 * <pre>
 * server
 * </pre>
 * to listen on port 1832, the ThoughtTreasure Server Protocol (TTSP)
 * registered port number listed by the Internet Assigned Numbers
 * Authority (IANA), or:
 * <pre>
 * server -port PORT
 * </pre>
 * to listen on port number <code>PORT</code>.
 * <p>
 * The discourse context is normally maintained across calls for
 * a given <code>TTConnection</code>. This allows, for example, pronouns
 * to be understood and generated. To clear the connection's discourse
 * context, use <code>clearContext</code>.
 *
 * @author Erik T. Mueller
 * @version 0.00023
 */

public class TTConnection {

/**
 * Whether debugging information should be printed to <code>System.out</code>;
 * <code>false</code> by default.
 */
public static boolean DEBUG = false;

/**
 * Whether to cache results for later reuse without contacting
 * the ThoughtTreasure server; <code>true</code> by default.
 */
public static boolean USECACHE = true;

/**
 * The ThoughtTreasure Server Protocol (TTSP) registered port number
 * listed by the Internet Assigned Numbers Authority (IANA); the port
 * number is 1832.
 */
public static int PORT = 1832;

protected String         host;
protected InetAddress    address;
protected int            port;
protected Socket         socket;
protected BufferedReader in;
protected OutputStream   out;
protected Hashtable      cache;

/**
 * Creates a new connection to the specified ThoughtTreasure server.
 * The IANA-listed registered port number is assumed.
 *
 * @param host the host name of the ThoughtTreasure server
 * @exception IOException if an I/O error occurs when creating the socket
 */
public TTConnection(String host)
throws IOException
{
  this.host = host;
  this.port = PORT;
  init();
}

/**
 * Creates a new connection to the specified ThoughtTreasure server.
 * The IANA-listed registered port number is assumed.
 *
 * @param address the IP address of the ThoughtTreasure server
 * @exception IOException if an I/O error occurs when creating the socket
 */
public TTConnection(InetAddress address)
throws IOException
{
  this.address = address;
  this.port = PORT;
  init();
}

/**
 * Creates a new connection to the specified ThoughtTreasure server.
 *
 * @param host the host name of the ThoughtTreasure server
 * @param port the port number of the ThoughtTreasure server
 * @exception IOException if an I/O error occurs when creating the socket
 */
public TTConnection(String host, int port)
throws IOException
{
  this.host = host;
  this.port = port;
  init();
}

/**
 * Creates a new connection to the specified ThoughtTreasure server.
 *
 * @param address the IP address of the ThoughtTreasure server
 * @param port the port number of the ThoughtTreasure server
 * @exception IOException if an I/O error occurs when creating the socket
 */
public TTConnection(InetAddress address, int port)
throws IOException
{
  this.address = address;
  this.port = port;
  init();
}

protected void init()
throws IOException
{
  if (address != null) socket = new Socket(address, port);
  else socket = new Socket(host, port);
  in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
  out = socket.getOutputStream();
}

/**
 * Closes the connection.
 * @exception IOException if an I/O error occurs when closing the socket
 */
public void close()
throws IOException
{
  if (socket == null) return;
  socket.close();
  socket = null;
  in = null;
  out = null;
}

/**
 * Tests whether the specified object is of the specified class.
 *
 * @param classname the name of the class object
 * @param objname the name of the object
 * @return boolean
 * @exception IOException if an I/O error occurs communicating with the server
 */
public boolean ISA(String classname, String objname)
throws IOException
{
  return queryBoolean("ISA "+classname.trim()+" "+objname.trim()+"\n");
}

/**
 * Tests whether the specified object is a part of another specified object.
 *
 * @param part the name of the part object
 * @param whole the name of the whole object
 * @return boolean
 * @exception IOException if an I/O error occurs communicating with the server
 */
public boolean isPartOf(String part, String whole)
throws IOException
{
  return queryBoolean("IsPartOf "+part.trim()+" "+whole.trim()+"\n");
}

/**
 * Returns the parents of the specified object.
 *
 * @param objname the name of the object
 * @return Vector of String
 * @exception IOException if an I/O error occurs communicating with the server
 */
public Vector /* of String */ parents(String objname)
throws IOException
{
  return queryColonString("Parents "+objname.trim()+"\n");
}

/**
 * Returns the children of the specified object.
 *
 * @param objname the name of the object
 * @return Vector of String
 * @exception IOException if an I/O error occurs communicating with the server
 */
public Vector /* of String */ children(String objname)
throws IOException
{
  return queryColonString("Children "+objname.trim()+"\n");
}

/**
 * Returns the ancestors of the specified object.
 *
 * @param objname the name of the object
 * @return Vector of String
 * @exception IOException if an I/O error occurs communicating with the server
 */
public Vector /* of String */ ancestors(String objname)
throws IOException
{
  return queryColonString("Ancestors "+objname.trim()+"\n");
}

/**
 * Returns the descendants of the specified object.
 *
 * @param objname the name of the object
 * @return Vector of String
 * @exception IOException if an I/O error occurs communicating with the server
 */
public Vector /* of String */ descendants(String objname)
throws IOException
{
  return queryColonString("Descendants "+objname.trim()+"\n");
}

/**
 * Retrieves assertions from the database matching the specified pattern.
 * <p>
 * Returns a Vector of Java Objects representing ThoughtTreasure assertions.
 *
 * @param picki if not -1, the index of the object to be picked out
 *              of the result assertion
 * @param anci move up (mode="anc" or mode="ancdesc") the hierarchy on this
 *            index; should be -1 for mode not "anc" or "ancdesc"
 * @param desci move down (mode="desc" or "ancdesc") the hierarchy on this
 *              index; should be -1 for mode not "desc" or "ancdesc"
 * @param mode "exact" or "anc" or "desc" or "ancdesc"
 * @param ptn pattern such as "duration-of attend-play ?"
 * @return Vector of Object
 * @exception IOException if an I/O error occurs communicating with the server
 */
public Vector /* of Object */ retrieve(int picki, int anci,
                                       int desci, String mode,
                                       String ptn)
throws IOException
{
  return queryAssertionLines("Retrieve "+picki+" "+anci+" "+
                             desci+" "+mode.trim()+" "+
                             ptn.trim()+"\n");
}

/**
 * Returns the objects (atomic concepts) for the specified
 * word or phrase (lexical entry) in the lexicon.
 * <p>
 * Returns a Vector of TTLexEntryToObj.
 *
 * @param feat a String containing single-character ThoughtTreasure
 *             features such as <code>F_ENGLISH</code> or
 *             <code>F_FRENCH</code>. The feature codes are defined
 *             in the <code>TT</code> class.
 * @param phrase a ThoughtTreasure lexical entry
 * @return Vector of TTLexEntryToObj
 * @exception IOException if an I/O error occurs communicating with the server
 * @see TTLexEntryToObj
 * @see TT
 */
public Vector /* of TTLexEntryToObj */ phraseToConcepts(String feat,
                                                        String phrase)
throws IOException
{
  return queryLeos("PhraseToConcepts "+feat.trim()+" "+phrase.trim()+"\n");
}

/**
 * Returns the lexical entries associated with the specified object
 * (atomic concept) in the lexicon.
 * <p>
 * Returns a Vector of TTLexEntryToObj.
 *
 * @param objname the name of the object
 * @return Vector of TTLexEntryToObj
 * @exception IOException if an I/O error occurs communicating with the server
 * @see TTLexEntryToObj
 */
public Vector /* of TTLexEntryToObj */ conceptToLexEntries(String objname)
throws IOException
{
  return queryLeos("ConceptToLexEntries "+objname.trim()+"\n");
}

/**
 * Performs part-of-speech (and other) tagging of the specified
 * natural language text.
 * <p>
 * Returns a Vector of TTPNode.
 *
 * @param feat a String containing single-character ThoughtTreasure
 *             features such as <code>F_ENGLISH</code> or
 *             <code>F_FRENCH</code>. The feature codes are defined
 *             in the <code>TT</code> class.
 * @param text natural language text
 * @return Vector of TTPNode
 * @exception IOException if an I/O error occurs communicating with the server
 * @see TTPNode
 * @see TT
 */
public Vector /* of TTPNode */ tag(String feat, String text)
throws IOException
{
  return queryPNodes("Tag "+feat.trim()+" "+text+" \n");
    /* REALLY: ThoughtTreasure wants a space at the end of the text. */
}

/**
 * Syntactically parses the specified natural language text.
 * <p>
 * Returns a Vector of Java Objects representing parse trees.
 *
 * @param feat a String containing single-character ThoughtTreasure
 *             features such as <code>F_ENGLISH</code> or
 *             <code>F_FRENCH</code>. The feature codes are defined
 *             in the <code>TT</code> class.
 * @param text natural language text
 * @return Vector of Object
 * @exception IOException if an I/O error occurs communicating with the server
 * @see TT
 */
public Vector /* of Object */ syntacticParse(String feat, String text)
throws IOException
{
  return queryAssertionLines("SyntacticParse "+feat.trim()+" "+text+" \n");
    /* REALLY: ThoughtTreasure wants a space at the end of the text. */
}

/**
 * Semantically parses the specified natural language text.
 * <p>
 * Returns a Vector of Java Objects representing ThoughtTreasure assertions
 * and parse trees.
 *
 * @param feat a String containing single-character ThoughtTreasure
 *             features such as <code>F_ENGLISH</code> or
 *             <code>F_FRENCH</code>. The feature codes are defined
 *             in the <code>TT</code> class.
 * @param text natural language text
 * @return Vector of Object
 * @exception IOException if an I/O error occurs communicating with the server
 * @see TT
 */
public Vector /* of Object */ semanticParse(String feat, String text)
throws IOException
{
  return queryAssertionLines("SemanticParse "+feat.trim()+" "+text+" \n");
    /* REALLY: ThoughtTreasure wants a space at the end of the text. */
}

/**
 * Generates the specified ThoughtTreasure assertion in natural language.
 *
 * @param feat a String containing single-character ThoughtTreasure
 *             features such as <code>F_ENGLISH</code> or
 *             <code>F_FRENCH</code>. The feature codes are defined
 *             in the <code>TT</code> class.
 * @param obj Java Object representing a ThoughtTreasure assertion
 * @return String
 * @exception IOException if an I/O error occurs communicating with the server
 * @see TT
 */
public String generate(String feat, Object obj)
throws IOException
{
  return queryString("Generate "+feat.trim()+" "+
                     TT.objectToString(obj)+"\n");
}

/**
 * Sends natural language input text to the chatterbot and returns the
 * natural language response.
 *
 * @param feat a String containing single-character ThoughtTreasure
 *             features such as <code>F_ENGLISH</code> or
 *             <code>F_FRENCH</code>. The feature codes are defined
 *             in the <code>TT</code> class.
 * @param text natural language input text
 * @return String
 * @exception IOException if an I/O error occurs communicating with the server
 * @see TT
 */
public String chatterbot(String feat, String text)
throws IOException
{
  return queryString("Chatterbot "+feat.trim()+" "+text+" \n");
    /* REALLY: ThoughtTreasure wants a space at the end of the text. */
}

/**
 * Clears the discourse context.
 * <p>
 * If this does not seem to be working, remember that results are
 * cached unless <code>USECACHE</code> is set to <code>false</code>.
 * @return boolean
 * @see TTConnection#USECACHE
 */
public boolean clearContext()
{
  try {
    write("ClearContext\n");
    return readBoolean().booleanValue();
  } catch (Exception e) {
    return false;
  }
}

/**
 * Returns the status of the ThoughtTreasure server: "up" or "down".
 * @return String
 */
public String status()
{
  try {
    write("Status\n");
    String s = readline();
    if (s.equals("up")) return s;
    return "down";
  } catch (Exception e) {
    return "down";
  }
}

/**
 * Forces ThoughtTreasure to break out of the server select loop,
 * so that it can process further ThoughtTreasure shell commands.
 * <p>
 * If you would like ThoughtTreasure to exit in this situation,
 * start ThoughtTreasure with a script:
 * <pre>
 * tt -f server.tt
 * </pre>
 * where <code>server.tt</code> contains:
 * <pre>
 * server
 * quit
 * </pre>
 */
public void bringdown()
{
  try {
    write("Bringdown\n");
  } catch (Exception e) {
  }
}

/**
 * Clears the cache.
 */
public void clearCache()
{
  cache = null;
}

protected boolean queryBoolean(String query)
throws IOException
{
  if (USECACHE) {
    if (cache == null) cache = new Hashtable();
    Object r = cache.get(query);
    if (r != null) return ((Boolean)r).booleanValue();
    Boolean r1 = queryBoolean1(query);
    cache.put(query, r1);
    return r1.booleanValue();
  } else {
    return queryBoolean1(query).booleanValue();
  }
}

protected Boolean queryBoolean1(String query)
throws IOException
{
  write(query);
  return readBoolean();
}

protected String queryString(String query)
throws IOException
{
  if (USECACHE) {
    if (cache == null) cache = new Hashtable();
    Object r = cache.get(query);
    if (r != null) return (String)r;
    String r1 = queryString1(query);
    cache.put(query, r1);
    return r1;
  } else {
    return queryString1(query);
  }
}

protected String queryString1(String query)
throws IOException
{
  write(query);
  return readline().trim();
}

protected Vector /* of String */ queryColonString(String query)
throws IOException
{
  if (USECACHE) {
    if (cache == null) cache = new Hashtable();
    Object r = cache.get(query);
    if (r != null) return (Vector)r;
    Vector r1 = queryColonString1(query);
    cache.put(query, r1);
    return r1;
  } else {
    return queryColonString1(query);
  }
}

protected Vector /* of String */ queryColonString1(String query)
throws IOException
{
  write(query);
  return stringToVector(readline(), ':');
}

protected Vector /* of Object */ queryAssertionLines(String query)
throws IOException
{
  if (USECACHE) {
    if (cache == null) cache = new Hashtable();
    Object r = cache.get(query);
    if (r != null) return (Vector)r;
    Vector r1 = queryAssertionLines1(query);
    cache.put(query, r1);
    return r1;
  } else {
    return queryAssertionLines1(query);
  }
}

protected Vector /* of Object */ queryAssertionLines1(String query)
throws IOException
{
  write(query);
  Vector v = readlines();
  Vector r = new Vector();
  for (Enumeration e = v.elements(); e.hasMoreElements(); ) {
    r.addElement(TT.stringToObject((String)e.nextElement()));
  }
  return r;
}

protected Vector /* of TTLexEntryToObj */ queryLeos(String query)
throws IOException
{
  if (USECACHE) {
    if (cache == null) cache = new Hashtable();
    Object r = cache.get(query);
    if (r != null) return (Vector)r;
    Vector r1 = queryLeos1(query);
    cache.put(query, r1);
    return r1;
  } else {
    return queryLeos1(query);
  }
}

protected Vector /* of TTLexEntryToObj */ queryLeos1(String query)
throws IOException
{
  write(query);
  Vector lines = readlines();
  Vector r = new Vector();
  for (Enumeration e = lines.elements(); e.hasMoreElements() ; ) {
    Vector line = stringToVector((String)e.nextElement(), ':');

    TTLexEntry le = new TTLexEntry();
    le.citationForm = (String)line.elementAt(0);
    le.features = (String)line.elementAt(1);
    le.inflection = (String)line.elementAt(2);
    le.inflFeatures = (String)line.elementAt(3);

    TTLexEntryToObj leo = new TTLexEntryToObj();
    leo.lexentry = le;
    leo.objname = (String)line.elementAt(4);
    leo.features = (String)line.elementAt(5);

    r.addElement(leo);
  }
  return r;
}

protected Vector /* of TTPNode */ queryPNodes(String query)
throws IOException
{
  if (USECACHE) {
    if (cache == null) cache = new Hashtable();
    Object r = cache.get(query);
    if (r != null) return (Vector)r;
    Vector r1 = queryPNodes1(query);
    cache.put(query, r1);
    return r1;
  } else {
    return queryPNodes1(query);
  }
}

protected Vector /* of TTPNode */ queryPNodes1(String query)
throws IOException
{
  write(query);
  Vector lines = readlines();
  Vector r = new Vector();
  for (Enumeration e = lines.elements(); e.hasMoreElements() ; ) {
    Vector line = stringToVector((String)e.nextElement(), ':');

    TTLexEntry le = new TTLexEntry();
    le.citationForm = (String)line.elementAt(0);
    le.features = (String)line.elementAt(1);
    le.inflection = (String)line.elementAt(2);
    le.inflFeatures = (String)line.elementAt(3);

    TTLexEntryToObj leo = new TTLexEntryToObj();
    leo.lexentry = le;
    leo.objname = (String)line.elementAt(4);
    leo.features = (String)line.elementAt(5);

    TTPNode pn = new TTPNode();
    pn.feature = TT.featureGet(le.features, TT.FT_POS, TT.F_NULL);
    pn.leo = leo;
    pn.startpos = TT.longParse((String)line.elementAt(6), true);
    pn.endpos = TT.longParse((String)line.elementAt(7), true);

    r.addElement(pn);
  }
  return r;
}

protected void write(String s)
throws IOException
{
  byte[] buf = s.getBytes();
  out.write(buf, 0, buf.length);
  out.flush();
  if (DEBUG) dbg("TO TT: <"+s+">");
}

protected String readline()
throws IOException
{
  String s = in.readLine();
  if (DEBUG) dbg("FROM TT: <"+s+">");
  if (s.startsWith("error:")) {
    dbg("ThoughtTreasure "+s);
    return "";
  }
  return s;
}

protected Vector /* of String */ readlines()
throws IOException
{
  Vector r = new Vector();
  while (true) {
    String s = in.readLine();
    if (DEBUG) dbg("FROM TT: <"+s+">");
    if (s.equals(".")) break;
    if (s.startsWith("error:")) break;
    r.addElement(s);
  }
  return r;
}

protected Boolean readBoolean()
throws IOException
{
  String s = readline();
  if (s.equals("0")) return Boolean.FALSE;
  else if (s.equals("1")) return Boolean.TRUE;
  if (s.length() != 0) {
    dbg("TTConnection.readBoolean: unable to parse <"+s+">");
  }
  return Boolean.FALSE;
}

static class ResultChar {
  char ch = 0;
}

protected static int indexOfAny(String s, String set, int fromIndex,
                                /* RESULTS */ ResultChar rc)
{
  int minIndex = 999999;
  char minChar = 0;
  boolean found = false;
  int len = set.length();
  for (int i = 0; i < len; i++) {
    char ch = set.charAt(i);
    int j = s.indexOf(set.charAt(i), fromIndex);
    if (j >= 0) {
      if ((!found) || j < minIndex) {
        found = true;
        minIndex = j;
        minChar = ch;
      }
    }
  }
  rc.ch = minChar;
  if (found) return minIndex;
  return -1;
}

protected static Vector /* of String */ stringToVector(String s, int delim)
{
  StringBuffer elem = new StringBuffer();
  Vector r = new Vector();
  int len = s.length();
  char c;
  for (int i = 0; i < len; i++) {
    c = s.charAt(i);
    if (c==delim) {
      r.addElement(elem.toString());
      elem = new StringBuffer();
    } else elem.append(c);
  }
  r.addElement(elem.toString());
  return r;
}

protected static void dbg(String s)
{
  System.out.println(s);
  System.out.flush();
}

}

/* End of file. */
