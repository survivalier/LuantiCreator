#pragma once
// Éditeur de pixels (textures) et visionneuse 3D filaire (sans dépendance OpenGL).

#include <QtWidgets>
#include <QMatrix4x4>
#include <QVector3D>
#include "mesh.h"
#include "uihooks.h"

// ------------------------------------------------------------ éditeur de pixels

class PixelCanvas : public QWidget {
public:
    QImage img{16, 16, QImage::Format_ARGB32};
    QColor color{Qt::black};

    PixelCanvas()
    {
        img.fill(Qt::transparent);
        setMinimumSize(384, 384);
    }

    void resizeImage(int n)
    {
        img = QImage(n, n, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        update();
    }

protected:
    int cell() const { return qMax(1, qMin(width(), height()) / qMax(1, img.width())); }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const int c = cell();
        for (int y = 0; y < img.height(); ++y)
            for (int x = 0; x < img.width(); ++x) {
                p.fillRect(x * c, y * c, c, c, ((x + y) % 2) ? QColor(200, 200, 200) : QColor(240, 240, 240));
                const QColor px = img.pixelColor(x, y);
                if (px.alpha() > 0) p.fillRect(x * c, y * c, c, c, px);
            }
    }

    void paintAt(const QPoint &pos, bool erase)
    {
        const int c = cell();
        const int x = pos.x() / c, y = pos.y() / c;
        if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) return;
        img.setPixelColor(x, y, erase ? QColor(Qt::transparent) : color);
        update();
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        paintAt(e->position().toPoint(), e->button() == Qt::RightButton);
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (e->buttons() & (Qt::LeftButton | Qt::RightButton))
            paintAt(e->position().toPoint(), (e->buttons() & Qt::RightButton) != 0);
    }
};

class PixelEditor : public QWidget {
public:
    PixelEditor(QWidget *parent = nullptr) : QWidget(parent)
    {
        canvas_ = new PixelCanvas;
        auto *h = new QHBoxLayout(this);
        auto *side = new QVBoxLayout;

        auto *hint = new QLabel(tr("Clic gauche : dessiner\nClic droit : gommer"));
        colorBtn_ = new QPushButton(tr("Couleur personnalisée..."));
        UiHooks::bind(colorBtn_, "btn.color", "pixel");
        sizeBox_ = new QComboBox;
        sizeBox_->addItems({"8", "16", "32", "64"});
        sizeBox_->setCurrentText("16");
        auto *clear = new QPushButton(tr("Tout effacer"));
        UiHooks::bind(clear, "btn.clear", "pixel");
        auto *open = new QPushButton(tr("Ouvrir un PNG..."));
        UiHooks::bind(open, "btn.open", "pixel");
        auto *save = new QPushButton(tr("Enregistrer en PNG..."));
        UiHooks::bind(save, "btn.save", "pixel");

        side->addWidget(hint);
        side->addWidget(new QLabel(tr("Taille (pixels) :")));
        side->addWidget(sizeBox_);
        side->addWidget(colorBtn_);

        auto *pal = new QGridLayout;
        const char *cols[] = {"#000000", "#ffffff", "#7f7f7f", "#3f3f3f", "#ff0000", "#ff7f00",
                              "#ffd800", "#7fbf00", "#00a000", "#006030", "#00c0c0", "#0060ff",
                              "#0000a0", "#8000ff", "#ff60c0", "#7a4a20", "#c8a060", "#e0d0a0"};
        int i = 0;
        for (const char *c : cols) {
            auto *b = new QPushButton;
            b->setFixedSize(28, 28);
            b->setStyleSheet(QString("background-color:%1;border:1px solid #444;").arg(c));
            const QColor qc(c);
            connect(b, &QPushButton::clicked, this, [this, qc] { canvas_->color = qc; });
            pal->addWidget(b, i / 6, i % 6);
            ++i;
        }
        side->addLayout(pal);
        side->addWidget(clear);
        side->addWidget(open);
        side->addWidget(save);
        side->addStretch();

        h->addWidget(canvas_, 3);
        h->addLayout(side, 1);

        connect(colorBtn_, &QPushButton::clicked, this, [this] {
            const QColor c = QColorDialog::getColor(canvas_->color, this, tr("Couleur"),
                                                    QColorDialog::ShowAlphaChannel);
            if (c.isValid()) canvas_->color = c;
        });
        connect(sizeBox_, &QComboBox::currentTextChanged, this,
                [this](const QString &t) { canvas_->resizeImage(t.toInt()); });
        connect(clear, &QPushButton::clicked, this, [this] {
            canvas_->img.fill(Qt::transparent);
            canvas_->update();
        });
        connect(open, &QPushButton::clicked, this, [this] {
            const QString p = QFileDialog::getOpenFileName(this, tr("Ouvrir"), QString(), tr("Images (*.png)"));
            if (p.isEmpty()) return;
            QImage im(p);
            if (im.isNull()) return;
            canvas_->img = im.convertToFormat(QImage::Format_ARGB32);
            canvas_->update();
        });
        connect(save, &QPushButton::clicked, this, [this] {
            QString p = QFileDialog::getSaveFileName(this, tr("Enregistrer"), "texture.png", tr("Images (*.png)"));
            if (p.isEmpty()) return;
            if (!p.endsWith(".png", Qt::CaseInsensitive)) p += ".png";
            if (!canvas_->img.save(p, "PNG"))
                QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'enregistrer %1").arg(p));
        });
    }

private:
    PixelCanvas *canvas_;
    QPushButton *colorBtn_;
    QComboBox *sizeBox_;
};

// ----------------------------------------------------------- visionneuse 3D

class ModelView : public QWidget {
public:
    ModelView(QWidget *parent = nullptr) : QWidget(parent) { setMinimumSize(340, 340); }

    void setMesh(const MeshPtr &m, double scale)
    {
        meshp_ = m;
        scale_ = scale > 1e-6 ? scale : 1.0;
        update();
    }
    void setMesh(const Mesh &m, double scale) { setMesh(MeshPtr(new Mesh(m)), scale); }
    void setBoxes(const QVector<Box6> &b)
    {
        boxes_ = b;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Mesh &mesh_ = *meshp_;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor(30, 32, 38));

        QMatrix4x4 rot;
        rot.rotate(float(pitch_), 1, 0, 0);
        rot.rotate(float(yaw_), 0, 1, 0);
        const double s = qMin(width(), height()) * 0.3 * zoom_;
        const QPointF c(width() / 2.0, height() / 2.0);

        auto P = [&](const QVector3D &v) {
            const QVector3D q = rot.map(v);
            return QPointF(c.x() + q.x() * s, c.y() - q.y() * s);
        };
        auto line = [&](const QVector3D &a, const QVector3D &b) { p.drawLine(P(a), P(b)); };
        auto cube = [&](double x1, double y1, double z1, double x2, double y2, double z2) {
            QVector3D k[8];
            for (int i = 0; i < 8; ++i)
                k[i] = QVector3D(float((i & 1) ? x2 : x1), float((i & 2) ? y2 : y1), float((i & 4) ? z2 : z1));
            const int e[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3},
                                  {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
            for (const auto &ed : e) line(k[ed[0]], k[ed[1]]);
        };

        // node de référence (1x1x1)
        p.setPen(QPen(QColor(130, 130, 130), 1, Qt::DashLine));
        cube(-0.5, -0.5, -0.5, 0.5, 0.5, 0.5);
        // axes
        p.setPen(QPen(QColor(220, 70, 70), 1.5));
        line(QVector3D(0, 0, 0), QVector3D(0.8f, 0, 0));
        p.setPen(QPen(QColor(70, 200, 70), 1.5));
        line(QVector3D(0, 0, 0), QVector3D(0, 0.8f, 0));
        p.setPen(QPen(QColor(80, 120, 240), 1.5));
        line(QVector3D(0, 0, 0), QVector3D(0, 0, 0.8f));

        // modèle
        if (mesh_.valid) {
            p.setPen(QPen(QColor(110, 170, 255, 170), 1));
            if (mesh_.hasGeometry) {
                const int step = mesh_.tris.size() / 25000 + 1;
                for (int i = 0; i < mesh_.tris.size(); i += step) {
                    const auto &t = mesh_.tris[i];
                    const QVector3D a = mesh_.v[t[0]] / float(scale_);
                    const QVector3D b = mesh_.v[t[1]] / float(scale_);
                    const QVector3D d = mesh_.v[t[2]] / float(scale_);
                    line(a, b);
                    line(b, d);
                    line(d, a);
                }
            } else {
                cube(mesh_.mn.x() / scale_, mesh_.mn.y() / scale_, mesh_.mn.z() / scale_,
                     mesh_.mx.x() / scale_, mesh_.mx.y() / scale_, mesh_.mx.z() / scale_);
            }
        } else {
            p.setPen(QColor(180, 180, 180));
            p.drawText(rect(), Qt::AlignCenter, tr("Aucun aperçu"));
        }

        // selection boxes
        p.setPen(QPen(QColor(255, 80, 80), 1.5));
        for (const Box6 &b : boxes_) cube(b[0], b[1], b[2], b[3], b[4], b[5]);

        p.setPen(QColor(170, 170, 170));
        p.drawText(8, height() - 8, tr("Glisser : tourner — Molette : zoom — rouge : selectionbox"));
    }

    void mousePressEvent(QMouseEvent *e) override { last_ = e->position().toPoint(); }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        const QPoint cur = e->position().toPoint();
        const QPoint d = cur - last_;
        last_ = cur;
        yaw_ += d.x() * 0.6;
        pitch_ = qBound(-89.0, pitch_ + d.y() * 0.6, 89.0);
        update();
    }
    void wheelEvent(QWheelEvent *e) override
    {
        zoom_ = qBound(0.2, zoom_ * std::pow(1.0015, e->angleDelta().y()), 8.0);
        update();
    }

private:
    MeshPtr meshp_{new Mesh};
    double scale_ = 1.0;
    QVector<Box6> boxes_;
    double yaw_ = 30, pitch_ = 20, zoom_ = 1.0;
    QPoint last_;
};
