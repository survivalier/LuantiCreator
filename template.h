#pragma once
// Moteur de modèles (templates) pour les fonctionnalités des plugins. C++ standard uniquement.
//
//   {{cle}}                    valeur brute              {{cle|lua}}   chaîne Lua entre guillemets
//   {{cle|ident}}              a-z 0-9 _                 {{cle|num}}   nombre (0 si invalide)
//   {{cle|bool}}               true / false              {{cle|ref}}   mod:nom (ajoute le mod si absent)
//   {{cle|upper}} {{cle|lower}} {{cle|trim}} {{cle|raw}}  (les filtres se chaînent : {{cle|ref|lua}})
//   {{#if cle}} … {{#else}} … {{/if}}      {{#unless cle}} … {{/unless}}      {{#if !cle}} … {{/if}}
//   {{#each cle}} {{this}} {{@index}} {{/each}}   (une itération par ligne d'un champ multiligne)
//
// Une balise de bloc seule sur sa ligne ({{#if}}, {{#else}}, {{/if}}, {{#each}}, {{/each}}) ne laisse pas de ligne vide.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace lctpl {

inline std::string fmtNum(double v)
{
    if (!std::isfinite(v)) return "0";
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.6f", v);
    std::string s = buf;
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    if (s == "-0" || s.empty()) s = "0";
    return s;
}

struct Value {
    enum Kind { Str, Bool, Num, List };
    Kind kind = Str;
    std::string s;
    bool b = false;
    double n = 0;
    std::vector<std::string> list;

    static Value str(const std::string &v)
    {
        Value x;
        x.kind = Str;
        x.s = v;
        return x;
    }
    static Value boolean(bool v)
    {
        Value x;
        x.kind = Bool;
        x.b = v;
        return x;
    }
    static Value number(double v)
    {
        Value x;
        x.kind = Num;
        x.n = v;
        x.s = fmtNum(v);
        return x;
    }
    static Value lines(const std::vector<std::string> &v)
    {
        Value x;
        x.kind = List;
        x.list = v;
        return x;
    }
};

using Vars = std::map<std::string, Value>;

inline std::string trim(const std::string &s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
    return s.substr(a, b - a);
}

inline bool truthyStr(const std::string &s)
{
    const std::string t = trim(s);
    return !(t.empty() || t == "0" || t == "false");
}

inline bool truthy(const Value &v)
{
    switch (v.kind) {
    case Value::Str: return truthyStr(v.s);
    case Value::Bool: return v.b;
    case Value::Num: return v.n != 0;
    case Value::List: return !v.list.empty();
    }
    return false;
}

inline std::string asString(const Value &v)
{
    switch (v.kind) {
    case Value::Str: return v.s;
    case Value::Bool: return v.b ? "true" : "false";
    case Value::Num: return v.s;
    case Value::List: {
        std::string r;
        for (size_t i = 0; i < v.list.size(); ++i) {
            if (i) r += "\n";
            r += v.list[i];
        }
        return r;
    }
    }
    return std::string();
}

inline std::string luaQuote(const std::string &s)
{
    std::string r = "\"";
    for (char c : s) {
        switch (c) {
        case '\\': r += "\\\\"; break;
        case '"': r += "\\\""; break;
        case '\n': r += "\\n"; break;
        case '\r': break;
        case '\t': r += "\\t"; break;
        default: r += c;
        }
    }
    return r + "\"";
}

inline std::string identOf(const std::string &s)
{
    std::string r;
    for (char c : trim(s)) {
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
        r += ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') ? c : '_';
    }
    return r;
}

inline bool applyFilters(const Value &v, const std::vector<std::string> &filters, const Vars &vars,
                         std::string &out, std::string &err)
{
    std::string cur = asString(v);
    bool first = true;
    for (const std::string &f : filters) {
        if (f == "raw") {
        } else if (f == "lua") {
            cur = luaQuote(cur);
        } else if (f == "ident") {
            cur = identOf(cur);
        } else if (f == "num") {
            char *end = nullptr;
            const double d = std::strtod(cur.c_str(), &end);
            cur = (end == cur.c_str()) ? std::string("0") : fmtNum(d);
        } else if (f == "bool") {
            cur = (first ? truthy(v) : truthyStr(cur)) ? "true" : "false";
        } else if (f == "upper") {
            for (char &c : cur) c = char(std::toupper(static_cast<unsigned char>(c)));
        } else if (f == "lower") {
            for (char &c : cur) c = char(std::tolower(static_cast<unsigned char>(c)));
        } else if (f == "trim") {
            cur = trim(cur);
        } else if (f == "ref") {
            cur = trim(cur);
            if (!cur.empty() && cur.find(':') == std::string::npos) {
                auto it = vars.find("modname");
                cur = (it != vars.end() ? asString(it->second) : std::string("mod")) + ":" + identOf(cur);
            }
        } else {
            err = "filtre inconnu : " + f;
            return false;
        }
        first = false;
    }
    out = cur;
    return true;
}

// ------------------------------------------------------------------ analyse

struct Tok {
    bool tag = false;
    std::string text;
};

inline bool tokenize(const std::string &t, std::vector<Tok> &out, std::string &err)
{
    size_t i = 0;
    std::string text;
    auto flush = [&] {
        if (!text.empty()) {
            Tok k;
            k.text = text;
            out.push_back(k);
            text.clear();
        }
    };
    while (i < t.size()) {
        const size_t open = t.find("{{", i);
        if (open == std::string::npos) {
            text += t.substr(i);
            break;
        }
        const size_t close = t.find("}}", open + 2);
        if (close == std::string::npos) {
            err = "« {{ » non refermé";
            return false;
        }
        text += t.substr(i, open - i);
        const std::string inner = trim(t.substr(open + 2, close - open - 2));
        size_t after = close + 2;
        const bool block = !inner.empty() && (inner[0] == '#' || inner[0] == '/');
        if (block) {
            size_t j = open;
            while (j > 0 && (t[j - 1] == ' ' || t[j - 1] == '\t')) --j;
            const bool left = (j == 0 || t[j - 1] == '\n');
            size_t k = after;
            while (k < t.size() && (t[k] == ' ' || t[k] == '\t')) ++k;
            const bool right = (k >= t.size() || t[k] == '\n' || (t[k] == '\r' && k + 1 < t.size() && t[k + 1] == '\n'));
            if (left && right) { // balise seule sur sa ligne : on supprime la ligne
                text.resize(text.size() - (open - j));
                if (k < t.size() && t[k] == '\r') ++k;
                after = (k < t.size() && t[k] == '\n') ? k + 1 : k;
            }
        }
        flush();
        Tok k;
        k.tag = true;
        k.text = inner;
        out.push_back(k);
        i = after;
    }
    flush();
    return true;
}

struct Node {
    enum Kind { Text, Var, If, Each };
    Kind kind = Text;
    std::string text;
    bool neg = false;
    std::vector<Node> yes, no;
};

inline bool startsWith(const std::string &s, const char *p)
{
    return s.rfind(p, 0) == 0;
}

inline bool parseNodes(const std::vector<Tok> &toks, size_t &i, std::vector<Node> &out, std::string &err,
                       const std::string &closer, bool *elseFlag)
{
    while (i < toks.size()) {
        const Tok &t = toks[i];
        if (!t.tag) {
            Node n;
            n.kind = Node::Text;
            n.text = t.text;
            out.push_back(n);
            ++i;
            continue;
        }
        const std::string &c = t.text;
        if (c.empty()) {
            err = "balise vide : {{}}";
            return false;
        }
        if (c == "#else") {
            if (closer != "if" || !elseFlag) {
                err = "{{#else}} en dehors d'un {{#if}}";
                return false;
            }
            *elseFlag = true;
            ++i;
            return true;
        }
        if (c == "/if" || c == "/unless") {
            if (closer != "if") {
                err = "{{" + c + "}} sans bloc ouvert";
                return false;
            }
            ++i;
            return true;
        }
        if (c == "/each") {
            if (closer != "each") {
                err = "{{/each}} sans bloc ouvert";
                return false;
            }
            ++i;
            return true;
        }
        if (startsWith(c, "#if ") || startsWith(c, "#unless ")) {
            Node n;
            n.kind = Node::If;
            const bool unless = c[1] == 'u';
            std::string cond = trim(c.substr(unless ? 8 : 4));
            if (!cond.empty() && cond[0] == '!') {
                n.neg = true;
                cond = trim(cond.substr(1));
            }
            if (unless) n.neg = !n.neg;
            if (cond.empty()) {
                err = "condition manquante dans {{" + c + "}}";
                return false;
            }
            n.text = cond;
            ++i;
            bool ef = false;
            if (!parseNodes(toks, i, n.yes, err, "if", &ef)) return false;
            if (ef) {
                bool ef2 = false;
                if (!parseNodes(toks, i, n.no, err, "if", &ef2)) return false;
                if (ef2) {
                    err = "deux {{#else}} dans le même {{#if}}";
                    return false;
                }
            }
            out.push_back(std::move(n));
            continue;
        }
        if (startsWith(c, "#each ")) {
            Node n;
            n.kind = Node::Each;
            n.text = trim(c.substr(6));
            if (n.text.empty()) {
                err = "champ manquant dans {{#each}}";
                return false;
            }
            ++i;
            if (!parseNodes(toks, i, n.yes, err, "each", nullptr)) return false;
            out.push_back(std::move(n));
            continue;
        }
        if (c[0] == '#' || c[0] == '/') {
            err = "balise inconnue : {{" + c + "}}";
            return false;
        }
        Node n;
        n.kind = Node::Var;
        n.text = c;
        out.push_back(n);
        ++i;
    }
    if (!closer.empty()) {
        err = "bloc {{#" + closer + "}} non refermé";
        return false;
    }
    return true;
}

// ------------------------------------------------------------------ rendu

struct Ctx {
    const Vars *vars = nullptr;
    std::vector<std::pair<Value, int>> frames; // pile des {{#each}} : (this, index)
    std::vector<std::string> *warns = nullptr;
};

inline void warnOnce(Ctx &c, const std::string &w)
{
    if (c.warns && std::find(c.warns->begin(), c.warns->end(), w) == c.warns->end()) c.warns->push_back(w);
}

inline bool lookup(const Ctx &c, const std::string &key, Value &out)
{
    if (key == "this") {
        if (c.frames.empty()) return false;
        out = c.frames.back().first;
        return true;
    }
    if (key == "@index") {
        if (c.frames.empty()) return false;
        out = Value::number(c.frames.back().second);
        return true;
    }
    auto it = c.vars->find(key);
    if (it == c.vars->end()) return false;
    out = it->second;
    return true;
}

inline std::vector<std::string> splitLines(const std::string &s)
{
    std::vector<std::string> r;
    size_t a = 0;
    while (a <= s.size()) {
        size_t b = s.find('\n', a);
        if (b == std::string::npos) b = s.size();
        const std::string l = trim(s.substr(a, b - a));
        if (!l.empty()) r.push_back(l);
        a = b + 1;
    }
    return r;
}

inline bool renderNodes(const std::vector<Node> &nodes, Ctx &c, std::string &out, std::string &err)
{
    for (const Node &n : nodes) {
        switch (n.kind) {
        case Node::Text: out += n.text; break;
        case Node::Var: {
            std::vector<std::string> parts;
            size_t a = 0;
            while (a <= n.text.size()) {
                size_t b = n.text.find('|', a);
                if (b == std::string::npos) b = n.text.size();
                parts.push_back(trim(n.text.substr(a, b - a)));
                a = b + 1;
            }
            Value v;
            if (!lookup(c, parts[0], v)) {
                warnOnce(c, "variable inconnue : " + parts[0]);
                v = Value::str("");
            }
            const std::vector<std::string> filters(parts.begin() + 1, parts.end());
            std::string r;
            if (!applyFilters(v, filters, *c.vars, r, err)) return false;
            out += r;
            break;
        }
        case Node::If: {
            Value v;
            const bool known = lookup(c, n.text, v);
            if (!known) warnOnce(c, "variable inconnue : " + n.text);
            bool t = known && truthy(v);
            if (n.neg) t = !t;
            if (!renderNodes(t ? n.yes : n.no, c, out, err)) return false;
            break;
        }
        case Node::Each: {
            Value v;
            if (!lookup(c, n.text, v)) {
                warnOnce(c, "variable inconnue : " + n.text);
                break;
            }
            const std::vector<std::string> items = (v.kind == Value::List) ? v.list : splitLines(asString(v));
            int idx = 1;
            for (const std::string &item : items) {
                c.frames.push_back(std::make_pair(Value::str(item), idx++));
                const bool ok = renderNodes(n.yes, c, out, err);
                c.frames.pop_back();
                if (!ok) return false;
            }
            break;
        }
        }
    }
    return true;
}

inline bool parse(const std::string &tpl, std::vector<Node> &nodes, std::string &err)
{
    std::vector<Tok> toks;
    if (!tokenize(tpl, toks, err)) return false;
    size_t i = 0;
    return parseNodes(toks, i, nodes, err, "", nullptr);
}

inline bool render(const std::string &tpl, const Vars &vars, std::string &out, std::string &err,
                   std::vector<std::string> *warnings = nullptr)
{
    std::vector<Node> nodes;
    if (!parse(tpl, nodes, err)) return false;
    Ctx c;
    c.vars = &vars;
    c.warns = warnings;
    out.clear();
    return renderNodes(nodes, c, out, err);
}

}  // namespace lctpl
