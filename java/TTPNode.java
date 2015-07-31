/*
 * TTPNode
 *
 * Copyright 1998, 1999, 2015 Erik Thomas Mueller
 *
 * 19981109T074747: begun
 * 19981116T152241: added constituents
 */

package net.erikmueller.tt;

/**
 * A ThoughtTreasure parse node, representing information about
 * a textual entity (such as a word or phrase) in a text.
 *
 * @author Erik T. Mueller
 * @version 0.00023
 */

public class TTPNode {

/**
 * The part of speech feature (from <code>TT.FT_POS</code>) or constituent
 * feature (from <code>TT.FT_CONSTIT</code>).
 *
 * @see TT
 */
public char feature;

/**
 * A word-meaning association found for the textual entity in
 * the text.
 */
public TTLexEntryToObj leo;

/**
 * The start position of the textual entity in the text.
 */
public long startpos;

/**
 * The end position of the textual entity in the text.
 */
public long endpos;

public String toString()
{
  StringBuffer r = new StringBuffer();
  r.append(feature);
  if (leo != null) r.append(":"+leo);
  if (startpos != -1) r.append(":"+startpos);
  if (endpos != -1) r.append(":"+endpos);
  return r.toString();
}

}

/* End of file. */
