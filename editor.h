#pragma once
// Éditeur générique « liste à gauche / formulaire au centre / aperçu à droite ».

#include <QtWidgets>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>

#include "uihooks.h"

enum FieldType { TText, TInt, TDouble, TCombo, TFile, TCheck, TMulti, TRef, TRefList };

struct Field {
    QString key, label;
    FieldType type;
    QStringList opts;                  // TCombo
    QString def;                       // valeur par défaut
    int mx = 9999;                     // TInt / TDouble : maximum
    int mn = 0;                        // TInt / TDouble : minimum
    QString filter = "Images (*.png)"; // TFile
    QString hint;                      // info-bulle
};

// Fournisseurs globaux pour les champs de type TRef (référence à un bloc/objet).
struct EditorHooks {
    static inline std::function<QStringList()> refList;
    static inline std::function<QIcon(const QString &)> refIcon;
    // ouvre le sélecteur de blocs ; renvoie la référence choisie ou une chaîne vide si annulé
    static inline std::function<QString(QWidget *, const QString &)> pickRef;
    // modèles prédéfinis des plugins pour une page (clé de page -> liste)
    static inline std::function<QVector<QPair<QString, QVariantMap>>(const QString &)> presets;
};

class EditorTab : public QWidget {
public:
    std::function<void()> onSelected;                      // sélection changée
    std::function<void(const QVariantMap &)> onChanged;    // une valeur a changé (live)
    std::function<QIcon(const QVariantMap &)> iconFn;      // icône dans la liste

    EditorTab(const QString &prefix, const QVector<Field> &f, QWidget *parent = nullptr)
        : QWidget(parent), prefix_(prefix), fields_(f)
    {
        root_ = new QHBoxLayout(this);
        auto *left = new QVBoxLayout;
        list_ = new QListWidget;
        list_->setIconSize(QSize(28, 28));
        auto *add = new QPushButton(tr("Ajouter"));
        auto *dup = new QPushButton(tr("Dupliquer"));
        auto *del = new QPushButton(tr("Supprimer"));
        presetBtn_ = new QToolButton;
        presetBtn_->setText(tr("Modèles"));
        UiHooks::bind(presetBtn_, "btn.presets", "editor");
        presetBtn_->setPopupMode(QToolButton::InstantPopup);
        presetBtn_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        presetBtn_->setToolTip(tr("Modèles prédéfinis fournis par les plugins"));
        presetBtn_->setVisible(false);
        left->addWidget(list_);
        auto *btnRow = new QBoxLayout(QBoxLayout::TopToBottom);
        UiHooks::bindRow(btnRow, {{add, "btn.add"}, {dup, "btn.dup"}, {del, "btn.del"}}, "editor");
        scoped_ << add << dup << del;
        btnRow->insertWidget(1, presetBtn_); // « Modèles » (visible seulement si des plugins en fournissent)
        left->addLayout(btnRow);

        form_ = new QWidget;
        auto *form = new QFormLayout(form_);
        for (const Field &fd : fields_) {
            QWidget *w = nullptr;
            QWidget *shown = nullptr;
            switch (fd.type) {
            case TText: w = shown = new QLineEdit; break;
            case TInt: {
                auto *s = new QSpinBox;
                s->setRange(fd.mn, fd.mx);
                w = shown = s;
                break;
            }
            case TDouble: {
                auto *s = new QDoubleSpinBox;
                s->setDecimals(3);
                s->setSingleStep(0.1);
                s->setRange(fd.mn, fd.mx);
                w = shown = s;
                break;
            }
            case TCombo: {
                auto *c = new QComboBox;
                c->addItems(fd.opts);
                w = shown = c;
                break;
            }
            case TRef: {
                auto *c = new QComboBox;
                c->setEditable(true);
                c->setInsertPolicy(QComboBox::NoInsert);
                c->setIconSize(QSize(20, 20));
                c->setMaxVisibleItems(18);
                auto *box = new QWidget;
                auto *hl = new QHBoxLayout(box);
                hl->setContentsMargins(0, 0, 0, 0);
                auto *pick = new QPushButton("…");
                pick->setToolTip(tr("Choisir dans la liste des blocs (avec aperçu des textures)"));
                pick->setFixedWidth(34);
                UiHooks::bind(pick, "btn.pick", "editor");
                scoped_ << pick;
                hl->addWidget(c, 1);
                hl->addWidget(pick);
                connect(pick, &QPushButton::clicked, this, [this, c] {
                    if (!EditorHooks::pickRef) return;
                    const QString r = EditorHooks::pickRef(this, c->currentText());
                    if (!r.isEmpty()) c->setCurrentText(r);
                });
                w = c;
                shown = box;
                break;
            }
            case TRefList: {
                auto *le = new QLineEdit;
                auto *box = new QWidget;
                auto *hl = new QHBoxLayout(box);
                hl->setContentsMargins(0, 0, 0, 0);
                auto *pick = new QPushButton("+");
                pick->setToolTip(tr("Ajouter un bloc à la liste (aperçu des textures)"));
                pick->setFixedWidth(34);
                UiHooks::bind(pick, "btn.pick", "editor");
                scoped_ << pick;
                hl->addWidget(le, 1);
                hl->addWidget(pick);
                connect(pick, &QPushButton::clicked, this, [this, le] {
                    if (!EditorHooks::pickRef) return;
                    const QString r = EditorHooks::pickRef(this, QString());
                    if (r.isEmpty()) return;
                    const QString cur = le->text().trimmed();
                    le->setText(cur.isEmpty() ? r : cur + ", " + r);
                });
                w = le;
                shown = box;
                break;
            }
            case TCheck: w = shown = new QCheckBox; break;
            case TMulti: {
                auto *pt = new QPlainTextEdit;
                pt->setFixedHeight(110);
                w = shown = pt;
                break;
            }
            case TFile: {
                auto *le = new QLineEdit;
                auto *thumb = new QLabel;
                thumb->setFixedSize(30, 30);
                auto *b = new QPushButton("...");
                UiHooks::bind(b, "btn.pick", "editor");
                auto *box = new QWidget;
                auto *hl = new QHBoxLayout(box);
                hl->setContentsMargins(0, 0, 0, 0);
                hl->addWidget(thumb);
                hl->addWidget(le);
                hl->addWidget(b);
                const QString flt = fd.filter;
                connect(b, &QPushButton::clicked, this, [this, le, flt] {
                    const QString p = QFileDialog::getOpenFileName(this, tr("Choisir un fichier"),
                                                                   QString(), flt);
                    if (!p.isEmpty()) le->setText(p);
                });
                connect(le, &QLineEdit::textChanged, this, [thumb](const QString &p) {
                    QPixmap pm;
                    if (p.endsWith(".png", Qt::CaseInsensitive)) pm.load(p);
                    if (pm.isNull()) thumb->clear();
                    else thumb->setPixmap(pm.scaled(30, 30, Qt::KeepAspectRatio, Qt::FastTransformation));
                });
                w = le;
                shown = box;
                break;
            }
            }
            if (!fd.hint.isEmpty()) shown->setToolTip(fd.hint);
            widgets_[fd.key] = w;
            hookLive(w, fd.type);
            form->addRow(fd.label, shown);
        }
        auto *scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(form_);

        root_->addLayout(left, 1);
        root_->addWidget(scroll, 3);

        connect(add, &QPushButton::clicked, this, [this] { addEntry(); });
        connect(dup, &QPushButton::clicked, this, [this] { duplicateEntry(); });
        connect(del, &QPushButton::clicked, this, [this] { removeEntry(); });
        add->setToolTip(tr("Ajouter un élément (Ctrl+N)"));
        dup->setToolTip(tr("Dupliquer l'élément sélectionné (Ctrl+D)"));
        del->setToolTip(tr("Supprimer l'élément sélectionné (Suppr) — Ctrl+Z annule la suppression"));
        // raccourcis : Ctrl+N / Ctrl+D dans toute la page, Suppr et Ctrl+Z quand la liste a le focus
        // (ainsi les champs de texte gardent leur propre Suppr / Ctrl+Z)
        auto addSc = [this](const QKeySequence &k, QWidget *ctx, Qt::ShortcutContext c, std::function<void()> fn) {
            auto *s = new QShortcut(k, ctx);
            s->setContext(c);
            connect(s, &QShortcut::activated, this, [fn] { fn(); });
        };
        addSc(QKeySequence("Ctrl+N"), this, Qt::WidgetWithChildrenShortcut, [this] { addEntry(); });
        addSc(QKeySequence("Ctrl+D"), this, Qt::WidgetWithChildrenShortcut, [this] { duplicateEntry(); });
        addSc(QKeySequence(Qt::Key_Delete), list_, Qt::WidgetShortcut, [this] { removeEntry(); });
        addSc(QKeySequence(QKeySequence::Undo), list_, Qt::WidgetShortcut, [this] { undoRemove(); });
        connect(list_, &QListWidget::currentRowChanged, this, [this](int r) {
            commit(cur_);
            cur_ = r;
            load(r);
        });
        load(-1);
    }

    // clé de page (« biomes », « plugin.x.y »...) : sert à trouver les modèles prédéfinis des plugins
    void setTabKey(const QString &k)
    {
        tabKey_ = k;
        for (const QPointer<QAbstractButton> &b : scoped_) // icônes propres à cette page : « editor.<page>.btn.add »
            if (b) UiHooks::setScope(b.data(), "editor." + k);
        refreshUi();
    }

    // reconstruit le menu des modèles prédéfinis (après un changement de plugins)
    void refreshUi()
    {
        auto *menu = presetBtn_->menu();
        if (menu) {
            presetBtn_->setMenu(nullptr);
            delete menu;
        }
        const QVector<QPair<QString, QVariantMap>> pr = (EditorHooks::presets && !tabKey_.isEmpty())
                                                            ? EditorHooks::presets(tabKey_)
                                                            : QVector<QPair<QString, QVariantMap>>();
        presetBtn_->setVisible(!pr.isEmpty());
        if (pr.isEmpty()) return;
        menu = new QMenu(presetBtn_);
        for (const auto &p : pr) {
            const QVariantMap vals = p.second;
            menu->addAction(p.first, this, [this, vals, nm = p.first] { addEntryWith(vals, nm); });
        }
        presetBtn_->setMenu(menu);
    }

    // ajoute une entrée en appliquant des valeurs (modèle prédéfini)
    void addEntryWith(const QVariantMap &vals, const QString &baseName)
    {
        addEntry();
        for (auto it = vals.begin(); it != vals.end(); ++it) {
            bool known = false;
            for (const Field &fd : fields_) known = known || fd.key == it.key();
            if (known) setValue(it.key(), it.value());
        }
        if (!baseName.isEmpty()) setValue("name", uniqueName(identOfName(baseName)));
    }

    // panneau d'aperçu à droite du formulaire
    void setSide(QWidget *w) { root_->addWidget(w, 2); }

    int count() const { return items_.size(); }
    const QList<QVariantMap> &rawItems() const { return items_; } // sans commit (pas de récursion)

    void commit(int i)
    {
        if (i < 0 || i >= items_.size()) return;
        QVariantMap &m = items_[i];
        for (const Field &fd : fields_) m[fd.key] = widgetValue(fd, widgets_[fd.key]);
        if (auto *it = list_->item(i)) {
            it->setText(m.value("name").toString());
            if (iconFn) it->setIcon(iconFn(m));
        }
    }

    QList<QVariantMap> data()
    {
        commit(cur_);
        return items_;
    }

    QVariantMap currentMap()
    {
        commit(cur_);
        if (cur_ >= 0 && cur_ < items_.size()) return items_[cur_];
        return QVariantMap();
    }

    void setValue(const QString &key, const QVariant &v)
    {
        for (const Field &fd : fields_)
            if (fd.key == key) setWidgetValue(fd, widgets_[key], v);
        commit(cur_);
    }

    QJsonArray toJson()
    {
        commit(cur_);
        QJsonArray a;
        for (const auto &m : items_) a.append(QJsonObject::fromVariantMap(m));
        return a;
    }

    void fromJson(const QJsonArray &a)
    {
        cur_ = -1;
        items_.clear();
        list_->clear();
        for (const auto &v : a) {
            QVariantMap m = v.toObject().toVariantMap();
            items_.append(m);
            makeItem(m);
        }
        if (!items_.isEmpty()) list_->setCurrentRow(0);
    }

    // met à jour le contenu des listes déroulantes de références (TRef)
    void refreshRefs()
    {
        if (!EditorHooks::refList) return;
        const QStringList refs = EditorHooks::refList();
        loading_ = true;
        for (const Field &fd : fields_) {
            if (fd.type != TRef) continue;
            auto *c = static_cast<QComboBox *>(widgets_[fd.key]);
            const QString cur = c->currentText();
            c->clear();
            for (const QString &r : refs)
                c->addItem(EditorHooks::refIcon ? EditorHooks::refIcon(r) : QIcon(), r);
            c->setCurrentText(cur);
        }
        loading_ = false;
    }

    void refreshIcons()
    {
        if (!iconFn) return;
        for (int i = 0; i < items_.size(); ++i)
            if (auto *it = list_->item(i)) it->setIcon(iconFn(items_[i]));
    }

private:
    void hookLive(QWidget *w, FieldType t)
    {
        auto go = [this] { live(); };
        switch (t) {
        case TText: case TFile: case TRefList: connect(static_cast<QLineEdit *>(w), &QLineEdit::textChanged, this, go); break;
        case TInt: connect(static_cast<QSpinBox *>(w), QOverload<int>::of(&QSpinBox::valueChanged), this, go); break;
        case TDouble: connect(static_cast<QDoubleSpinBox *>(w), QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, go); break;
        case TCombo: connect(static_cast<QComboBox *>(w), &QComboBox::currentTextChanged, this, go); break;
        case TRef: connect(static_cast<QComboBox *>(w), &QComboBox::editTextChanged, this, go); break;
        case TCheck: connect(static_cast<QCheckBox *>(w), &QCheckBox::toggled, this, go); break;
        case TMulti: connect(static_cast<QPlainTextEdit *>(w), &QPlainTextEdit::textChanged, this, go); break;
        }
    }

    void live()
    {
        if (loading_) return;
        commit(cur_);
        if (cur_ >= 0 && cur_ < items_.size() && onChanged) onChanged(items_[cur_]);
    }

    void makeItem(const QVariantMap &m)
    {
        auto *it = new QListWidgetItem(m.value("name").toString());
        if (iconFn) it->setIcon(iconFn(m));
        list_->addItem(it);
    }

    static QVariant widgetValue(const Field &fd, QWidget *w)
    {
        switch (fd.type) {
        case TText: case TFile: case TRefList: return static_cast<QLineEdit *>(w)->text();
        case TInt: return static_cast<QSpinBox *>(w)->value();
        case TDouble: return static_cast<QDoubleSpinBox *>(w)->value();
        case TCombo: case TRef: return static_cast<QComboBox *>(w)->currentText();
        case TCheck: return static_cast<QCheckBox *>(w)->isChecked();
        case TMulti: return static_cast<QPlainTextEdit *>(w)->toPlainText();
        }
        return QVariant();
    }

    // valeur par défaut d'un champ (pour les projets créés avant l'ajout du champ)
    static QVariant defaultValue(const Field &fd)
    {
        switch (fd.type) {
        case TInt: return fd.def.toInt();
        case TDouble: return fd.def.toDouble();
        case TCheck: return fd.def == "1";
        case TCombo: return (fd.def.isEmpty() && !fd.opts.isEmpty()) ? fd.opts.first() : fd.def;
        default: return fd.def;
        }
    }

    static void setWidgetValue(const Field &fd, QWidget *w, const QVariant &v)
    {
        switch (fd.type) {
        case TText: case TFile: case TRefList: static_cast<QLineEdit *>(w)->setText(v.toString()); break;
        case TInt: static_cast<QSpinBox *>(w)->setValue(v.toInt()); break;
        case TDouble: static_cast<QDoubleSpinBox *>(w)->setValue(v.toDouble()); break;
        case TCombo: {
            auto *c = static_cast<QComboBox *>(w);
            const int idx = c->findText(v.toString());
            c->setCurrentIndex(idx < 0 ? 0 : idx);
            break;
        }
        case TRef: static_cast<QComboBox *>(w)->setCurrentText(v.toString()); break;
        case TCheck: static_cast<QCheckBox *>(w)->setChecked(v.toBool()); break;
        case TMulti: static_cast<QPlainTextEdit *>(w)->setPlainText(v.toString()); break;
        }
    }

    QString uniqueName(const QString &base) const
    {
        QSet<QString> used;
        for (const auto &m : items_) used.insert(m.value("name").toString());
        QString n = base;
        int k = 2;
        while (used.contains(n)) n = base + "_" + QString::number(k++);
        return n;
    }

    void addEntry()
    {
        commit(cur_);
        QVariantMap m;
        for (const Field &fd : fields_) {
            switch (fd.type) {
            case TInt: m[fd.key] = fd.def.toInt(); break;
            case TDouble: m[fd.key] = fd.def.toDouble(); break;
            case TCheck: m[fd.key] = (fd.def == "1"); break;
            case TCombo: m[fd.key] = (fd.def.isEmpty() && !fd.opts.isEmpty()) ? fd.opts.first() : fd.def; break;
            default: m[fd.key] = fd.def;
            }
        }
        m["name"] = uniqueName(prefix_ + QString::number(items_.size() + 1));
        items_.append(m);
        makeItem(m);
        list_->setCurrentRow(items_.size() - 1);
    }

    void duplicateEntry()
    {
        commit(cur_);
        if (cur_ < 0 || cur_ >= items_.size()) return;
        QVariantMap m = items_[cur_];
        m["name"] = uniqueName(m["name"].toString() + "_copie");
        items_.append(m);
        makeItem(m);
        list_->setCurrentRow(items_.size() - 1);
    }

    void removeEntry()
    {
        const int r = list_->currentRow();
        if (r < 0) return;
        commit(cur_); // garde les dernières saisies : elles seront restaurées si on annule
        removedItem_ = items_[r];
        removedRow_ = r;
        cur_ = -1;
        items_.removeAt(r);
        delete list_->takeItem(r);
        notify(tr("« %1 » supprimé — Ctrl+Z (liste) pour annuler.").arg(removedItem_.value("name").toString()));
    }

    // annule la dernière suppression (l'élément revient à sa place)
    void undoRemove()
    {
        if (removedRow_ < 0) return;
        commit(cur_);
        const int row = qMin(removedRow_, int(items_.size()));
        cur_ = -1;
        items_.insert(row, removedItem_);
        auto *it = new QListWidgetItem(removedItem_.value("name").toString());
        if (iconFn) it->setIcon(iconFn(removedItem_));
        list_->insertItem(row, it);
        removedRow_ = -1;
        list_->setCurrentRow(row);
        notify(tr("Suppression annulée."));
    }

    void notify(const QString &msg)
    {
        if (auto *mw = qobject_cast<QMainWindow *>(window())) mw->statusBar()->showMessage(msg, 6000);
    }

    void load(int i)
    {
        const bool ok = i >= 0 && i < items_.size();
        form_->setEnabled(ok);
        loading_ = true;
        for (const Field &fd : fields_)
            setWidgetValue(fd, widgets_[fd.key],
                           ok ? (items_[i].contains(fd.key) ? items_[i].value(fd.key) : defaultValue(fd)) : QVariant());
        loading_ = false;
        if (onSelected) onSelected();
        if (onChanged) onChanged(ok ? items_[i] : QVariantMap());
    }

    static QString identOfName(const QString &s)
    {
        QString r = s.toLower().trimmed();
        r.replace(QRegularExpression("[^a-z0-9_]"), "_");
        return r;
    }

    QString prefix_, tabKey_;
    QVariantMap removedItem_; // dernier élément supprimé (annulable)
    int removedRow_ = -1;
    QToolButton *presetBtn_ = nullptr;
    QList<QPointer<QAbstractButton>> scoped_; // boutons dont l'icône dépend de la page
    QVector<Field> fields_;
    QMap<QString, QWidget *> widgets_;
    QList<QVariantMap> items_;
    QListWidget *list_;
    QWidget *form_;
    QHBoxLayout *root_;
    int cur_ = -1;
    bool loading_ = false;
};
