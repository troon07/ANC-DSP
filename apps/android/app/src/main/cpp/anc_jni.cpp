#include <jni.h>
#include "anc/simulation.h"
extern "C" JNIEXPORT jdouble JNICALL Java_com_universalanc_NativeReference_simulate(JNIEnv*,jobject) {
    return anc::simulate("broadband",[](int,float,float,float,float){}).attenuation;
}
