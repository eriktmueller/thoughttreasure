/*
 * ThoughtTreasure
 * Copyright 1996, 1997, 1998, 1999, 2015 Erik Thomas Mueller.
 * All Rights Reserved.
 *
 * 19951114: begun
 */

#include "tt.h"
#include "toolfilt.h"
#include "repbasic.h"
#include "utildbg.h"

/* See Greenbaum and Quirk (1990, p. 161) */

void Tool_Filter_BZ_Z(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "---If B and NO GOOD, add feature °:\n");
  fprintf(stream, "° *%s he ran upstairs.\n", cword);
  fprintf(stream, "° *%s he disappeared.\n", cword);
  fprintf(stream, "° *%s he wanted to finish the painting.\n", cword);
}

void Tool_Filter_BW_W(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "---If B and NO GOOD, add feature Þ:\n");
  fprintf(stream, "Þ *He %s ran upstairs.\n", word);
  fprintf(stream, "Þ *He %s wanted to finish the painting.\n", word);
}

void Tool_Filter_WB_W(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "---If B and NO GOOD, add feature ð:\n");
  fprintf(stream, "ð *He ran %s upstairs.\n", word);
  fprintf(stream, "ð *He wanted %s to finish the painting.\n", word);
}

void Tool_Filter_ZB_Z(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "---If B and NO GOOD, add feature ¿:\n");
  fprintf(stream, "¿ *He ran upstairs %s.\n", word);
  fprintf(stream, "¿ *He wanted to finish the painting %s.\n", word);
  fprintf(stream, "¿ *He disappeared %s.\n", word);
}

void Tool_Filter_BE_E(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "---If B and NO GOOD, add feature Æ:\n");
  fprintf(stream, "Æ *He is %s funny.\n", word);
  fprintf(stream, "Æ *That is a %s funny thing.\n", word);
}

void Tool_Filter_B(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "ADVERB (feature B):_____________________________\n");
  Tool_Filter_BZ_Z(stream, word, cword);
  Tool_Filter_BW_W(stream, word, cword);
  Tool_Filter_WB_W(stream, word, cword);
  Tool_Filter_ZB_Z(stream, word, cword);
  Tool_Filter_BE_E(stream, word, cword);
}

void Tool_Filter_KK(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "---If K and good, add ±:\n");
  fprintf(stream, "± He is working hard %s finishing the painting.\n", word);
  fprintf(stream, "---else if K and good, add Ï:\n");
  fprintf(stream, "Ï He is working hard %s to finish the painting.\n", word);
  fprintf(stream, "---else if K and good, add ÷ (or nothing):\n");
  fprintf(stream, "÷ He is tired %s he is happy.\n", word);
  fprintf(stream, "÷ He works hard %s he never finishes a painting.\n", word);
}

void Tool_Filter_Kw(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "---If K and good, add ±:\n");
  fprintf(stream, "± %s finishing the painting, he is working hard.\n", cword);
  fprintf(stream, "---else if K and good, add Ï:\n");
  fprintf(stream, "Ï %s to finish the painting, he is working hard.\n", cword);
  fprintf(stream, "---else----if K and NO GOOD, add feature w:\n");
  fprintf(stream, "w *%s he is tired, he is happy.\n", cword);
  fprintf(stream, "w *%s he works hard, he never finishes a painting.\n",
          cword);
  fprintf(stream,
          "---else----if K and feature w, the following should be OK:\n");
  fprintf(stream, "  He is happy, %s he is tired.\n", word);
  fprintf(stream, "  He never finishes a painting, %s he works hard.\n", word);
  fprintf(stream, "-----------if K and good, add feature ©:\n");
  fprintf(stream, "© Carrie %s Karen work hard.\n", word);
  fprintf(stream, "© Karen works %s plays hard.\n", word);
  fprintf(stream, "© Karen is smart %s funny.\n", word);
  fprintf(stream, "© Karen sent a copy to the store %s to the newspaper.\n",
          word);

}

void Tool_Filter_K(FILE *stream, char *word, char *cword)
{
  fprintf(stream, "CONJUNCTION (feature K):________________________\n");
  Tool_Filter_KK(stream, word, cword);
  Tool_Filter_Kw(stream, word, cword);
}

void Tool_Filter1(FILE *stream, char *word, char *cword)
{
  Tool_Filter_K(stream, word, cword);
  Tool_Filter_B(stream, word, cword);
}

void Tool_Filter(FILE *out)
{
  char	phrase[PHRASELEN], cphrase[PHRASELEN];
  printf("Welcome to the filter feature tool.\n");
  while (1) {
    if (!StreamAskStd("Enter phrase: ", PHRASELEN, phrase)) return;
    if (phrase[0] != TERM) {
      strcpy(cphrase, phrase);
      cphrase[0] = CharToUpper(phrase[0]);
      Tool_Filter1(out, phrase, cphrase);
      StreamSep(out);
    }
  }
}

/* End of file. */
