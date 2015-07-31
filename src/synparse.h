/* synparse.c */
void StopAtInit(void);
void StopAtNodes(PNode *pn1, PNode *pn2, int feat);
void StopAtSet(int sa1, int sa2, int safeat);
void Syn_ParseSem_Parse(PNode *pn, Channel *ch, int tgtlang, Discourse *dc);
void Syn_ParseFragments(Channel *ch, int tgtlang, Discourse *dc, int eoschar);
void Syn_ParseParse(Channel *ch, Discourse *dc, size_t in_lowerb, size_t in_upperb, int eoschar);
void Syn_ParseParseDone(Channel *ch);
void Syn_ParseAssignPnum(Channel *ch, PNode *pn);
Bool Syn_ParseIsTopLevelSentence(PNode *pn);
PNode *Syn_ParseGetSemParseConstituent(PNode *pn, PNode *pn1, PNode *pn2);
void Syn_ParseAdd(Channel *ch, Discourse *dc, char feature, PNode *pn1, PNode *pn2, Float score, size_t lowerb, size_t upperb, int lang);
Bool Syn_ParseApplySingletonRules(Channel *ch, Discourse *dc, PNode *pn, int lang);
Bool Syn_ParseApplyRules(Channel *ch, Discourse *dc, PNode *pn1, int lang);
void Syn_ParsePrintPNodes(FILE *stream, Channel *ch);
Bool Syn_ParseIsVPRestrict(PNode *w1, PNode *w2);
Bool Syn_IsBadVP(PNode *pn, int lang);
Bool Syn_ParseIsValidMAX(PNode *pn1, PNode *pn2, int k, int lang);
Float Syn_ParseDoesRuleFire(PNode *pn1, PNode *pn2, int k, int lang, Discourse *dc);
