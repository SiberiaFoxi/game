#include "Config.h"
#include <cstdlib>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace stalker {

static std::string Trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

static std::string Lower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

static void StripComment(std::string& line) {
    // ';' или '#' — комментарий
    size_t pos = line.find_first_of(";#");
    if (pos != std::string::npos) line = line.substr(0, pos);
}

bool Config::Load(const char* path) {
    m_path = path;
    std::ifstream f(path);
    if (!f.good()) return false;

    std::string line, section;
    while (std::getline(f, line)) {
        StripComment(line);
        line = Trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            section = Lower(Trim(line.substr(1, line.size() - 2)));
            continue;
        }

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key   = Lower(Trim(line.substr(0, eq)));
        std::string value = Trim(line.substr(eq + 1));
        if (key.empty()) continue;
        m_data[section][key] = value;
    }
    return true;
}

std::string Config::GetString(const std::string& section, const std::string& key, const std::string& def) const {
    auto is = m_data.find(Lower(section));
    if (is == m_data.end()) return def;
    auto ik = is->second.find(Lower(key));
    if (ik == is->second.end()) return def;
    return ik->second;
}

int Config::GetInt(const std::string& section, const std::string& key, int def) const {
    std::string s = GetString(section, key);
    if (s.empty()) return def;
    return std::atoi(s.c_str());
}

float Config::GetFloat(const std::string& section, const std::string& key, float def) const {
    std::string s = GetString(section, key);
    if (s.empty()) return def;
    return static_cast<float>(std::atof(s.c_str()));
}

bool Config::GetBool(const std::string& section, const std::string& key, bool def) const {
    std::string s = Lower(GetString(section, key));
    if (s.empty()) return def;
    return s == "true" || s == "1" || s == "yes" || s == "on";
}

bool Config::HasSection(const std::string& section) const {
    return m_data.find(Lower(section)) != m_data.end();
}

std::vector<std::string> Config::GetSectionsWithPrefix(const std::string& prefix) const {
    std::vector<std::string> out;
    std::string p = Lower(prefix);
    for (auto& kv : m_data) {
        if (kv.first.compare(0, p.size(), p) == 0)
            out.push_back(kv.first);
    }
    std::sort(out.begin(), out.end());
    return out;
}

Vec3 ParseVec3(const std::string& s, Vec3 def) {
    std::stringstream ss(s);
    std::string item;
    float v[3] = { def.x, def.y, def.z };
    int i = 0;
    while (std::getline(ss, item, ',') && i < 3) {
        item = Trim(item);
        if (!item.empty()) v[i] = static_cast<float>(std::atof(item.c_str()));
        ++i;
    }
    return { v[0], v[1], v[2] };
}

} // namespace stalker
