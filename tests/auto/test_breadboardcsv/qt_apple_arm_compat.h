#ifndef TEST_BREADBOARDCSV_QT_APPLE_ARM_COMPAT_H
#define TEST_BREADBOARDCSV_QT_APPLE_ARM_COMPAT_H

/*
 * Qt 6.10 may use the Arm __yield intrinsic before declaring it.
 * Keep this compiler pre-include safe for universal macOS builds.
 */
#if defined(__APPLE__) && defined(__aarch64__) && __has_include(<arm_acle.h>)
#include <arm_acle.h>
#endif

#endif
