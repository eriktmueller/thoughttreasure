/*
 * TTLexEntryToObj
 *
 * Copyright 1998, 1999, 2015 Erik Thomas Mueller
 *
 * 19981109T074747: begun
 */

package net.erikmueller.tt;

/**
 * A link between a ThoughtTreasure lexical entry and a ThoughtTreasure
 * object. That is, a word-meaning association.
 *
 * @author Erik T. Mueller
 * @version 0.00023
 */

public class TTLexEntryToObj {

/**
 * A lexical entry (word or phrase).
 */
public TTLexEntry lexentry;

/**
 * An object name (meaning).
 */
public String objname;

/**
 * A String containing single-character ThoughtTreasure feature
 * codes, specifying features that apply to this particular
 * word-meaning association. The feature codes are defined in
 * the <code>TT</code> class.
 *
 * @see TT
 */
public String features;

public String toString()
{
  return lexentry+":"+
         objname+":"+
         features;
}

}

/* End of file. */
