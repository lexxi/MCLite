#pragma once
#include <cmath>
namespace mclite {
// Liang-Barsky clipping keeps world pixels out of LVGL's int16 coordinates.
inline bool clipPathLine(double& x0, double& y0, double& x1, double& y1,
                         double width, double height) {
    if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1)) return false;
    double dx = x1 - x0, dy = y1 - y0, low = 0, high = 1;
    double p[] = {-dx, dx, -dy, dy};
    double q[] = {x0, width - x0, y0, height - y0};
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0) { if (q[i] < 0) return false; continue; }
        double r = q[i] / p[i];
        if (p[i] < 0) { if (r > high) return false; if (r > low) low = r; }
        else { if (r < low) return false; if (r < high) high = r; }
    }
    x1 = x0 + high * dx; y1 = y0 + high * dy;
    x0 += low * dx; y0 += low * dy;
    return true;
}
}
