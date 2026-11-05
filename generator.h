#pragma once
// Génération du code Lua du mod à partir des données des éditeurs.

#include <QtCore>
#include "editor.h"
#include "mesh.h"
#include "texcompose.h"

struct Tabs {
    EditorTab *nodes, *models, *items, *foods, *tools, *crafts,
        *ores, *biomes, *decos, *entities, *cmds;
};

inline QString identOf(const QString &s)
{
    QString r = s.toLower().trimmed();
    r.replace(QRegularExpression("[^a-z0-9_]"), "_");
    return r;
}

inline QString luaStr(QString s)
{
    s.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n");
    return "\"" + s + "\"";
}

class Gen {
public:
    // root vide = aperçu (aucun fichier n'est copié ni créé)
    Gen(const Tabs &t, const QString &modid, const QString &root)
        : t_(t), id_(modid), root_(root) {}

    // Lua supplémentaire (plugins) ajouté à la fin de init.lua
    void setExtra(const QString &lua) { extra_ = lua; }

    QString run(QStringList &warns)
    {
        warns_ = &warns;
        for (const auto &m : t_.biomes->data()) biomeNames_.insert(identOf(m["name"].toString()));
        if (!root_.isEmpty()) {
            QDir().mkpath(root_ + "/textures");
            QDir().mkpath(root_ + "/models");
        }
        QString out;
        QTextStream s(&out);
        s << "-- Généré par LuantiCreator\n\n";
        genNodes(s);
        genMeshNodes(s);
        genItems(s);
        genFoods(s);
        genTools(s);
        genCrafts(s);
        genOres(s);
        genBiomes(s);
        genDecos(s);
        genEntities(s);
        genCommands(s);
        if (!extra_.isEmpty()) s << "-- Plugins\n" << extra_;
        s.flush();
        return out;
    }

private:
    const Tabs &t_;
    QString id_, root_, extra_;
    QStringList *warns_ = nullptr;
    QSet<QString> biomeNames_;

    void warn(const QString &w)
    {
        if (warns_ && !warns_->contains(w)) warns_->append(w);
    }

    QString ref(const QString &n) const
    {
        const QString t = n.trimmed();
        if (t.isEmpty()) return "";
        if (t.contains(':')) return t;
        return id_ + ":" + identOf(t);
    }

    QString copyTexture(const QString &src, const QString &name, const QString &fallback)
    {
        const QString dst = id_ + "_" + identOf(name) + ".png";
        if (src.isEmpty() || !QFile::exists(src)) {
            warn(QString("« %1 » n'a pas de texture valide (texture par défaut utilisée).").arg(name));
            return fallback;
        }
        if (!root_.isEmpty()) {
            QFile::remove(root_ + "/textures/" + dst);
            QFile::copy(src, root_ + "/textures/" + dst);
        }
        return dst;
    }

    // Résultat de la préparation d'une texture : nom du fichier dans le mod et animation éventuelle.
    struct TexOut {
        QString file;
        lctx::Anim anim;
        QSize frame; // taille d'une image de l'animation
    };

    // Prépare la texture `key` d'un élément : copie simple, ou fusion des textures par-dessus (un seul PNG),
    // ou première image seulement si `animatable` est faux (objets : Luanti n'anime pas inventory_image).
    TexOut texture(const QVariantMap &m, const QString &key, const QString &name, const QString &fallback,
                   bool overlays, bool animatable)
    {
        TexOut o;
        o.file = fallback;
        const QString src = m[key].toString();
        if (src.isEmpty() || !QFile::exists(src)) {
            warn(QString("« %1 » n'a pas de texture valide (texture par défaut utilisée).").arg(name));
            return o;
        }
        const QString dst = id_ + "_" + identOf(name) + ".png";
        const QImage base = lctx::loadImg(src);
        if (base.isNull()) { // format inconnu de Qt : copie telle quelle, comme avant
            if (!root_.isEmpty()) {
                QFile::remove(root_ + "/textures/" + dst);
                QFile::copy(src, root_ + "/textures/" + dst);
            }
            o.file = dst;
            return o;
        }
        lctx::Anim a = lctx::animFor(m, base);
        const lctx::Mode mode = lctx::modeOf(m);
        if (animatable && a.type == lctx::Anim::None && (mode == lctx::Mode::Vertical || mode == lctx::Mode::Sheet))
            warn(QString("« %1 » : animation ignorée, les dimensions de l'image (%2×%3) ne correspondent pas au découpage demandé.")
                     .arg(name).arg(base.width()).arg(base.height()));
        QList<QImage> ovs;
        if (overlays) {
            for (const char *k : {"overlay", "overlay2"}) {
                const QString op = m.value(k).toString();
                if (op.isEmpty()) continue;
                const QImage oi = lctx::loadImg(op);
                if (oi.isNull()) warn(QString("« %1 » : texture par-dessus illisible (%2), ignorée.").arg(name, op));
                else ovs << oi;
            }
        }
        QImage out;
        bool rewrite = false;
        if (!ovs.isEmpty()) {
            out = lctx::compose(base, ovs, lctx::opacityOf(m), a);
            rewrite = true;
        }
        if (!animatable && a.type != lctx::Anim::None) {
            out = lctx::frame(out.isNull() ? base : out, a, 0);
            a = lctx::Anim();
            rewrite = true;
            warn(QString("« %1 » : les objets ne peuvent pas être animés dans l'inventaire, première image utilisée.").arg(name));
        }
        if (!root_.isEmpty()) {
            QFile::remove(root_ + "/textures/" + dst);
            if (rewrite) out.save(root_ + "/textures/" + dst, "PNG");
            else QFile::copy(src, root_ + "/textures/" + dst);
        }
        o.file = dst;
        o.anim = a;
        o.frame = lctx::frameSize(rewrite ? out : base, a);
        return o;
    }

    // Entrée de « tiles » : le nom du fichier, ou une table avec animation
    static QString tileLua(const TexOut &t)
    {
        if (t.anim.type == lctx::Anim::None) return luaStr(t.file);
        if (t.anim.type == lctx::Anim::Vertical)
            return QString("{name = %1, animation = {type = \"vertical_frames\", aspect_w = %2, aspect_h = %3, length = %4}}")
                .arg(luaStr(t.file)).arg(t.frame.width()).arg(t.frame.height()).arg(numStr(t.anim.time));
        return QString("{name = %1, animation = {type = \"sheet_2d\", frames_w = %2, frames_h = %3, frame_length = %4}}")
            .arg(luaStr(t.file)).arg(t.anim.cols).arg(t.anim.rows).arg(numStr(t.anim.frameSeconds()));
    }

    QString copyModel(const QString &src, const QString &name)
    {
        const QString dst = id_ + "_" + identOf(name) + "." + QFileInfo(src).suffix().toLower();
        if (!root_.isEmpty()) {
            QFile::remove(root_ + "/models/" + dst);
            QFile::copy(src, root_ + "/models/" + dst);
        }
        return dst;
    }

    static QString boxesLua(const QVector<Box6> &boxes, const QString &indent)
    {
        QStringList l;
        for (const Box6 &b : boxes) {
            QStringList n;
            for (double v : b) n << numStr(v);
            l << "{" + n.join(", ") + "}";
        }
        return "{" + l.join(",\n" + indent) + "}";
    }

    QString csvRefs(const QString &csv) const
    {
        QStringList l;
        for (const QString &p : csv.split(',', Qt::SkipEmptyParts)) {
            const QString r = ref(p);
            if (!r.isEmpty()) l << luaStr(r);
        }
        return l.join(", ");
    }

    // ---------------------------------------------------------------- blocs
    void genNodes(QTextStream &s)
    {
        for (const auto &m : t_.nodes->data()) {
            const QString n = identOf(m["name"].toString());
            const TexOut tex = texture(m, "texture", n, "unknown_node.png", true, true);
            const QString dt = m["drawtype"].toString();
            const int light = m["light"].toInt();
            s << "core.register_node(" << luaStr(id_ + ":" + n) << ", {\n";
            s << "    description = " << luaStr(m["description"].toString()) << ",\n";
            s << "    tiles = {" << tileLua(tex) << "},\n";
            if (dt != "normal") s << "    drawtype = " << luaStr(dt) << ",\n";
            if (dt != "normal" || light > 0) s << "    paramtype = \"light\",\n";
            if (dt == "glasslike" || dt == "allfaces" || dt == "plantlike" || dt == "torchlike")
                s << "    sunlight_propagates = true,\n";
            if (light > 0) s << "    light_source = " << light << ",\n";
            s << "    walkable = " << (m["walkable"].toBool() ? "true" : "false") << ",\n";
            s << "    stack_max = " << m["stack"].toInt() << ",\n";
            const QString drop = ref(m["drop"].toString());
            if (!drop.isEmpty()) s << "    drop = " << luaStr(drop) << ",\n";
            s << "    groups = {" << m["group"].toString() << " = " << qMax(1, m["level"].toInt()) << "},\n";
            s << "})\n\n";
        }
    }

    // ------------------------------------------------------------ blocs 3D
    void genMeshNodes(QTextStream &s)
    {
        for (const auto &m : t_.models->data()) {
            const QString n = identOf(m["name"].toString());
            const QString file = m["file"].toString();
            const double scale = m["scale"].toDouble() > 0 ? m["scale"].toDouble() : 1.0;
            Mesh mesh = loadModel(file);
            if (!QFile::exists(file)) {
                warn(QString("Le bloc 3D « %1 » n'a pas de fichier de modèle : ignoré.").arg(n));
                continue;
            }
            const QString mname = copyModel(file, n);

            QStringList tiles;
            for (int i = 1; i <= 4; ++i) {
                const QString key = QString("tex%1").arg(i);
                if (m[key].toString().isEmpty()) continue;
                tiles << tileLua(texture(m, key, n + "_" + QString::number(i), "unknown_node.png", false, true));
            }
            if (tiles.isEmpty()) {
                warn(QString("« %1 » : aucune texture, texture par défaut utilisée.").arg(n));
                tiles << luaStr("unknown_node.png");
            }

            QVector<Box6> boxes = parseBoxes(m["boxes"].toString());
            if (boxes.isEmpty()) {
                if (mesh.valid) {
                    boxes = generateBoxes(mesh, 0, scale, 0, 0, 0);
                    warn(QString("« %1 » : selectionbox générée automatiquement (boîte englobante).").arg(n));
                } else {
                    boxes.append(mkbox(-0.5, -0.5, -0.5, 0.5, 0.5, 0.5));
                    warn(QString("« %1 » : selectionbox = cube complet (modèle non analysable).").arg(n));
                }
            }

            const int light = m["light"].toInt();
            s << "core.register_node(" << luaStr(id_ + ":" + n) << ", {\n";
            s << "    description = " << luaStr(m["description"].toString()) << ",\n";
            s << "    drawtype = \"mesh\",\n";
            s << "    mesh = " << luaStr(mname) << ",\n";
            s << "    tiles = {" << tiles.join(", ") << "},\n";
            s << "    paramtype = \"light\",\n";
            if (m["rotate"].toBool()) s << "    paramtype2 = \"facedir\",\n";
            s << "    sunlight_propagates = true,\n";
            if (light > 0) s << "    light_source = " << light << ",\n";
            s << "    selection_box = {\n        type = \"fixed\",\n        fixed = "
              << boxesLua(boxes, "            ") << ",\n    },\n";
            if (m["collision"].toBool())
                s << "    collision_box = {\n        type = \"fixed\",\n        fixed = "
                  << boxesLua(boxes, "            ") << ",\n    },\n";
            s << "    groups = {" << m["group"].toString() << " = " << qMax(1, m["level"].toInt()) << "},\n";
            s << "})\n\n";
        }
    }

    // --------------------------------------------------------------- objets
    void genItems(QTextStream &s)
    {
        for (const auto &m : t_.items->data()) {
            const QString n = identOf(m["name"].toString());
            const QString tex = texture(m, "texture", n, "unknown_item.png", true, false).file;
            s << "core.register_craftitem(" << luaStr(id_ + ":" + n) << ", {\n";
            s << "    description = " << luaStr(m["description"].toString()) << ",\n";
            s << "    inventory_image = " << luaStr(tex) << ",\n";
            s << "    stack_max = " << m["stack"].toInt() << ",\n";
            s << "})\n\n";
        }
    }

    void genFoods(QTextStream &s)
    {
        for (const auto &m : t_.foods->data()) {
            const QString n = identOf(m["name"].toString());
            const QString tex = texture(m, "texture", n, "unknown_item.png", true, false).file;
            s << "core.register_craftitem(" << luaStr(id_ + ":" + n) << ", {\n";
            s << "    description = " << luaStr(m["description"].toString()) << ",\n";
            s << "    inventory_image = " << luaStr(tex) << ",\n";
            s << "    stack_max = " << m["stack"].toInt() << ",\n";
            s << "    on_use = core.item_eat(" << m["hp"].toInt() << "),\n";
            s << "})\n\n";
        }
    }

    void genTools(QTextStream &s)
    {
        for (const auto &m : t_.tools->data()) {
            const QString n = identOf(m["name"].toString());
            const QString tex = texture(m, "texture", n, "unknown_item.png", true, false).file;
            const int lvl = qMax(1, m["level"].toInt());
            s << "core.register_tool(" << luaStr(id_ + ":" + n) << ", {\n";
            s << "    description = " << luaStr(m["description"].toString()) << ",\n";
            s << "    inventory_image = " << luaStr(tex) << ",\n";
            s << "    tool_capabilities = {\n";
            s << "        full_punch_interval = 1.0,\n";
            s << "        max_drop_level = " << lvl << ",\n";
            s << "        groupcaps = {\n";
            s << "            " << m["group"].toString() << " = {\n";
            s << "                times = {" << numStr(m["t1"].toInt() / 10.0) << ", "
              << numStr(m["t2"].toInt() / 10.0) << ", " << numStr(m["t3"].toInt() / 10.0) << "},\n";
            s << "                uses = " << m["uses"].toInt() << ",\n";
            s << "                maxlevel = " << lvl << ",\n";
            s << "            },\n        },\n    },\n})\n\n";
        }
    }

    // -------------------------------------------------------------- recettes
    void genCrafts(QTextStream &s)
    {
        for (const auto &m : t_.crafts->data()) {
            const QString kind = m["kind"].toString();
            const QString first = ref(m["r00"].toString());
            QString out = ref(m["output"].toString());
            const int cnt = qMax(1, m["count"].toInt());
            if (kind == "fuel") {
                if (first.isEmpty()) { warn(QString("Recette « %1 » ignorée : pas de combustible.").arg(m["name"].toString())); continue; }
                s << "core.register_craft({\n    type = \"fuel\",\n    recipe = " << luaStr(first)
                  << ",\n    burntime = " << m["time"].toInt() << ",\n})\n\n";
                continue;
            }
            if (out.isEmpty()) {
                warn(QString("Recette « %1 » ignorée : pas de résultat.").arg(m["name"].toString()));
                continue;
            }
            if (cnt > 1) out += " " + QString::number(cnt);
            if (kind == "cooking") {
                if (first.isEmpty()) { warn(QString("Recette « %1 » ignorée : pas d'ingrédient.").arg(m["name"].toString())); continue; }
                s << "core.register_craft({\n    type = \"cooking\",\n    output = " << luaStr(out)
                  << ",\n    recipe = " << luaStr(first) << ",\n    cooktime = " << m["time"].toInt() << ",\n})\n\n";
                continue;
            }
            s << "core.register_craft({\n";
            if (kind == "shapeless") {
                QStringList l;
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c) {
                        const QString x = ref(m[QString("r%1%2").arg(r).arg(c)].toString());
                        if (!x.isEmpty()) l << luaStr(x);
                    }
                s << "    type = \"shapeless\",\n    output = " << luaStr(out) << ",\n    recipe = {"
                  << l.join(", ") << "},\n";
            } else {
                s << "    output = " << luaStr(out) << ",\n    recipe = {\n";
                for (int r = 0; r < 3; ++r) {
                    QStringList l;
                    for (int c = 0; c < 3; ++c)
                        l << luaStr(ref(m[QString("r%1%2").arg(r).arg(c)].toString()));
                    s << "        {" << l.join(", ") << "},\n";
                }
                s << "    },\n";
            }
            s << "})\n\n";
        }
    }

    // ------------------------------------------------- minerais / biomes / déco
    void genOres(QTextStream &s)
    {
        for (const auto &m : t_.ores->data()) {
            const QString ore = ref(m["ore"].toString());
            if (ore.isEmpty()) { warn(QString("Minerai « %1 » ignoré : pas de bloc.").arg(m["name"].toString())); continue; }
            s << "core.register_ore({\n";
            s << "    ore_type = " << luaStr(m["type"].toString()) << ",\n";
            s << "    ore = " << luaStr(ore) << ",\n";
            s << "    wherein = " << luaStr(ref(m["wherein"].toString())) << ",\n";
            s << "    clust_scarcity = " << m["scarcity"].toInt() << ",\n";
            s << "    clust_num_ores = " << m["num"].toInt() << ",\n";
            s << "    clust_size = " << m["size"].toInt() << ",\n";
            s << "    y_min = " << m["ymin"].toInt() << ",\n";
            s << "    y_max = " << m["ymax"].toInt() << ",\n";
            s << "})\n\n";
        }
    }

    void genBiomes(QTextStream &s)
    {
        const auto list = t_.biomes->data();
        bool clear = false;
        for (const auto &m : list) clear = clear || m["replace"].toBool();
        if (clear) s << "core.clear_registered_biomes()\ncore.clear_registered_decorations()\n\n";
        for (const auto &m : list) {
            const QString n = identOf(m["name"].toString());
            s << "core.register_biome({\n";
            s << "    name = " << luaStr(id_ + "_" + n) << ",\n";
            auto node = [&](const char *key, const char *lua) {
                const QString r = ref(m[key].toString());
                if (!r.isEmpty()) s << "    " << lua << " = " << luaStr(r) << ",\n";
            };
            node("node_top", "node_top");
            s << "    depth_top = " << m["depth_top"].toInt() << ",\n";
            node("node_filler", "node_filler");
            s << "    depth_filler = " << m["depth_filler"].toInt() << ",\n";
            node("node_stone", "node_stone");
            s << "    y_max = " << m["ymax"].toInt() << ",\n";
            s << "    y_min = " << m["ymin"].toInt() << ",\n";
            s << "    heat_point = " << m["heat"].toInt() << ",\n";
            s << "    humidity_point = " << m["humid"].toInt() << ",\n";
            s << "})\n\n";
        }
    }

    void genDecos(QTextStream &s)
    {
        for (const auto &m : t_.decos->data()) {
            const QString deco = ref(m["deco"].toString());
            if (deco.isEmpty()) { warn(QString("Décoration « %1 » ignorée : pas de bloc.").arg(m["name"].toString())); continue; }
            s << "core.register_decoration({\n";
            s << "    deco_type = \"simple\",\n";
            s << "    place_on = {" << csvRefs(m["place_on"].toString()) << "},\n";
            s << "    sidelen = 16,\n";
            s << "    fill_ratio = " << numStr(m["fill"].toInt() / 1000.0) << ",\n";
            QStringList biomes;
            for (const QString &b : m["biomes"].toString().split(',', Qt::SkipEmptyParts)) {
                const QString id = identOf(b);
                biomes << luaStr(biomeNames_.contains(id) ? id_ + "_" + id : b.trimmed());
            }
            if (!biomes.isEmpty()) s << "    biomes = {" << biomes.join(", ") << "},\n";
            s << "    y_min = " << m["ymin"].toInt() << ",\n";
            s << "    y_max = " << m["ymax"].toInt() << ",\n";
            s << "    decoration = " << luaStr(deco) << ",\n";
            s << "})\n\n";
        }
    }

    // --------------------------------------------------------------- entités
    void genEntities(QTextStream &s)
    {
        for (const auto &m : t_.entities->data()) {
            const QString n = identOf(m["name"].toString());
            const QString visual = m["visual"].toString();
            const QString behaviour = m["behaviour"].toString();
            const double speed = m["speed"].toDouble();
            const double unit = m["scale"].toDouble() > 0 ? m["scale"].toDouble() : 1.0;
            const QString tex = copyTexture(m["texture"].toString(), n, "unknown_node.png");

            QVector<Box6> cb;
            QString meshName;
            if (visual == "mesh") {
                const QString file = m["file"].toString();
                if (!QFile::exists(file)) {
                    warn(QString("Entité « %1 » : pas de fichier de modèle, ignorée.").arg(n));
                    continue;
                }
                meshName = copyModel(file, n);
                if (m["auto_cbox"].toBool()) {
                    const Mesh mesh = loadModel(file);
                    if (mesh.valid) cb = generateBoxes(mesh, 0, unit, 0, 0, 0);
                    else warn(QString("Entité « %1 » : collisionbox auto impossible, valeur manuelle utilisée.").arg(n));
                }
            }
            if (cb.isEmpty()) cb = parseBoxes(m["cbox"].toString());
            if (cb.isEmpty()) cb.append(mkbox(-0.3, -0.5, -0.3, 0.3, 0.5, 0.3));

            const QString full = luaStr(id_ + ":" + n);
            s << "core.register_entity(" << full << ", {\n";
            s << "    initial_properties = {\n";
            s << "        physical = true,\n        collide_with_objects = true,\n";
            {
                QStringList nums;
                for (double v : cb[0]) nums << numStr(v);
                s << "        collisionbox = {" << nums.join(", ") << "},\n";
            }
            s << "        visual = " << luaStr(visual) << ",\n";
            if (visual == "mesh") {
                s << "        mesh = " << luaStr(meshName) << ",\n";
                s << "        textures = {" << luaStr(tex) << "},\n";
                s << "        visual_size = {x = 1, y = 1},\n";
            } else if (visual == "cube") {
                s << "        textures = {" << QStringList({luaStr(tex), luaStr(tex), luaStr(tex),
                                                            luaStr(tex), luaStr(tex), luaStr(tex)}).join(", ") << "},\n";
                s << "        visual_size = {x = 1, y = 1, z = 1},\n";
            } else {
                s << "        textures = {" << luaStr(tex) << "},\n";
                s << "        visual_size = {x = 1, y = 1},\n";
            }
            s << "        hp_max = " << m["hp"].toInt() << ",\n";
            s << "        makes_footstep_sound = true,\n    },\n";
            s << "    _timer = 0,\n";
            s << "    on_activate = function(self, staticdata)\n"
                 "        self.object:set_acceleration({x = 0, y = -9.8, z = 0})\n"
                 "        self.object:set_armor_groups({fleshy = 100})\n"
                 "    end,\n";
            if (behaviour == "wander") {
                s << "    on_step = function(self, dtime)\n"
                     "        self._timer = self._timer - dtime\n"
                     "        if self._timer > 0 then return end\n"
                     "        self._timer = math.random(2, 5)\n"
                     "        local a = math.random() * 2 * math.pi\n"
                     "        local v = self.object:get_velocity()\n"
                     "        self.object:set_velocity({x = math.cos(a) * "
                  << numStr(speed) << ", y = v.y, z = math.sin(a) * " << numStr(speed) << "})\n"
                     "        self.object:set_yaw(a - math.pi / 2)\n"
                     "    end,\n";
            } else if (behaviour == "follow") {
                s << "    on_step = function(self, dtime)\n"
                     "        local pos = self.object:get_pos()\n"
                     "        local v = self.object:get_velocity()\n"
                     "        local best, target\n"
                     "        for _, p in ipairs(core.get_connected_players()) do\n"
                     "            local d = vector.distance(pos, p:get_pos())\n"
                     "            if d < 12 and (not best or d < best) then best = d; target = p end\n"
                     "        end\n"
                     "        if target then\n"
                     "            local dir = vector.direction(pos, target:get_pos())\n"
                     "            self.object:set_velocity({x = dir.x * "
                  << numStr(speed) << ", y = v.y, z = dir.z * " << numStr(speed) << "})\n"
                     "            self.object:set_yaw(math.atan2(dir.z, dir.x) - math.pi / 2)\n"
                     "        else\n"
                     "            self.object:set_velocity({x = 0, y = v.y, z = 0})\n"
                     "        end\n"
                     "    end,\n";
            }
            s << "})\n\n";

            // commande de test pour faire apparaître l'entité
            s << "core.register_chatcommand(" << luaStr("spawn_" + n) << ", {\n"
               "    description = " << luaStr("Fait apparaître " + n) << ",\n"
               "    privs = {give = true},\n"
               "    func = function(name)\n"
               "        local player = core.get_player_by_name(name)\n"
               "        if not player then return false, \"Joueur introuvable\" end\n"
               "        core.add_entity(vector.add(player:get_pos(), {x = 0, y = 1, z = 0}), "
                << full << ")\n"
               "        return true, \"Entité créée\"\n"
               "    end,\n})\n\n";
        }
    }

    // -------------------------------------------------------------- commandes
    void genCommands(QTextStream &s)
    {
        for (const auto &m : t_.cmds->data()) {
            const QString n = identOf(m["name"].toString());
            QStringList parts = m["message"].toString().split("{player}");
            for (QString &p : parts) p = luaStr(p);
            s << "core.register_chatcommand(" << luaStr(n) << ", {\n";
            s << "    params = " << luaStr(m["params"].toString()) << ",\n";
            s << "    description = " << luaStr(m["description"].toString()) << ",\n";
            const QString priv = identOf(m["priv"].toString());
            if (!priv.isEmpty()) s << "    privs = {" << priv << " = true},\n";
            s << "    func = function(name, param)\n";
            s << "        return true, " << parts.join(" .. name .. ") << "\n";
            s << "    end,\n})\n\n";
        }
    }
};
