/*
 * TTLexEntry
 *
 * Copyright 1998, 1999, 2015 Erik Thomas Mueller
 *
 * 19981109T074747: begun
 */

package net.erikmueller.tt;

/**
 * A ThoughtTreasure lexical entry (word or phrase).
 *
 * @author Erik T. Mueller
 * @version 0.00023
 */

public class TTLexEntry {

/**
 * The citation form of the lexical entry, such as the infinitive
 * form of a verb or the singular form of a noun.
 */
public String citationForm;

/**
 * A String containing single-character ThoughtTreasure feature
 * codes (such as part of speech and language) for the lexical entry.
 * The feature codes are defined in the <code>TT</code> class.
 *
 * @see TT
 */
public String features;

/**
 * A particular inflection of the lexical entry, such as the
 * third-person singular form of a verb.
 */
public String inflection;

/**
 * A String containing single-character ThoughtTreasure feature
 * codes (such as <code>F_SINGULAR</code> or <code>F_PLURAL</code>)
 * for the particular inflection of the lexical entry. The feature
 * codes are defined in the <code>TT</code> class.
 *
 * @see TT
 */
public String inflFeatures;

public String toString()
{
  return citationForm+":"+
         features+":"+
         inflection+":"+
         inflFeatures;
}

}

/* End of file. */
