/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_vadd_coprocJni_CoprocJni */

#ifndef _Included_com_vadd_coprocJni_CoprocJni
#define _Included_com_vadd_coprocJni_CoprocJni
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_vadd_coprocJni_CoprocJni
 * Method:    initCoproc
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_vadd_coprocJni_CoprocJni_initCoproc
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_vadd_coprocJni_CoprocJni
 * Method:    exitCoproc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_vadd_coprocJni_CoprocJni_exitCoproc
  (JNIEnv *, jclass);

/*
 * Class:     com_vadd_coprocJni_CoprocJni
 * Method:    vadd
 * Signature: (J[J[J[J)J
 */
JNIEXPORT jlong JNICALL Java_com_vadd_coprocJni_CoprocJni_vadd
  (JNIEnv *, jclass, jlong, jlongArray, jlongArray, jlongArray);

#ifdef __cplusplus
}
#endif
#endif
