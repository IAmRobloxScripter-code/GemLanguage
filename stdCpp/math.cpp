#include <cmath>
// STD Math Library
extern "C" {
    double gem_abs(double n) {
        return std::abs(n);
    }
    
    double gem_sin(double n) {
        return std::sin(n);
    }
    
    double gem_cos(double n) {
        return std::cos(n);
    }
    
    double gem_tan(double n) {
        return std::tan(n);
    }
    
    double gem_asin(double n) {
        return std::asin(n);
    }

    double gem_acos(double n) {
        return std::acos(n);
    }

    double gem_atan(double n) {
        return std::atan(n);
    }
    
    double gem_simplelog(double n) {
        return std::log(n);
    }

    double gem_complexlog(double n, double base) {
        return std::log(n) / std::log(base);
    }

    double gem_floor(double n) {
        return std::floor(n);
    }

    double gem_round(double n) {
        return std::round(n);
    }

    double gem_ceil(double n) {
        return std::ceil(n);
    }
}
