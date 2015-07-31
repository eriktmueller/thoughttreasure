/* utilbb.c */
ABrainTask *BBrainBegin(Obj *process, Dur timelimit, int recursion_limit);
void BBrainEnd(ABrainTask *abt);
void BBrainABrainRecursion(ABrainTask *abt);
Bool BBrainStopsABrain(ABrainTask *abt);
