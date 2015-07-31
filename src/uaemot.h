/* uaemot.c */
Obj *UA_Emotion_GCtoEC(Obj *rel, Obj *goal_class);
Obj *UA_Emotion_GSCtoEC(Obj *goal_status, Obj *goal_class);
Obj *UA_Emotion_GStoAEC(Obj *goal_status);
Bool UA_Emotion_IsForGoal(Obj *rel, Obj *emot_class, Obj *goal);
Float UA_Emotion_Decay(Actor *ac, Float factor);
Float UA_Emotion_Overall(Actor *ac);
void UA_EmotionAdd(Actor *ac, Ts *ts, Obj *emotion, Obj *goal, Obj *action);
void UA_EmotionGoal(Actor *ac, Ts *ts, Obj *a, Obj *goal, ObjList *causes);
void UA_Emotion_FortunesOfOthers(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *other, Float weight, Obj *other_emot_class);
Bool UA_Emotion_ExistingGoals(Actor *ac, Ts *ts, Obj *a, Obj *in);
Bool UA_Emotion_NewGoals(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *rel, Obj *goal_status);
void UA_Emotion1(Actor *ac, Ts *ts, Obj *a, Obj *in);
void UA_Emotion(Actor *ac, Ts *ts, Obj *a, Obj *in);
void UA_Emotion_TsrIn(Actor *ac, Ts *ts, Obj *a, TsRange *tsr);
Float UA_EmotionSignedWeight(Obj *emot);
