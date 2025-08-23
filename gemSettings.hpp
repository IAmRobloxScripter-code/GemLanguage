#pragma once
#include <string>

class Settings {
public:
    bool optimize = false;
    bool verbose = false;
    
    static Settings& get() {
        static Settings instance;
        return instance;
    }

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    Settings(Settings&&) = delete;
    Settings& operator=(Settings&&) = delete;

private:
    Settings() = default;
};

inline Settings& settings = Settings::get();