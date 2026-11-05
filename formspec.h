#pragma once
// Éditeur graphique de formspec : on glisse des éléments (texte, boutons, champs, cases, listes d'inventaire...)
// sur un canevas qui sert d'aperçu, on les déplace / redimensionne à la souris, on règle leurs propriétés, et
// LuantiCreator produit la chaîne formspec (formspec_version 6) et le code Lua qui l'affiche.
//
// Les formulaires sont enregistrés dans le projet (clé « formspecs » du .lcproj).

#include <QtWidgets>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>

#include "codehl.h"
#include "generator.h"
#include "preview.h"
#include "uihooks.h"

namespace lcfs {

// ------------------------------------------------------------------ modèle

struct FsElem {
    QString type;       // label button button_exit field pwdfield textarea checkbox dropdown image item_image box list
    double x = 0, y = 0, w = 1, h = 1;
    QString name;       // nom du champ (fields.<name>) ; pour une liste : nom de la liste d'inventaire
    QString text;       // texte affiché / étiquette
    QString extra;      // valeur par défaut, éléments du menu, texture, objet, couleur ou emplacement de la liste
    bool flag = false;  // case cochée par défaut

    QJsonObject toJson() const
    {
        QJsonObject o;
        o["type"] = type;
        o["x"] = x;
        o["y"] = y;
        o["w"] = w;
        o["h"] = h;
        o["name"] = name;
        o["text"] = text;
        o["extra"] = extra;
        o["flag"] = flag;
        return o;
    }
    static FsElem fromJson(const QJsonObject &o)
    {
        FsElem e;
        e.type = o["type"].toString();
        e.x = o["x"].toDouble();
        e.y = o["y"].toDouble();
        e.w = o["w"].toDouble(1);
        e.h = o["h"].toDouble(1);
        e.name = o["name"].toString();
        e.text = o["text"].toString();
        e.extra = o["extra"].toString();
        e.flag = o["flag"].toBool();
        return e;
    }
};

struct FsForm {
    QString name = "formulaire1";
    double w = 10, h = 7;
    QString bg;               // couleur de fond (« #RRGGBB » ou « #RRGGBBAA »), vide = fond par défaut
    bool command = true;      // commande de chat /<nom> pour ouvrir le formulaire
    QString onFields;         // code Lua exécuté à la réception des champs (variables : player, fields)
    QVector<FsElem> elems;

    QJsonObject toJson() const
    {
        QJsonObject o;
        o["name"] = name;
        o["w"] = w;
        o["h"] = h;
        o["bg"] = bg;
        o["command"] = command;
        o["on_fields"] = onFields;
        QJsonArray a;
        for (const FsElem &e : elems) a.append(e.toJson());
        o["elems"] = a;
        return o;
    }
    static FsForm fromJson(const QJsonObject &o)
    {
        FsForm f;
        f.name = o["name"].toString("formulaire1");
        f.w = o["w"].toDouble(10);
        f.h = o["h"].toDouble(7);
        f.bg = o["bg"].toString();
        f.command = o["command"].toBool(true);
        f.onFields = o["on_fields"].toString();
        for (const QJsonValue &v : o["elems"].toArray()) f.elems.append(FsElem::fromJson(v.toObject()));
        return f;
    }
};

struct TypeInfo {
    QString id, title, emoji;
    double w, h; // taille par défaut (en unités de formspec)
};

inline const QVector<TypeInfo> &types()
{
    static const QVector<TypeInfo> t = {
        {"label", QObject::tr("Texte"), "", 3, 0.5},
        {"button", QObject::tr("Bouton"), "", 3, 0.8},
        {"button_exit", QObject::tr("Bouton qui ferme"), "", 3, 0.8},
        {"field", QObject::tr("Champ de saisie"), "", 4, 0.8},
        {"pwdfield", QObject::tr("Mot de passe"), "", 4, 0.8},
        {"textarea", QObject::tr("Zone de texte"), "", 5, 2},
        {"checkbox", QObject::tr("Case à cocher"), "", 3, 0.5},
        {"dropdown", QObject::tr("Menu déroulant"), "", 4, 0.8},
        {"image", QObject::tr("Image"), "", 2, 2},
        {"item_image", QObject::tr("Image d'objet"), "", 1, 1},
        {"box", QObject::tr("Rectangle coloré"), "", 2, 1},
        {"list", QObject::tr("Inventaire"), "", 8, 4},
    };
    return t;
}

inline const TypeInfo *typeInfo(const QString &id)
{
    for (const TypeInfo &t : types())
        if (t.id == id) return &t;
    return nullptr;
}

// label et checkbox n'ont pas de taille dans le formspec : leur largeur dépend du texte
inline bool hasSize(const QString &t) { return t != "label" && t != "checkbox"; }
inline bool hasName(const QString &t)
{
    return t == "button" || t == "button_exit" || t == "field" || t == "pwdfield" || t == "textarea" ||
           t == "checkbox" || t == "dropdown" || t == "list";
}
inline bool hasText(const QString &t)
{
    return t == "label" || t == "button" || t == "button_exit" || t == "field" || t == "pwdfield" || t == "textarea" ||
           t == "checkbox";
}
inline bool hasExtra(const QString &t)
{
    return t == "field" || t == "textarea" || t == "dropdown" || t == "image" || t == "item_image" || t == "box" ||
           t == "list";
}

// largeur / hauteur réelles occupées dans le formulaire (en unités)
inline double effW(const FsElem &e)
{
    if (e.type == "label") return qMax(0.6, e.text.size() * 0.2 + 0.2);
    if (e.type == "checkbox") return 0.7 + e.text.size() * 0.2;
    if (e.type == "list") return e.w * 1.25 - 0.25;
    return e.w;
}
inline double effH(const FsElem &e)
{
    if (e.type == "label" || e.type == "checkbox") return 0.5;
    if (e.type == "list") return e.h * 1.25 - 0.25;
    return e.h;
}

// ------------------------------------------------------------------ génération

inline QString num(double v)
{
    QString s = QString::number(v, 'f', 3);
    if (s.contains('.')) {
        while (s.endsWith('0')) s.chop(1);
        if (s.endsWith('.')) s.chop(1);
    }
    if (s == "-0") s = "0";
    return s;
}

// échappement d'un texte dans un formspec
inline QString esc(const QString &s)
{
    QString r;
    for (const QChar c : s) {
        if (c == '\\' || c == '[' || c == ']' || c == ';' || c == ',' || c == '$') r += '\\';
        r += c;
    }
    return r;
}

inline QString fieldName(const FsElem &e)
{
    QString n = identOf(e.name);
    return n.isEmpty() ? e.type : n;
}

inline QString elemLine(const FsElem &e)
{
    const QString t = e.type;
    const QString xy = num(e.x) + "," + num(e.y);
    const QString wh = num(e.w) + "," + num(e.h);
    if (t == "label") return "label[" + num(e.x) + "," + num(e.y + 0.25) + ";" + esc(e.text) + "]"; // y = milieu du texte
    if (t == "button" || t == "button_exit")
        return t + "[" + xy + ";" + wh + ";" + esc(fieldName(e)) + ";" + esc(e.text) + "]";
    if (t == "field" || t == "textarea")
        return t + "[" + xy + ";" + wh + ";" + esc(fieldName(e)) + ";" + esc(e.text) + ";" + esc(e.extra) + "]";
    if (t == "pwdfield") return "pwdfield[" + xy + ";" + wh + ";" + esc(fieldName(e)) + ";" + esc(e.text) + "]";
    if (t == "checkbox")
        return "checkbox[" + num(e.x) + "," + num(e.y + 0.25) + ";" + esc(fieldName(e)) + ";" + esc(e.text) + ";" +
               (e.flag ? "true" : "false") + "]";
    if (t == "dropdown") {
        QStringList items;
        for (const QString &it : e.extra.split(',', Qt::SkipEmptyParts)) items << esc(it.trimmed());
        return "dropdown[" + xy + ";" + wh + ";" + esc(fieldName(e)) + ";" + items.join(",") + ";1]";
    }
    if (t == "image") return "image[" + xy + ";" + wh + ";" + esc(e.extra) + "]";
    if (t == "item_image") return "item_image[" + xy + ";" + wh + ";" + esc(e.extra) + "]";
    if (t == "box") return "box[" + xy + ";" + wh + ";" + (e.extra.isEmpty() ? QString("#4a90d980") : e.extra) + "]";
    if (t == "list") {
        const QString loc = e.extra.isEmpty() ? QString("current_player") : e.extra;
        const QString ln = e.name.isEmpty() ? QString("main") : e.name;
        return "list[" + esc(loc) + ";" + esc(ln) + ";" + xy + ";" + num(qMax(1.0, e.w)) + "," + num(qMax(1.0, e.h)) + ";]";
    }
    return QString();
}

// lignes du formspec (sans échappement Lua)
inline QStringList formLines(const FsForm &f)
{
    QStringList l;
    l << "formspec_version[6]";
    l << "size[" + num(f.w) + "," + num(f.h) + "]";
    if (!f.bg.trimmed().isEmpty()) l << "box[0,0;" + num(f.w) + "," + num(f.h) + ";" + f.bg.trimmed() + "]";
    for (const FsElem &e : f.elems) {
        const QString s = elemLine(e);
        if (!s.isEmpty()) l << s;
    }
    return l;
}

inline QString formIdent(const FsForm &f)
{
    QString n = identOf(f.name);
    return n.isEmpty() ? QString("formulaire") : n;
}

// code Lua d'un formulaire
inline QString formLua(const FsForm &f, const QString &modid)
{
    const QString id = formIdent(f);
    const QString var = "fs_" + id;
    const QString formname = modid + ":" + id;
    QString s;
    s += "-- Formspec « " + f.name + " » (éditeur de formspec)\n";
    s += "local " + var + " = table.concat({\n";
    for (const QString &l : formLines(f)) s += "    " + luaStr(l) + ",\n";
    s += "}, \"\")\n\n";
    if (f.command) {
        s += "core.register_chatcommand(" + luaStr(id) + ", {\n";
        s += "    description = " + luaStr(QObject::tr("Ouvre le formulaire « %1 »").arg(f.name)) + ",\n";
        s += "    func = function(name)\n";
        s += "        core.show_formspec(name, " + luaStr(formname) + ", " + var + ")\n";
        s += "        return true\n";
        s += "    end,\n";
        s += "})\n\n";
    }
    QStringList fieldNames;
    for (const FsElem &e : f.elems)
        if (hasName(e.type) && e.type != "list") fieldNames << "fields." + fieldName(e);
    s += "core.register_on_player_receive_fields(function(player, formname, fields)\n";
    s += "    if formname ~= " + luaStr(formname) + " then return end\n";
    if (!fieldNames.isEmpty()) s += "    -- champs disponibles : " + fieldNames.join(", ") + "\n";
    if (!f.onFields.trimmed().isEmpty()) {
        for (const QString &line : f.onFields.split('\n')) s += "    " + line + "\n";
    }
    s += "end)\n\n";
    return s;
}

inline QColor parseColor(const QString &s, const QColor &def)
{
    const QString t = s.trimmed();
    if (t.isEmpty()) return def;
    if (t.startsWith('#') && t.size() == 9) { // Luanti : #RRGGBBAA (Qt : #AARRGGBB)
        QColor c(t.left(7));
        if (!c.isValid()) return def;
        bool ok = false;
        const int a = t.mid(7, 2).toInt(&ok, 16);
        c.setAlpha(ok ? a : 255);
        return c;
    }
    const QColor c(t);
    return c.isValid() ? c : def;
}

// ------------------------------------------------------------------ palette (source du glisser-déposer)

class FsPalette : public QListWidget {
public:
    static const char *mime() { return "application/x-lc-fselem"; }
    std::function<void(const QString &)> onActivate; // double-clic : ajoute l'élément

    FsPalette()
    {
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragOnly);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setIconSize(QSize(24, 24));
        for (const TypeInfo &t : types()) {
            auto *it = new QListWidgetItem(t.title);
            it->setData(Qt::UserRole, t.id);
            addItem(it);
        }
        connect(this, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *it) {
            if (it && onActivate) onActivate(it->data(Qt::UserRole).toString());
        });
    }

protected:
    void startDrag(Qt::DropActions) override
    {
        QListWidgetItem *it = currentItem();
        if (!it) return;
        auto *md = new QMimeData;
        md->setData(mime(), it->data(Qt::UserRole).toString().toUtf8());
        auto *d = new QDrag(this);
        d->setMimeData(md);
        d->exec(Qt::CopyAction);
    }
};

// ------------------------------------------------------------------ canevas = aperçu

class FsCanvas : public QWidget {
public:
    static constexpr double S = 52.0; // pixels par unité de formspec
    static constexpr double M = 14.0; // marge autour du formulaire

    std::function<void()> onChanged;      // un élément a été ajouté / déplacé / modifié
    std::function<void(int)> onSelect;    // sélection modifiée (-1 = aucune)

    FsCanvas(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAcceptDrops(true);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        fit();
    }

    void setForm(FsForm *f)
    {
        form_ = f;
        sel_ = -1;
        fit();
        update();
    }
    void setPreview(bool on)
    {
        preview_ = on;
        update();
    }
    void setSnap(double s) { snap_ = s; }
    int selected() const { return sel_; }
    void select(int i)
    {
        sel_ = (form_ && i >= 0 && i < form_->elems.size()) ? i : -1;
        update();
        if (onSelect) onSelect(sel_);
    }

    // taille du formulaire modifiée
    void fit()
    {
        if (!form_) {
            setFixedSize(240, 120);
            return;
        }
        setFixedSize(int(form_->w * S + 2 * M), int(form_->h * S + 2 * M));
    }

    // ajoute un élément du type donné à (x, y) en unités
    void addElem(const QString &type, double x, double y)
    {
        if (!form_) return;
        const TypeInfo *ti = typeInfo(type);
        if (!ti) return;
        FsElem e;
        e.type = type;
        e.w = ti->w;
        e.h = ti->h;
        e.x = qMax(0.0, qMin(snapv(x), form_->w - 0.5));
        e.y = qMax(0.0, qMin(snapv(y), form_->h - 0.5));
        if (type == "list") e.name = "main"; // liste d'inventaire du joueur
        else if (hasName(type)) e.name = uniqueName(type);
        if (type == "label") e.text = QObject::tr("Texte");
        else if (type == "button") e.text = QObject::tr("Bouton");
        else if (type == "button_exit") e.text = QObject::tr("Fermer");
        else if (type == "field" || type == "pwdfield" || type == "textarea") e.text = QObject::tr("Étiquette");
        else if (type == "checkbox") e.text = QObject::tr("Option");
        if (type == "dropdown") e.extra = QObject::tr("Choix 1,Choix 2,Choix 3");
        if (type == "item_image") e.extra = "default:dirt";
        if (type == "box") e.extra = "#4a90d980";
        if (type == "list") e.extra = "current_player";
        if (type == "image") e.extra = "texture.png";
        form_->elems.append(e);
        select(int(form_->elems.size()) - 1);
        changed();
    }

    void deleteSelected()
    {
        if (!form_ || sel_ < 0 || sel_ >= form_->elems.size()) return;
        form_->elems.removeAt(sel_);
        select(-1);
        changed();
    }

    void duplicateSelected()
    {
        if (!form_ || sel_ < 0 || sel_ >= form_->elems.size()) return;
        FsElem e = form_->elems[sel_];
        e.x += 0.5;
        e.y += 0.5;
        if (hasName(e.type) && e.type != "list") e.name = uniqueName(e.type);
        form_->elems.append(e);
        select(int(form_->elems.size()) - 1);
        changed();
    }

    void moveSelectedToFront(bool front)
    {
        if (!form_ || sel_ < 0 || sel_ >= form_->elems.size()) return;
        const FsElem e = form_->elems.takeAt(sel_);
        if (front) form_->elems.append(e);
        else form_->elems.prepend(e);
        select(front ? int(form_->elems.size()) - 1 : 0);
        changed();
    }

    // à appeler quand les propriétés de l'élément changent depuis le panneau
    void refresh()
    {
        update();
    }

    QSize sizeHint() const override { return size(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor(0x1b, 0x1b, 0x1b));
        if (!form_) {
            p.setPen(Qt::gray);
            p.drawText(rect(), Qt::AlignCenter, QObject::tr("Créez un formulaire"));
            return;
        }
        const QRectF area(M, M, form_->w * S, form_->h * S);
        p.fillRect(area, parseColor(form_->bg, QColor(0x30, 0x30, 0x30)));
        if (!preview_) {
            p.setPen(QColor(255, 255, 255, 24));
            for (int i = 1; i < int(form_->w + 0.001); ++i) p.drawLine(QPointF(M + i * S, M), QPointF(M + i * S, M + form_->h * S));
            for (int j = 1; j < int(form_->h + 0.001); ++j) p.drawLine(QPointF(M, M + j * S), QPointF(M + form_->w * S, M + j * S));
        }
        p.setPen(QColor(255, 255, 255, 90));
        p.setBrush(Qt::NoBrush);
        p.drawRect(area);
        QFont f = font();
        f.setPixelSize(int(S * 0.3));
        p.setFont(f);
        for (const FsElem &e : form_->elems) drawElem(p, e);
        if (!preview_ && sel_ >= 0 && sel_ < form_->elems.size()) {
            const FsElem &e = form_->elems[sel_];
            QPen pen(QColor("#4a90d9"), 2, Qt::DashLine);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRect(rectPx(e).adjusted(-2, -2, 2, 2));
            if (hasSize(e.type)) {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor("#4a90d9"));
                p.drawRect(handleRect(e));
            }
        }
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (!form_ || e->button() != Qt::LeftButton) return;
        setFocus();
        const QPointF p = e->position();
        if (!preview_ && sel_ >= 0 && sel_ < form_->elems.size()) {
            const FsElem &s = form_->elems[sel_];
            if (hasSize(s.type) && handleRect(s).adjusted(-3, -3, 3, 3).contains(p)) {
                mode_ = Resize;
                return;
            }
        }
        const int hit = hitTest(p);
        select(hit);
        if (hit >= 0 && !preview_) {
            const FsElem &s = form_->elems[hit];
            mode_ = Move;
            grab_ = QPointF((p.x() - M) / S - s.x, (p.y() - M) / S - s.y);
        }
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (!form_) return;
        const QPointF p = e->position();
        if (mode_ == None || sel_ < 0 || sel_ >= form_->elems.size()) {
            if (!preview_ && sel_ >= 0 && sel_ < form_->elems.size() && hasSize(form_->elems[sel_].type) &&
                handleRect(form_->elems[sel_]).adjusted(-3, -3, 3, 3).contains(p))
                setCursor(Qt::SizeFDiagCursor);
            else if (!preview_ && hitTest(p) >= 0)
                setCursor(Qt::SizeAllCursor);
            else
                unsetCursor();
            return;
        }
        FsElem &s = form_->elems[sel_];
        const double ux = (p.x() - M) / S, uy = (p.y() - M) / S;
        if (mode_ == Move) {
            s.x = qMax(0.0, qMin(snapv(ux - grab_.x()), form_->w - 0.25));
            s.y = qMax(0.0, qMin(snapv(uy - grab_.y()), form_->h - 0.25));
        } else if (mode_ == Resize) {
            if (s.type == "list") { // w et h = nombre d'emplacements
                s.w = qMax(1.0, double(qRound((ux - s.x + 0.25) / 1.25)));
                s.h = qMax(1.0, double(qRound((uy - s.y + 0.25) / 1.25)));
            } else {
                s.w = qMax(0.25, snapv(ux - s.x));
                s.h = qMax(0.25, snapv(uy - s.y));
            }
        }
        update();
        changed();
    }

    void mouseReleaseEvent(QMouseEvent *) override { mode_ = None; }

    void mouseDoubleClickEvent(QMouseEvent *e) override
    {
        if (!form_ || preview_) return;
        const int hit = hitTest(e->position());
        if (hit < 0) return;
        select(hit);
        FsElem &s = form_->elems[hit];
        if (!hasText(s.type)) return;
        bool ok = false;
        const QString t = QInputDialog::getText(this, QObject::tr("Texte de l'élément"), QObject::tr("Texte :"),
                                                QLineEdit::Normal, s.text, &ok);
        if (!ok) return;
        s.text = t;
        update();
        changed();
    }

    void keyPressEvent(QKeyEvent *e) override
    {
        if (!form_ || sel_ < 0 || sel_ >= form_->elems.size()) {
            QWidget::keyPressEvent(e);
            return;
        }
        FsElem &s = form_->elems[sel_];
        const double st = snap_ > 0 ? snap_ : 0.05;
        switch (e->key()) {
        case Qt::Key_Delete:
        case Qt::Key_Backspace: deleteSelected(); return;
        case Qt::Key_Left: s.x = qMax(0.0, s.x - st); break;
        case Qt::Key_Right: s.x = qMin(form_->w - 0.25, s.x + st); break;
        case Qt::Key_Up: s.y = qMax(0.0, s.y - st); break;
        case Qt::Key_Down: s.y = qMin(form_->h - 0.25, s.y + st); break;
        case Qt::Key_D:
            if (e->modifiers() & Qt::ControlModifier) {
                duplicateSelected();
                return;
            }
            QWidget::keyPressEvent(e);
            return;
        default: QWidget::keyPressEvent(e); return;
        }
        update();
        changed();
    }

    void contextMenuEvent(QContextMenuEvent *e) override
    {
        if (!form_ || preview_) return;
        const int hit = hitTest(e->pos());
        select(hit);
        if (hit < 0) return;
        QMenu m;
        QAction *dup = m.addAction(QObject::tr("Dupliquer"));
        QAction *front = m.addAction(QObject::tr("Premier plan"));
        QAction *back = m.addAction(QObject::tr("Arrière-plan"));
        m.addSeparator();
        QAction *del = m.addAction(QObject::tr("Supprimer"));
        UiHooks::bind(dup, "menu.dup", "formspec");
        UiHooks::bind(front, "menu.front", "formspec");
        UiHooks::bind(back, "menu.back", "formspec");
        UiHooks::bind(del, "menu.del", "formspec");
        QAction *r = m.exec(e->globalPos());
        if (r == dup) duplicateSelected();
        else if (r == front) moveSelectedToFront(true);
        else if (r == back) moveSelectedToFront(false);
        else if (r == del) deleteSelected();
    }

    void dragEnterEvent(QDragEnterEvent *e) override
    {
        if (e->mimeData()->hasFormat(FsPalette::mime())) e->acceptProposedAction();
    }
    void dragMoveEvent(QDragMoveEvent *e) override
    {
        if (e->mimeData()->hasFormat(FsPalette::mime())) e->acceptProposedAction();
    }
    void dropEvent(QDropEvent *e) override
    {
        if (!e->mimeData()->hasFormat(FsPalette::mime())) return;
        const QString type = QString::fromUtf8(e->mimeData()->data(FsPalette::mime()));
        const QPointF p = e->position();
        addElem(type, (p.x() - M) / S, (p.y() - M) / S);
        e->acceptProposedAction();
        setFocus();
    }

private:
    enum Mode { None, Move, Resize };

    void changed()
    {
        if (onChanged) onChanged();
    }

    double snapv(double v) const
    {
        const double s = snap_ > 0 ? snap_ : 0.05;
        return qRound(v / s) * s;
    }

    QString uniqueName(const QString &base) const
    {
        QSet<QString> used;
        if (form_)
            for (const FsElem &e : form_->elems) used.insert(e.name);
        for (int i = 1; i < 1000; ++i) {
            const QString n = base + QString::number(i);
            if (!used.contains(n)) return n;
        }
        return base;
    }

    QRectF rectPx(const FsElem &e) const { return QRectF(M + e.x * S, M + e.y * S, effW(e) * S, effH(e) * S); }
    QRectF handleRect(const FsElem &e) const
    {
        const QRectF r = rectPx(e);
        return QRectF(r.right() - 5, r.bottom() - 5, 10, 10);
    }

    int hitTest(const QPointF &p) const
    {
        if (!form_) return -1;
        for (int i = int(form_->elems.size()) - 1; i >= 0; --i) {
            QRectF r = rectPx(form_->elems[i]);
            if (form_->elems[i].type == "label") r.adjust(-2, -2, 2, 2);
            if (r.contains(p)) return i;
        }
        return -1;
    }

    void drawElem(QPainter &p, const FsElem &e) const
    {
        const QRectF r = rectPx(e);
        const QString t = e.type;
        const QColor txt(0xe8, 0xe8, 0xe8);
        p.save();
        if (t == "label") {
            p.setPen(txt);
            p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, e.text);
        } else if (t == "button" || t == "button_exit") {
            QLinearGradient g(r.topLeft(), r.bottomLeft());
            g.setColorAt(0, QColor(0x7a, 0x7a, 0x7a));
            g.setColorAt(1, QColor(0x55, 0x55, 0x55));
            p.setBrush(g);
            p.setPen(QColor(0x20, 0x20, 0x20));
            p.drawRoundedRect(r, 3, 3);
            p.setPen(Qt::white);
            p.drawText(r, Qt::AlignCenter, e.text);
        } else if (t == "field" || t == "pwdfield" || t == "textarea") {
            if (!e.text.isEmpty()) {
                QFont sf = p.font();
                sf.setPixelSize(int(S * 0.24));
                p.setFont(sf);
                p.setPen(QColor(0xc8, 0xc8, 0xc8));
                p.drawText(QRectF(r.left(), r.top() - S * 0.34, r.width(), S * 0.32), Qt::AlignLeft | Qt::AlignBottom, e.text);
                sf.setPixelSize(int(S * 0.3));
                p.setFont(sf);
            }
            p.setBrush(QColor(0x14, 0x14, 0x14));
            p.setPen(QColor(0x70, 0x70, 0x70));
            p.drawRect(r);
            p.setPen(QColor(0xdd, 0xdd, 0xdd));
            const QString shown = t == "pwdfield" ? QString(e.extra.size() ? e.extra.size() : 0, QChar(0x2022)) : e.extra;
            p.drawText(r.adjusted(6, 4, -6, -4), (t == "textarea" ? Qt::AlignLeft | Qt::AlignTop : Qt::AlignLeft | Qt::AlignVCenter) | Qt::TextWordWrap, shown);
        } else if (t == "checkbox") {
            const QRectF box(r.left(), r.top() + (r.height() - S * 0.4) / 2, S * 0.4, S * 0.4);
            p.setBrush(QColor(0x14, 0x14, 0x14));
            p.setPen(QColor(0x80, 0x80, 0x80));
            p.drawRect(box);
            if (e.flag) {
                p.setPen(QPen(QColor("#43a047"), 2.5));
                p.drawLine(box.topLeft() + QPointF(4, box.height() / 2), box.center() + QPointF(-1, box.height() / 4));
                p.drawLine(box.center() + QPointF(-1, box.height() / 4), box.topRight() + QPointF(-3, 4));
            }
            p.setPen(txt);
            p.drawText(QRectF(box.right() + 8, r.top(), r.width() - box.width() - 8, r.height()), Qt::AlignLeft | Qt::AlignVCenter, e.text);
        } else if (t == "dropdown") {
            p.setBrush(QColor(0x14, 0x14, 0x14));
            p.setPen(QColor(0x70, 0x70, 0x70));
            p.drawRect(r);
            p.setPen(txt);
            const QString first = e.extra.split(',', Qt::SkipEmptyParts).value(0).trimmed();
            p.drawText(r.adjusted(6, 0, -S * 0.5, 0), Qt::AlignLeft | Qt::AlignVCenter, first);
            p.drawText(QRectF(r.right() - S * 0.5, r.top(), S * 0.5, r.height()), Qt::AlignCenter, QString(QChar(0x25BE)));
        } else if (t == "image" || t == "item_image") {
            QImage img;
            if (t == "item_image") img = imageForRef(e.extra);
            else if (QFileInfo::exists(e.extra)) img.load(e.extra);
            if (!img.isNull()) {
                p.drawImage(r, img.scaled(r.size().toSize(), Qt::KeepAspectRatio, Qt::FastTransformation));
            } else {
                p.setBrush(QColor(0x44, 0x44, 0x44, 160));
                p.setPen(QPen(QColor(0x90, 0x90, 0x90), 1, Qt::DashLine));
                p.drawRect(r);
                p.drawLine(r.topLeft(), r.bottomRight());
                p.drawLine(r.topRight(), r.bottomLeft());
                p.setPen(txt);
                QFont sf = p.font();
                sf.setPixelSize(int(S * 0.22));
                p.setFont(sf);
                p.drawText(r.adjusted(2, 2, -2, -2), Qt::AlignCenter | Qt::TextWordWrap, e.extra);
            }
        } else if (t == "box") {
            p.setBrush(parseColor(e.extra, QColor(0x4a, 0x90, 0xd9, 0x80)));
            p.setPen(Qt::NoPen);
            p.drawRect(r);
        } else if (t == "list") {
            const int cols = qMax(1, int(e.w)), rows = qMax(1, int(e.h));
            for (int j = 0; j < rows; ++j)
                for (int i = 0; i < cols; ++i) {
                    const QRectF c(r.left() + i * S * 1.25, r.top() + j * S * 1.25, S, S);
                    p.setBrush(QColor(0, 0, 0, 110));
                    p.setPen(QColor(255, 255, 255, 60));
                    p.drawRect(c);
                }
        }
        p.restore();
    }

    FsForm *form_ = nullptr;
    int sel_ = -1;
    bool preview_ = false;
    double snap_ = 0.25;
    Mode mode_ = None;
    QPointF grab_;
};

} // namespace lcfs

// ------------------------------------------------------------------ page complète

class FormspecDesigner : public QWidget {
public:
    FormspecDesigner(QWidget *parent = nullptr) : QWidget(parent)
    {
        using namespace lcfs;
        auto *root = new QHBoxLayout(this);

        // ---------------- colonne de gauche : formulaires + palette
        auto *left = new QVBoxLayout;
        left->addWidget(new QLabel(tr("<b>Formulaires</b>")));
        list_ = new QListWidget;
        list_->setMaximumHeight(130);
        left->addWidget(list_);
        auto *bAdd = new QPushButton(tr("Ajouter"));
        auto *bDup = new QPushButton(tr("Dupliquer"));
        auto *bDel = new QPushButton(tr("Supprimer"));
        auto *btnRow = new QBoxLayout(QBoxLayout::LeftToRight);
        UiHooks::bindRow(btnRow, {{bAdd, "btn.add"}, {bDup, "btn.dup"}, {bDel, "btn.del"}}, "editor.formspec");
        left->addLayout(btnRow);
        left->addSpacing(6);
        left->addWidget(new QLabel(tr("<b>Éléments</b><br><small>Glissez-les sur le formulaire<br>(ou double-clic pour ajouter).</small>")));
        palette_ = new FsPalette;
        left->addWidget(palette_, 1);
        auto *bText = new QPushButton(tr("Ajouter du texte"));
        UiHooks::bind(bText, "btn.addtext", "editor.formspec");
        left->addWidget(bText);
        root->addLayout(left, 0);

        // ---------------- centre : barre + canevas
        auto *center = new QVBoxLayout;
        auto *bar = new QHBoxLayout;
        auto *prev = new QCheckBox(tr("Aperçu (sans grille ni poignées)"));
        auto *snap = new QComboBox;
        snap->addItem(tr("Grille 0,25"), 0.25);
        snap->addItem(tr("Grille 0,5"), 0.5);
        snap->addItem(tr("Grille 1"), 1.0);
        snap->addItem(tr("Libre"), 0.0);
        auto *bUndo = new QPushButton(tr("Annuler"));
        auto *bRedo = new QPushButton(tr("Rétablir"));
        bUndo->setToolTip(tr("Annuler la dernière modification (Ctrl+Z)"));
        bRedo->setToolTip(tr("Rétablir (Ctrl+Maj+Z ou Ctrl+Y)"));
        bar->addWidget(bUndo);
        bar->addWidget(bRedo);
        bar->addWidget(prev);
        bar->addStretch(1);
        bar->addWidget(new QLabel(tr("Magnétisme :")));
        bar->addWidget(snap);
        center->addLayout(bar);
        canvas_ = new FsCanvas;
        auto *scroll = new QScrollArea;
        scroll->setWidget(canvas_);
        scroll->setAlignment(Qt::AlignCenter);
        center->addWidget(scroll, 1);
        auto *hint = new QLabel(tr("Clic : sélectionner — glisser : déplacer — coin bleu : redimensionner — double-clic : modifier le texte — "
                                   "clic droit : menu — Suppr : supprimer — flèches : ajuster — Ctrl+Z : annuler."));
        hint->setWordWrap(true);
        hint->setProperty("muted", true);
        center->addWidget(hint);
        root->addLayout(center, 1);

        // ---------------- colonne de droite : propriétés + code
        auto *tabs = new QTabWidget;
        tabs->setMinimumWidth(330);
        tabs->setMaximumWidth(420);

        auto *propsW = new QWidget;
        auto *pv = new QVBoxLayout(propsW);
        auto *formBox = new QGroupBox(tr("Formulaire"));
        auto *ff = new QFormLayout(formBox);
        fName_ = new QLineEdit;
        fW_ = new QDoubleSpinBox;
        fH_ = new QDoubleSpinBox;
        for (QDoubleSpinBox *s : {fW_, fH_}) {
            s->setRange(2, 40);
            s->setDecimals(2);
            s->setSingleStep(0.5);
        }
        fBg_ = new QLineEdit;
        fBg_->setPlaceholderText(tr("#RRGGBB ou #RRGGBBAA (vide = défaut)"));
        fCmd_ = new QCheckBox(tr("Ouvrir avec la commande /nom dans le jeu"));
        fOn_ = new QPlainTextEdit;
        fOn_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        fOn_->setPlaceholderText(tr("-- ex : if fields.bouton1 then core.chat_send_player(player:get_player_name(), \"Clic !\") end"));
        fOn_->setMaximumHeight(90);
        ff->addRow(tr("Nom"), fName_);
        ff->addRow(tr("Largeur"), fW_);
        ff->addRow(tr("Hauteur"), fH_);
        ff->addRow(tr("Fond"), fBg_);
        ff->addRow(fCmd_);
        ff->addRow(tr("Lua à la réception des champs (player, fields)"), fOn_);
        pv->addWidget(formBox);

        elemBox_ = new QGroupBox(tr("Élément sélectionné"));
        eForm_ = new QFormLayout(elemBox_);
        eType_ = new QLabel;
        eX_ = new QDoubleSpinBox;
        eY_ = new QDoubleSpinBox;
        eW_ = new QDoubleSpinBox;
        eH_ = new QDoubleSpinBox;
        for (QDoubleSpinBox *s : {eX_, eY_, eW_, eH_}) {
            s->setRange(0, 100);
            s->setDecimals(2);
            s->setSingleStep(0.25);
        }
        eName_ = new QLineEdit;
        eText_ = new QLineEdit;
        eExtra_ = new QLineEdit;
        eFlag_ = new QCheckBox(tr("Cochée par défaut"));
        eForm_->addRow(tr("Type"), eType_);
        eForm_->addRow(tr("X"), eX_);
        eForm_->addRow(tr("Y"), eY_);
        eForm_->addRow(tr("Largeur"), eW_);
        eForm_->addRow(tr("Hauteur"), eH_);
        eForm_->addRow(tr("Nom du champ"), eName_);
        eForm_->addRow(tr("Texte"), eText_);
        eForm_->addRow(tr("Valeur"), eExtra_);
        eForm_->addRow(eFlag_);
        pv->addWidget(elemBox_);
        pv->addStretch(1);
        tabs->addTab(propsW, tr("Propriétés"));

        auto *codeW = new QWidget;
        auto *cv = new QVBoxLayout(codeW);
        cv->addWidget(new QLabel(tr("<b>Formspec</b> (à coller dans core.show_formspec)")));
        specOut_ = new QPlainTextEdit;
        specOut_->setReadOnly(true);
        specOut_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        specOut_->setLineWrapMode(QPlainTextEdit::NoWrap);
        lcAttachHighlighter(specOut_, LcLang::Formspec);
        cv->addWidget(specOut_, 1);
        auto *bCopy = new QPushButton(tr("Copier le formspec"));
        UiHooks::bind(bCopy, "btn.copy", "editor.formspec");
        cv->addWidget(bCopy);
        cv->addWidget(new QLabel(tr("<b>Code Lua généré</b> (ajouté automatiquement au mod)")));
        luaOut_ = new QPlainTextEdit;
        luaOut_->setReadOnly(true);
        luaOut_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        luaOut_->setLineWrapMode(QPlainTextEdit::NoWrap);
        lcAttachHighlighter(luaOut_, LcLang::Lua);
        cv->addWidget(luaOut_, 2);
        tabs->addTab(codeW, tr("Code"));
        root->addWidget(tabs, 0);

        // ---------------- connexions
        palette_->onActivate = [this](const QString &type) {
            if (!cur()) return;
            const double off = 0.5 * (cur()->elems.size() % 8);
            canvas_->addElem(type, 0.5 + off, 0.5 + off);
        };
        connect(bText, &QPushButton::clicked, this, [this] {
            if (!cur()) return;
            const double off = 0.5 * (cur()->elems.size() % 8);
            canvas_->addElem("label", 0.5 + off, 0.5 + off);
        });
        connect(bUndo, &QPushButton::clicked, this, [this] { undo(); });
        connect(bRedo, &QPushButton::clicked, this, [this] { redo(); });
        // Ctrl+Z / Ctrl+Y dans toute la page (les champs de texte gardent leur propre annulation)
        auto *scUndo = new QShortcut(QKeySequence(QKeySequence::Undo), this);
        scUndo->setContext(Qt::WidgetWithChildrenShortcut);
        connect(scUndo, &QShortcut::activated, this, [this] { undo(); });
        auto *scRedo = new QShortcut(QKeySequence(QKeySequence::Redo), this);
        scRedo->setContext(Qt::WidgetWithChildrenShortcut);
        connect(scRedo, &QShortcut::activated, this, [this] { redo(); });
        auto *scRedo2 = new QShortcut(QKeySequence("Ctrl+Y"), this);
        scRedo2->setContext(Qt::WidgetWithChildrenShortcut);
        connect(scRedo2, &QShortcut::activated, this, [this] { redo(); });
        connect(prev, &QCheckBox::toggled, this, [this](bool on) { canvas_->setPreview(on); });
        connect(snap, &QComboBox::currentIndexChanged, this, [this, snap](int) { canvas_->setSnap(snap->currentData().toDouble()); });
        canvas_->onChanged = [this] {
            loadElemProps();
            updateCode();
        };
        canvas_->onSelect = [this](int) { loadElemProps(); };

        connect(list_, &QListWidget::currentRowChanged, this, [this](int row) { setCurrent(row); });
        connect(bAdd, &QPushButton::clicked, this, [this] { addForm(); });
        connect(bDup, &QPushButton::clicked, this, [this] { dupForm(); });
        connect(bDel, &QPushButton::clicked, this, [this] { delForm(); });
        connect(bCopy, &QPushButton::clicked, this, [this] {
            if (cur()) QApplication::clipboard()->setText(lcfs::formLines(*cur()).join(""));
        });

        // propriétés du formulaire
        connect(fName_, &QLineEdit::textEdited, this, [this](const QString &t) {
            if (building_ || !cur()) return;
            cur()->name = t;
            if (auto *it = list_->item(cur_)) it->setText(t.isEmpty() ? tr("(sans nom)") : t);
            updateCode();
        });
        auto sizeChanged = [this] {
            if (building_ || !cur()) return;
            cur()->w = fW_->value();
            cur()->h = fH_->value();
            canvas_->fit();
            canvas_->refresh();
            updateCode();
        };
        connect(fW_, &QDoubleSpinBox::valueChanged, this, sizeChanged);
        connect(fH_, &QDoubleSpinBox::valueChanged, this, sizeChanged);
        connect(fBg_, &QLineEdit::textEdited, this, [this](const QString &t) {
            if (building_ || !cur()) return;
            cur()->bg = t;
            canvas_->refresh();
            updateCode();
        });
        connect(fCmd_, &QCheckBox::toggled, this, [this](bool on) {
            if (building_ || !cur()) return;
            cur()->command = on;
            updateCode();
        });
        connect(fOn_, &QPlainTextEdit::textChanged, this, [this] {
            if (building_ || !cur()) return;
            cur()->onFields = fOn_->toPlainText();
            updateCode();
        });

        // propriétés de l'élément
        auto elemEdit = [this](std::function<void(lcfs::FsElem &)> fn) {
            return [this, fn] {
                if (building_ || !cur()) return;
                const int s = canvas_->selected();
                if (s < 0 || s >= cur()->elems.size()) return;
                fn(cur()->elems[s]);
                canvas_->refresh();
                updateCode();
            };
        };
        connect(eX_, &QDoubleSpinBox::valueChanged, this, elemEdit([this](lcfs::FsElem &e) { e.x = eX_->value(); }));
        connect(eY_, &QDoubleSpinBox::valueChanged, this, elemEdit([this](lcfs::FsElem &e) { e.y = eY_->value(); }));
        connect(eW_, &QDoubleSpinBox::valueChanged, this, elemEdit([this](lcfs::FsElem &e) { e.w = eW_->value(); }));
        connect(eH_, &QDoubleSpinBox::valueChanged, this, elemEdit([this](lcfs::FsElem &e) { e.h = eH_->value(); }));
        connect(eName_, &QLineEdit::textEdited, this, elemEdit([this](lcfs::FsElem &e) { e.name = eName_->text(); }));
        connect(eText_, &QLineEdit::textEdited, this, elemEdit([this](lcfs::FsElem &e) { e.text = eText_->text(); }));
        connect(eExtra_, &QLineEdit::textEdited, this, elemEdit([this](lcfs::FsElem &e) { e.extra = eExtra_->text(); }));
        connect(eFlag_, &QCheckBox::toggled, this, elemEdit([this](lcfs::FsElem &e) { e.flag = eFlag_->isChecked(); }));

        fromJson(QJsonArray());
    }

    // ------------------------------------------------------------ projet
    QJsonArray toJson() const
    {
        QJsonArray a;
        for (const lcfs::FsForm &f : forms_) a.append(f.toJson());
        return a;
    }

    void fromJson(const QJsonArray &a)
    {
        forms_.clear();
        for (const QJsonValue &v : a) forms_.append(lcfs::FsForm::fromJson(v.toObject()));
        if (forms_.isEmpty()) {
            lcfs::FsForm f;
            f.name = "formulaire1";
            forms_.append(f);
        }
        restoring_ = true; // chargement d'un projet : pas une modification à annuler
        rebuildList(0);
        restoring_ = false;
        undo_.clear();
        redo_.clear();
        lastState_ = toJson();
        sinceChange_.invalidate();
    }

    // Lua de tous les formulaires (ajouté à init.lua)
    QString toLua(const QString &modid) const
    {
        QString out;
        bool any = false;
        QSet<QString> seen;
        for (const lcfs::FsForm &f : forms_) {
            if (f.elems.isEmpty() && f.onFields.trimmed().isEmpty()) continue; // formulaire vide : rien à générer
            if (seen.contains(lcfs::formIdent(f))) continue;                   // nom en double : ignoré
            seen.insert(lcfs::formIdent(f));
            if (!any) out += "-- Formulaires (éditeur de formspec)\n";
            any = true;
            out += lcfs::formLua(f, modid);
        }
        return out;
    }

private:
    lcfs::FsForm *cur() { return (cur_ >= 0 && cur_ < forms_.size()) ? &forms_[cur_] : nullptr; }

    void rebuildList(int select)
    {
        building_ = true;
        list_->clear();
        for (const lcfs::FsForm &f : forms_) list_->addItem(f.name.isEmpty() ? tr("(sans nom)") : f.name);
        building_ = false;
        cur_ = -1;
        list_->setCurrentRow(qBound(0, select, int(forms_.size()) - 1));
        setCurrent(list_->currentRow());
    }

    void setCurrent(int row)
    {
        if (building_) return;
        cur_ = (row >= 0 && row < forms_.size()) ? row : -1;
        canvas_->setForm(cur());
        loadFormProps();
        loadElemProps();
        updateCode();
    }

    QString freshName() const
    {
        QSet<QString> used;
        for (const lcfs::FsForm &f : forms_) used.insert(lcfs::formIdent(f));
        for (int i = 1; i < 1000; ++i) {
            const QString n = "formulaire" + QString::number(i);
            if (!used.contains(n)) return n;
        }
        return "formulaire";
    }

    void addForm()
    {
        lcfs::FsForm f;
        f.name = freshName();
        forms_.append(f);
        rebuildList(int(forms_.size()) - 1);
    }

    void dupForm()
    {
        if (!cur()) return;
        lcfs::FsForm f = *cur();
        f.name = freshName();
        forms_.append(f);
        rebuildList(int(forms_.size()) - 1);
    }

    void delForm()
    {
        if (!cur()) return;
        if (QMessageBox::question(this, tr("Supprimer"), tr("Supprimer le formulaire « %1 » ?").arg(cur()->name)) != QMessageBox::Yes) return;
        forms_.removeAt(cur_);
        if (forms_.isEmpty()) {
            lcfs::FsForm f;
            f.name = "formulaire1";
            forms_.append(f);
        }
        rebuildList(qMin(cur_, int(forms_.size()) - 1));
    }

    void loadFormProps()
    {
        building_ = true;
        const lcfs::FsForm *f = cur();
        fName_->setText(f ? f->name : QString());
        fW_->setValue(f ? f->w : 10);
        fH_->setValue(f ? f->h : 7);
        fBg_->setText(f ? f->bg : QString());
        fCmd_->setChecked(f ? f->command : true);
        fOn_->setPlainText(f ? f->onFields : QString());
        building_ = false;
    }

    static void showRow(QFormLayout *fl, QWidget *w, bool on)
    {
        if (QWidget *lab = fl->labelForField(w)) lab->setVisible(on);
        w->setVisible(on);
    }

    void loadElemProps()
    {
        using namespace lcfs;
        building_ = true;
        const int s = canvas_->selected();
        const FsElem *e = (cur() && s >= 0 && s < cur()->elems.size()) ? &cur()->elems[s] : nullptr;
        elemBox_->setEnabled(e != nullptr);
        if (!e) {
            eType_->setText(tr("(aucun — cliquez sur un élément)"));
            building_ = false;
            return;
        }
        const TypeInfo *ti = typeInfo(e->type);
        eType_->setText(ti ? ti->emoji + " " + ti->title : e->type);
        eX_->setValue(e->x);
        eY_->setValue(e->y);
        eW_->setValue(e->w);
        eH_->setValue(e->h);
        eName_->setText(e->name);
        eText_->setText(e->text);
        eExtra_->setText(e->extra);
        eFlag_->setChecked(e->flag);
        showRow(eForm_, eW_, hasSize(e->type));
        showRow(eForm_, eH_, hasSize(e->type));
        showRow(eForm_, eName_, hasName(e->type));
        showRow(eForm_, eText_, hasText(e->type));
        showRow(eForm_, eExtra_, hasExtra(e->type));
        eFlag_->setVisible(e->type == "checkbox");
        const QString t = e->type;
        if (QLabel *l = qobject_cast<QLabel *>(eForm_->labelForField(eName_)))
            l->setText(t == "list" ? tr("Nom de la liste") : tr("Nom du champ"));
        if (QLabel *l = qobject_cast<QLabel *>(eForm_->labelForField(eText_)))
            l->setText(t == "label" ? tr("Texte") : (t == "button" || t == "button_exit" || t == "checkbox") ? tr("Texte affiché") : tr("Étiquette"));
        if (QLabel *l = qobject_cast<QLabel *>(eForm_->labelForField(eExtra_))) {
            QString lab = tr("Valeur par défaut");
            if (t == "dropdown") lab = tr("Choix (séparés par des virgules)");
            else if (t == "image") lab = tr("Texture (fichier .png du mod)");
            else if (t == "item_image") lab = tr("Objet (mod:nom)");
            else if (t == "box") lab = tr("Couleur (#RRGGBBAA)");
            else if (t == "list") lab = tr("Emplacement (current_player, context...)");
            l->setText(lab);
        }
        if (t == "list") { // w et h = nombre d'emplacements
            eW_->setDecimals(0);
            eH_->setDecimals(0);
            eW_->setSingleStep(1);
            eH_->setSingleStep(1);
        } else {
            eW_->setDecimals(2);
            eH_->setDecimals(2);
            eW_->setSingleStep(0.25);
            eH_->setSingleStep(0.25);
        }
        eW_->setValue(e->w);
        eH_->setValue(e->h);
        building_ = false;
    }

    // Historique d'annulation : chaque modification passe par updateCode(). Les changements rapprochés
    // (déplacement à la souris, saisie) sont regroupés en une seule étape.
    void trackChange()
    {
        if (restoring_) return;
        const QJsonArray now = toJson();
        if (now == lastState_) return;
        if (!sinceChange_.isValid() || sinceChange_.elapsed() > 700) {
            undo_.append(lastState_);
            if (undo_.size() > 100) undo_.removeFirst();
        }
        redo_.clear();
        lastState_ = now;
        sinceChange_.restart();
    }

    void applyState(const QJsonArray &st)
    {
        const int keep = cur_;
        restoring_ = true;
        forms_.clear();
        for (const QJsonValue &v : st) forms_.append(lcfs::FsForm::fromJson(v.toObject()));
        if (forms_.isEmpty()) {
            lcfs::FsForm f;
            f.name = "formulaire1";
            forms_.append(f);
        }
        rebuildList(keep);
        restoring_ = false;
        lastState_ = st;
        sinceChange_.invalidate();
    }

    void undo()
    {
        if (undo_.isEmpty()) return;
        redo_.append(lastState_);
        const QJsonArray st = undo_.takeLast();
        applyState(st);
    }

    void redo()
    {
        if (redo_.isEmpty()) return;
        undo_.append(lastState_);
        const QJsonArray st = redo_.takeLast();
        applyState(st);
    }

    void updateCode()
    {
        trackChange();
        const lcfs::FsForm *f = cur();
        if (!f) {
            specOut_->clear();
            luaOut_->clear();
            return;
        }
        specOut_->setPlainText(lcfs::formLines(*f).join("\n"));
        luaOut_->setPlainText(lcfs::formLua(*f, "mon_mod"));
    }

    QVector<lcfs::FsForm> forms_;
    QVector<QJsonArray> undo_, redo_; // historique d'annulation (états complets des formulaires)
    QJsonArray lastState_;
    QElapsedTimer sinceChange_;
    bool restoring_ = false;
    int cur_ = -1;
    bool building_ = false;
    QListWidget *list_ = nullptr;
    lcfs::FsPalette *palette_ = nullptr;
    lcfs::FsCanvas *canvas_ = nullptr;
    QLineEdit *fName_ = nullptr, *fBg_ = nullptr;
    QDoubleSpinBox *fW_ = nullptr, *fH_ = nullptr;
    QCheckBox *fCmd_ = nullptr;
    QPlainTextEdit *fOn_ = nullptr, *specOut_ = nullptr, *luaOut_ = nullptr;
    QGroupBox *elemBox_ = nullptr;
    QFormLayout *eForm_ = nullptr;
    QLabel *eType_ = nullptr;
    QDoubleSpinBox *eX_ = nullptr, *eY_ = nullptr, *eW_ = nullptr, *eH_ = nullptr;
    QLineEdit *eName_ = nullptr, *eText_ = nullptr, *eExtra_ = nullptr;
    QCheckBox *eFlag_ = nullptr;
};
