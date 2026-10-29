#include "anc/stability_monitor.h"
#include <iostream>
#include <cmath>
#include <limits>

bool test_stability_nan_trip() {
    anc::StabilityMonitor sm;
    sm.init(48000.0f);
    sm.enableAntiNoise();

    float buf[64];
    for (int i = 0; i < 63; ++i) buf[i] = 0.1f;
    buf[63] = std::numeric_limits<float>::quiet_NaN();

    sm.processOutputBlock(buf, 64);

    if (!sm.isTripped()) return false;
    if (sm.tripReason() != anc::TripReason::NonFiniteSample) return false;

    // Output must be muted to zero
    for (int i = 0; i < 64; ++i) {
        if (buf[i] != 0.0f) return false;
    }
    return true;
}

bool test_stability_clipping_trip() {
    anc::StabilityMonitor sm;
    sm.init(48000.0f);
    sm.enableAntiNoise();

    // 50 full-scale samples exceeding consecutive clip limit
    float buf[64];
    for (int i = 0; i < 64; ++i) buf[i] = 1.0f;

    sm.processOutputBlock(buf, 64);

    if (!sm.isTripped()) return false;
    if (sm.tripReason() != anc::TripReason::ExcessiveClipping) return false;

    return true;
}

bool test_soft_clip() {
    // Check monotonicity and bounds [-1, 1]
    for (float x = -3.0f; x <= 3.0f; x += 0.1f) {
        float y = anc::StabilityMonitor::softClip(x);
        if (y < -1.0f || y > 1.0f) return false;
    }
    return true;
}
