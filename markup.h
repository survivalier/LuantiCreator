#pragma once
// Lecture / écriture des manifestes de plugin en YAML et en XML (sans dépendance, C++ standard uniquement).
//
//   plugin.yaml  ou  plugin.yml  ou  plugin.xml
//
// Les deux formats sont convertis dans le même arbre de valeurs (lcmk::Value), que plugins.h exploite ensuite.
// YAML : sous-ensemble utile (blocs « clé: valeur », listes « - », [a, b], {a: b}, "texte", 'texte', | et >, # commentaires).
// XML  : éléments / attributs, listes et tables reconnues par leur nom (voir PLUGINS.md).

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace lcmk {

// ====================================================================== arbre de valeurs
struct Value {
    enum Type { Null, Bool, Num, Str, Arr, Obj } t = Null;
    bool b = false;
    double n = 0;
    std::string s;
    bool quoted = false; // chaîne écrite entre guillemets (jamais convertie en nombre / booléen)
    std::vector<Value> a;
    std::vector<std::pair<std::string, Value>> o;

    static Value str(const std::string &v, bool q = false)
    {
        Value r;
        r.t = Str;
        r.s = v;
        r.quoted = q;
        return r;
    }
    static Value boolean(bool v)
    {
        Value r;
        r.t = Bool;
        r.b = v;
        return r;
    }
    static Value num(double v)
    {
        Value r;
        r.t = Num;
        r.n = v;
        return r;
    }
    static Value arr()
    {
        Value r;
        r.t = Arr;
        return r;
    }
    static Value obj()
    {
        Value r;
        r.t = Obj;
        return r;
    }
    const Value *get(const std::string &k) const
    {
        for (const auto &kv : o)
            if (kv.first == k) return &kv.second;
        return nullptr;
    }
    Value *get(const std::string &k)
    {
        for (auto &kv : o)
            if (kv.first == k) return &kv.second;
        return nullptr;
    }
    void set(const std::string &k, const Value &v)
    {
        if (Value *e = get(k)) *e = v;
        else o.emplace_back(k, v);
    }
    bool scalar() const { return t != Arr && t != Obj; }
};

inline bool operator==(const Value &x, const Value &y)
{
    if (x.t != y.t) return false;
    switch (x.t) {
    case Value::Null: return true;
    case Value::Bool: return x.b == y.b;
    case Value::Num: return x.n == y.n;
    case Value::Str: return x.s == y.s;
    case Value::Arr: return x.a == y.a;
    case Value::Obj: return x.o == y.o;
    }
    return false;
}

namespace detail {

inline std::string rtrim(const std::string &s)
{
    size_t b = s.size();
    while (b > 0 && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
    return s.substr(0, b);
}
inline std::string ltrim(const std::string &s)
{
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
    return s.substr(a);
}
inline std::string trim(const std::string &s) { return ltrim(rtrim(s)); }

inline void appendUtf8(std::string &o, unsigned cp)
{
    if (cp < 0x80) o += char(cp);
    else if (cp < 0x800) {
        o += char(0xC0 | (cp >> 6));
        o += char(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        o += char(0xE0 | (cp >> 12));
        o += char(0x80 | ((cp >> 6) & 0x3F));
        o += char(0x80 | (cp & 0x3F));
    } else {
        o += char(0xF0 | (cp >> 18));
        o += char(0x80 | ((cp >> 12) & 0x3F));
        o += char(0x80 | ((cp >> 6) & 0x3F));
        o += char(0x80 | (cp & 0x3F));
    }
}

// vrai si toute la chaîne est un nombre (entier ou décimal, pas de inf / nan / hexadécimal)
inline bool isNumber(const std::string &s, double *out = nullptr)
{
    if (s.empty()) return false;
    size_t i = 0;
    if (s[i] == '-' || s[i] == '+') ++i;
    bool digit = false, dot = false;
    for (; i < s.size(); ++i) {
        if (s[i] >= '0' && s[i] <= '9') digit = true;
        else if (s[i] == '.' && !dot) dot = true;
        else if ((s[i] == 'e' || s[i] == 'E') && digit && i + 1 < s.size()) {
            ++i;
            if (s[i] == '-' || s[i] == '+') ++i;
            if (i >= s.size()) return false;
            for (; i < s.size(); ++i)
                if (s[i] < '0' || s[i] > '9') return false;
            break;
        } else return false;
    }
    if (!digit) return false;
    if (out) *out = std::strtod(s.c_str(), nullptr);
    return true;
}

// ====================================================================== YAML : lecture
struct YamlParser {
    struct Line {
        int no = 0;          // numéro de ligne (à partir de 1)
        int indent = 0;
        bool blank = false;  // vide ou commentaire seul
        bool tabbed = false; // tabulation dans l'indentation
        std::string text;    // sans indentation ni commentaire
        std::string raw;     // ligne d'origine
    };
    std::vector<Line> L;
    size_t p = 0;
    std::string err;
    int errLine = 0;

    bool fail(int line, const std::string &m)
    {
        if (err.empty()) {
            err = m;
            errLine = line;
        }
        return false;
    }

    static std::string stripComment(const std::string &s)
    {
        char q = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            const char c = s[i];
            if (q) {
                if (q == '"' && c == '\\') ++i;
                else if (c == q) {
                    if (q == '\'' && i + 1 < s.size() && s[i + 1] == '\'') ++i;
                    else q = 0;
                }
                continue;
            }
            const char prev = i ? s[i - 1] : ' ';
            if ((c == '"' || c == '\'') && (prev == ' ' || prev == '[' || prev == '{' || prev == ',' || prev == '\t')) q = c;
            else if (c == '#' && (i == 0 || prev == ' ' || prev == '\t')) return s.substr(0, i);
        }
        return s;
    }

    void split(const std::string &src)
    {
        size_t pos = 0;
        int no = 0;
        while (pos <= src.size()) {
            size_t e = src.find('\n', pos);
            if (e == std::string::npos) e = src.size();
            std::string raw = src.substr(pos, e - pos);
            if (!raw.empty() && raw.back() == '\r') raw.pop_back();
            ++no;
            Line ln;
            ln.no = no;
            ln.raw = raw;
            size_t i = 0;
            while (i < raw.size() && raw[i] == ' ') ++i;
            ln.indent = int(i);
            ln.tabbed = i < raw.size() && raw[i] == '\t';
            std::string rest = ln.tabbed ? ltrim(raw) : raw.substr(i);
            if (no == 1 && rest.compare(0, 3, "\xEF\xBB\xBF") == 0) rest = rest.substr(3); // BOM
            ln.text = rtrim(stripComment(rest));
            ln.blank = ln.text.empty();
            if (ln.text == "---" || ln.text == "...") ln.blank = true;
            L.push_back(ln);
            pos = e + 1;
            if (e == src.size()) break;
        }
    }

    void skipBlank()
    {
        while (p < L.size() && L[p].blank) ++p;
    }

    static bool isSeqItem(const std::string &t) { return !t.empty() && t[0] == '-' && (t.size() == 1 || t[1] == ' '); }

    // position du « : » qui termine une clé, ou npos
    static size_t keyColon(const std::string &t)
    {
        if (t.empty() || t[0] == '[' || t[0] == '{' || t[0] == '|' || t[0] == '>') return std::string::npos;
        size_t i = 0;
        if (t[0] == '"' || t[0] == '\'') {
            const char q = t[0];
            for (i = 1; i < t.size(); ++i) {
                if (q == '"' && t[i] == '\\') ++i;
                else if (t[i] == q) {
                    if (q == '\'' && i + 1 < t.size() && t[i + 1] == '\'') ++i;
                    else break;
                }
            }
            if (i >= t.size()) return std::string::npos;
            ++i;
            while (i < t.size() && t[i] == ' ') ++i;
            return (i < t.size() && t[i] == ':' && (i + 1 == t.size() || t[i + 1] == ' ')) ? i : std::string::npos;
        }
        for (i = 0; i < t.size(); ++i)
            if (t[i] == ':' && (i + 1 == t.size() || t[i + 1] == ' ')) return i;
        return std::string::npos;
    }

    bool unquote(const std::string &s, size_t &i, std::string &out, int line)
    {
        const char q = s[i++];
        out.clear();
        while (i < s.size()) {
            const char c = s[i];
            if (q == '\'') {
                if (c == '\'') {
                    if (i + 1 < s.size() && s[i + 1] == '\'') {
                        out += '\'';
                        i += 2;
                        continue;
                    }
                    ++i;
                    return true;
                }
                out += c;
                ++i;
            } else {
                if (c == '"') {
                    ++i;
                    return true;
                }
                if (c == '\\') {
                    if (i + 1 >= s.size()) break;
                    const char e = s[i + 1];
                    i += 2;
                    switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case '0': out += '\0'; break;
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'u':
                    case 'x': {
                        const size_t nd = e == 'u' ? 4 : 2;
                        if (i + nd > s.size()) return fail(line, "séquence d'échappement \\u incomplète");
                        unsigned cp = 0;
                        for (size_t k = 0; k < nd; ++k) {
                            const char h = s[i + k];
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp |= unsigned(h - '0');
                            else if (h >= 'a' && h <= 'f') cp |= unsigned(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') cp |= unsigned(h - 'A' + 10);
                            else return fail(line, "séquence d'échappement invalide");
                        }
                        i += nd;
                        appendUtf8(out, cp);
                        break;
                    }
                    default: return fail(line, std::string("échappement inconnu « \\") + e + " »");
                    }
                } else {
                    out += c;
                    ++i;
                }
            }
        }
        return fail(line, "guillemet non fermé");
    }

    // ----- valeurs en ligne : scalaire, [flux], {flux}
    bool flowValue(const std::string &s, size_t &i, Value &out, int line, int depth)
    {
        if (depth > 40) return fail(line, "imbrication trop profonde");
        while (i < s.size() && s[i] == ' ') ++i;
        if (i >= s.size()) return fail(line, "valeur attendue");
        if (s[i] == '[') {
            ++i;
            out = Value::arr();
            for (;;) {
                while (i < s.size() && s[i] == ' ') ++i;
                if (i >= s.size()) return fail(line, "« ] » attendu");
                if (s[i] == ']') {
                    ++i;
                    return true;
                }
                Value v;
                if (!flowValue(s, i, v, line, depth + 1)) return false;
                out.a.push_back(v);
                while (i < s.size() && s[i] == ' ') ++i;
                if (i < s.size() && s[i] == ',') ++i;
                else if (i < s.size() && s[i] == ']') continue;
                else return fail(line, "« , » ou « ] » attendu");
            }
        }
        if (s[i] == '{') {
            ++i;
            out = Value::obj();
            for (;;) {
                while (i < s.size() && s[i] == ' ') ++i;
                if (i >= s.size()) return fail(line, "« } » attendu");
                if (s[i] == '}') {
                    ++i;
                    return true;
                }
                std::string key;
                if (s[i] == '"' || s[i] == '\'') {
                    if (!unquote(s, i, key, line)) return false;
                } else {
                    size_t e = i;
                    while (e < s.size() && s[e] != ':' && s[e] != ',' && s[e] != '}') ++e;
                    key = trim(s.substr(i, e - i));
                    i = e;
                }
                while (i < s.size() && s[i] == ' ') ++i;
                if (i >= s.size() || s[i] != ':') return fail(line, "« : » attendu après la clé « " + key + " »");
                ++i;
                Value v;
                if (!flowValue(s, i, v, line, depth + 1)) return false;
                if (out.get(key)) return fail(line, "clé en double : « " + key + " »");
                out.o.emplace_back(key, v);
                while (i < s.size() && s[i] == ' ') ++i;
                if (i < s.size() && s[i] == ',') ++i;
                else if (i < s.size() && s[i] == '}') continue;
                else return fail(line, "« , » ou « } » attendu");
            }
        }
        if (s[i] == '"' || s[i] == '\'') {
            std::string v;
            if (!unquote(s, i, v, line)) return false;
            out = Value::str(v, true);
            return true;
        }
        size_t e = i;
        while (e < s.size() && s[e] != ',' && s[e] != ']' && s[e] != '}') ++e;
        const std::string v = trim(s.substr(i, e - i));
        i = e;
        out = (v == "~" || v == "null") ? Value() : Value::str(v);
        return true;
    }

    bool inlineValue(const std::string &s, int line, Value &out)
    {
        const std::string t = trim(s);
        if (t.empty()) {
            out = Value();
            return true;
        }
        if (t[0] == '[' || t[0] == '{') {
            size_t i = 0;
            if (!flowValue(t, i, out, line, 0)) return false;
            while (i < t.size() && t[i] == ' ') ++i;
            if (i != t.size()) return fail(line, "texte inattendu après la valeur");
            return true;
        }
        if (t[0] == '"' || t[0] == '\'') {
            size_t i = 0;
            std::string v;
            if (!unquote(t, i, v, line)) return false;
            while (i < t.size() && t[i] == ' ') ++i;
            if (i != t.size()) return fail(line, "texte inattendu après la chaîne entre guillemets");
            out = Value::str(v, true);
            return true;
        }
        if (t[0] == '&' || t[0] == '*' || t[0] == '!' || t[0] == '@' || t[0] == '`' || t[0] == '%')
            return fail(line, std::string("« ") + t[0] + " » non pris en charge (ancres et étiquettes YAML ignorées) : mettez la valeur entre guillemets");
        out = (t == "~" || t == "null") ? Value() : Value::str(t);
        return true;
    }

    // ----- scalaire en bloc | ou >  (la ligne p est celle de l'indicateur ; parentIndent = indentation de la clé)
    bool blockScalar(const std::string &indicator, int parentIndent, Value &out)
    {
        const int headLine = L[p].no;
        const bool fold = indicator[0] == '>';
        char chomp = 'c';
        for (size_t i = 1; i < indicator.size(); ++i) {
            if (indicator[i] == '-') chomp = '-';
            else if (indicator[i] == '+') chomp = '+';
            else return fail(headLine, "indicateur de bloc non pris en charge : « " + indicator + " »");
        }
        ++p;
        std::vector<std::string> lines;
        int bi = -1;
        while (p < L.size()) {
            const Line &ln = L[p];
            const bool empty = trim(ln.raw).empty();
            if (!empty && ln.indent <= parentIndent) break;
            if (!empty && bi < 0) bi = ln.indent;
            lines.push_back(empty ? std::string() : ln.raw.substr(size_t(std::min<int>(bi, int(ln.raw.size())))));
            ++p;
        }
        while (!lines.empty() && lines.back().empty()) lines.pop_back(); // le « keep » ne garde qu'une fin de ligne
        std::string r;
        if (!fold) {
            for (size_t i = 0; i < lines.size(); ++i) {
                if (i) r += '\n';
                r += lines[i];
            }
        } else {
            for (size_t i = 0; i < lines.size(); ++i) {
                if (i) {
                    if (lines[i].empty()) r += '\n';
                    else if (!lines[i - 1].empty()) r += ' ';
                }
                r += lines[i];
            }
        }
        if (chomp != '-' && !lines.empty()) r += '\n';
        out = Value::str(r, true);
        return true;
    }

    bool isBlockIndicator(const std::string &t) const
    {
        if (t.empty() || (t[0] != '|' && t[0] != '>')) return false;
        for (size_t i = 1; i < t.size(); ++i)
            if (t[i] != '-' && t[i] != '+') return false;
        return true;
    }

    // ----- structures
    bool node(int minIndent, Value &out, int depth)
    {
        if (depth > 60) return fail(L[p].no, "imbrication trop profonde");
        skipBlank();
        if (p >= L.size() || L[p].indent < minIndent) {
            out = Value();
            return true;
        }
        const Line &ln = L[p];
        if (ln.tabbed) return fail(ln.no, "tabulation dans l'indentation (utilisez des espaces)");
        if (isSeqItem(ln.text)) return seq(ln.indent, out, depth);
        if (keyColon(ln.text) != std::string::npos) return map(ln.indent, out, depth);
        const int no = ln.no;
        const std::string t = ln.text;
        ++p;
        return inlineValue(t, no, out);
    }

    bool map(int indent, Value &out, int depth)
    {
        out = Value::obj();
        for (;;) {
            skipBlank();
            if (p >= L.size()) return true;
            Line &ln = L[p];
            if (ln.indent < indent) return true;
            if (ln.tabbed) return fail(ln.no, "tabulation dans l'indentation (utilisez des espaces)");
            if (ln.indent > indent) return fail(ln.no, "indentation inattendue");
            if (isSeqItem(ln.text)) return true;
            const size_t c = keyColon(ln.text);
            if (c == std::string::npos) return fail(ln.no, "« clé: valeur » attendu");
            std::string key;
            {
                size_t i = 0;
                if (ln.text[0] == '"' || ln.text[0] == '\'') {
                    if (!unquote(ln.text, i, key, ln.no)) return false;
                } else key = trim(ln.text.substr(0, c));
            }
            if (key.empty()) return fail(ln.no, "clé vide");
            if (out.get(key)) return fail(ln.no, "clé en double : « " + key + " »");
            const std::string rest = trim(ln.text.substr(c + 1));
            const int no = ln.no;
            Value v;
            if (rest.empty()) {
                ++p;
                skipBlank();
                if (p < L.size() && L[p].indent > indent) {
                    if (!node(indent + 1, v, depth + 1)) return false;
                } else if (p < L.size() && L[p].indent == indent && isSeqItem(L[p].text)) {
                    if (!seq(indent, v, depth + 1)) return false;
                }
            } else if (isBlockIndicator(rest)) {
                if (!blockScalar(rest, indent, v)) return false;
            } else {
                ++p;
                if (!inlineValue(rest, no, v)) return false;
            }
            out.o.emplace_back(key, v);
        }
    }

    bool seq(int indent, Value &out, int depth)
    {
        out = Value::arr();
        for (;;) {
            skipBlank();
            if (p >= L.size()) return true;
            Line &ln = L[p];
            if (ln.indent < indent) return true;
            if (ln.tabbed) return fail(ln.no, "tabulation dans l'indentation (utilisez des espaces)");
            if (ln.indent > indent) return fail(ln.no, "indentation inattendue dans la liste");
            if (!isSeqItem(ln.text)) return true;
            size_t k = 1;
            while (k < ln.text.size() && ln.text[k] == ' ') ++k;
            const std::string rest = ln.text.substr(k);
            const int col = indent + int(k);
            Value v;
            if (rest.empty()) {
                ++p;
                skipBlank();
                if (p < L.size() && L[p].indent > indent) {
                    if (!node(indent + 1, v, depth + 1)) return false;
                }
            } else if (isBlockIndicator(rest)) {
                if (!blockScalar(rest, indent, v)) return false;
            } else if (isSeqItem(rest) || keyColon(rest) != std::string::npos) {
                // « - clé: valeur » ou « - - x » : la ligne devient une ligne d'indentation `col`
                ln.indent = col;
                ln.text = rest;
                if (!node(col, v, depth + 1)) return false;
            } else {
                const int no = ln.no;
                ++p;
                if (!inlineValue(rest, no, v)) return false;
            }
            out.a.push_back(v);
        }
    }
};

// ====================================================================== YAML : écriture
inline bool needsQuote(const std::string &s)
{
    if (s.empty()) return true;
    if (s.front() == ' ' || s.back() == ' ' || s.back() == ':') return true;
    static const char *special = "-?:,[]{}#&*!|>'\"%@`";
    if (std::strchr(special, s[0])) return true;
    if (s.find(": ") != std::string::npos || s.find(" #") != std::string::npos) return true;
    for (unsigned char c : s)
        if (c < 0x20 || c == 0x7F) return true;
    std::string low;
    for (char c : s) low += char(std::tolower((unsigned char)c));
    for (const char *w : {"true", "false", "null", "yes", "no", "on", "off", "~", "y", "n"})
        if (low == w) return true;
    return isNumber(s);
}

inline std::string dq(const std::string &s)
{
    std::string r = "\"";
    for (unsigned char c : s) {
        switch (c) {
        case '"': r += "\\\""; break;
        case '\\': r += "\\\\"; break;
        case '\n': r += "\\n"; break;
        case '\t': r += "\\t"; break;
        case '\r': r += "\\r"; break;
        default:
            if (c < 0x20 || c == 0x7F) {
                char b[8];
                std::snprintf(b, sizeof b, "\\u%04X", c);
                r += b;
            } else r += char(c);
        }
    }
    return r + "\"";
}

inline std::string numText(double n)
{
    char b[40];
    if (std::fabs(n) < 1e15 && n == std::floor(n)) std::snprintf(b, sizeof b, "%.0f", n);
    else std::snprintf(b, sizeof b, "%.15g", n);
    return b;
}

inline std::string yamlScalar(const Value &v)
{
    switch (v.t) {
    case Value::Null: return "null";
    case Value::Bool: return v.b ? "true" : "false";
    case Value::Num: return numText(v.n);
    case Value::Str: return needsQuote(v.s) ? dq(v.s) : v.s;
    case Value::Arr: return "[]";
    case Value::Obj: return "{}";
    }
    return "null";
}

inline std::string yamlKey(const std::string &k) { return needsQuote(k) ? dq(k) : k; }

inline void yamlEmit(const Value &v, int indent, std::string &out);

inline void yamlEmitSeq(const Value &v, int indent, std::string &out)
{
    const std::string pad(size_t(indent), ' ');
    for (const Value &it : v.a) {
        if (it.t == Value::Obj && !it.o.empty()) {
            std::string tmp;
            yamlEmit(it, indent + 2, tmp);
            out += pad + "- " + tmp.substr(size_t(indent) + 2);
        } else if (it.t == Value::Arr && !it.a.empty()) {
            out += pad + "-\n";
            yamlEmitSeq(it, indent + 2, out);
        } else out += pad + "- " + yamlScalar(it) + "\n";
    }
}

inline void yamlEmit(const Value &v, int indent, std::string &out)
{
    const std::string pad(size_t(indent), ' ');
    for (const auto &kv : v.o) {
        const Value &x = kv.second;
        out += pad + yamlKey(kv.first) + ":";
        if (x.t == Value::Obj && !x.o.empty()) {
            out += "\n";
            yamlEmit(x, indent + 2, out);
        } else if (x.t == Value::Arr && !x.a.empty()) {
            out += "\n";
            yamlEmitSeq(x, indent + 2, out);
        } else out += " " + yamlScalar(x) + "\n";
    }
}

// ====================================================================== XML : lecture
struct XNode {
    std::string name, text;
    std::vector<std::pair<std::string, std::string>> attrs;
    std::vector<XNode> kids;
    int line = 1;
};

struct XmlParser {
    const std::string &s;
    size_t i = 0;
    std::string err;
    int errLine = 0;
    explicit XmlParser(const std::string &src) : s(src) {}

    int lineAt(size_t pos) const
    {
        int n = 1;
        for (size_t k = 0; k < pos && k < s.size(); ++k)
            if (s[k] == '\n') ++n;
        return n;
    }
    bool fail(const std::string &m)
    {
        if (err.empty()) {
            err = m;
            errLine = lineAt(i);
        }
        return false;
    }
    bool starts(const char *lit) const { return s.compare(i, std::strlen(lit), lit) == 0; }
    void ws()
    {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i;
    }
    static bool nameStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
    static bool nameChar(char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.' || c == ':'; }

    bool skipMisc()
    {
        for (;;) {
            ws();
            if (starts("<?")) {
                const size_t e = s.find("?>", i);
                if (e == std::string::npos) return fail("« ?> » attendu");
                i = e + 2;
            } else if (starts("<!--")) {
                const size_t e = s.find("-->", i);
                if (e == std::string::npos) return fail("commentaire non fermé");
                i = e + 3;
            } else if (starts("<!DOCTYPE")) {
                const size_t e = s.find('>', i);
                if (e == std::string::npos) return fail("DOCTYPE non fermé");
                i = e + 1;
            } else return true;
        }
    }

    bool decode(const std::string &in, std::string &out)
    {
        out.clear();
        for (size_t k = 0; k < in.size(); ++k) {
            if (in[k] != '&') {
                out += in[k];
                continue;
            }
            const size_t e = in.find(';', k);
            if (e == std::string::npos) return fail("« & » sans « ; » (écrivez &amp;)");
            const std::string ent = in.substr(k + 1, e - k - 1);
            if (ent == "lt") out += '<';
            else if (ent == "gt") out += '>';
            else if (ent == "amp") out += '&';
            else if (ent == "quot") out += '"';
            else if (ent == "apos") out += '\'';
            else if (!ent.empty() && ent[0] == '#') {
                const bool hex = ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X');
                const std::string d = ent.substr(hex ? 2 : 1);
                if (d.empty()) return fail("référence numérique vide");
                char *end = nullptr;
                const unsigned long cp = std::strtoul(d.c_str(), &end, hex ? 16 : 10);
                if (*end || cp == 0 || cp > 0x10FFFF) return fail("référence numérique invalide &" + ent + ";");
                appendUtf8(out, unsigned(cp));
            } else return fail("entité inconnue &" + ent + ";");
            k = e;
        }
        return true;
    }

    bool element(XNode &n, int depth)
    {
        if (depth > 60) return fail("imbrication trop profonde");
        if (i >= s.size() || s[i] != '<') return fail("« < » attendu");
        n.line = lineAt(i);
        ++i;
        size_t b = i;
        if (i >= s.size() || !nameStart(s[i])) return fail("nom d'élément attendu");
        while (i < s.size() && nameChar(s[i])) ++i;
        n.name = s.substr(b, i - b);
        for (;;) {
            ws();
            if (i >= s.size()) return fail("balise non fermée");
            if (s[i] == '/' || s[i] == '>') break;
            b = i;
            if (!nameStart(s[i])) return fail("attribut attendu");
            while (i < s.size() && nameChar(s[i])) ++i;
            const std::string an = s.substr(b, i - b);
            ws();
            if (i >= s.size() || s[i] != '=') return fail("« = » attendu après l'attribut « " + an + " »");
            ++i;
            ws();
            if (i >= s.size() || (s[i] != '"' && s[i] != '\'')) return fail("valeur d'attribut entre guillemets attendue");
            const char q = s[i++];
            const size_t e = s.find(q, i);
            if (e == std::string::npos) return fail("guillemet non fermé");
            std::string val;
            const std::string rawv = s.substr(i, e - i);
            i = e + 1;
            if (!decode(rawv, val)) return false;
            for (const auto &a : n.attrs)
                if (a.first == an) return fail("attribut « " + an + " » en double");
            n.attrs.emplace_back(an, val);
        }
        if (s[i] == '/') {
            ++i;
            if (i >= s.size() || s[i] != '>') return fail("« > » attendu");
            ++i;
            return true;
        }
        ++i; // '>'
        std::string text;
        for (;;) {
            if (i >= s.size()) return fail("balise <" + n.name + "> non fermée");
            if (s[i] != '<') {
                const size_t e = s.find('<', i);
                const std::string raw = s.substr(i, e == std::string::npos ? std::string::npos : e - i);
                std::string dec;
                if (!decode(raw, dec)) return false;
                text += dec;
                i = e == std::string::npos ? s.size() : e;
                continue;
            }
            if (starts("<!--")) {
                const size_t e = s.find("-->", i);
                if (e == std::string::npos) return fail("commentaire non fermé");
                i = e + 3;
            } else if (starts("<![CDATA[")) {
                const size_t e = s.find("]]>", i);
                if (e == std::string::npos) return fail("CDATA non fermé");
                text += s.substr(i + 9, e - i - 9);
                i = e + 3;
            } else if (starts("</")) {
                i += 2;
                const size_t b2 = i;
                while (i < s.size() && nameChar(s[i])) ++i;
                const std::string cn = s.substr(b2, i - b2);
                ws();
                if (i >= s.size() || s[i] != '>') return fail("« > » attendu");
                if (cn != n.name) return fail("« </" + cn + "> » ne ferme pas <" + n.name + ">");
                ++i;
                break;
            } else if (starts("<?")) {
                const size_t e = s.find("?>", i);
                if (e == std::string::npos) return fail("« ?> » attendu");
                i = e + 2;
            } else {
                XNode k;
                if (!element(k, depth + 1)) return false;
                n.kids.push_back(std::move(k));
            }
        }
        n.text = text;
        return true;
    }

    bool parse(XNode &root)
    {
        if (!skipMisc()) return false;
        if (!element(root, 0)) return false;
        if (!skipMisc()) return false;
        if (i < s.size()) return fail("contenu après l'élément racine");
        return true;
    }
};

// ====================================================================== schéma du manifeste
// Listes : <blocks><block/>…</blocks> ; tables : <icons><icon key="…" file="…"/></icons>.
inline const char *listItemTag(const std::string &k)
{
    static const std::pair<const char *, const char *> t[] = {
        {"blocks", "block"}, {"blocksets", "blockset"}, {"presets", "preset"}, {"snippets", "snippet"},
        {"features", "feature"}, {"scripts", "script"}, {"fields", "field"}, {"names", "name"}, {"options", "option"}};
    for (const auto &e : t)
        if (k == e.first) return e.second;
    return nullptr;
}
struct MapSpec {
    const char *container, *item, *keyAttr, *valAttr; // valAttr == nullptr : la valeur est le texte de l'élément
};
inline const MapSpec *mapSpec(const std::string &k)
{
    static const MapSpec t[] = {{"icons", "icon", "key", "file"}, {"textures", "texture", "ref", "file"}, {"values", "value", "key", nullptr}};
    for (const auto &e : t)
        if (k == e.container) return &e;
    return nullptr;
}

inline std::string normKey(std::string k)
{
    for (char &c : k)
        if (c == '-') c = '_';
    return k;
}

inline bool truthy(const std::string &v) { return v == "true"; }

inline Value xmlValue(const XNode &n, const std::string &asKey);

inline Value xmlLeafOrObject(const XNode &n)
{
    if (n.attrs.empty() && n.kids.empty()) return Value::str(trim(n.text));
    Value o = Value::obj();
    for (const auto &a : n.attrs) o.o.emplace_back(normKey(a.first), Value::str(a.second));
    for (const XNode &k : n.kids) {
        const std::string key = normKey(k.name);
        if (o.get(key)) continue; // doublon : le premier gagne (signalé par la validation du schéma)
        o.o.emplace_back(key, xmlValue(k, key));
    }
    return o;
}

inline Value xmlValue(const XNode &n, const std::string &asKey)
{
    if (listItemTag(asKey)) {
        Value a = Value::arr();
        for (const XNode &k : n.kids) a.a.push_back(xmlLeafOrObject(k));
        return a;
    }
    if (const MapSpec *ms = mapSpec(asKey)) {
        Value o = Value::obj();
        for (const XNode &k : n.kids) {
            std::string key, val;
            bool haveVal = false;
            Value extra = Value::obj();
            for (const auto &a : k.attrs) {
                if (a.first == ms->keyAttr) key = a.second;
                else if (ms->valAttr && a.first == ms->valAttr) {
                    val = a.second;
                    haveVal = true;
                } else extra.o.emplace_back(normKey(a.first), Value::str(a.second));
            }
            if (!haveVal) val = trim(k.text);
            if (key.empty() || o.get(key)) continue;
            if (extra.o.empty()) o.o.emplace_back(key, Value::str(val));
            else {
                Value e = Value::obj();
                e.o.emplace_back(ms->valAttr ? ms->valAttr : "value", Value::str(val));
                for (auto &kv : extra.o) e.o.push_back(kv);
                o.o.emplace_back(key, e);
            }
        }
        return o;
    }
    return xmlLeafOrObject(n);
}

// Convertit les chaînes non quotées en booléens / nombres là où le schéma l'attend.
inline void typeify(Value &v, const std::string &key, bool inValues)
{
    if (v.t == Value::Str && !v.quoted) {
        double d;
        if (inValues) {
            if (v.s == "true" || v.s == "false") v = Value::boolean(v.s == "true");
            else if (isNumber(v.s, &d)) v = Value::num(d);
        } else if (key == "icon_only" || key == "template" || key == "default" || key == "core") {
            if (v.s == "true" || v.s == "false") v = Value::boolean(v.s == "true");
        } else if (key == "min" || key == "max" || key == "format") {
            if (isNumber(v.s, &d)) v = Value::num(d);
        }
    } else if (v.t == Value::Obj) {
        for (auto &kv : v.o) typeify(kv.second, kv.first, inValues || kv.first == "values");
    } else if (v.t == Value::Arr) {
        for (auto &x : v.a) typeify(x, key, inValues);
    }
}

// ====================================================================== XML : écriture
inline std::string xesc(const std::string &s, bool attr)
{
    std::string r;
    for (char c : s) {
        switch (c) {
        case '&': r += "&amp;"; break;
        case '<': r += "&lt;"; break;
        case '>': r += "&gt;"; break;
        case '"':
            if (attr) r += "&quot;";
            else r += c;
            break;
        case '\n':
            if (attr) r += "&#10;";
            else r += c;
            break;
        default: r += c;
        }
    }
    return r;
}

inline std::string scalarText(const Value &v)
{
    switch (v.t) {
    case Value::Bool: return v.b ? "true" : "false";
    case Value::Num: return numText(v.n);
    case Value::Str: return v.s;
    default: return std::string();
    }
}

inline bool attrFits(const std::string &key, const Value &v)
{
    if (!v.scalar() || v.t == Value::Null) return false;
    if (key == "description" || key == "text") return false;
    const std::string t = scalarText(v);
    return t.size() <= 80 && t.find('\n') == std::string::npos;
}

inline void xmlEmitElement(const std::string &tag, const Value &v, int indent, std::string &out,
                           const std::vector<std::pair<std::string, std::string>> &lead = {})
{
    const std::string pad(size_t(indent) * 2, ' ');
    std::string attrs;
    for (const auto &a : lead) attrs += " " + a.first + "=\"" + xesc(a.second, true) + "\"";
    if (v.t != Value::Obj) { // scalaire : <tag>texte</tag>
        out += pad + "<" + tag + attrs + ">" + xesc(scalarText(v), false) + "</" + tag + ">\n";
        return;
    }
    std::vector<const std::pair<std::string, Value> *> kids;
    for (const auto &kv : v.o) {
        if (attrFits(kv.first, kv.second)) attrs += " " + kv.first + "=\"" + xesc(scalarText(kv.second), true) + "\"";
        else if (kv.second.t != Value::Null) kids.push_back(&kv);
    }
    if (kids.empty()) {
        out += pad + "<" + tag + attrs + "/>\n";
        return;
    }
    out += pad + "<" + tag + attrs + ">\n";
    const std::string pad2(size_t(indent + 1) * 2, ' ');
    for (const auto *kv : kids) {
        const std::string &k = kv->first;
        const Value &x = kv->second;
        if (const char *it = listItemTag(k); it && x.t == Value::Arr) {
            if (x.a.empty()) {
                out += pad2 + "<" + k + "/>\n";
                continue;
            }
            out += pad2 + "<" + k + ">\n";
            for (const Value &e : x.a) xmlEmitElement(it, e, indent + 2, out);
            out += pad2 + "</" + k + ">\n";
        } else if (const MapSpec *ms = mapSpec(k); ms && x.t == Value::Obj) {
            if (x.o.empty()) {
                out += pad2 + "<" + k + "/>\n";
                continue;
            }
            out += pad2 + "<" + k + ">\n";
            for (const auto &e : x.o) {
                std::vector<std::pair<std::string, std::string>> ld = {{ms->keyAttr, e.first}};
                if (e.second.t == Value::Obj) { // {file, icon_only}
                    Value cp = e.second;
                    std::string main;
                    if (ms->valAttr) {
                        if (const Value *f = cp.get(ms->valAttr)) main = scalarText(*f);
                        ld.emplace_back(ms->valAttr, main);
                        Value rest = Value::obj();
                        for (const auto &kv2 : cp.o)
                            if (kv2.first != ms->valAttr) rest.o.push_back(kv2);
                        xmlEmitElement(ms->item, rest, indent + 2, out, ld);
                    } else xmlEmitElement(ms->item, cp, indent + 2, out, ld);
                } else if (ms->valAttr) {
                    ld.emplace_back(ms->valAttr, scalarText(e.second));
                    out += std::string(size_t(indent + 2) * 2, ' ') + "<" + ms->item;
                    for (const auto &a : ld) out += " " + a.first + "=\"" + xesc(a.second, true) + "\"";
                    out += "/>\n";
                } else {
                    xmlEmitElement(ms->item, e.second, indent + 2, out, ld);
                }
            }
            out += pad2 + "</" + k + ">\n";
        } else if (x.t == Value::Arr) { // liste inconnue : <k><item/>…</k>
            out += pad2 + "<" + k + ">\n";
            for (const Value &e : x.a) xmlEmitElement("item", e, indent + 2, out);
            out += pad2 + "</" + k + ">\n";
        } else {
            xmlEmitElement(k, x, indent + 1, out);
        }
    }
    out += pad + "</" + tag + ">\n";
}

} // namespace detail

// ====================================================================== interface
// ext : "yaml", "yml" ou "xml". En cas d'erreur, `line` vaut la ligne fautive (0 si inconnue).
inline bool parseManifest(const std::string &data, const std::string &ext, Value &out, std::string &err, int *line = nullptr)
{
    if (line) *line = 0;
    if (data.size() > 4 * 1024 * 1024) {
        err = "fichier trop gros";
        return false;
    }
    if (ext == "xml") {
        detail::XmlParser xp(data);
        detail::XNode root;
        if (!xp.parse(root)) {
            err = xp.err;
            if (line) *line = xp.errLine;
            return false;
        }
        if (root.name != "plugin") {
            err = "l'élément racine doit être <plugin>, pas <" + root.name + ">";
            if (line) *line = root.line;
            return false;
        }
        out = detail::xmlLeafOrObject(root);
        if (out.t != Value::Obj) out = Value::obj(); // <plugin/> vide
    } else {
        detail::YamlParser yp;
        yp.split(data);
        Value v;
        if (!yp.node(0, v, 0)) {
            err = yp.err;
            if (line) *line = yp.errLine;
            return false;
        }
        yp.skipBlank();
        if (yp.p < yp.L.size()) {
            err = "contenu inattendu (indentation incorrecte ?)";
            if (line) *line = yp.L[yp.p].no;
            return false;
        }
        if (v.t == Value::Null) v = Value::obj();
        if (v.t != Value::Obj) {
            err = "le document doit être une table « clé: valeur »";
            if (line) *line = 1;
            return false;
        }
        out = v;
    }
    detail::typeify(out, "", false);
    return true;
}

inline std::string writeManifest(const Value &v, const std::string &ext)
{
    std::string out;
    if (ext == "xml") {
        out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        detail::xmlEmitElement("plugin", v, 0, out);
    } else {
        detail::yamlEmit(v, 0, out);
    }
    return out;
}

} // namespace lcmk
