/* taname.c */
Name *NameCreate(void);
void NameFree(Name *nm);
void NamePrint(FILE *stream, Name *nm);
void TA_NameInit(void);
void ReportAffix(Text *text, Discourse *dc);
void TA_NameParse1(char *phrase, Obj *class, int relax_ok, int always_relax, int *found_exact, int *found_relaxed, ObjList **objs);
Bool LenGE(int len, int desired_len, int len_exact, int *IsPosttitle, int o);
Bool TA_NameParse2(Name *nm, int len, int *term, int *IsGivenname, int *IsSurname, int *IsInitial, int *IsGivennameRelaxed, int *IsSurnameRelaxed, int *IsPosttitle, ObjList **GivennameObjs, ObjList **SurnameObjs, char **words, char *orig_in, int gender, int len_exact, int revok, int *offset);
Bool NameIsParticle(char *out);
Name *TA_NameParse(char *in, int gender, int known_to_be_name, int revok, char **nextp, char *trailpunc);
Name *TA_NameParseKnown(char *name, int gender, int revok);
Bool TA_Name(char *in, Discourse *dc, Channel *ch, char **nextp);
Bool TA_FrenchPolity(char *in, Discourse *dc, Channel *ch, char **nextp);
