#pragma once
// Page HTML (CSS + JavaScript, sans aucune dépendance externe ni accès réseau) affichée par la fenêtre
// « Documentation de LuantiCreator » dans un WebView (Qt WebEngine). Elle sait :
//  - rendre le Markdown de la documentation (titres, listes imbriquées, tableaux, citations, code) ;
//  - colorer le code (yaml, lua, python, xml, json, bash, javascript) ;
//  - proposer une icône « Copier » sur chaque bloc de code (et un clic sur le code en ligne le copie) ;
//  - afficher un sommaire filtrable qui suit la lecture, et une recherche avec surlignage ;
//  - suivre le thème de LuantiCreator (couleurs transmises par setTheme()).
// API appelée depuis C++ : lcDoc.setTheme(couleurs, sombre), lcDoc.render(markdown), lcDoc.goTo(titre), lcDoc.focusSearch().
// La copie passe par window.prompt("lc-copy", texte), intercepté par C++ qui renseigne le presse-papiers (voir apidocwindow.h).

#include <QString>

namespace lcdocview {
inline QString html()
{
    return QString::fromUtf8(R"LCHTML(<!doctype html>
<html lang="fr"><head><meta charset="utf-8"><title>Documentation de LuantiCreator</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
:root{--bg:#1e2024;--surface:#272a30;--surface2:#2f3238;--border:#3a3e46;--text:#e4e6ea;--muted:#9096a2;--accent:#5b8def;
 --accentText:#ffffff;--hover:#33373e;--scroll:#454a53;--danger:#ef6b6b;
 --c-kw:#c678dd;--c-str:#98c379;--c-num:#d19a66;--c-com:#7f8794;--c-fn:#61afef;--c-key:#e5c07b;--c-tp:#56b6c2;--c-bi:#e06c75;--c-var:#e06c75;
 --mono:ui-monospace,"JetBrains Mono","Cascadia Code","DejaVu Sans Mono",Menlo,Consolas,monospace;
 --sans:system-ui,-apple-system,"Segoe UI",Roboto,"Noto Sans",Ubuntu,sans-serif}
:root[data-theme="light"]{--c-kw:#a626a4;--c-str:#50a14f;--c-num:#986801;--c-com:#8a8f9a;--c-fn:#4078f2;--c-key:#b76b01;--c-tp:#0184bc;--c-bi:#e45649;--c-var:#e45649}
*{box-sizing:border-box}
html,body{height:100%;margin:0;background:var(--bg);color:var(--text);font:15px/1.65 var(--sans);overflow:hidden}
#app{display:flex;height:100%}
#side{width:300px;min-width:240px;max-width:40vw;display:flex;flex-direction:column;background:var(--surface);border-right:1px solid var(--border)}
#sidehead{padding:12px 12px 8px}
#toc{flex:1;overflow:auto;padding:2px 8px 16px}
#main{flex:1;overflow:auto;scroll-behavior:smooth;position:relative}
::-webkit-scrollbar{width:12px;height:12px}::-webkit-scrollbar-thumb{background:var(--scroll);border-radius:8px;border:3px solid transparent;background-clip:content-box}
::-webkit-scrollbar-thumb:hover{background:var(--muted);background-clip:content-box;border:3px solid transparent}::-webkit-scrollbar-track{background:transparent}
input[type=search],input[type=text]{width:100%;background:var(--bg);color:var(--text);border:1px solid var(--border);border-radius:8px;padding:7px 10px;font:inherit;font-size:14px;outline:none}
input:focus{border-color:var(--accent);box-shadow:0 0 0 3px color-mix(in srgb,var(--accent) 25%,transparent)}
input::placeholder{color:var(--muted)}
.ti{display:flex;align-items:center;gap:2px;border-radius:7px;color:var(--muted);cursor:pointer;text-decoration:none;font-size:13.5px;line-height:1.35;padding:4px 6px;margin:1px 0}
.ti:hover{background:var(--hover);color:var(--text)}
.ti.l1{font-weight:650;color:var(--text);margin-top:8px}.ti.l2{padding-left:18px}.ti.l3{padding-left:34px;font-size:13px}
.ti.active{background:color-mix(in srgb,var(--accent) 18%,transparent);color:var(--accent)}
.ti .tx{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.ti .chev{width:18px;height:18px;flex:none;display:inline-flex;align-items:center;justify-content:center;border-radius:5px;opacity:.8}
.ti .chev:hover{background:var(--border)}.ti .chev svg{width:12px;height:12px;transition:transform .15s}.ti .chev.open svg{transform:rotate(90deg)}
.ti .chev.none{visibility:hidden}.ti.hide{display:none}
#bar{position:sticky;top:0;z-index:5;display:flex;align-items:center;gap:8px;padding:10px 28px;background:var(--bg);border-bottom:1px solid var(--border)}
#bar .sw{position:relative;flex:1;max-width:560px}#bar input{padding-left:32px}
#bar .sw svg{position:absolute;left:10px;top:50%;width:15px;height:15px;transform:translateY(-50%);color:var(--muted);pointer-events:none}
#count{font-size:13px;color:var(--muted);min-width:70px;white-space:nowrap}
.ib{display:inline-flex;align-items:center;justify-content:center;width:30px;height:30px;border:1px solid var(--border);background:var(--surface);color:var(--text);border-radius:8px;cursor:pointer;padding:0}
.ib:hover{background:var(--hover);border-color:var(--accent)}.ib svg{width:15px;height:15px}
#content{max-width:900px;margin:0 auto;padding:10px 32px 120px}
#content h1,#content h2,#content h3,#content h4{line-height:1.3;scroll-margin-top:66px;position:relative}
#content h1{font-size:2em;margin:1.7em 0 .5em;padding-bottom:.3em;border-bottom:1px solid var(--border)}
#content h1:first-child{margin-top:.9em}
#content h2{font-size:1.5em;margin:1.6em 0 .5em}#content h3{font-size:1.2em;margin:1.4em 0 .4em}#content h4{font-size:1.02em;margin:1.2em 0 .3em;color:var(--muted)}
#content .anchor{position:absolute;left:-1.1em;color:var(--muted);text-decoration:none;opacity:0;font-weight:400}
#content h1:hover .anchor,#content h2:hover .anchor,#content h3:hover .anchor,#content h4:hover .anchor{opacity:.8}
#content p{margin:.7em 0}#content a{color:var(--accent);text-decoration:none}#content a:hover{text-decoration:underline}
#content ul,#content ol{padding-left:1.6em;margin:.5em 0}#content li{margin:.2em 0}#content li>p{margin:.2em 0}
#content strong{font-weight:650}
#content blockquote{margin:1em 0;padding:.4em 1em;border-left:4px solid var(--accent);background:color-mix(in srgb,var(--accent) 9%,transparent);border-radius:0 8px 8px 0}
#content blockquote p{margin:.4em 0}
#content hr{border:none;border-top:1px solid var(--border);margin:1.6em 0}
#content code{font-family:var(--mono);font-size:.88em;background:var(--surface2);border:1px solid var(--border);border-radius:6px;padding:.08em .38em;cursor:copy}
#content code:hover{border-color:var(--accent)}
.tw{overflow-x:auto;margin:1em 0;border:1px solid var(--border);border-radius:10px}
.tw table{border-collapse:collapse;width:100%;font-size:14px}
.tw th{background:var(--surface2);text-align:left;font-weight:650}.tw th,.tw td{padding:7px 12px;border-bottom:1px solid var(--border);vertical-align:top}
.tw tr:last-child td{border-bottom:none}.tw td+td,.tw th+th{border-left:1px solid var(--border)}.tw tbody tr:hover td{background:color-mix(in srgb,var(--hover) 60%,transparent)}
.code{margin:1em 0;border:1px solid var(--border);border-radius:10px;background:var(--surface);overflow:hidden}
.code-head{display:flex;align-items:center;justify-content:space-between;padding:5px 8px 5px 14px;background:var(--surface2);border-bottom:1px solid var(--border);font-size:12px;color:var(--muted)}
.code-head .lang{text-transform:uppercase;letter-spacing:.06em;font-weight:650}
.copy{display:inline-flex;align-items:center;gap:6px;border:1px solid transparent;background:transparent;color:var(--muted);border-radius:7px;padding:3px 9px;font:inherit;font-size:12px;cursor:pointer}
.copy:hover{background:var(--hover);color:var(--text);border-color:var(--border)}.copy svg{width:14px;height:14px}
.copy.done{color:var(--c-str);border-color:var(--c-str)}
.code pre{margin:0;padding:12px 16px;overflow:auto;font-family:var(--mono);font-size:13.5px;line-height:1.55;tab-size:4}
#content .code pre code{all:unset;font-family:inherit;white-space:pre;cursor:text;display:block;min-width:max-content}
.tk-k{color:var(--c-kw)}.tk-s{color:var(--c-str)}.tk-n{color:var(--c-num)}.tk-c{color:var(--c-com);font-style:italic}.tk-f{color:var(--c-fn)}
.tk-key{color:var(--c-key)}.tk-b{color:var(--c-bi)}.tk-v{color:var(--c-var)}.tk-p{color:var(--muted)}
.tk-tp{color:var(--c-tp);background:color-mix(in srgb,var(--c-tp) 14%,transparent);border-radius:4px;padding:0 2px}
mark.hit{background:#f2c94c66;color:inherit;border-radius:3px;padding:0}mark.hit.cur{background:#f2994a;color:#111;outline:2px solid #f2994a}
#top{position:fixed;right:26px;bottom:24px;width:40px;height:40px;border-radius:50%;opacity:0;pointer-events:none;transition:opacity .2s;box-shadow:0 4px 14px #0006;z-index:6}
#top.show{opacity:1;pointer-events:auto}
#toast{position:fixed;left:50%;bottom:28px;transform:translate(-50%,20px);background:var(--surface2);color:var(--text);border:1px solid var(--border);border-radius:9px;padding:7px 14px;font-size:13px;opacity:0;pointer-events:none;transition:all .2s;z-index:9}
#toast.show{opacity:1;transform:translate(-50%,0)}
#loading{display:flex;flex-direction:column;align-items:center;gap:14px;margin-top:22vh;color:var(--muted)}
.spin{width:30px;height:30px;border-radius:50%;border:3px solid var(--border);border-top-color:var(--accent);animation:sp .8s linear infinite}@keyframes sp{to{transform:rotate(360deg)}}
@media (max-width:760px){#side{display:none}#content{padding:10px 18px 100px}#bar{padding:10px 14px}}
</style></head><body>
<div id="app">
 <aside id="side"><div id="sidehead"><input id="tocfilter" type="search" placeholder="Filtrer les sections…" autocomplete="off" spellcheck="false"></div><nav id="toc"></nav></aside>
 <div id="main">
  <div id="bar">
   <div class="sw"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="11" cy="11" r="7"/><path d="M20 20l-3.5-3.5"/></svg>
    <input id="search" type="search" placeholder="Rechercher dans la documentation (Entrée : suivant, Maj+Entrée : précédent)" autocomplete="off" spellcheck="false"></div>
   <span id="count"></span>
   <button class="ib" id="prev" title="Résultat précédent (Maj+Entrée)"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M6 15l6-6 6 6"/></svg></button>
   <button class="ib" id="next" title="Résultat suivant (Entrée)"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9l6 6 6-6"/></svg></button>
  </div>
  <article id="content"><div id="loading"><div class="spin"></div><div>Chargement de la documentation…</div></div></article>
 </div>
</div>
<button class="ib" id="top" title="Haut de la page"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 19V6M6 12l6-6 6 6"/></svg></button>
<div id="toast"></div>
<script>
(function(){
"use strict";
var $=function(s){return document.querySelector(s)};
var IS_QT=/QtWebEngine/.test(navigator.userAgent);
var ICON_COPY='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="12" height="12" rx="2"/><path d="M5 15H4a1 1 0 0 1-1-1V4a1 1 0 0 1 1-1h10a1 1 0 0 1 1 1v1"/></svg>';
var ICON_OK='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round"><path d="M20 6L9 17l-5-5"/></svg>';
var ICON_CHEV='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.6" stroke-linecap="round" stroke-linejoin="round"><path d="M9 6l6 6-6 6"/></svg>';
function esc(s){return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;")}

/* ------------------------------------------------------------------ coloration du code */
function set(s){var o={};s.split(" ").forEach(function(w){if(w)o[w]=1});return o}
var LANGS={};
function def(names,o){
  o.kw=set(o.kw||"");o.lit=set(o.lit||"");o.bi=set(o.bi||"");
  o.cls=o.rules.map(function(r){return r[0]});
  o.re=new RegExp(o.rules.map(function(r){return "("+r[1].source+")"}).join("|"),"gm");
  names.split(" ").forEach(function(n){LANGS[n]=o});
}
var NUM=/\b0x[\da-fA-F]+\b|\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?\b/;
var DQ=/"(?:\\[\s\S]|[^"\\\n])*"/, SQ=/'(?:\\[\s\S]|[^'\\\n])*'/;
var WORD=/[A-Za-z_][\w]*/;
var TPL=/\{\{[#\/@!^]?[\w.@ !|:\\-]*\}\}/;
def("lua",{kw:"and break do else elseif end for function goto if in local not or repeat return then until while",lit:"true false nil",
 bi:"core minetest vector table string math os io print pairs ipairs next type tostring tonumber assert error pcall select require setmetatable getmetatable rawget rawset unpack self",
 rules:[["c",/--\[\[[\s\S]*?\]\]|--[^\n]*/],["s",new RegExp(DQ.source+"|"+SQ.source+"|\\[\\[[\\s\\S]*?\\]\\]")],["tp",TPL],["n",NUM],["w",WORD],["p",/[{}()\[\],;=.:+\-*\/%^#<>~]+/]]});
def("python py",{kw:"and as assert async await break class continue def del elif else except finally for from global if import in is lambda nonlocal not or pass raise return try while with yield",
 lit:"True False None",bi:"print len range str int float bool list dict set tuple open isinstance super self cls enumerate zip map filter sorted min max sum abs any all repr format json os sys re lc core",
 rules:[["c",/#[^\n]*/],["s",/[rRfFbBuU]{0,2}(?:"""[\s\S]*?"""|'''[\s\S]*?'''|"(?:\\[\s\S]|[^"\\\n])*"|'(?:\\[\s\S]|[^'\\\n])*')/],["f",/@[A-Za-z_][\w.]*/],["n",NUM],["w",WORD],["p",/[{}()\[\],;=.:+\-*\/%<>!]+/]]});
def("yaml yml",{lit:"true false null",
 rules:[["c",/(?<![^\s])#[^\n]*/],["s",/"(?:\\[\s\S]|[^"\\\n])*"|'[^'\n]*'/],["tp",TPL],["key",/[A-Za-z_][\w.\-]*(?=[ \t]*:(?:[ \t]|$))/],["w",/\b(?:true|false|null)\b/],["n",NUM],["p",/[{}\[\],:|>]|^\s*-(?=\s)|(?<=\s)-(?=\s)/]]});
def("xml html svg",{
 rules:[["c",/<!--[\s\S]*?-->/],["s",/<!\[CDATA\[[\s\S]*?\]\]>/],["k",/<\/?[A-Za-z_][\w:.\-]*/],["k",/\/?>/],["key",/[A-Za-z_][\w:.\-]*(?=[ \t]*=)/],["s",/"[^"\n]*"|'[^'\n]*'/],["tp",TPL],["n",/&#?\w+;/]]});
def("json",{lit:"true false null",
 rules:[["key",/"(?:\\[\s\S]|[^"\\\n])*"(?=\s*:)/],["s",DQ],["n",NUM],["w",WORD],["p",/[{}\[\],:]/]]});
def("bash sh shell console",{kw:"if then else elif fi for while do done case esac in function return export local set unset",
 bi:"echo cd ls cp mv rm mkdir cat sudo apt git cmake make python python3 pip chmod chown tar unzip zip cp grep sed awk",
 rules:[["c",/(?<![^\s])#[^\n]*/],["s",new RegExp(DQ.source+"|"+SQ.source)],["v",/\$(?:\{[^}\n]*\}|\w+|[@#?*$!])/],["key",/(?<=\s)--?[A-Za-z][\w-]*/],["n",NUM],["w",WORD]]});
def("javascript js",{kw:"async await break case catch class const continue default delete do else export extends finally for from function if import in instanceof let new of return static super switch this throw try typeof var void while yield",
 lit:"true false null undefined",bi:"console document window JSON Math Object Array String Number Promise Map Set",
 rules:[["c",/\/\/[^\n]*|\/\*[\s\S]*?\*\//],["s",/"(?:\\[\s\S]|[^"\\\n])*"|'(?:\\[\s\S]|[^'\\\n])*'|`(?:\\[\s\S]|[^`\\])*`/],["n",NUM],["w",WORD],["p",/[{}()\[\],;=.:+\-*\/%<>!&|?]+/]]});
var CLS={c:"tk-c",s:"tk-s",n:"tk-n",k:"tk-k",f:"tk-f",key:"tk-key",b:"tk-b",v:"tk-v",p:"tk-p",tp:"tk-tp",lit:"tk-n"};
function highlight(code,lang){
  var L=LANGS[(lang||"").toLowerCase()];
  if(!L)return esc(code);
  var re=L.re,out="",last=0,m,prev="";
  re.lastIndex=0;
  while((m=re.exec(code))){
    if(m[0]===""){re.lastIndex++;continue}
    out+=esc(code.slice(last,m.index));
    var gi=1;while(m[gi]===undefined)gi++;
    var cls=L.cls[gi-1],txt=m[0];
    if(cls==="w"){
      var after=code.slice(re.lastIndex,re.lastIndex+40);
      if(L.kw[txt])cls="k";
      else if(L.lit[txt])cls="lit";
      else if(prev==="def"||prev==="class"||prev==="function"||/^[ \t]*\(/.test(after))cls="f";
      else if(L.bi[txt])cls="b";
      else cls=null;
      prev=txt;
    }
    var h=esc(txt);
    if(cls==="s")h=h.replace(/(\{\{[#\/@!^]?[\w.@ !|:\\-]*\}\})/g,'<span class="tk-tp">$1</span>');
    out+=cls?'<span class="'+CLS[cls]+'">'+h+"</span>":h;
    last=re.lastIndex;
  }
  return out+esc(code.slice(last));
}

/* ------------------------------------------------------------------ Markdown */
var heads=[],codes=[],slugs={};
function slug(t){
  var s=t.toLowerCase().normalize("NFD").replace(/[\u0300-\u036f]/g,"").replace(/[^a-z0-9]+/g,"-").replace(/^-+|-+$/g,"")||"section";
  var n=slugs[s]=(slugs[s]||0)+1;return n>1?s+"-"+n:s;
}
function plain(t){return t.replace(/`+/g,"").replace(/\*\*?|~~/g,"").replace(/\\(.)/g,"$1").replace(/\[([^\]]+)\]\([^)]*\)/g,"$1").trim()}
function inline(src){
  var stash=[];
  function keep(h){stash.push(h);return "\u0000"+(stash.length-1)+"\u0000"}
  var s=src.replace(/(`+)([^`]|[^`][\s\S]*?[^`])\1(?!`)/g,function(m,t,c){return keep("<code>"+esc(c.replace(/^ (.*[^ ].*) $/,"$1"))+"</code>")});
  s=s.replace(/\\([\\`*_{}\[\]()#+\-.!|~<>])/g,function(m,c){return keep(esc(c))});
  s=esc(s);
  s=s.replace(/\[([^\]]+)\]\(([^)\s]+)(?:\s+&quot;[^&]*&quot;)?\)/g,function(m,t,u){return keep('<a href="'+u+'">')+t+keep("</a>")});
  s=s.replace(/&lt;(https?:\/\/[^\s&]+)&gt;/g,function(m,u){return keep('<a href="'+u+'">'+u+"</a>")});
  s=s.replace(/\*\*(?=\S)([\s\S]+?)(?<=\S)\*\*/g,"<strong>$1</strong>");
  s=s.replace(/(^|[^\w*])\*(?=[^\s*])([^*\n]+?)(?<=[^\s*])\*(?![\w*])/g,"$1<em>$2</em>");
  s=s.replace(/~~(?=\S)([\s\S]+?)(?<=\S)~~/g,"<del>$1</del>");
  return s.replace(/\u0000(\d+)\u0000/g,function(m,i){return stash[+i]});
}
function paragraph(lines){
  var t=lines.map(function(l,i){return (i<lines.length-1&&/(?: {2,}|\\)$/.test(l))?l.replace(/(?: {2,}|\\)$/,"")+"\u0001":l.trim()}).join("\n");
  return "<p>"+inline(t).replace(/\u0001\n?/g,"<br>").replace(/\n/g," ")+"</p>";
}
var RE_FENCE=/^(\s*)(`{3,}|~{3,})\s*([\w+#.-]*)[^\n]*$/;
var RE_LIST=/^(\s*)([-*+]|\d+[.)])\s+(.*)$/;
var RE_HR=/^\s*([-*_])(\s*\1){2,}\s*$/;
var RE_SEP=/^\s*\|?\s*:?-{2,}:?\s*(\|\s*:?-{2,}:?\s*)*\|?\s*$/;
function isBlank(l){return /^\s*$/.test(l)}
function indentOf(l){return l.match(/^\s*/)[0].replace(/\t/g,"    ").length}
function codeBlock(code,lang){
  var l=(lang||"").toLowerCase(),idx=codes.push(code)-1;
  var label=l&&l!=="text"&&l!=="txt"?l:"texte";
  return '<div class="code"><div class="code-head"><span class="lang">'+esc(label)+'</span><button class="copy" data-i="'+idx+'" title="Copier le code">'+ICON_COPY+"<span>Copier</span></button></div><pre><code>"+highlight(code,l)+"</code></pre></div>";
}
function cells(row){
  row=row.trim().replace(/^\|/,"").replace(/\|$/,"");
  return row.split(/(?<!\\)\|/).map(function(c){return c.trim().replace(/\\\|/g,"|")});
}
function startsBlock(l){return RE_FENCE.test(l)||/^#{1,6}\s/.test(l)||/^\s*>/.test(l)||RE_HR.test(l)||RE_LIST.test(l)}
function list(lines){
  var base=indentOf(lines[0]),ordered=/\d/.test(lines[0].match(RE_LIST)[2]),start=ordered?parseInt(lines[0].match(RE_LIST)[2],10):1;
  var items=[],cur=null;
  lines.forEach(function(l){
    var m=l.match(RE_LIST);
    if(m&&indentOf(l)<=base+1){
      cur={ind:m[1].replace(/\t/g,"    ").length+m[2].length+1,lines:[m[3]]};items.push(cur);
    }else if(cur){
      var strip=Math.min(cur.ind,indentOf(l));
      cur.lines.push(isBlank(l)?"":l.replace(/^\t/,"    ").slice(strip));
    }
  });
  var tag=ordered?"ol":"ul";
  return "<"+tag+(ordered&&start!==1?' start="'+start+'"':"")+">"+items.map(function(it){return "<li>"+blocks(it.lines)+"</li>"}).join("")+"</"+tag+">";
}
function blocks(lines){
  var out=[],i=0,m;
  while(i<lines.length){
    var l=lines[i];
    if(isBlank(l)){i++;continue}
    if((m=l.match(RE_FENCE))){
      var ind=m[1].length,ch=m[2][0],n=m[2].length,lang=m[3],buf=[];i++;
      var close=new RegExp("^\\s*"+(ch==="`"?"`":"~")+"{"+n+",}\\s*$");
      while(i<lines.length&&!close.test(lines[i])){buf.push(lines[i].replace(new RegExp("^ {0,"+ind+"}"),""));i++}
      i++;out.push(codeBlock(buf.join("\n"),lang));continue;
    }
    if((m=l.match(/^(#{1,6})\s+(.*?)\s*$/))){
      var lv=m[1].length,txt=m[2].replace(/\s+#+$/,""),id=slug(plain(txt));
      heads.push({level:lv,text:plain(txt),id:id});
      out.push("<h"+lv+' id="'+id+'"><a class="anchor" href="#'+id+'">#</a>'+inline(txt)+"</h"+lv+">");i++;continue;
    }
    if(RE_HR.test(l)){out.push("<hr>");i++;continue}
    if(/^\s*\|/.test(l)&&i+1<lines.length&&RE_SEP.test(lines[i+1])){
      var head=cells(l),al=cells(lines[i+1]).map(function(c){return /^:-*:$/.test(c)?"center":/-:$/.test(c)?"right":""});
      i+=2;var rows=[];
      while(i<lines.length&&/^\s*\|/.test(lines[i])){rows.push(cells(lines[i]));i++}
      var td=function(tag,c,k){return "<"+tag+(al[k]?' style="text-align:'+al[k]+'"':"")+">"+inline(c)+"</"+tag+">"};
      out.push('<div class="tw"><table><thead><tr>'+head.map(function(c,k){return td("th",c,k)}).join("")+"</tr></thead><tbody>"+
        rows.map(function(r){return "<tr>"+head.map(function(_,k){return td("td",r[k]||"",k)}).join("")+"</tr>"}).join("")+"</tbody></table></div>");
      continue;
    }
    if(/^\s*>/.test(l)){
      var q=[];
      while(i<lines.length&&/^\s*>/.test(lines[i])){q.push(lines[i].replace(/^\s*>\s?/,""));i++}
      out.push("<blockquote>"+blocks(q)+"</blockquote>");continue;
    }
    if(RE_LIST.test(l)){
      var base=indentOf(l),grp=[];
      while(i<lines.length){
        var x=lines[i];
        if(isBlank(x)){
          var j=i+1;while(j<lines.length&&isBlank(lines[j]))j++;
          if(j>=lines.length)break;
          var nx=lines[j],nix=indentOf(nx);
          if(nix>base||(RE_LIST.test(nx)&&nix>=base&&nix<=base+1)){grp.push("");i++;continue}
          break;
        }
        var ix=indentOf(x);
        if(RE_LIST.test(x)&&ix>=base){grp.push(x);i++;continue}
        if(ix>base){grp.push(x);i++;continue}
        if(!startsBlock(x)&&grp.length&&!isBlank(grp[grp.length-1])){grp.push("   "+x.trim());i++;continue}
        break;
      }
      out.push(list(grp));continue;
    }
    var p=[];
    while(i<lines.length&&!isBlank(lines[i])&&(p.length===0||!startsBlock(lines[i]))&&!(/^\s*\|/.test(lines[i])&&i+1<lines.length&&RE_SEP.test(lines[i+1]))){p.push(lines[i]);i++}
    if(!p.length){p.push(lines[i]);i++}
    out.push(paragraph(p));
  }
  return out.join("\n");
}

/* ------------------------------------------------------------------ interface */
var content=$("#content"),main=$("#main"),toc=$("#toc"),search=$("#search"),countEl=$("#count");
var tocNodes=[],openSet={},activeIdx=-1,hits=[],cur=-1,toastTimer=0;
function toast(t){var e=$("#toast");e.textContent=t;e.classList.add("show");clearTimeout(toastTimer);toastTimer=setTimeout(function(){e.classList.remove("show")},1500)}
function copyText(t){
  if(IS_QT){try{window.prompt("lc-copy",t)}catch(e){}return Promise.resolve()}
  if(navigator.clipboard&&window.isSecureContext)return navigator.clipboard.writeText(t);
  return new Promise(function(res){var a=document.createElement("textarea");a.value=t;document.body.appendChild(a);a.select();try{document.execCommand("copy")}catch(e){}a.remove();res()});
}
function buildToc(){
  toc.innerHTML="";tocNodes=[];
  var stack=[];
  heads.forEach(function(h,i){
    if(h.level>3)return;
    while(stack.length&&stack[stack.length-1].level>=h.level)stack.pop();
    var node={i:i,h:h,parent:stack.length?stack[stack.length-1]:null,kids:0,el:null,chev:null};
    if(node.parent)node.parent.kids++;
    stack.push(node);tocNodes.push(node);
  });
  tocNodes.forEach(function(n){
    var a=document.createElement("a");a.className="ti l"+n.h.level;a.href="#"+n.h.id;a.title=n.h.text;
    var c=document.createElement("span");c.className="chev"+(n.kids?"":" none");c.innerHTML=ICON_CHEV;
    var t=document.createElement("span");t.className="tx";t.textContent=n.h.text;
    a.appendChild(c);a.appendChild(t);toc.appendChild(a);n.el=a;n.chev=c;
    c.addEventListener("click",function(e){e.preventDefault();e.stopPropagation();openSet[n.h.id]=!openSet[n.h.id];layoutToc()});
  });
  layoutToc();
}
function layoutToc(){
  var q=$("#tocfilter").value.trim().toLowerCase(),match={};
  if(q)tocNodes.forEach(function(n){
    if(n.h.text.toLowerCase().indexOf(q)>=0)for(var p=n;p;p=p.parent)match[p.h.id]=1;
  });
  tocNodes.forEach(function(n){
    var vis=true;
    if(q)vis=!!match[n.h.id];
    else for(var p=n.parent;p;p=p.parent)if(!openSet[p.h.id]){vis=false;break}
    n.el.classList.toggle("hide",!vis);
    n.chev.classList.toggle("open",q?!!n.kids:!!openSet[n.h.id]);
  });
}
function setActive(idx){
  if(idx===activeIdx)return;
  activeIdx=idx;
  tocNodes.forEach(function(n){n.el.classList.toggle("active",n.i===idx)});
  var node=tocNodes.filter(function(n){return n.i===idx})[0];
  if(!node)return;
  for(var p=node.parent;p;p=p.parent)openSet[p.h.id]=true;
  if(node.kids&&node.h.level===1)openSet[node.h.id]=true;
  layoutToc();
  if(!node.el.classList.contains("hide"))node.el.scrollIntoView({block:"nearest"});
}
var ticking=false;
function onScroll(){
  $("#top").classList.toggle("show",main.scrollTop>600);
  if(ticking)return;ticking=true;
  requestAnimationFrame(function(){
    ticking=false;
    var best=-1,limit=main.getBoundingClientRect().top+90;
    for(var k=0;k<tocNodes.length;k++){
      var el=document.getElementById(tocNodes[k].h.id);
      if(el&&el.getBoundingClientRect().top<=limit)best=tocNodes[k].i;else if(el)break;
    }
    if(best<0&&tocNodes.length)best=tocNodes[0].i;
    setActive(best);
  });
}
function goId(id,smooth){
  var el=document.getElementById(id);if(!el)return;
  main.style.scrollBehavior=smooth===false?"auto":"smooth";
  el.scrollIntoView({block:"start"});
}
function goTo(title){
  var t=(title||"").toLowerCase();
  for(var k=0;k<heads.length;k++)if(heads[k].text.toLowerCase().indexOf(t)>=0){goId(heads[k].id);return true}
  return false;
}
/* recherche */
function fold(s){var o="";for(var i=0;i<s.length;i++)o+=s.charAt(i).normalize("NFD").charAt(0).toLowerCase();return o}
function clearMarks(){
  content.querySelectorAll("mark.hit").forEach(function(m){var p=m.parentNode;p.replaceChild(document.createTextNode(m.textContent),m);p.normalize()});
  hits=[];cur=-1;
}
function runSearch(){
  clearMarks();
  var q=fold(search.value.trim());
  if(q.length<2){countEl.textContent="";return}
  var w=document.createTreeWalker(content,NodeFilter.SHOW_TEXT,{acceptNode:function(n){
    return n.parentNode.closest(".code-head")||n.parentNode.closest("#loading")?NodeFilter.FILTER_REJECT:NodeFilter.FILTER_ACCEPT}});
  var nodes=[],n;while((n=w.nextNode()))nodes.push(n);
  nodes.forEach(function(node){
    if(hits.length>3000)return;
    var f=fold(node.nodeValue),pos=f.indexOf(q);
    if(pos<0)return;
    var frag=document.createDocumentFragment(),text=node.nodeValue,last=0;
    while(pos>=0){
      if(pos>last)frag.appendChild(document.createTextNode(text.slice(last,pos)));
      var mk=document.createElement("mark");mk.className="hit";mk.textContent=text.slice(pos,pos+q.length);
      frag.appendChild(mk);hits.push(mk);last=pos+q.length;pos=f.indexOf(q,last);
    }
    if(last<text.length)frag.appendChild(document.createTextNode(text.slice(last)));
    node.parentNode.replaceChild(frag,node);
  });
  if(hits.length)activate(0);else countEl.textContent="Aucun résultat";
}
function activate(i){
  if(!hits.length)return;
  if(cur>=0&&hits[cur])hits[cur].classList.remove("cur");
  cur=(i+hits.length)%hits.length;
  var h=hits[cur];h.classList.add("cur");
  main.style.scrollBehavior="auto";
  h.scrollIntoView({block:"center"});
  countEl.textContent=(cur+1)+" / "+hits.length+(hits.length>3000?"+":"");
}
function focusSearch(){search.focus();search.select()}

/* événements */
document.addEventListener("click",function(e){
  var b=e.target.closest(".copy");
  if(b){
    var span=b.querySelector("span"),old=ICON_COPY;
    copyText(codes[+b.dataset.i]).then(function(){
      b.classList.add("done");b.innerHTML=ICON_OK+"<span>Copié</span>";
      setTimeout(function(){b.classList.remove("done");b.innerHTML=old+"<span>Copier</span>"},1400);
    });
    return;
  }
  var c=e.target.closest("#content code");
  if(c&&!c.closest("pre")){
    if(String(window.getSelection())==="")copyText(c.textContent).then(function(){toast("Copié : "+(c.textContent.length>48?c.textContent.slice(0,48)+"…":c.textContent))});
    return;
  }
  var a=e.target.closest("a[href^='#']");
  if(a){e.preventDefault();goId(decodeURIComponent(a.getAttribute("href").slice(1)))}
});
$("#top").addEventListener("click",function(){main.style.scrollBehavior="smooth";main.scrollTo({top:0})});
main.addEventListener("scroll",onScroll);
var st=0;search.addEventListener("input",function(){clearTimeout(st);st=setTimeout(runSearch,140)});
search.addEventListener("keydown",function(e){
  if(e.key==="Enter"){e.preventDefault();if(hits.length)activate(cur+(e.shiftKey?-1:1));else runSearch()}
  else if(e.key==="Escape"){search.value="";runSearch();search.blur()}
});
$("#next").addEventListener("click",function(){activate(cur+1)});
$("#prev").addEventListener("click",function(){activate(cur-1)});
$("#tocfilter").addEventListener("input",layoutToc);
document.addEventListener("keydown",function(e){
  if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==="f"){e.preventDefault();focusSearch()}
  else if(e.key==="F3"){e.preventDefault();activate(cur+(e.shiftKey?-1:1))}
  else if(e.key==="/"&&document.activeElement.tagName!=="INPUT"){e.preventDefault();focusSearch()}
});

window.lcDoc={
  compact:function(){
    var s=document.createElement("style");
    s.textContent="#side,#bar{display:none!important}#app{display:block}#main{height:100%;width:100%}#content{padding:20px 28px 80px}";
    document.head.appendChild(s);
  },
  setTheme:function(colors,dark){
    var r=document.documentElement;
    r.setAttribute("data-theme",dark?"dark":"light");
    Object.keys(colors||{}).forEach(function(k){r.style.setProperty("--"+k,colors[k])});
  },
  render:function(md){
    heads=[];codes=[];slugs={};openSet={};activeIdx=-1;
    content.innerHTML=blocks(String(md).replace(/\r\n?/g,"\n").replace(/\t/g,"    ").split("\n"));
    buildToc();
    if(tocNodes.length)tocNodes.filter(function(n){return n.h.level===1}).slice(0,1).forEach(function(n){openSet[n.h.id]=true});
    layoutToc();onScroll();
    return heads.length;
  },
  goTo:goTo,focusSearch:focusSearch,
  stats:function(){return {heads:heads.length,codes:codes.length,tables:content.querySelectorAll("table").length,toc:tocNodes.length}}
};
})();
</script></body></html>)LCHTML");
}
} // namespace lcdocview
