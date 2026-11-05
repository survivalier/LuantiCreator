#pragma once
// Points d'accroche de l'interface pour les plugins : icônes de boutons, d'actions et de catégories.
//
// Les widgets s'enregistrent avec une clé (« btn.add », « card.biomes »...). Quand les plugins
// changent, refresh() réapplique les icônes fournies par les plugins (ou retire celles qui ont disparu).

#include <QtWidgets>
#include <functional>

class UiHooksPaletteFilter : public QObject {
public:
    UiHooksPaletteFilter(std::function<void()> callback, QObject *parent) : QObject(parent), callback_(std::move(callback)) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::PaletteChange) callback_();
        return QObject::eventFilter(watched, event);
    }

private:
    std::function<void()> callback_;
};

struct UiHooks {
    // installé par le programme : renvoie l'icône d'une clé (QIcon() si aucun plugin n'en fournit)
    static inline std::function<QIcon(const QString &)> icon;

    // installé par le programme : true si l'icône de cette clé doit s'afficher SANS texte (« icon_only »)
    static inline std::function<bool(const QString &)> iconOnly;

    struct Binding {
        QPointer<QObject> obj;
        QString key;
        QString text, tip; // texte et info-bulle d'origine
        QString scope;     // contexte (« home », « server », « editor.nodes »...) : voir candidates()
    };
    static inline QList<Binding> &bindings()
    {
        static QList<Binding> b;
        return b;
    }

    // Groupe de boutons : en colonne par défaut, sur UNE ligne quand tous ont une icône.
    struct Row {
        QPointer<QBoxLayout> layout;
        QList<QPointer<QAbstractButton>> buttons;
    };
    static inline QList<Row> &rows()
    {
        static QList<Row> r;
        return r;
    }

    // Clés rencontrées à l'exécution (chaque bind() / bindWindow() déclare sa clé et ses clés englobantes) :
    // complète la liste générée dans uikeys.h (voir gen_uikeys.py) pour la validation des plugins.
    static inline QSet<QString> &seenKeys()
    {
        static QSet<QString> s;
        return s;
    }
    static void declare(const QString &scope, const QString &key)
    {
        for (const QString &c : candidates(scope, key)) seenKeys().insert(c);
    }

    static QIcon get(const QString &key) { return icon ? icon(key) : QIcon(); }

    static QIcon tintMonochrome(const QIcon &icon, const QColor &color)
    {
        if (icon.isNull() || !color.isValid()) return icon;
        const QPixmap source = icon.pixmap(QSize(64, 64));
        if (source.isNull()) return icon;
        QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32);

        bool monochrome = true, hasTransparency = false;
        for (int y = 0; y < image.height() && monochrome; ++y)
            for (int x = 0; x < image.width(); ++x) {
                const QRgb px = image.pixel(x, y);
                const int alpha = qAlpha(px);
                if (alpha == 0) {
                    hasTransparency = true;
                    continue;
                }
                const int low = qMin(qRed(px), qMin(qGreen(px), qBlue(px)));
                const int high = qMax(qRed(px), qMax(qGreen(px), qBlue(px)));
                if (high - low > 12) {
                    monochrome = false;
                    break;
                }
            }
        if (!monochrome) return icon;

        int background = 255, maxContrast = 0;
        if (!hasTransparency) {
            int borderCounts[256] = {};
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x) {
                    if (x != 0 && y != 0 && x != image.width() - 1 && y != image.height() - 1) continue;
                    const QRgb px = image.pixel(x, y);
                    if (qAlpha(px) == 0) continue;
                    ++borderCounts[(qRed(px) + qGreen(px) + qBlue(px)) / 3];
                }
            int mostCommon = 0;
            for (int value = 0; value < 256; ++value)
                if (borderCounts[value] > mostCommon) {
                    mostCommon = borderCounts[value];
                    background = value;
                }
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x) {
                    const QRgb px = image.pixel(x, y);
                    if (qAlpha(px) == 0) continue;
                    const int luminance = (qRed(px) + qGreen(px) + qBlue(px)) / 3;
                    maxContrast = qMax(maxContrast, qAbs(luminance - background));
                }
        }

        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x) {
                const QRgb px = image.pixel(x, y);
                int alpha = qAlpha(px);
                if (!hasTransparency && maxContrast > 0) {
                    const int luminance = (qRed(px) + qGreen(px) + qBlue(px)) / 3;
                    alpha = alpha * qAbs(luminance - background) / maxContrast;
                }
                image.setPixel(x, y, qRgba(color.red(), color.green(), color.blue(), alpha));
            }
        return QIcon(QPixmap::fromImage(image));
    }

    static void watchPalette(QWidget *widget)
    {
        if (!widget || widget->property("_lcIconPaletteWatched").toBool()) return;
        widget->setProperty("_lcIconPaletteWatched", true);
        auto *filter = new UiHooksPaletteFilter([] { refresh(); }, widget);
        widget->installEventFilter(filter);
    }

    // Clés essayées dans l'ordre, de la plus précise à la plus générale :
    //   scope « editor.nodes », clé « btn.add » -> editor.nodes.btn.add, editor.btn.add, btn.add
    static QStringList candidates(const QString &scope, const QString &key)
    {
        QStringList r;
        QString s = scope;
        while (!s.isEmpty()) {
            r << s + "." + key;
            const int i = s.lastIndexOf('.');
            s = i < 0 ? QString() : s.left(i);
        }
        r << key;
        return r;
    }
    // première clé qui a une icône (QString() si aucune)
    static QString resolve(const QString &scope, const QString &key)
    {
        for (const QString &k : candidates(scope, key))
            if (!get(k).isNull()) return k;
        return QString();
    }
    static bool isIconOnly(const QString &key) { return iconOnly && iconOnly(key); }

    static void apply(const Binding &b)
    {
        if (!b.obj) return;
        const QString rk = resolve(b.scope, b.key);
        const QIcon ic = rk.isEmpty() ? QIcon() : get(rk);
        const bool only = !ic.isNull() && isIconOnly(rk);
        const QString text = only ? QString() : b.text;
        const QString tip = only ? (b.tip.isEmpty() ? QString(b.text).remove('&') : b.tip) : b.tip;
        if (auto *btn = qobject_cast<QAbstractButton *>(b.obj.data())) {
            watchPalette(btn);
            btn->setIcon(tintMonochrome(ic, btn->palette().color(QPalette::ButtonText)));
            btn->setText(text);
            btn->setToolTip(tip);
        } else if (auto *act = qobject_cast<QAction *>(b.obj.data())) {
            auto *context = qobject_cast<QWidget *>(act->parent());
            if (context) watchPalette(context);
            const QColor textColor = context ? context->palette().color(QPalette::WindowText) : QColor();
            act->setIcon(tintMonochrome(ic, textColor));
            act->setToolTip(tip); // le nom de l'action est conservé (menus) ; seul le texte de la barre d'outils disparaît
            act->setIconText(only ? QString(" ") : b.text);
        } else if (auto *win = qobject_cast<QWidget *>(b.obj.data())) {
            // fenêtre : « window.<nom> », sinon l'icône de l'application (« app »)
            watchPalette(win);
            const QIcon windowIcon = ic.isNull() ? get("app") : ic;
            win->setWindowIcon(tintMonochrome(windowIcon, win->palette().color(QPalette::WindowText)));
        }
    }

    static void bind(QAbstractButton *w, const QString &key, const QString &scope = QString())
    {
        if (!w) return;
        bindings().append({w, key, w->text(), w->toolTip(), scope});
        declare(scope, key);
        apply(bindings().last());
    }
    static void bind(QAction *a, const QString &key, const QString &scope = QString())
    {
        if (!a) return;
        bindings().append({a, key, a->text(), a->toolTip(), scope});
        declare(scope, key);
        apply(bindings().last());
    }
    // Icône d'une fenêtre ou d'une boîte de dialogue : clé « window.<nom> » (sinon « app »).
    static void bindWindow(QWidget *w, const QString &name)
    {
        if (!w) return;
        bindings().append({w, "window." + name, QString(), QString(), QString()});
        seenKeys().insert("window." + name);
        apply(bindings().last());
    }

    // Boutons d'une QDialogButtonBox : « btn.ok », « btn.cancel » et « btn.accept » (bouton d'action personnalisé),
    // dans le contexte « dialog.<nom> ».
    static void bindBox(QDialogButtonBox *bb, const QString &name)
    {
        if (!bb) return;
        for (QAbstractButton *b : bb->buttons()) {
            const auto sb = bb->standardButton(b);
            const QString key = sb == QDialogButtonBox::Ok ? "btn.ok" : sb == QDialogButtonBox::Cancel ? "btn.cancel"
                                : bb->buttonRole(b) == QDialogButtonBox::AcceptRole ? "btn.accept" : "btn.other";
            bind(b, key, "dialog." + name);
        }
    }

    // change le contexte d'un widget déjà lié (ex : un éditeur qui apprend sa page après sa création)
    static void setScope(QObject *o, const QString &scope)
    {
        for (Binding &b : bindings())
            if (b.obj == o) {
                b.scope = scope;
                apply(b);
            }
        for (const Row &r : rows()) applyRow(r);
    }

    // Place les boutons dans `layout` (à vous de l'insérer dans la mise en page) ; chaque bouton a sa clé d'icône.
    static void bindRow(QBoxLayout *layout, const QList<QPair<QAbstractButton *, QString>> &items,
                        const QString &scope = QString())
    {
        if (!layout) return;
        Row r;
        r.layout = layout;
        for (const auto &it : items) {
            bind(it.first, it.second, scope);
            layout->addWidget(it.first);
            r.buttons.append(it.first);
        }
        rows().append(r);
        applyRow(rows().last());
    }

    static void applyRow(const Row &r)
    {
        if (!r.layout) return;
        bool all = !r.buttons.isEmpty();
        for (const auto &b : r.buttons)
            if (b && b->icon().isNull()) all = false;
        r.layout->setDirection(all ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
        if (all)
            for (const auto &b : r.buttons)
                if (b) b->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    static void refresh()
    {
        auto &l = bindings();
        for (int i = l.size() - 1; i >= 0; --i)
            if (!l[i].obj) l.removeAt(i);
        for (const Binding &b : l) apply(b);
        auto &rl = rows();
        for (int i = rl.size() - 1; i >= 0; --i)
            if (!rl[i].layout) rl.removeAt(i);
        for (const Row &r : rl) applyRow(r);
    }
};
