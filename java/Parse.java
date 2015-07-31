/* Example of how to invoke ThoughtTreasure's syntactic and
 * semantic parser from Java.
 */

package net.erikmueller.tt;

import java.lang.*;
import java.util.*;
import net.erikmueller.tt.*;

public class Parse {

public static void main(String args[])
{
  if (args.length != 1) {
    System.err.println("Usage: java Parse \"sentence\"");
    System.exit(1);
  }
  String s = args[0];
  try {
    /* Open a connection to a ThoughtTreasure server. */
    TTConnection tt = new TTConnection("somehost", 1832);

    /* Run the syntactic parser. */
    System.out.println(
      TT.objectToPrettyString(
        tt.syntacticParse(String.valueOf(TT.F_ENGLISH), s)));

    /* Run the semantic parser. */
    System.out.println(
      TT.objectToPrettyString(
        tt.semanticParse(String.valueOf(TT.F_ENGLISH), s)));

    /* Close the connection. */
    tt.close();
  } catch (Exception e) {
    System.err.println("Trouble parsing \""+s+"\":");
    e.printStackTrace(System.out);
    System.exit(1);
  }
}

}
