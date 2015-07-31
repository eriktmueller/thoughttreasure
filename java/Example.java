/* Example of how to use ThoughtTreasure from Java. */

package net.erikmueller.tt;

import java.lang.*;
import java.util.*;
import net.erikmueller.tt.*;

public class Example {

public static void main(String args[])
{
  try {
    /* Open a connection to a ThoughtTreasure server. */
    TTConnection tt = new TTConnection("somehost");

    /* Ask the server questions. */
    System.out.println("> What's the status of the server?");
    System.out.println(tt.status());

    System.out.println("> Is Evian a beverage?");
    System.out.println(tt.ISA("beverage", "Evian"));

    System.out.println("> Is friendship an interpersonal relation?");
    System.out.println(tt.ISA("ipr", "friend-of"));

    System.out.println("> Is a fingernail a part of a human?");
    System.out.println(tt.isPartOf("fingernail", "human"));

    System.out.println("> What is Evian?");
    System.out.println(tt.parents("Evian"));
    System.out.println(tt.ancestors("Evian"));

    System.out.println("> What are the phases of life?");
    System.out.println(tt.children("phase-of-life"));
    System.out.println(tt.descendants("phase-of-life"));

    System.out.println("> At what age is someone called a young adult?");
    Vector r = tt.retrieve(-1,-1,-1,"exact","min-value-of young-adult ?");
    System.out.println(r);
    for (Enumeration e = r.elements(); e.hasMoreElements(); ) {
      System.out.println(TT.objectToPrettyString(e.nextElement()));
    }

    System.out.println("> How long does a play typically last?");
    r = tt.retrieve(2,1,-1,"anc","duration-of attend-play ?");
    System.out.println(r);

    System.out.println("> Where is a mini-bar found?");
    r = tt.retrieve(-1,-1,1,"desc","at-grid mini-bar ?");
    for (Enumeration e = r.elements(); e.hasMoreElements(); ) {
      System.out.println(TT.objectToPrettyString(e.nextElement()));
    }

    System.out.println("> What concepts does 'expensive' express?");
    System.out.println(
      tt.phraseToConcepts(String.valueOf(TT.F_ENGLISH), "expensive"));

    System.out.println("> What concepts does 'granddads' express?");
    System.out.println(
      tt.phraseToConcepts(String.valueOf(TT.F_ENGLISH), "granddads"));

    System.out.println("> What words or phrases express adult?");
    System.out.println(tt.conceptToLexEntries("adult"));

    System.out.println(
      "> What words or phrases are in 'bright blues and greens'?");
    System.out.println(
      tt.tag(String.valueOf(TT.F_ENGLISH), "bright blues and greens"));

    System.out.println(
      "> What are the syntactic parse trees of 'Mary had a little lamb.'?");
    System.out.println(
      tt.syntacticParse(String.valueOf(TT.F_ENGLISH),
                        "Mary had a little lamb."));

    System.out.println(
      "> What assertions does 'Mary had a little lamb.' express?");
    System.out.println(
      tt.semanticParse(String.valueOf(TT.F_ENGLISH),
                       "Mary had a little lamb."));

    System.out.println(
      "> What sentence expresses the assertion [occupation-of Jim artist]?");
    System.out.println(
      tt.generate(String.valueOf(TT.F_ENGLISH),
                  TT.stringToObject("[occupation-of Jim artist]")));

    System.out.println("> Can you clear the context?");
    System.out.println(tt.clearContext());
      /* Prevent "he" from being generated in next call to generate. */

    System.out.println(
      "> What sentence expresses the assertion [good Jim] in slang?");
    System.out.println(
      tt.generate(String.valueOf(TT.F_ENGLISH)+
                  String.valueOf(TT.F_SLANG),
                  TT.stringToObject("[good Jim]")));

    System.out.println("> Who created Bugs Bunny?");
    System.out.println(
      tt.chatterbot(String.valueOf(TT.F_ENGLISH), "Who created Bugs Bunny?"));

    /* Close the connection. */
    tt.close();
  } catch (Exception e) {
    e.printStackTrace(System.out);
  }
}

}

/* The output of the above is:

> What's the status of the server?
up
> Is Evian a beverage?
true
> Is friendship an interpersonal relation?
true
> Is a fingernail a part of a human?
true
> What is Evian?
[flat-water]
[db/all.txt, concept, object, matter, physical-object, db/fooddrug.txt, food, beverage, drinking-water, flat-water]
> What are the phases of life?
[not-alive, alive]
[dead, nonborn, not-yet-conceived, not-alive, very-old-adult, old-adult, middle-aged-adult, thirty-something, young-adult, adult-animal, adult, teenager, infant, child, alive]
> At what age is someone called a young adult?
[[min-value-of, young-adult, [NUMBER, 18.0, year]]]
[min-value-of
 young-adult
 [NUMBER
  18.0u
  year]]
> How long does a play typically last?
[[NUMBER, 7200.0, second]]
> Where is a mini-bar found?
[at-grid
 mini-bar1934
 Park-Plaza-1E
 [GRIDSUBSPACE
  1
  19]]
> What concepts does 'expensive' express?
[expensive:Az:expensive:Az:expensive:¹:0.1:1.0]
> What concepts does 'granddads' express?
[granddad:MNz:granddads:PNz:grandfather-of:T:0.1:1.0]
> What words or phrases express adult?
[come:Vz¸:come:Vz¸:adult:q:0.1:1.0, mature:Vz:mature:Vz:adult::0.1:1.0, grow:Vz¸:grow:Vz¸:adult::0.1:1.0, adult life:Nz:adult life:Nz:adult:mÎ:0.1:1.0, maturity:Nz:maturity:Nz:adult:mÎ:0.1:1.0, adulthood:Nz:adulthood:Nz:adult:mÎ:0.1:1.0, grown woman:FNz:grown woman:FNz:adult:ş:0.1:1.0, grown man:MNz:grown man:MNz:adult:ş:0.1:1.0, grown up:Az:grown up:Az:adult:T:0.1:1.0, grown up:Nz:grown up:Nz:adult:T:0.1:1.0, adult:Az¸:adult:Az¸:adult:¹:0.1:1.0, adult:Nz¸:adult:Nz¸:adult:¹:0.1:1.0]
> What words or phrases are in 'bright blues and greens'?
[bright:Az¸:bright:Az¸:bright:¹:0.1:1.0:0:6, blues:·Nz¸:blues:·Nz¸:blues:m:0.1:1.0:7:12, blue:·Nz¸:blues:P·Nz¸:other-uniform:Pï:0.1:1.0:7:12, blue:·Nz¸:blues:P·Nz¸:blue:¹:0.1:1.0:7:12, and:Kz¸:and:Kz¸:and:¹w©:0.1:1.0:13:16, and:Kz¸:and:Kz¸:add:T:0.1:1.0:13:16, green:Nz¸:greens:PNz¸:green:¹:0.1:1.0:17:23, green:Nz¸:greens:PNz¸:aasn-Green::0.1:1.0:17:23]
> What are the syntactic parse trees of 'Mary had a little lamb.'?
[SENTENCE, [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:iVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [null:-1:-1, [little:·Az¸:little:6·Az¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:dVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [null:-1:-1, [little:·Az¸:little:6·Az¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:iVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a little:Az:a little:Az:null::0.1:1.0:-1:-1]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:dVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a little:Az:a little:Az:null::0.1:1.0:-1:-1]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:iVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [little:·Nz¸:little:S·Nz¸:null::0.1:1.0:-1:-1]]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:dVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [little:·Nz¸:little:S·Nz¸:null::0.1:1.0:-1:-1]]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [null:-1:-1, [have:Vz¸:had:dVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [little:·Nz¸:little:S·Nz¸:null::0.1:1.0:-1:-1]]]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [null:-1:-1, [have:Vz¸:had:iVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [little:·Nz¸:little:S·Nz¸:null::0.1:1.0:-1:-1]]]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]]
> What assertions does 'Mary had a little lamb.' express?
[SENTENCE, [past-participle, [cpart-of, [such-that, lamb-food, [equiv, lamb-food, [indefinite-article, small-quantity]]], [such-that, human, [NAME-of, human, [NAME, Mary]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:dVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [little:·Nz¸:little:S·Nz¸:null::0.1:1.0:-1:-1]]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [past-participle, [eat, [such-that, human, [NAME-of, human, [NAME, Mary]]], [such-that, lamb-food, [equiv, lamb-food, [indefinite-article, small-quantity]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:dVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [little:·Nz¸:little:S·Nz¸:null::0.1:1.0:-1:-1]]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [preterit-indicative, [cpart-of, [such-that, lamb-food, [equiv, lamb-food, [indefinite-article, small-quantity]]], [such-that, human, [NAME-of, human, [NAME, Mary]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:iVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [little:·Nz¸:little:S·Nz¸:null::0.1:1.0:-1:-1]]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [preterit-indicative, [eat, [such-that, human, [NAME-of, human, [NAME, Mary]]], [such-that, lamb-food, [equiv, lamb-food, [indefinite-article, small-quantity]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:iVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a:·Dz¸:a:S·Dz¸:null::0.1:1.0:-1:-1], [null:-1:-1, [little:·Nz¸:little:S·Nz¸:null::0.1:1.0:-1:-1]]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [past-participle, [cpart-of, [such-that, lamb-food, [a-little, lamb-food]], [such-that, human, [NAME-of, human, [NAME, Mary]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:dVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a little:Az:a little:Az:null::0.1:1.0:-1:-1]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [past-participle, [eat, [such-that, human, [NAME-of, human, [NAME, Mary]]], [such-that, lamb-food, [a-little, lamb-food]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:dVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a little:Az:a little:Az:null::0.1:1.0:-1:-1]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]], [preterit-indicative, [cpart-of, [such-that, lamb-food, [a-little, lamb-food]], [such-that, human, [NAME-of, human, [NAME, Mary]]]]], [null:-1:-1, [null:-1:-1, [NAME:Mary:N:NAME:Mary:N:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [have:Vz¸:had:iVz¸:null::0.1:1.0:-1:-1]], [null:-1:-1, [null:-1:-1, [a little:Az:a little:Az:null::0.1:1.0:-1:-1]], [null:-1:-1, [lamb:Nz¸:lamb:SNz¸:null::0.1:1.0:-1:-1]]]]]]
> What sentence expresses the assertion [occupation-of Jim artist]?
Jim Garnier is an artist.
> Can you clear the context?
true
> What sentence expresses the assertion [good Jim] in slang?
Jim Garnier is slamming.
> Who created Bugs Bunny?
Tex Avery created Bugs Bunny.

*/
