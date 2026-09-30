#pragma once
// ============================================================================
//  Простой INI-парсер конфигов мода (STALKER_MOD/*.cfg).
//  Поддерживает:
//    [section]
//    key = value        ; комментарий
//    ; строковый комментарий
//  Значения: строка, int, float, bool (true/false/1/0).
// ============================================================================
#include <string>
#include <vector>
#include <unordered_map>

namespace stalker {

class Config {
public:
    bool Load(const char* path);

    // Значения с значениями по умолчанию
    std::string GetString(const std::string& section, const std::string& key, const std::string& def = "") const;
    int         GetInt(const std::string& section, const std::string& key, int def = 0) const;
    float       GetFloat(const std::string& section, const std::string& key, float def = 0.f) const;
    bool        GetBool(const std::string& section, const std::string& key, bool def = false) const;

    bool HasSection(const std::string& section) const;
    std::vector<std::string> GetSectionsWithPrefix(const std::string& prefix) const;

    // Путь до загруженного файла
    const std::string& SourcePath() const { return m_path; }

private:
    // section -> (key -> value)
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_data;
    std::string m_path;
};

// Небольшой помощник для чтения float-векторов вида "x, y, z"
struct Vec3 { float x, y, z; };
Vec3 ParseVec3(const std::string& s, Vec3 def = {0, 0, 0});

} // namespace stalker
