/*
 * WhereDoIFind.java
 *
 * Uses ThoughtTreasure to determine where a physical object is
 * typically found.
 *
 * Usage: java WhereDoIFind "name of a physical object"
 */

package net.erikmueller.tt;

import java.io.*;
import java.lang.*;
import java.util.*;
import net.erikmueller.tt.*;

public class WhereDoIFind {

protected static final double NA = (double)-999999999.0;
protected static boolean DEBUG = false;

public static void main(String args[])
{
  if (args.length != 1) {
    System.err.println("Usage: java WhereDoIFind \"physical object\"");
    System.exit(1);
  }
  String physobj = args[0];
  try {
    TTConnection tt = new TTConnection("somehost");
    whereDoIFind(physobj, tt);
    tt.close();
  } catch (Exception e) {
    System.err.println("Trouble processing \""+physobj+"\":");
    e.printStackTrace(System.out);
    System.exit(1);
  }
}

protected static void whereDoIFind(String physobj, TTConnection tt)
throws IOException
{
  Vector parseNodes = tt.tag(String.valueOf(TT.F_ENGLISH), physobj);
  for (Enumeration e = parseNodes.elements(); e.hasMoreElements(); ) {
    TTPNode parseNode = (TTPNode)e.nextElement();
    if (parseNode.leo == null || parseNode.leo.objname == null) continue;
    whereDoIFindObj(parseNode.leo.objname, tt);
  }
}

protected static void whereDoIFindObj(String enclosed0, TTConnection tt)
throws IOException
{
  Vector objects = tt.retrieve(-1, -1, 1, "desc", "at-grid "+enclosed0);
  for (Enumeration e = objects.elements(); e.hasMoreElements(); ) {
    try {
      Vector assertion = (Vector)e.nextElement();
      String enclosed = (String)assertion.elementAt(1);
      String grid = (String)assertion.elementAt(2);
      Vector gss = (Vector)assertion.elementAt(3);
      int x = getCoord((Vector)gss.elementAt(1));
      int y = getCoord((Vector)gss.elementAt(2));
      whatIsInGridAt(grid, x, y, enclosed, tt);
    } catch (Exception f) {
      f.printStackTrace(System.out);
    }
  }
}

protected static void whatIsInGridAt(String grid, int x, int y, String enclosed,
                                     TTConnection tt)
throws IOException
{
  Vector objects = tt.retrieve(1, -1, -1, "exact", "at-grid ? "+grid);
  for (Enumeration e = objects.elements(); e.hasMoreElements(); ) {
    try {
      String enclosing = (String)e.nextElement();
      if (enclosing.equals(enclosed)) continue;
      if (isContainedIn(grid, x, y, enclosing, tt)) {
        if (DEBUG) System.out.print(enclosed+" in "+enclosing+": ");
        System.out.println(tt.generate(String.valueOf(TT.F_ENGLISH),
                                       enclosing));
      }
    } catch (Exception f) {
      f.printStackTrace(System.out);
    }
  }
}

protected static boolean isContainedIn(String grid, int x0, int y0,
                                       String enclosing, TTConnection tt)
throws IOException
{
  Vector objects =
    tt.retrieve(-1, -1, 1, "desc", "at-grid "+enclosing+" "+grid);
  for (Enumeration e = objects.elements(); e.hasMoreElements(); ) {
    try {
      Vector assertion = (Vector)e.nextElement();
      Vector gss = (Vector)assertion.elementAt(3);
      int size = gss.size();
      if (size < 4) continue;
      for (int i = 1; i < size; i += 2) {
        int x1 = getCoord((Vector)gss.elementAt(i));
        int y1 = getCoord((Vector)gss.elementAt(i+1));
        if (x0 == x1 && y0 == y1) return true;
      }
    } catch (Exception f) {
      f.printStackTrace(System.out);
    }
  }
  return false;
}

protected static int getCoord(Vector number)
{
  try {
    return ((Double)number.elementAt(1)).intValue();
  } catch (Exception e) {
    e.printStackTrace(System.out);
    return -1;
  }
}

}

/* Sample runs:

$ java WhereDoIFind "Tropicana orange juice"
A small city food store.
$ java WhereDoIFind lettuce
A small city food store.
$ java WhereDoIFind "minibar"
A hotel room.
$ java WhereDoIFind "runway"
A John F. Kennedy international terminal.
$

*/
