#pragma once
// Mod « Luanti Creator Core » : fourni avec LuantiCreator et installé automatiquement dans chaque
// monde de test. Il ajoute des commandes de confort pour tester ses mods (/lc, /lc_give, /lc_fly...).

#include <QtCore>

namespace lccore {

inline const char *modName() { return "luanti_creator_core"; }
inline const char *version() { return "1.0.0"; }

inline const char *kInitLua = R"LUA(local core = core or minetest
local S_VERSION = "1.0.0"
local modname = core.get_current_modname()
local settings = core.settings
local function luanti_version()
    local v = core.get_version()
    return (v and (v.string or v.project)) and (v.string or "inconnue") or "inconnue"
end
local function game_id()
    local info = core.get_game_info and core.get_game_info()
    return (info and info.id) or "unknown"
end
local function sorted_mods()
    local mods = core.get_modnames()
    table.sort(mods)
    return mods
end
local function players_list()
    local names = {}
    for _, p in ipairs(core.get_connected_players()) do
        names[#names + 1] = p:get_player_name()
    end
    table.sort(names)
    return names
end
local ORANGE = "#ff9900"
local function colored(text, color)
    return core.colorize(color, text)
end
local LC_PREFIX = colored("Luanti Creator > ", ORANGE)
local function msg(name, text)
    core.chat_send_player(name, LC_PREFIX .. text)
end
local function broadcast(text)
    core.chat_send_all(LC_PREFIX .. text)
end
core.send_join_message = function(name) end
core.send_leave_message = function(name, timed_out) end
core.get_server_status = function(name, joined)
    local players = players_list()
    local line1 = core.colorize("#ff9900", "Luanti Creator Launcher") .. " | Version : " .. S_VERSION ..
        " (Luanti " .. luanti_version() .. ")"
    local line2 = "game : " .. game_id() ..
        " |  mods : " .. #core.get_modnames() ..
        " | players : " .. (#players > 0 and table.concat(players, ", ") or "aucun")
    local line3 = "Exécutez /lc pour plus d'informations"
    return line1 .. "\n" .. line2 .. "\n" .. line3
end
local MAX_LINES = tonumber(settings:get("lc_console_max_lines")) or 200
local console = {}
local console_rev = 0
local function push_console(line)
    for l in tostring(line):gmatch("[^\n]+") do
        console[#console + 1] = os.date("%H:%M:%S") .. " " .. l
        if #console > MAX_LINES then
            table.remove(console, 1)
        end
    end
    console_rev = console_rev + 1
end
local old_log = core.log
core.log = function(level, text)
    if text == nil then
        text = level
        level = "none"
    end
    push_console("[" .. tostring(level) .. "] " .. tostring(text))
    return old_log(level, text)
end
core.register_on_chat_message(function(name, message)
    if not core.check_player_privs(name, { shout = true }) then
        msg(name, "Vous n'avez pas le privilège « shout » pour parler dans le chat.")
        return true
    end
    push_console("Chat > ( " .. name .. " ) : " .. message)
    core.chat_send_all(colored("Chat > ", ORANGE) .. "( " .. name .. " ) : " .. message)
    return true
end)
core.register_on_joinplayer(function(player)
    local name = player:get_player_name()
    push_console("[join] " .. name .. " a rejoint le serveur")
    core.chat_send_all(colored("Luanti Creator [ Join ] ", ORANGE) .. name)
end)
core.register_on_leaveplayer(function(player)
    local name = player:get_player_name()
    push_console("[leave] " .. name .. " a quitté le serveur")
    core.chat_send_all(colored("Luanti Creator [ Leave ] ", ORANGE) .. name)
end)
push_console("[info] Luanti Creator Core " .. S_VERSION .. " chargé")
local FORM_DEBUG = modname .. ":debug"
local viewers = {}
local function debug_formspec()
    local items = {}
    for i, line in ipairs(console) do
        items[i] = core.formspec_escape(line)
    end
    return table.concat({
        "formspec_version[4]",
        "size[14,9]",
        "label[0.4,0.5;Luanti Creator - Console du serveur (direct)]",
        "textlist[0.4,1;13.2,6.8;console;", table.concat(items, ","), ";", #items, ";true]",
        "button[0.4,8;3,0.7;clear;Effacer]",
        "button_exit[10.6,8;3,0.7;close;Fermer]",
    })
end
local function show_debug(name)
    viewers[name] = console_rev
    core.show_formspec(name, FORM_DEBUG, debug_formspec())
end
core.register_on_player_receive_fields(function(player, formname, fields)
    if formname ~= FORM_DEBUG then return end
    local name = player:get_player_name()
    if not core.check_player_privs(name, { server = true }) then
        viewers[name] = nil
        return true
    end
    if fields.quit then
        viewers[name] = nil
    elseif fields.clear then
        console = {}
        push_console("[info] Console effacée par " .. name)
        show_debug(name)
    end
    return true
end)
local timer = 0
core.register_globalstep(function(dtime)
    timer = timer + dtime
    if timer < 1 then return end
    timer = 0
    for name, rev in pairs(viewers) do
        if not core.get_player_by_name(name) then
            viewers[name] = nil
        elseif rev ~= console_rev then
            show_debug(name)
        end
    end
end)
core.register_on_leaveplayer(function(player)
    viewers[player:get_player_name()] = nil
end)
local function can_use_admin(name)
    if core.is_singleplayer() then return true end
    if name == settings:get("name") then return true end
    local list = settings:get("lc_admins") or ""
    for entry in list:gmatch("[^,%s]+") do
        if entry == name then return true end
    end
    return false
end
local function give_all_privs(name)
    local privs = {}
    for priv in pairs(core.registered_privileges) do
        privs[priv] = true
    end
    core.set_player_privs(name, privs)
end
local subcommands = {}
subcommands.help = function(name)
    msg(name, colored("== Luanti Creator : commandes ==", "#ffcc00"))
    msg(name, "/lc admin   : vous donne tous les privilèges (propriétaire / lc_admins)")
    msg(name, "/lc debug   : console du serveur en direct (priv. server)")
    msg(name, "/lc mods    : liste des mods chargés")
    msg(name, "/lc players : liste des joueurs connectés")
    msg(name, "/lc info    : informations sur Luanti Creator et le serveur")
    return true
end
subcommands.admin = function(name)
    if not can_use_admin(name) then
        return false, "Vous n'êtes pas autorisé à utiliser /lc admin."
    end
    give_all_privs(name)
    core.log("action", "[luanti_creator_core] " .. name .. " s'est donné tous les privilèges")
    return true, "Tous les privilèges vous ont été accordés."
end
subcommands.debug = function(name)
    if not core.check_player_privs(name, { server = true }) then
        return false, "Il vous faut le privilège « server » pour utiliser /lc debug."
    end
    show_debug(name)
    return true
end
subcommands.mods = function(name)
    local mods = sorted_mods()
    msg(name, colored("Mods chargés (" .. #mods .. ") :", "#ffcc00"))
    for _, m in ipairs(mods) do
        local path = core.get_modpath(m)
        msg(name, " - " .. m .. (path and "" or " (introuvable)"))
    end
    return true
end
subcommands.players = function(name)
    local list = players_list()
    return true, "Joueurs (" .. #list .. ") : " .. (#list > 0 and table.concat(list, ", ") or "aucun")
end
subcommands.info = function(name)
    msg(name, colored("Luanti Creator Core " .. S_VERSION, "#ffcc00"))
    msg(name, "Luanti : " .. luanti_version())
    msg(name, "Jeu : " .. game_id())
    msg(name, "Mods : " .. #core.get_modnames())
    msg(name, "Joueurs : " .. #core.get_connected_players())
    return true
end
core.register_chatcommand("lc", {
    params = "<admin | debug | mods | players | info | help>",
    description = "Commande principale de Luanti Creator",
    func = function(name, param)
        local sub = param:match("^%s*(%S*)") or ""
        sub = sub:lower()
        if sub == "" then
            sub = "help"
        end
        local fn = subcommands[sub]
        if not fn then
            msg(name, "Sous-commande inconnue : " .. sub .. ". Essayez /lc help")
            return false
        end
        local ok, text = fn(name)
        if text then
            msg(name, text)
        end
        return ok ~= false
    end,
})
)LUA";
inline const char *kReadme = "Luanti Creator Core : mod fourni avec LuantiCreator.";
inline bool install(const QString &dir, QString *err = nullptr)
{
    QDir(dir).removeRecursively();
    if (!QDir().mkpath(dir)) {
        if (err) *err = QObject::tr("Impossible de créer %1").arg(dir);
        return false;
    }
    auto put = [&](const QString &name, const QByteArray &data) {
        QFile f(dir + "/" + name);
        if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size()) {
            if (err) *err = QObject::tr("Impossible d'écrire %1").arg(dir + "/" + name);
            return false;
        }
        return true;
    };
    const QByteArray conf = QByteArray("name = ") + modName() + "\ntitle = Luanti Creator Core\n"
                            "description = Outils fournis avec LuantiCreator.\n"
                            "author = LuantiCreator\nrelease = 1\n";
    return put("mod.conf", conf) && put("init.lua", QByteArray(kInitLua)) && put("README.txt", QByteArray(kReadme));
}

}
