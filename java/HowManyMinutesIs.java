/*
 * HowManyMinutesIs.java
 *
 * Uses ThoughtTreasure to guess the length of a calendar event
 * in minutes.
 *
 * Usage: java HowManyMinutesIs "summary of calendar event"
 * 
 */

package net.erikmueller.tt;

import java.io.*;
import java.lang.*;
import java.util.*;
import net.erikmueller.tt.*;

public class HowManyMinutesIs {

protected static final double NA = (double)-999999999.0;
protected static boolean DEBUG = false;

public static void main(String args[])
{
  if (args.length != 1) {
    System.err.println("Usage: java HowManyMinutesIs \"summary\"");
    System.exit(1);
  }
  String summary = args[0];
  try {
    TTConnection tt = new TTConnection("somehost");
    TTConnection.DEBUG = DEBUG;
    double length = guessEventLength(summary, tt);
    if (length != NA) {
      System.out.println(length);
    } else {
      System.out.println("unknown");
    }
    tt.close();
  } catch (Exception e) {
    System.err.println("Trouble processing \""+summary+"\":");
    e.printStackTrace(System.out);
    System.exit(1);
  }
}

protected static double guessEventLength(String summary, TTConnection tt)
throws IOException
{
  Vector parseNodes = tt.tag(String.valueOf(TT.F_ENGLISH), summary);
  double sum = 0.0, count = 0.0;
  for (Enumeration e1 = parseNodes.elements(); e1.hasMoreElements(); ) {
    TTPNode parseNode = (TTPNode)e1.nextElement();
    if (parseNode.leo == null || parseNode.leo.objname == null) continue;
    String objname = parseNode.leo.objname;
    Vector objects = tt.retrieve(2, 1, -1, "anc", "duration-of "+objname+" ?");
    for (Enumeration e2 = objects.elements(); e2.hasMoreElements(); ) {
      double minutes = getMinutes(e2.nextElement());
      if (minutes != NA) {
        if (DEBUG) System.err.println(objname+": "+minutes);
        if (minutes < 720.0) {
          sum += minutes;
          count += 1.0;
        }
      }
    }
  }
  if (count == 0.0) return NA;
  return sum/count;
}

protected static double getMinutes(Object o)
{
  try {
    return (((Double)((Vector)o).elementAt(1)).doubleValue())/60.0;
  } catch (Exception e) {
    return NA;
  }
}

}

/* Sample runs:

$ java HowManyMinutesIs "shave"
1.0
$ java HowManyMinutesIs "breakfast"
30.0
$ java HowManyMinutesIs "dog training course"
60.0
$ java HowManyMinutesIs "tennis w/Susanna"
150.0
$ java HowManyMinutesIs "surf the web"
60.0
$ java HowManyMinutesIs "dinner with Alex and Chris"
120.0
$ java HowManyMinutesIs "zzzz"
unknown
$

*/
