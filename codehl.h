#pragma once
// Coloration syntaxique des éditeurs / visualiseurs de code (QPlainTextEdit).
//
// Langages : Lua, Python, YAML, JSON, XML/HTML, fichiers .conf (minetest.conf, mod.conf, settingtypes.txt),
//            formspec Luanti, Markdown, shell.
// Les couleurs suivent le thème sombre / clair (palette de l'application) ; lcApplyTheme() appelle
// LcHighlighter::rehighlightAll() pour que les vues déjà ouvertes changent de couleurs avec le thème.
//
// Utilisation :
//     auto *hl = lcAttachHighlighter(edit, LcLang::Lua);        // langage fixe (aperçu de code généré...)
//     hl->setLang(lcLangForFile("init.lua"), false);            // langage selon le nom du fichier (éditeur de fichiers)
// Pour ajouter un langage : ajouter une valeur à LcLang, la reconnaître dans lcLangForFile() / lcLangByName(),
// écrire une méthode de la classe (voir lua(), yaml()...) et l'appeler dans highlightBlock().

#include <QtWidgets>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

enum class LcLang { None, Lua, Python, Yaml, Json, Xml, Conf, Formspec, Markdown, Shell };

// Langage d'après le nom (ou le chemin) d'un fichier.
inline LcLang lcLangForFile(const QString &path)
{
    const QString name = QFileInfo(path).fileName().toLower();
    const QString ext = QFileInfo(name).suffix();
    if (ext == "lua") return LcLang::Lua;
    if (ext == "py" || ext == "pyw") return LcLang::Python;
    if (ext == "yaml" || ext == "yml") return LcLang::Yaml;
    if (ext == "json" || ext == "lcproj" || ext == "mcmeta") return LcLang::Json;
    if (ext == "xml" || ext == "html" || ext == "htm" || ext == "svg" || ext == "ui" || ext == "qrc") return LcLang::Xml;
    if (ext == "md" || ext == "markdown") return LcLang::Markdown;
    if (ext == "sh" || ext == "bash") return LcLang::Shell;
    if (ext == "conf" || ext == "cfg" || ext == "ini" || name == "settingtypes.txt") return LcLang::Conf;
    return LcLang::None;
}

// Langage d'après un nom comme dans les blocs de code Markdown (```lua).
inline LcLang lcLangByName(const QString &lang)
{
    const QString l = lang.trimmed().toLower();
    if (l == "lua") return LcLang::Lua;
    if (l == "python" || l == "py") return LcLang::Python;
    if (l == "yaml" || l == "yml") return LcLang::Yaml;
    if (l == "json") return LcLang::Json;
    if (l == "xml" || l == "html" || l == "svg") return LcLang::Xml;
    if (l == "conf" || l == "ini") return LcLang::Conf;
    if (l == "formspec") return LcLang::Formspec;
    if (l == "markdown" || l == "md") return LcLang::Markdown;
    if (l == "bash" || l == "sh" || l == "shell") return LcLang::Shell;
    return LcLang::None;
}

class LcHighlighter : public QSyntaxHighlighter
{
public:
    explicit LcHighlighter(QTextDocument *doc, LcLang lang = LcLang::None) : QSyntaxHighlighter(doc), lang_(lang)
    {
        refreshColors();
        registry().append(this);
    }

    LcLang lang() const { return lang_; }
    // Change de langage. `rehighlight` : mettre false juste avant un setPlainText() (le texte sera de toute façon recoloré).
    void setLang(LcLang l, bool rehighlightNow = true)
    {
        if (l == lang_) return;
        lang_ = l;
        if (rehighlightNow) rehighlight();
    }

    // Recalcule les couleurs d'après le thème courant et recolore toutes les vues vivantes.
    static void rehighlightAll()
    {
        QList<QPointer<LcHighlighter>> &r = registry();
        for (int i = r.size() - 1; i >= 0; --i) {
            if (!r[i]) {
                r.removeAt(i);
                continue;
            }
            r[i]->refreshColors();
            r[i]->rehighlight();
        }
    }

protected:
    void highlightBlock(const QString &text) override
    {
        setCurrentBlockState(0);
        switch (lang_) {
        case LcLang::Lua: lua(text); break;
        case LcLang::Python: python(text); break;
        case LcLang::Yaml: yaml(text); break;
        case LcLang::Json: json(text); break;
        case LcLang::Xml: xml(text); break;
        case LcLang::Conf: conf(text); break;
        case LcLang::Formspec: formspec(text); break;
        case LcLang::Markdown: markdown(text); break;
        case LcLang::Shell: shell(text); break;
        case LcLang::None: break;
        }
    }

private:
    enum Tok { Kw, Str, Num, Com, Fn, Bi, Const, Key, Tag, Attr, Punct, Esc, Head, Link, Type, TokCount };

    static QList<QPointer<LcHighlighter>> &registry()
    {
        static QList<QPointer<LcHighlighter>> r;
        return r;
    }

    void refreshColors()
    {
        const bool dark = qApp->palette().color(QPalette::Base).lightness() < 128;
        auto mk = [&](const char *darkCol, const char *lightCol, bool bold = false, bool italic = false) {
            QTextCharFormat f;
            f.setForeground(QColor(dark ? darkCol : lightCol));
            if (bold) f.setFontWeight(QFont::Bold);
            if (italic) f.setFontItalic(true);
            return f;
        };
        fm_[Kw] = mk("#c678dd", "#a626a4");
        fm_[Str] = mk("#98c379", "#3f8f3f");
        fm_[Num] = mk("#d19a66", "#986801");
        fm_[Com] = mk("#7f8794", "#8a8f98", false, true);
        fm_[Fn] = mk("#61afef", "#2f64d8");
        fm_[Bi] = mk("#56b6c2", "#0184bc");
        fm_[Const] = mk("#e5c07b", "#a06a00");
        fm_[Key] = mk("#e06c75", "#d0382c");
        fm_[Tag] = mk("#e06c75", "#d0382c");
        fm_[Attr] = mk("#d19a66", "#986801");
        fm_[Punct] = mk("#9aa1ad", "#6b7280");
        fm_[Esc] = mk("#56b6c2", "#0184bc");
        fm_[Head] = mk("#61afef", "#2f64d8", true);
        fm_[Link] = mk("#5b8def", "#2f5cb8");
        fm_[Link].setFontUnderline(true);
        fm_[Type] = mk("#e5c07b", "#a06a00");
    }

    void paint(int start, int len, Tok t)
    {
        if (len > 0) setFormat(start, len, fm_[t]);
    }

    static bool isIdStart(QChar c) { return c.isLetter() || c == u'_'; }
    static bool isIdChar(QChar c) { return c.isLetterOrNumber() || c == u'_'; }

    // Chaîne entre guillemets ou apostrophes commençant en i ; renvoie l'indice juste après (ou la fin de la ligne si elle n'est pas fermée).
    int quoted(const QString &t, int i, bool escapes = true)
    {
        const QChar q = t.at(i);
        int j = i + 1;
        while (j < t.size()) {
            if (escapes && t.at(j) == u'\\') {
                j += 2;
                continue;
            }
            if (t.at(j) == q) {
                ++j;
                break;
            }
            ++j;
        }
        j = qMin(j, int(t.size()));
        paint(i, j - i, Str);
        if (escapes)
            for (int k = i + 1; k < j - 1; ++k)
                if (t.at(k) == u'\\') {
                    paint(k, qMin(2, j - k), Esc);
                    ++k;
                }
        return j;
    }

    // Nombre commençant en i (décimal, flottant, exposant, hexadécimal) ; renvoie l'indice juste après.
    int number(const QString &t, int i)
    {
        const int n = t.size();
        int j = i;
        if (t.at(j) == u'0' && j + 1 < n && (t.at(j + 1) == u'x' || t.at(j + 1) == u'X')) {
            j += 2;
            while (j < n && (t.at(j).isDigit() || (t.at(j).toLower() >= u'a' && t.at(j).toLower() <= u'f'))) ++j;
        } else {
            while (j < n && (t.at(j).isDigit() || (t.at(j) == u'.' && j + 1 < n && t.at(j + 1).isDigit()))) ++j;
            if (j < n && (t.at(j) == u'e' || t.at(j) == u'E')) {
                int k = j + 1;
                if (k < n && (t.at(k) == u'+' || t.at(k) == u'-')) ++k;
                if (k < n && t.at(k).isDigit()) {
                    j = k;
                    while (j < n && t.at(j).isDigit()) ++j;
                }
            }
        }
        paint(i, j - i, Num);
        return j;
    }

    int skipSpaces(const QString &t, int i) const
    {
        while (i < t.size() && t.at(i).isSpace()) ++i;
        return i;
    }

    // ------------------------------------------------------------------ Lua
    // États : 0 normal ; 1000 + niveau : commentaire long --[==[ ; 2000 + niveau : chaîne longue [==[
    static bool luaLongOpen(const QString &t, int p, int &level, int &len)
    {
        if (p >= t.size() || t.at(p) != u'[') return false;
        int j = p + 1;
        while (j < t.size() && t.at(j) == u'=') ++j;
        if (j >= t.size() || t.at(j) != u'[') return false;
        level = j - p - 1;
        len = j - p + 1;
        return true;
    }

    void lua(const QString &t)
    {
        static const QSet<QString> kw = {"and", "break", "do", "else", "elseif", "end", "for", "function", "goto", "if", "in",
                                         "local", "not", "or", "repeat", "return", "then", "until", "while"};
        static const QSet<QString> cst = {"true", "false", "nil"};
        static const QSet<QString> bi = {"print", "pairs", "ipairs", "next", "type", "tostring", "tonumber", "require", "dofile",
                                         "loadstring", "load", "pcall", "xpcall", "error", "assert", "select", "unpack", "rawget",
                                         "rawset", "rawequal", "setmetatable", "getmetatable", "table", "string", "math", "os",
                                         "io", "coroutine", "_G", "_VERSION", "self", "bit"};
        static const QSet<QString> ns = {"core", "minetest", "vector", "VoxelArea", "PseudoRandom", "PcgRandom", "SecureRandom",
                                         "ItemStack", "Settings", "PerlinNoise", "PerlinNoiseMap", "dump", "dump2"};
        const int n = t.size();
        int i = 0;
        const int st = qMax(0, previousBlockState());

        // ouvre un crochet long en p (comment : commentaire ou chaîne) ; start = début de la couleur
        auto longBlock = [&](int start, int p, bool comment, int from) {
            int lvl = 0, olen = 0;
            if (!luaLongOpen(t, p, lvl, olen)) return false;
            const QString close = "]" + QString(lvl, QChar('=')) + "]";
            const int e = t.indexOf(close, from < 0 ? p + olen : from);
            const Tok tk = comment ? Com : Str;
            if (e < 0) {
                paint(start, n - start, tk);
                setCurrentBlockState((comment ? 1000 : 2000) + lvl);
                i = n;
            } else {
                paint(start, e + close.size() - start, tk);
                i = e + close.size();
            }
            return true;
        };

        if (st >= 1000) { // on est au milieu d'un commentaire / d'une chaîne longue
            const bool com = st < 2000;
            const int lvl = st % 1000;
            const QString close = "]" + QString(lvl, QChar('=')) + "]";
            const int e = t.indexOf(close);
            const Tok tk = com ? Com : Str;
            if (e < 0) {
                paint(0, n, tk);
                setCurrentBlockState(st);
                return;
            }
            paint(0, e + close.size(), tk);
            i = e + close.size();
        }

        QString prevWord;
        while (i < n) {
            const QChar c = t.at(i);
            if (c.isSpace()) {
                ++i;
                continue;
            }
            if (c == u'-' && i + 1 < n && t.at(i + 1) == u'-') {
                if (longBlock(i, i + 2, true, -1)) continue;
                paint(i, n - i, Com);
                return;
            }
            if (c == u'[' && longBlock(i, i, false, -1)) continue;
            if (c == u'"' || c == u'\'') {
                i = quoted(t, i);
                prevWord.clear();
                continue;
            }
            if (c.isDigit() || (c == u'.' && i + 1 < n && t.at(i + 1).isDigit())) {
                i = number(t, i);
                prevWord.clear();
                continue;
            }
            if (isIdStart(c)) {
                int j = i + 1;
                while (j < n && isIdChar(t.at(j))) ++j;
                const QString w = t.mid(i, j - i);
                const int nx = skipSpaces(t, j);
                const bool afterDot = i > 0 && (t.at(i - 1) == u'.' || t.at(i - 1) == u':') && !(i > 1 && t.at(i - 2) == u'.');
                if (prevWord == "function") { // nom de fonction : a.b:c
                    int k = j;
                    while (k < n && (isIdChar(t.at(k)) || t.at(k) == u'.' || t.at(k) == u':')) ++k;
                    paint(i, k - i, Fn);
                    i = k;
                    prevWord.clear();
                    continue;
                }
                if (!afterDot && kw.contains(w)) paint(i, j - i, Kw);
                else if (!afterDot && cst.contains(w)) paint(i, j - i, Const);
                else if (!afterDot && ns.contains(w) && nx < n && t.at(nx) == u'.') paint(i, j - i, Type);
                else if (!afterDot && bi.contains(w)) paint(i, j - i, Bi);
                else if (nx < n && (t.at(nx) == u'(' || t.at(nx) == u'"' || t.at(nx) == u'\'' || t.at(nx) == u'{')) paint(i, j - i, Fn);
                prevWord = w;
                i = j;
                continue;
            }
            prevWord.clear();
            ++i;
        }
    }

    // ------------------------------------------------------------------ Python
    // États : 0 normal ; 1 : dans une chaîne ''' ; 2 : dans une chaîne """
    void python(const QString &t)
    {
        static const QSet<QString> kw = {"and", "as", "assert", "async", "await", "break", "class", "continue", "def", "del", "elif",
                                         "else", "except", "finally", "for", "from", "global", "if", "import", "in", "is", "lambda",
                                         "nonlocal", "not", "or", "pass", "raise", "return", "try", "while", "with", "yield", "match",
                                         "case"};
        static const QSet<QString> cst = {"True", "False", "None"};
        static const QSet<QString> bi = {"print", "len", "range", "int", "str", "float", "list", "dict", "set", "tuple", "bool", "bytes",
                                         "open", "isinstance", "issubclass", "super", "enumerate", "zip", "map", "filter", "sorted",
                                         "reversed", "min", "max", "sum", "abs", "any", "all", "type", "repr", "format", "getattr",
                                         "setattr", "hasattr", "id", "input", "iter", "next", "object", "round", "self", "cls",
                                         "Exception", "ValueError", "KeyError", "TypeError", "OSError", "__name__", "__file__"};
        const int n = t.size();
        int i = 0;
        const int st = previousBlockState();

        // indice juste après la fermeture d'une chaîne triple cherchée à partir de `from`, ou -1
        auto tripleClose = [&](int from, QChar q) {
            const QString close(3, q);
            int e = from;
            while ((e = t.indexOf(close, e)) >= 0) {
                int bs = 0; // guillemet échappé : précédé d'un nombre impair d'antislashs
                for (int k = e - 1; k >= from && t.at(k) == u'\\'; --k) ++bs;
                if (bs % 2 == 0) return e + 3;
                ++e;
            }
            return -1;
        };

        if (st == 1 || st == 2) { // on est au milieu d'une chaîne triple
            const int e = tripleClose(0, st == 1 ? QChar(u'\'') : QChar(u'"'));
            if (e < 0) {
                paint(0, n, Str);
                setCurrentBlockState(st);
                return;
            }
            paint(0, e, Str);
            i = e;
        }

        QString prevWord;
        while (i < n) {
            const QChar c = t.at(i);
            if (c.isSpace()) {
                ++i;
                continue;
            }
            if (c == u'#') {
                paint(i, n - i, Com);
                return;
            }
            if (c == u'@' && t.left(i).trimmed().isEmpty()) { // décorateur
                int j = i + 1;
                while (j < n && (isIdChar(t.at(j)) || t.at(j) == u'.')) ++j;
                paint(i, j - i, Type);
                i = j;
                continue;
            }
            // chaîne, éventuellement avec préfixe (r, b, f, u, rb, fr...)
            int qpos = -1;
            bool raw = false;
            if (c == u'"' || c == u'\'') {
                qpos = i;
            } else if (isIdStart(c)) {
                int j = i;
                while (j < n && isIdChar(t.at(j)) && j - i < 3) ++j;
                if (j < n && (t.at(j) == u'"' || t.at(j) == u'\'')) {
                    const QString pre = t.mid(i, j - i).toLower();
                    static const QRegularExpression preRe("^(?:[rbfu]|rb|br|rf|fr)$");
                    if (preRe.match(pre).hasMatch()) {
                        qpos = j;
                        raw = pre.contains('r');
                    }
                }
            }
            if (qpos >= 0) {
                const QChar q = t.at(qpos);
                if (qpos + 2 < n && t.at(qpos + 1) == q && t.at(qpos + 2) == q) { // chaîne triple
                    const int e = tripleClose(qpos + 3, q);
                    if (e < 0) {
                        paint(i, n - i, Str);
                        setCurrentBlockState(q == u'\'' ? 1 : 2);
                        return;
                    }
                    paint(i, e - i, Str);
                    i = e;
                    prevWord.clear();
                    continue;
                }
                const int e = quoted(t, qpos, !raw);
                if (qpos > i) paint(i, qpos - i, Str);
                i = e;
                prevWord.clear();
                continue;
            }
            if (c.isDigit() || (c == u'.' && i + 1 < n && t.at(i + 1).isDigit())) {
                i = number(t, i);
                prevWord.clear();
                continue;
            }
            if (isIdStart(c)) {
                int j = i + 1;
                while (j < n && isIdChar(t.at(j))) ++j;
                const QString w = t.mid(i, j - i);
                const int nx = skipSpaces(t, j);
                const bool afterDot = i > 0 && t.at(i - 1) == u'.';
                if (prevWord == "def") paint(i, j - i, Fn);
                else if (prevWord == "class") paint(i, j - i, Type);
                else if (!afterDot && kw.contains(w)) paint(i, j - i, Kw);
                else if (!afterDot && cst.contains(w)) paint(i, j - i, Const);
                else if (!afterDot && bi.contains(w)) paint(i, j - i, Bi);
                else if (nx < n && t.at(nx) == u'(') paint(i, j - i, Fn);
                prevWord = w;
                i = j;
                continue;
            }
            prevWord.clear();
            ++i;
        }
    }

    // ------------------------------------------------------------------ YAML
    // États : 0 normal ; 100 + indentation : dans un bloc littéral (| ou >)
    void yaml(const QString &t)
    {
        const int n = t.size();
        const int st = previousBlockState();
        int indent = 0;
        while (indent < n && t.at(indent) == u' ') ++indent;

        if (st >= 100) {
            if (t.trimmed().isEmpty() || indent > st - 100) {
                paint(0, n, Str);
                setCurrentBlockState(st);
                return;
            }
        }

        // début d'un commentaire (# précédé d'un espace, hors guillemets)
        int comment = -1;
        {
            QChar q;
            for (int i = 0; i < n; ++i) {
                const QChar c = t.at(i);
                if (!q.isNull()) {
                    if (c == u'\\' && q == u'"') ++i;
                    else if (c == q) q = QChar();
                } else if ((c == u'"' || c == u'\'') && (i == 0 || t.at(i - 1).isSpace() || t.at(i - 1) == u'[' || t.at(i - 1) == u'{' ||
                                                           t.at(i - 1) == u',' || t.at(i - 1) == u':' || t.at(i - 1) == u'-')) {
                    q = c;
                } else if (c == u'#' && (i == 0 || t.at(i - 1).isSpace())) {
                    comment = i;
                    break;
                }
            }
        }
        const int end = comment < 0 ? n : comment;
        if (comment >= 0) paint(comment, n - comment, Com);
        const QString body = t.left(end);
        if (body.trimmed().isEmpty()) return;
        if (body.trimmed() == "---" || body.trimmed() == "...") {
            paint(0, end, Punct);
            return;
        }

        static const QRegularExpression keyRe(
            R"(^(\s*(?:-\s+)*)("[^"]*"|'[^']*'|[^\s#:'"\[\]{},&*!|>%@`][^#:]*?)(\s*)(:)(?=\s|$))");
        int vs = 0; // début de la valeur
        int keyIndent = indent;
        const QRegularExpressionMatch m = keyRe.match(body);
        if (m.hasMatch()) {
            const QString pre = m.captured(1);
            for (int k = 0; k < pre.size(); ++k)
                if (pre.at(k) == u'-') {
                    paint(k, 1, Punct);
                    keyIndent = k + 2;
                }
            paint(pre.size(), m.captured(2).size(), Key);
            paint(m.capturedStart(4), 1, Punct);
            vs = m.capturedEnd(4);
        } else {
            const int d = skipSpaces(body, 0);
            if (d < end && body.at(d) == u'-' && (d + 1 >= end || body.at(d + 1) == u' ')) {
                paint(d, 1, Punct);
                vs = d + 1;
                keyIndent = d + 2;
            }
        }

        static const QRegularExpression numRe(R"(^[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?$|^0x[0-9a-fA-F]+$)");
        static const QSet<QString> cst = {"true", "false", "yes", "no", "on", "off", "null", "~"};
        int depth = 0;
        bool first = true;
        int i = vs;
        while (i < end) {
            const QChar c = body.at(i);
            if (c.isSpace()) {
                ++i;
                continue;
            }
            if (c == u'"' || c == u'\'') {
                i = quoted(body, i, c == u'"');
                first = false;
                continue;
            }
            if (c == u'[' || c == u'{') {
                paint(i++, 1, Punct);
                ++depth;
                first = false;
                continue;
            }
            if (c == u']' || c == u'}') {
                paint(i++, 1, Punct);
                depth = qMax(0, depth - 1);
                continue;
            }
            if (c == u',' && depth > 0) {
                paint(i++, 1, Punct);
                continue;
            }
            if ((c == u'&' || c == u'*' || c == u'!') && (i == vs || body.at(i - 1).isSpace())) {
                int j = i + 1;
                while (j < end && !body.at(j).isSpace() && body.at(j) != u',' && body.at(j) != u']' && body.at(j) != u'}') ++j;
                paint(i, j - i, Type);
                i = j;
                continue;
            }
            if (first && depth == 0 && (c == u'|' || c == u'>')) { // bloc littéral
                paint(i, end - i, Punct);
                setCurrentBlockState(100 + keyIndent);
                return;
            }
            int j = i;
            if (depth > 0) {
                while (j < end && body.at(j) != u',' && body.at(j) != u']' && body.at(j) != u'}') ++j;
            } else {
                j = end;
            }
            const QString word = body.mid(i, j - i).trimmed();
            if (numRe.match(word).hasMatch()) paint(i, j - i, Num);
            else if (cst.contains(word.toLower())) paint(i, j - i, Const);
            i = j;
            first = false;
        }
    }

    // ------------------------------------------------------------------ JSON
    void json(const QString &t)
    {
        const int n = t.size();
        int i = 0;
        while (i < n) {
            const QChar c = t.at(i);
            if (c.isSpace()) {
                ++i;
                continue;
            }
            if (c == u'"') {
                const int s = i;
                i = quoted(t, i);
                const int nx = skipSpaces(t, i);
                if (nx < n && t.at(nx) == u':') paint(s, i - s, Key);
                continue;
            }
            if (c.isDigit() || (c == u'-' && i + 1 < n && t.at(i + 1).isDigit())) {
                int j = i;
                if (c == u'-') ++j;
                const int e = number(t, j);
                if (c == u'-') paint(i, 1, Num);
                i = e;
                continue;
            }
            if (c.isLetter()) {
                int j = i;
                while (j < n && t.at(j).isLetter()) ++j;
                const QString w = t.mid(i, j - i);
                if (w == "true" || w == "false" || w == "null") paint(i, j - i, Const);
                i = j;
                continue;
            }
            if (QString("{}[],:").contains(c)) paint(i, 1, Punct);
            ++i;
        }
    }

    // ------------------------------------------------------------------ XML / HTML
    // États : 0 texte ; 1 commentaire <!-- ; 2 à l'intérieur d'une balise
    void xml(const QString &t)
    {
        const int n = t.size();
        int i = 0;
        int st = qMax(0, previousBlockState());
        if (st == 1) {
            const int e = t.indexOf("-->");
            if (e < 0) {
                paint(0, n, Com);
                setCurrentBlockState(1);
                return;
            }
            paint(0, e + 3, Com);
            i = e + 3;
            st = 0;
        }
        while (i < n) {
            const QChar c = t.at(i);
            if (st == 2) {
                if (c.isSpace()) {
                    ++i;
                    continue;
                }
                if (c == u'>' || ((c == u'/' || c == u'?') && i + 1 < n && t.at(i + 1) == u'>')) {
                    const int len = c == u'>' ? 1 : 2;
                    paint(i, len, Tag);
                    i += len;
                    st = 0;
                    continue;
                }
                if (c == u'"' || c == u'\'') {
                    i = quoted(t, i, false);
                    continue;
                }
                if (c == u'=') {
                    paint(i++, 1, Punct);
                    continue;
                }
                int j = i;
                while (j < n && (isIdChar(t.at(j)) || t.at(j) == u'-' || t.at(j) == u':' || t.at(j) == u'.')) ++j;
                if (j == i) ++j;
                paint(i, j - i, Attr);
                i = j;
                continue;
            }
            if (t.mid(i, 4) == "<!--") {
                const int e = t.indexOf("-->", i + 4);
                if (e < 0) {
                    paint(i, n - i, Com);
                    setCurrentBlockState(1);
                    return;
                }
                paint(i, e + 3 - i, Com);
                i = e + 3;
                continue;
            }
            if (c == u'<') {
                int j = i + 1;
                if (j < n && (t.at(j) == u'/' || t.at(j) == u'?' || t.at(j) == u'!')) ++j;
                while (j < n && (isIdChar(t.at(j)) || t.at(j) == u'-' || t.at(j) == u':' || t.at(j) == u'.')) ++j;
                paint(i, j - i, Tag);
                i = j;
                st = 2;
                continue;
            }
            if (c == u'&') {
                const int e = t.indexOf(QChar(u';'), i);
                if (e > 0 && e - i <= 10) {
                    paint(i, e - i + 1, Esc);
                    i = e + 1;
                    continue;
                }
            }
            ++i;
        }
        setCurrentBlockState(st);
    }

    // ------------------------------------------------------------------ .conf (minetest.conf, mod.conf, settingtypes.txt)
    void conf(const QString &t)
    {
        const QString trm = t.trimmed();
        const int off = skipSpaces(t, 0);
        if (trm.isEmpty()) return;
        if (trm.startsWith('#')) {
            paint(off, t.size() - off, Com);
            return;
        }
        if (trm.startsWith('[') && trm.endsWith(']')) {
            paint(off, trm.size(), Type);
            return;
        }
        const int eq = t.indexOf(QChar(u'='));
        if (eq < 0) return;
        paint(off, eq - off, Key);
        paint(eq, 1, Punct);
        const int vs = skipSpaces(t, eq + 1);
        const QString v = t.mid(vs).trimmed();
        static const QRegularExpression numRe(R"(^[-+]?(?:\d+\.?\d*|\.\d+)$)");
        if (v.startsWith('"')) quoted(t, vs);
        else if (numRe.match(v).hasMatch()) paint(vs, v.size(), Num);
        else if (v == "true" || v == "false") paint(vs, v.size(), Const);
    }

    // ------------------------------------------------------------------ formspec Luanti : size[8,6]button[1,1;2,1;nom;Texte]
    void formspec(const QString &t)
    {
        const int n = t.size();
        int i = 0;
        while (i < n) {
            const QChar c = t.at(i);
            if (c == u'\\' && i + 1 < n) {
                paint(i, 2, Esc);
                i += 2;
                continue;
            }
            if (isIdStart(c) && (i == 0 || !isIdChar(t.at(i - 1)))) {
                int j = i + 1;
                while (j < n && isIdChar(t.at(j))) ++j;
                if (j < n && t.at(j) == u'[') paint(i, j - i, Fn);
                i = j;
                continue;
            }
            if (c == u'[' || c == u']' || c == u';' || c == u',') {
                paint(i++, 1, Punct);
                continue;
            }
            if (c == u'#') {
                int j = i + 1;
                while (j < n && j - i <= 8 && (t.at(j).isDigit() || (t.at(j).toLower() >= u'a' && t.at(j).toLower() <= u'f'))) ++j;
                if (j - i - 1 == 3 || j - i - 1 == 4 || j - i - 1 == 6 || j - i - 1 == 8) {
                    paint(i, j - i, Const);
                    i = j;
                    continue;
                }
            }
            if ((c.isDigit() || (c == u'-' && i + 1 < n && t.at(i + 1).isDigit())) && (i == 0 || !isIdChar(t.at(i - 1)))) {
                int j = i + 1;
                while (j < n && (t.at(j).isDigit() || t.at(j) == u'.')) ++j;
                paint(i, j - i, Num);
                i = j;
                continue;
            }
            ++i;
        }
    }

    // ------------------------------------------------------------------ Markdown (état 1 : bloc de code ```)
    void markdown(const QString &t)
    {
        const int n = t.size();
        const QString trm = t.trimmed();
        const bool inFence = previousBlockState() == 1;
        if (trm.startsWith("```") || trm.startsWith("~~~")) {
            paint(0, n, Str);
            setCurrentBlockState(inFence ? 0 : 1);
            return;
        }
        if (inFence) {
            paint(0, n, Str);
            setCurrentBlockState(1);
            return;
        }
        static const QRegularExpression headRe(R"(^\s{0,3}#{1,6}\s)");
        if (headRe.match(t).hasMatch()) {
            paint(0, n, Head);
            return;
        }
        if (trm.startsWith('>')) {
            paint(0, n, Com);
            return;
        }
        static const QRegularExpression listRe(R"(^\s*(?:[-*+]|\d+[.)])\s)");
        const QRegularExpressionMatch lm = listRe.match(t);
        if (lm.hasMatch()) paint(0, lm.capturedLength() - 1, Punct);
        static const QRegularExpression codeRe("`[^`]+`");
        for (auto it = codeRe.globalMatch(t); it.hasNext();) {
            const auto m = it.next();
            paint(m.capturedStart(), m.capturedLength(), Str);
        }
        static const QRegularExpression linkRe(R"(!?\[[^\]]*\]\([^)]*\))");
        for (auto it = linkRe.globalMatch(t); it.hasNext();) {
            const auto m = it.next();
            paint(m.capturedStart(), m.capturedLength(), Link);
        }
        static const QRegularExpression boldRe(R"(\*\*[^*]+\*\*|__[^_]+__)");
        for (auto it = boldRe.globalMatch(t); it.hasNext();) {
            const auto m = it.next();
            QTextCharFormat f = format(m.capturedStart());
            f.setFontWeight(QFont::Bold);
            setFormat(m.capturedStart(), m.capturedLength(), f);
        }
    }

    // ------------------------------------------------------------------ shell
    void shell(const QString &t)
    {
        static const QSet<QString> kw = {"if", "then", "else", "elif", "fi", "for", "while", "until", "do", "done", "case", "esac", "in",
                                         "function", "select", "return", "exit", "break", "continue", "local", "export", "unset", "set",
                                         "source", "shift", "trap", "readonly", "declare"};
        static const QSet<QString> bi = {"echo", "cd", "read", "test", "printf", "command", "eval", "exec", "true", "false", "pwd", "ls",
                                         "cp", "mv", "rm", "mkdir", "cat", "grep", "sed", "awk", "sudo", "apt", "python3", "cmake"};
        const int n = t.size();
        int i = 0;
        while (i < n) {
            const QChar c = t.at(i);
            if (c.isSpace()) {
                ++i;
                continue;
            }
            if (c == u'#' && (i == 0 || t.at(i - 1).isSpace())) {
                paint(i, n - i, Com);
                return;
            }
            if (c == u'\'') {
                const int e = t.indexOf(QChar(u'\''), i + 1);
                const int end = e < 0 ? n : e + 1;
                paint(i, end - i, Str);
                i = end;
                continue;
            }
            if (c == u'"') {
                i = quoted(t, i);
                continue;
            }
            if (c == u'$') {
                int j = i + 1;
                if (j < n && (t.at(j) == u'{' || t.at(j) == u'(')) {
                    const QChar close = t.at(j) == u'{' ? u'}' : u')';
                    const int e = t.indexOf(close, j);
                    j = e < 0 ? n : e + 1;
                } else {
                    while (j < n && (isIdChar(t.at(j)) || t.at(j) == u'?' || t.at(j) == u'@' || t.at(j) == u'#')) ++j;
                }
                paint(i, j - i, Bi);
                i = j;
                continue;
            }
            if (c == u'-' && i + 1 < n && (t.at(i + 1).isLetter() || t.at(i + 1) == u'-') && (i == 0 || t.at(i - 1).isSpace())) {
                int j = i + 1;
                while (j < n && (isIdChar(t.at(j)) || t.at(j) == u'-' || t.at(j) == u'=')) {
                    if (t.at(j) == u'=') break;
                    ++j;
                }
                paint(i, j - i, Attr);
                i = j;
                continue;
            }
            if (c.isDigit() && (i == 0 || !isIdChar(t.at(i - 1)))) {
                i = number(t, i);
                continue;
            }
            if (isIdStart(c)) {
                int j = i + 1;
                while (j < n && (isIdChar(t.at(j)) || t.at(j) == u'-')) ++j;
                const QString w = t.mid(i, j - i);
                if (kw.contains(w)) paint(i, j - i, Kw);
                else if (bi.contains(w)) paint(i, j - i, Fn);
                i = j;
                continue;
            }
            ++i;
        }
    }

    LcLang lang_;
    QTextCharFormat fm_[TokCount];
};

// Active la coloration sur un éditeur / visualiseur de code. Le coloriseur appartient au document de l'éditeur.
inline LcHighlighter *lcAttachHighlighter(QPlainTextEdit *edit, LcLang lang)
{
    return new LcHighlighter(edit->document(), lang);
}
