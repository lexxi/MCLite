#pragma once
#include <cmath>
#include <cstdint>

namespace mclite { namespace radar {
constexpr double kPi = 3.14159265358979323846;
inline bool validPosition(double lat, double lon) {
    return std::isfinite(lat) && std::isfinite(lon) &&
           lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180;
}
inline double distanceKm(double a, double b, double c, double d) {
    const double r = kPi / 180;
    const double x = std::sin((c-a)*r/2), y = std::sin((d-b)*r/2);
    double h = x*x + std::cos(a*r)*std::cos(c*r)*y*y;
    h = h < 0 ? 0 : h > 1 ? 1 : h;
    return 6371.0088 * 2 * std::atan2(std::sqrt(h), std::sqrt(1-h));
}
inline double bearingDeg(double a, double b, double c, double d) {
    const double r = kPi / 180;
    const double y = std::sin((d-b)*r)*std::cos(c*r);
    const double x = std::cos(a*r)*std::sin(c*r)-std::sin(a*r)*std::cos(c*r)*std::cos((d-b)*r);
    double v = std::atan2(y,x)/r;
    return v < 0 ? v+360 : v;
}
inline double stableBearing(const uint8_t* key) {
    uint32_t hash = 2166136261u;
    for (int i=0; i<32; ++i) hash = (hash ^ key[i]) * 16777619u;
    return hash % 360;
}
// Ring 1 = direct, ring 2 = one hop, ring 3 = two, ring 4 = 3+.
inline double hopRadius(uint8_t hops) { return (1 + (hops > 3 ? 3 : hops)) / 4.0; }
inline uint8_t ageOpacity(uint32_t now, uint32_t heard) {
    const uint32_t age = now - heard;  // wrap-safe uptime delta
    return age < 2000 ? 255 : age < 600000 ? 220 : age < 3600000 ? 150 : 80;
}
} }
