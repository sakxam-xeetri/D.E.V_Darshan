/*
 * ============================================================================
 *  D.E.V_Darshan — Premium WiFi Portal UI (PROGMEM)
 * ============================================================================
 *  Production-grade, ultra-modern web portal for the D.E.V_Darshan TXT Reader.
 *
 *  Features:
 *    - Fixed left vertical sidebar navigation (Dashboard, Files, Editor,
 *      Settings, Guide, About)
 *    - Deep matte dark background with rich crimson/red primary accents
 *    - Glassmorphism-style cards, smooth transitions, micro-interactions
 *    - Drag-and-drop file upload with progress bar
 *    - Full file management: upload, rename, delete (modal), download, create
 *    - Monospace text editor with active-line highlighting, char counter
 *    - Dashboard with device status, storage bar, uptime, WiFi info
 *    - Settings panel for WiFi configuration and reading behaviour
 *    - Comprehensive Guide with collapsible panels and SVG illustrations
 *    - About section with tech specs, hardware, author profile & branding
 *    - Toast notifications, confirmation modals, responsive layout
 *    - Inline SVG icons — zero external assets
 *
 *  Stored in flash (PROGMEM) to minimise RAM usage on ESP32.
 * ============================================================================
 */

#ifndef PORTAL_H
#define PORTAL_H

#include <pgmspace.h>

static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>D.E.V_Darshan — Portal</title>
<style>
:root{
--bg:#0C0C10;--bg2:#121218;--sidebar:#0F0F14;
--card:rgba(255,255,255,0.035);--card-h:rgba(255,255,255,0.055);
--glass-brd:rgba(255,255,255,0.07);
--pri:#DC143C;--pri-lt:#FF2D55;--pri-dk:#A01030;--pri-glow:rgba(220,20,60,0.22);
--txt:#E8E6E3;--txt2:#8A8A95;--txt3:#555560;
--brd:rgba(255,255,255,0.07);
--ok:#00E676;--ok-bg:rgba(0,230,118,0.1);
--err:#FF1744;--err-bg:rgba(255,23,68,0.1);
--warn:#FFD740;--warn-bg:rgba(255,215,64,0.1);
--sb-w:250px;--radius:14px;
--ease:cubic-bezier(.4,0,.2,1)
}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',-apple-system,BlinkMacSystemFont,'Inter',sans-serif;background:var(--bg);color:var(--txt);min-height:100vh;overflow-x:hidden}

/* ── Sidebar ── */
.sidebar{position:fixed;left:0;top:0;bottom:0;width:var(--sb-w);background:var(--sidebar);border-right:1px solid var(--brd);display:flex;flex-direction:column;z-index:200;transition:transform .35s var(--ease)}
.sb-logo{padding:28px 20px 22px;border-bottom:1px solid var(--brd);text-align:center}
.sb-logo h1{font-size:1.1em;color:var(--txt);display:flex;align-items:center;justify-content:center;gap:8px}
.sb-logo h1 svg{color:var(--pri);filter:drop-shadow(0 0 8px var(--pri-glow))}
.sb-logo p{font-size:.68em;color:var(--txt2);margin-top:5px;letter-spacing:1.5px;text-transform:uppercase}
.sb-nav{flex:1;padding:14px 0;overflow-y:auto}
.sb-nav button{width:100%;padding:13px 22px;border:none;background:transparent;color:var(--txt2);font-size:.86em;font-weight:500;cursor:pointer;display:flex;align-items:center;gap:13px;transition:all .25s var(--ease);border-left:3px solid transparent;position:relative}
.sb-nav button:hover{background:rgba(255,255,255,.03);color:var(--txt)}
.sb-nav button.active{background:rgba(220,20,60,.08);color:var(--pri);border-left-color:var(--pri);font-weight:600}
.sb-nav button.active svg{filter:drop-shadow(0 0 6px var(--pri-glow))}
.sb-nav button svg{width:20px;height:20px;flex-shrink:0}
.sb-footer{padding:16px 20px;border-top:1px solid var(--brd);font-size:.66em;color:var(--txt3);text-align:center;line-height:1.7}
.sb-footer span{color:var(--pri)}

/* ── Mobile Header ── */
.mob-hdr{display:none;position:fixed;top:0;left:0;right:0;height:56px;background:var(--sidebar);border-bottom:1px solid var(--brd);z-index:150;align-items:center;padding:0 14px;gap:12px}
.ham{width:38px;height:38px;border:none;background:transparent;color:var(--txt);cursor:pointer;display:flex;align-items:center;justify-content:center;border-radius:8px;transition:background .2s}
.ham:hover{background:rgba(255,255,255,.05)}
.mob-title{font-size:.95em;font-weight:600;color:var(--txt);display:flex;align-items:center;gap:8px}
.mob-title svg{color:var(--pri)}
.sb-bk{display:none;position:fixed;inset:0;background:rgba(0,0,0,.6);z-index:199;backdrop-filter:blur(2px);-webkit-backdrop-filter:blur(2px)}

/* ── Main ── */
.main{margin-left:var(--sb-w);min-height:100vh;padding:28px;transition:margin-left .35s var(--ease)}

/* ── Tabs ── */
.tab-panel{display:none;animation:fadeSlide .4s var(--ease)}
.tab-panel.active{display:block}
@keyframes fadeSlide{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:none}}

/* ── Page Header ── */
.pg-hd{margin-bottom:24px}
.pg-hd h2{font-size:1.45em;font-weight:700;display:flex;align-items:center;gap:10px}
.pg-hd h2 svg{color:var(--pri);flex-shrink:0}
.pg-hd p{font-size:.8em;color:var(--txt2);margin-top:4px}

/* ── Cards ── */
.card{background:var(--card);border:1px solid var(--glass-brd);border-radius:var(--radius);padding:20px;margin-bottom:20px;backdrop-filter:blur(12px);-webkit-backdrop-filter:blur(12px);transition:all .3s var(--ease)}
.card:hover{background:var(--card-h);border-color:rgba(255,255,255,.1);box-shadow:0 8px 32px rgba(0,0,0,.3)}
.card h3{font-size:.9em;color:var(--txt);margin-bottom:16px;display:flex;align-items:center;gap:10px;font-weight:600}
.card h3 svg{color:var(--pri);flex-shrink:0}

/* ── Stat Grid ── */
.stat-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:14px;margin-bottom:20px}
.stat-card{background:var(--card);border:1px solid var(--glass-brd);border-radius:var(--radius);padding:18px;backdrop-filter:blur(12px);transition:all .3s var(--ease);position:relative;overflow:hidden}
.stat-card::after{content:'';position:absolute;bottom:0;left:0;right:0;height:2px;background:linear-gradient(90deg,var(--pri),var(--pri-lt));opacity:0;transition:opacity .3s}
.stat-card:hover{background:var(--card-h);transform:translateY(-2px);box-shadow:0 8px 24px rgba(0,0,0,.3)}
.stat-card:hover::after{opacity:1}
.st-icon{width:38px;height:38px;border-radius:10px;display:flex;align-items:center;justify-content:center;margin-bottom:12px}
.st-icon.red{background:rgba(220,20,60,.12);color:var(--pri)}
.st-icon.grn{background:var(--ok-bg);color:var(--ok)}
.st-icon.amb{background:var(--warn-bg);color:var(--warn)}
.st-icon.blu{background:rgba(66,165,245,.12);color:#42a5f5}
.st-lbl{font-size:.7em;color:var(--txt2);text-transform:uppercase;letter-spacing:.5px;margin-bottom:4px}
.st-val{font-size:1.25em;font-weight:700;color:var(--txt)}

/* ── Storage Bar ── */
.storage-bar{height:8px;background:rgba(255,255,255,.06);border-radius:4px;overflow:hidden;margin:10px 0}
.storage-fill{height:100%;background:linear-gradient(90deg,var(--pri),var(--pri-lt));border-radius:4px;transition:width .8s var(--ease);box-shadow:0 0 12px var(--pri-glow)}
.storage-text{display:flex;justify-content:space-between;font-size:.73em;color:var(--txt2)}

/* ── Buttons ── */
.btn{padding:10px 20px;border:none;border-radius:10px;font-size:.84em;font-weight:600;cursor:pointer;transition:all .25s var(--ease);display:inline-flex;align-items:center;gap:8px;outline:none}
.btn:active{transform:scale(.96)}
.btn-pri{background:linear-gradient(135deg,var(--pri),var(--pri-dk));color:#fff;box-shadow:0 4px 16px var(--pri-glow)}
.btn-pri:hover{background:linear-gradient(135deg,var(--pri-lt),var(--pri));box-shadow:0 6px 24px rgba(220,20,60,.35);transform:translateY(-1px)}
.btn-sec{background:rgba(255,255,255,.06);color:var(--txt);border:1px solid var(--brd)}
.btn-sec:hover{background:rgba(255,255,255,.1);border-color:rgba(255,255,255,.15)}
.btn-ok{background:linear-gradient(135deg,#00C853,#00E676);color:#111}
.btn-ok:hover{box-shadow:0 4px 16px rgba(0,200,83,.3);transform:translateY(-1px)}
.btn-err{background:linear-gradient(135deg,var(--err),#D50000);color:#fff}
.btn:disabled{opacity:.4;cursor:not-allowed;transform:none!important;box-shadow:none!important}

/* ── Drop Zone ── */
.drop-zone{border:2px dashed rgba(255,255,255,.1);border-radius:var(--radius);padding:36px 20px;text-align:center;cursor:pointer;transition:all .3s var(--ease)}
.drop-zone:hover,.drop-zone.over{border-color:var(--pri);background:rgba(220,20,60,.04);box-shadow:inset 0 0 40px rgba(220,20,60,.04)}
.drop-zone svg{color:var(--pri);margin-bottom:10px}
.drop-zone p{font-size:.84em;color:var(--txt2)}

/* ── Progress ── */
.progress{display:none;margin-top:12px}
.progress-bar{height:5px;background:rgba(255,255,255,.06);border-radius:3px;overflow:hidden}
.progress-fill{height:100%;width:0;background:linear-gradient(90deg,var(--pri),var(--pri-lt));transition:width .3s;border-radius:3px;box-shadow:0 0 8px var(--pri-glow)}
.progress-text{font-size:.72em;color:var(--txt2);margin-top:5px;text-align:center}

/* ── File List ── */
.file-row{display:flex;justify-content:space-between;align-items:center;padding:12px 14px;border-bottom:1px solid rgba(255,255,255,.04);transition:all .25s var(--ease);border-radius:10px;margin:2px 0}
.file-row:last-child{border-bottom:none}
.file-row:hover{background:rgba(220,20,60,.04);transform:translateX(4px)}
.file-info{display:flex;align-items:center;gap:12px;flex:1;min-width:0}
.fi-icon{width:38px;height:38px;border-radius:10px;background:rgba(220,20,60,.1);display:flex;align-items:center;justify-content:center;flex-shrink:0;color:var(--pri)}
.file-name{font-size:.86em;font-weight:500;word-break:break-all;color:var(--txt)}
.file-meta{font-size:.7em;color:var(--txt2);margin-top:2px}
.file-actions{display:flex;gap:4px;flex-shrink:0}
.file-actions button{width:34px;height:34px;border:none;border-radius:8px;cursor:pointer;display:flex;align-items:center;justify-content:center;background:transparent;color:var(--txt2);transition:all .2s var(--ease)}
.file-actions button:hover{background:rgba(220,20,60,.12);color:var(--pri);transform:scale(1.1)}
.file-actions button.del:hover{background:var(--err-bg);color:var(--err)}

/* ── Editor ── */
.ed-hdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:14px;flex-wrap:wrap;gap:10px}
.ed-fname{font-size:.9em;font-weight:600;color:var(--pri);display:flex;align-items:center;gap:8px}
.ed-acts{display:flex;gap:8px}
.ed-wrap{position:relative;display:flex;height:55vh;min-height:240px;border:1px solid var(--brd);border-radius:10px;overflow:hidden;background:rgba(0,0,0,.3)}
.line-nums{background:rgba(255,255,255,.02);color:var(--txt3);padding:14px 10px;font:13px/1.5 'Consolas','Courier New',monospace;text-align:right;user-select:none;overflow:hidden;min-width:46px;border-right:1px solid var(--brd)}
.ed-area{flex:1;resize:none;border:none;padding:14px;font:13px/1.5 'Consolas','Courier New',monospace;outline:none;background:transparent;color:var(--txt);tab-size:4;caret-color:var(--pri)}
.line-hl{position:absolute;left:46px;right:0;height:19.5px;background:rgba(220,20,60,.06);pointer-events:none;transition:top .05s;border-left:2px solid rgba(220,20,60,.4)}
.ed-foot{display:flex;justify-content:space-between;margin-top:10px;font-size:.73em;color:var(--txt2)}

/* ── Info Rows ── */
.info-row{display:flex;justify-content:space-between;padding:11px 0;border-bottom:1px solid rgba(255,255,255,.04);font-size:.86em}
.info-row:last-child{border-bottom:none}
.info-label{color:var(--txt2)}
.info-value{font-weight:600;color:var(--txt)}

/* ── Accordion ── */
.acc-item{border-bottom:1px solid var(--brd)}
.acc-item:last-child{border-bottom:none}
.acc-btn{width:100%;padding:16px 4px;border:none;background:none;text-align:left;font-size:.88em;font-weight:600;color:var(--txt);cursor:pointer;display:flex;justify-content:space-between;align-items:center;transition:color .2s;gap:12px}
.acc-btn:hover{color:var(--pri)}
.acc-btn svg{transition:transform .3s;flex-shrink:0;color:var(--txt2);width:18px;height:18px}
.acc-btn.open svg{transform:rotate(180deg);color:var(--pri)}
.acc-body{max-height:0;overflow:hidden;transition:max-height .45s ease;font-size:.82em;line-height:1.8;color:var(--txt2)}
.acc-body.open{max-height:900px}
.acc-body p{padding:0 4px 12px}
.acc-body strong{color:var(--txt)}
.acc-illust{text-align:center;padding:16px 0 8px}
.acc-illust svg{color:var(--pri);opacity:.7}

/* ── Toast ── */
.toast-box{position:fixed;top:16px;right:16px;z-index:500;display:flex;flex-direction:column;gap:8px;pointer-events:none}
.toast{padding:14px 20px;border-radius:12px;font-size:.82em;font-weight:500;box-shadow:0 8px 32px rgba(0,0,0,.4);animation:toastIn .4s var(--ease);display:flex;align-items:center;gap:10px;max-width:340px;pointer-events:auto;backdrop-filter:blur(16px);-webkit-backdrop-filter:blur(16px)}
.toast.ok{background:rgba(0,230,118,.12);color:var(--ok);border:1px solid rgba(0,230,118,.2)}
.toast.err{background:rgba(255,23,68,.12);color:var(--err);border:1px solid rgba(255,23,68,.2)}
.toast.info{background:rgba(220,20,60,.12);color:var(--pri-lt);border:1px solid rgba(220,20,60,.2)}
@keyframes toastIn{from{transform:translateX(120%);opacity:0}to{transform:translateX(0);opacity:1}}

/* ── Modal ── */
.modal-ov{display:none;position:fixed;inset:0;background:rgba(0,0,0,.6);z-index:400;align-items:center;justify-content:center;padding:20px;backdrop-filter:blur(4px);-webkit-backdrop-filter:blur(4px)}
.modal-ov.show{display:flex}
.modal{background:var(--bg2);border:1px solid var(--glass-brd);border-radius:18px;padding:28px;max-width:420px;width:100%;box-shadow:0 24px 64px rgba(0,0,0,.5);animation:modalIn .3s var(--ease)}
@keyframes modalIn{from{transform:scale(.9);opacity:0}to{transform:scale(1);opacity:1}}
.modal h3{margin-bottom:8px;color:var(--txt);font-size:1.05em}
.modal p{color:var(--txt2);margin-bottom:18px;font-size:.86em;line-height:1.5}
.modal input{width:100%;padding:12px 14px;border:1px solid var(--brd);border-radius:10px;font-size:.88em;margin-bottom:18px;outline:none;background:rgba(255,255,255,.04);color:var(--txt);transition:border-color .2s}
.modal input:focus{border-color:var(--pri);box-shadow:0 0 0 3px var(--pri-glow)}
.modal-btns{display:flex;gap:10px;justify-content:flex-end}

/* ── Empty State ── */
.empty-st{text-align:center;color:var(--txt2);padding:36px 16px}
.empty-st svg{margin-bottom:12px;color:var(--txt3)}

/* ── About ── */
.about-hero{text-align:center;padding:44px 20px;background:linear-gradient(135deg,rgba(220,20,60,.08),rgba(220,20,60,.02));border-radius:var(--radius);margin-bottom:20px;border:1px solid var(--glass-brd)}
.about-hero h2{font-size:1.75em;font-weight:800;background:linear-gradient(135deg,var(--pri),var(--pri-lt));-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
.about-hero p{color:var(--txt2);margin-top:8px;font-size:.88em}
.author-card{text-align:center;padding:32px 20px}
.author-av{width:80px;height:80px;border-radius:50%;background:linear-gradient(135deg,var(--pri),var(--pri-dk));display:flex;align-items:center;justify-content:center;margin:0 auto 16px;font-size:1.7em;font-weight:700;color:#fff;box-shadow:0 8px 32px var(--pri-glow)}
.author-name{font-size:1.12em;font-weight:700;color:var(--txt)}
.author-role{font-size:.76em;color:var(--pri);text-transform:uppercase;letter-spacing:1.2px;margin-top:4px}
.author-bio{font-size:.83em;color:var(--txt2);line-height:1.8;margin-top:16px;max-width:520px;margin-left:auto;margin-right:auto}
.spec-tbl{width:100%;border-collapse:collapse}
.spec-tbl td{padding:10px 4px;border-bottom:1px solid rgba(255,255,255,.04);font-size:.84em}
.spec-tbl td:first-child{color:var(--txt2);width:42%}
.spec-tbl td:last-child{font-weight:600;color:var(--txt)}
.spec-tbl tr:last-child td{border-bottom:none}
.feat-list{list-style:none;display:grid;grid-template-columns:1fr 1fr;gap:10px}
.feat-list li{font-size:.83em;color:var(--txt2);display:flex;align-items:center;gap:8px}
.feat-list li svg{color:var(--pri);flex-shrink:0;width:16px;height:16px}
.vision-block{background:linear-gradient(135deg,rgba(220,20,60,.06),rgba(220,20,60,.02));border-left:3px solid var(--pri);border-radius:0 var(--radius) var(--radius) 0;padding:20px 24px;margin-top:16px;font-size:.84em;line-height:1.8;color:var(--txt2);font-style:italic}

/* ── Scrollbar ── */
::-webkit-scrollbar{width:5px;height:5px}
::-webkit-scrollbar-track{background:transparent}
::-webkit-scrollbar-thumb{background:rgba(255,255,255,.1);border-radius:3px}
::-webkit-scrollbar-thumb:hover{background:rgba(255,255,255,.2)}

/* ── Badge ── */
.badge{display:inline-block;padding:3px 10px;border-radius:20px;font-size:.7em;font-weight:600}
.badge-ok{background:var(--ok-bg);color:var(--ok)}
.badge-err{background:var(--err-bg);color:var(--err)}
.badge-pri{background:rgba(220,20,60,.12);color:var(--pri-lt)}

/* ── Responsive ── */
@media(max-width:768px){
.sidebar{transform:translateX(-100%)}
.sidebar.open{transform:translateX(0)}
.sb-bk.show{display:block}
.mob-hdr{display:flex}
.main{margin-left:0;padding:68px 12px 16px}
.stat-grid{grid-template-columns:repeat(2,1fr);gap:10px}
.file-row{flex-wrap:wrap;gap:8px}
.file-actions{width:100%;justify-content:flex-end}
.ed-wrap{height:45vh;min-height:200px}
.toast{max-width:280px}
.modal{padding:20px}
.feat-list{grid-template-columns:1fr}
.about-hero h2{font-size:1.35em}
.about-hero{padding:28px 16px}
}
@media(max-width:420px){
.stat-grid{grid-template-columns:1fr 1fr;gap:8px}
.stat-card{padding:14px}
.st-val{font-size:1.05em}
.pg-hd h2{font-size:1.15em}
}
</style>
</head>
<body>

<!-- ════ Toast Container ════ -->
<div class="toast-box" id="toastBox"></div>

<!-- ════ Modal ════ -->
<div class="modal-ov" id="modalOv" onclick="if(event.target===this)hideModal()">
<div class="modal" id="modal">
<h3 id="mTitle"></h3>
<p id="mMsg"></p>
<input id="mInput" style="display:none" placeholder="Enter filename">
<div class="modal-btns">
<button class="btn btn-sec" onclick="hideModal()">Cancel</button>
<button class="btn btn-pri" id="mOk">Confirm</button>
</div>
</div>
</div>

<!-- ════ Sidebar Backdrop (mobile) ════ -->
<div class="sb-bk" id="sbBk" onclick="closeSb()"></div>

<!-- ════ Sidebar ════ -->
<aside class="sidebar" id="sidebar">
<div class="sb-logo">
<h1>
<svg viewBox="0 0 24 24" width="24" height="24"><path d="M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z" fill="currentColor"/></svg>
D.E.V_Darshan
</h1>
<p>TXT Reader Portal</p>
</div>
<nav class="sb-nav" id="sbNav">
<button class="active" data-tab="dashboard">
<svg viewBox="0 0 24 24"><path d="M3 13h8V3H3v10zm0 8h8v-6H3v6zm10 0h8V11h-8v10zm0-18v6h8V3h-8z" fill="currentColor"/></svg>
Dashboard
</button>
<button data-tab="files">
<svg viewBox="0 0 24 24"><path d="M10 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z" fill="currentColor"/></svg>
Files
</button>
<button data-tab="editor">
<svg viewBox="0 0 24 24"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 000-1.41l-2.34-2.34a1 1 0 00-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" fill="currentColor"/></svg>
Editor
</button>
<button data-tab="settings">
<svg viewBox="0 0 24 24"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58a.49.49 0 00.12-.61l-1.92-3.32a.49.49 0 00-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54a.48.48 0 00-.48-.41h-3.84a.48.48 0 00-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96a.49.49 0 00-.59.22L2.74 8.87a.48.48 0 00.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58a.49.49 0 00-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6A3.6 3.6 0 1112 8.4a3.6 3.6 0 010 7.2z" fill="currentColor"/></svg>
Settings
</button>
<button data-tab="guide">
<svg viewBox="0 0 24 24"><path d="M21 5c-1.11-.35-2.33-.5-3.5-.5-1.95 0-4.05.4-5.5 1.5-1.45-1.1-3.55-1.5-5.5-1.5S2.45 4.9 1 6v14.65c0 .25.25.5.5.5.1 0 .15-.05.25-.05C3.1 20.45 5.05 20 6.5 20c1.95 0 4.05.4 5.5 1.5 1.35-.85 3.8-1.5 5.5-1.5 1.65 0 3.35.3 4.75 1.05.1.05.15.05.25.05.25 0 .5-.25.5-.5V6c-.6-.45-1.25-.75-2-1zm0 13.5c-1.1-.35-2.3-.5-3.5-.5-1.7 0-4.15.65-5.5 1.5V8c1.35-.85 3.8-1.5 5.5-1.5 1.2 0 2.4.15 3.5.5v11.5z" fill="currentColor"/></svg>
Guide
</button>
<button data-tab="about">
<svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z" fill="currentColor"/></svg>
About
</button>
</nav>
<div class="sb-footer">
D.E.V_Darshan <span>v1.0</span><br>
&copy; Sakshyam Bastakoti
</div>
</aside>

<!-- ════ Mobile Header ════ -->
<header class="mob-hdr" id="mobHdr">
<button class="ham" id="hamBtn" onclick="toggleSb()">
<svg viewBox="0 0 24 24" width="22" height="22"><path d="M3 18h18v-2H3v2zm0-5h18v-2H3v2zm0-7v2h18V6H3z" fill="currentColor"/></svg>
</button>
<span class="mob-title">
<svg viewBox="0 0 24 24" width="20" height="20"><path d="M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z" fill="currentColor"/></svg>
D.E.V_Darshan
</span>
</header>

<!-- ════════════════════════════════════════════════════════════════════════ -->
<!--  MAIN CONTENT                                                          -->
<!-- ════════════════════════════════════════════════════════════════════════ -->
<div class="main" id="mainArea">

<!-- ══════════ DASHBOARD ══════════ -->
<section id="tab-dashboard" class="tab-panel active">
<div class="pg-hd">
<h2>
<svg viewBox="0 0 24 24" width="26" height="26"><path d="M3 13h8V3H3v10zm0 8h8v-6H3v6zm10 0h8V11h-8v10zm0-18v6h8V3h-8z" fill="currentColor"/></svg>
Dashboard
</h2>
<p>Device status and system overview</p>
</div>

<div class="stat-grid" id="dashGrid">
<div class="stat-card">
<div class="st-icon red"><svg viewBox="0 0 24 24" width="20" height="20"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z" fill="currentColor"/></svg></div>
<div class="st-lbl">Firmware</div>
<div class="st-val" id="dFw">—</div>
</div>
<div class="stat-card">
<div class="st-icon red"><svg viewBox="0 0 24 24" width="20" height="20"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13z" fill="currentColor"/></svg></div>
<div class="st-lbl">Total Files</div>
<div class="st-val" id="dFiles">—</div>
</div>
<div class="stat-card">
<div class="st-icon amb"><svg viewBox="0 0 24 24" width="20" height="20"><path d="M2 20h20v-4H2v4zm2-3h2v2H4v-2zM2 4v4h20V4H2zm4 3H4V5h2v2zm-4 7h20v-4H2v4zm2-3h2v2H4v-2z" fill="currentColor"/></svg></div>
<div class="st-lbl">Storage</div>
<div class="st-val" id="dStorage">—</div>
</div>
<div class="stat-card">
<div class="st-icon grn"><svg viewBox="0 0 24 24" width="20" height="20"><path d="M2 20h20v-4H2v4zm2-3h2v2H4v-2zM2 4v4h20V4H2zm4 3H4V5h2v2zm-4 7h20v-4H2v4zm2-3h2v2H4v-2z" fill="currentColor"/></svg></div>
<div class="st-lbl">SD Health</div>
<div class="st-val" id="dSd">—</div>
</div>
<div class="stat-card">
<div class="st-icon blu"><svg viewBox="0 0 24 24" width="20" height="20"><path d="M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3a4.24 4.24 0 00-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z" fill="currentColor"/></svg></div>
<div class="st-lbl">WiFi Status</div>
<div class="st-val" id="dWifi">—</div>
</div>
<div class="stat-card">
<div class="st-icon red"><svg viewBox="0 0 24 24" width="20" height="20"><path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z" fill="currentColor"/></svg></div>
<div class="st-lbl">Uptime</div>
<div class="st-val" id="dUp">—</div>
</div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M2 20h20v-4H2v4zm2-3h2v2H4v-2zM2 4v4h20V4H2zm4 3H4V5h2v2zm-4 7h20v-4H2v4zm2-3h2v2H4v-2z" fill="currentColor"/></svg>
Storage Overview
</h3>
<div class="storage-bar"><div class="storage-fill" id="dsFill" style="width:0"></div></div>
<div class="storage-text">
<span id="dsUsed">Calculating...</span>
<span id="dsFree">&nbsp;</span>
</div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z" fill="currentColor"/></svg>
System Information
</h3>
<div id="dashInfo"><div class="empty-st"><p>Loading device data...</p></div></div>
</div>
</section>

<!-- ══════════ FILES ══════════ -->
<section id="tab-files" class="tab-panel">
<div class="pg-hd">
<h2>
<svg viewBox="0 0 24 24" width="26" height="26"><path d="M10 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z" fill="currentColor"/></svg>
File Manager
</h2>
<p>Upload, manage and organise your text files</p>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M19.35 10.04C18.67 6.59 15.64 4 12 4 9.11 4 6.6 5.64 5.35 8.04 2.34 8.36 0 10.91 0 14c0 3.31 2.69 6 6 6h13c2.76 0 5-2.24 5-5 0-2.64-2.05-4.78-4.65-4.96zM14 13v4h-4v-4H7l5-5 5 5h-3z" fill="currentColor"/></svg>
Upload File
</h3>
<div class="drop-zone" id="dropZone">
<svg viewBox="0 0 24 24" width="44" height="44"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm4 18H6V4h7v5h5v11zM8 15.01l1.41 1.41L11 14.84V19h2v-4.16l1.59 1.59L16 15.01 12.01 11 8 15.01z" fill="currentColor"/></svg>
<p>Tap to select or drag &amp; drop a .txt file</p>
<p style="font-size:.68em;color:var(--txt3);margin-top:6px">Max 2 MB &middot; .txt files only</p>
</div>
<input type="file" id="fileInput" accept=".txt,text/plain" hidden>
<div id="selFile" style="font-size:.78em;color:var(--txt2);margin-top:10px;word-break:break-all"></div>
<button class="btn btn-pri" id="uploadBtn" disabled style="width:100%;margin-top:12px;justify-content:center">
<svg viewBox="0 0 24 24" width="16" height="16"><path d="M19.35 10.04C18.67 6.59 15.64 4 12 4 9.11 4 6.6 5.64 5.35 8.04 2.34 8.36 0 10.91 0 14c0 3.31 2.69 6 6 6h13c2.76 0 5-2.24 5-5 0-2.64-2.05-4.78-4.65-4.96zM14 13v4h-4v-4H7l5-5 5 5h-3z" fill="currentColor"/></svg>
Upload
</button>
<div class="progress" id="progress">
<div class="progress-bar"><div class="progress-fill" id="progressFill"></div></div>
<div class="progress-text" id="progressText">0%</div>
</div>
</div>

<div class="card">
<h3 style="justify-content:space-between">
<span style="display:flex;align-items:center;gap:10px">
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13z" fill="currentColor"/></svg>
Files on SD Card
</span>
<button class="btn btn-sec" id="newFileBtn" style="padding:7px 14px;font-size:.78em">
<svg viewBox="0 0 24 24" width="14" height="14"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" fill="currentColor"/></svg>
New File
</button>
</h3>
<div id="fileList" style="max-height:440px;overflow-y:auto">
<div class="empty-st"><p>Loading files...</p></div>
</div>
</div>
</section>

<!-- ══════════ EDITOR ══════════ -->
<section id="tab-editor" class="tab-panel">
<div class="pg-hd">
<h2>
<svg viewBox="0 0 24 24" width="26" height="26"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 000-1.41l-2.34-2.34a1 1 0 00-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" fill="currentColor"/></svg>
Text Editor
</h2>
<p>Edit files directly in the browser with real-time preview</p>
</div>

<div class="card">
<div class="ed-hdr">
<span class="ed-fname" id="edFname">
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13z" fill="currentColor"/></svg>
<span id="fnTxt">No file open</span>
</span>
<div class="ed-acts" id="edActs" style="display:none">
<button class="btn btn-ok" id="saveBtn">
<svg viewBox="0 0 24 24" width="16" height="16"><path d="M17 3H5a2 2 0 00-2 2v14a2 2 0 002 2h14a2 2 0 002-2V7l-4-4zm-5 16a3 3 0 110-6 3 3 0 010 6zm3-10H5V5h10v4z" fill="currentColor"/></svg>
Save
</button>
<button class="btn btn-sec" id="cancelBtn">
<svg viewBox="0 0 24 24" width="16" height="16"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" fill="currentColor"/></svg>
Cancel
</button>
</div>
</div>
<div id="edBody" style="display:none">
<div class="ed-wrap" id="edWrap">
<div class="line-hl" id="lineHl" style="top:14px"></div>
<div class="line-nums" id="lineNums">1</div>
<textarea class="ed-area" id="edArea" spellcheck="false" placeholder="Start typing..."></textarea>
</div>
</div>
<div class="ed-foot" id="edFoot" style="display:none">
<span id="charCnt">0 characters</span>
<span id="edStatus"></span>
</div>
<div id="edEmpty" class="empty-st">
<svg viewBox="0 0 24 24" width="52" height="52"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 000-1.41l-2.34-2.34a1 1 0 00-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" fill="currentColor"/></svg>
<p style="margin-top:10px;font-size:.88em">Select a file from <strong>Files</strong> to begin editing</p>
<p style="font-size:.76em;margin-top:6px">Or create a new file using the <strong>New File</strong> button</p>
</div>
</div>
</section>

<!-- ══════════ SETTINGS ══════════ -->
<section id="tab-settings" class="tab-panel">
<div class="pg-hd">
<h2>
<svg viewBox="0 0 24 24" width="26" height="26"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58a.49.49 0 00.12-.61l-1.92-3.32a.49.49 0 00-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54a.48.48 0 00-.48-.41h-3.84a.48.48 0 00-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96a.49.49 0 00-.59.22L2.74 8.87a.48.48 0 00.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58a.49.49 0 00-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6A3.6 3.6 0 1112 8.4a3.6 3.6 0 010 7.2z" fill="currentColor"/></svg>
Settings
</h2>
<p>Device configuration and preferences</p>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3a4.24 4.24 0 00-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z" fill="currentColor"/></svg>
WiFi Configuration
</h3>
<div id="wifiInfo"><div class="empty-st"><p>Loading...</p></div></div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58a.49.49 0 00.12-.61l-1.92-3.32a.49.49 0 00-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54a.48.48 0 00-.48-.41h-3.84a.48.48 0 00-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96a.49.49 0 00-.59.22L2.74 8.87a.48.48 0 00.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58a.49.49 0 00-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6A3.6 3.6 0 1112 8.4a3.6 3.6 0 010 7.2z" fill="currentColor"/></svg>
Reading Behaviour
</h3>
<div id="readInfo">
<div class="info-row"><span class="info-label">Characters per Line</span><span class="info-value">21</span></div>
<div class="info-row"><span class="info-label">Display Lines</span><span class="info-value">4</span></div>
<div class="info-row"><span class="info-label">Bookmark Auto-save</span><span class="info-value">Every 10 lines</span></div>
<div class="info-row"><span class="info-label">Sleep Timeout</span><span class="info-value">5 minutes</span></div>
<div class="info-row"><span class="info-label">Max File Size (Editor)</span><span class="info-value">64 KB</span></div>
<div class="info-row"><span class="info-label">Max Upload Size</span><span class="info-value">2 MB</span></div>
</div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M18 2h-8L4.02 8 4 20c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 18H6v-8h12v8z" fill="currentColor"/></svg>
Storage Details
</h3>
<div id="storageInfo"><div class="empty-st"><p>Loading...</p></div></div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M15.67 4H14V2h-4v2H8.33C7.6 4 7 4.6 7 5.33v15.34C7 21.4 7.6 22 8.33 22h7.34c.73 0 1.33-.6 1.33-1.33V5.33C17 4.6 16.4 4 15.67 4z" fill="currentColor"/></svg>
System Resources
</h3>
<div id="sysInfo"><div class="empty-st"><p>Loading...</p></div></div>
</div>
</section>

<!-- ══════════ GUIDE ══════════ -->
<section id="tab-guide" class="tab-panel">
<div class="pg-hd">
<h2>
<svg viewBox="0 0 24 24" width="26" height="26"><path d="M21 5c-1.11-.35-2.33-.5-3.5-.5-1.95 0-4.05.4-5.5 1.5-1.45-1.1-3.55-1.5-5.5-1.5S2.45 4.9 1 6v14.65c0 .25.25.5.5.5.1 0 .15-.05.25-.05C3.1 20.45 5.05 20 6.5 20c1.95 0 4.05.4 5.5 1.5 1.35-.85 3.8-1.5 5.5-1.5 1.65 0 3.35.3 4.75 1.05.1.05.15.05.25.05.25 0 .5-.25.5-.5V6c-.6-.45-1.25-.75-2-1zm0 13.5c-1.1-.35-2.3-.5-3.5-.5-1.7 0-4.15.65-5.5 1.5V8c1.35-.85 3.8-1.5 5.5-1.5 1.2 0 2.4.15 3.5.5v11.5z" fill="currentColor"/></svg>
User Guide
</h2>
<p>Complete documentation for your D.E.V_Darshan reader</p>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z" fill="currentColor"/></svg>
Documentation
</h3>

<div class="acc-item">
<button class="acc-btn" onclick="toggleAcc(this)">
<span>Getting Started — WiFi Connection</span>
<svg viewBox="0 0 24 24"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="acc-body">
<div class="acc-illust">
<svg viewBox="0 0 120 60" width="120" height="60"><rect x="35" y="8" width="50" height="44" rx="6" fill="none" stroke="currentColor" stroke-width="2"/><rect x="40" y="14" width="40" height="28" rx="2" fill="rgba(220,20,60,0.1)" stroke="currentColor" stroke-width="1"/><circle cx="60" cy="50" r="2" fill="currentColor"/><path d="M60 4c-6 0-11 2-15 5l3 3c3-2 7-4 12-4s9 2 12 4l3-3c-4-3-9-5-15-5z" fill="currentColor" opacity=".4"/><path d="M60 10c-4 0-8 1.5-11 4l3 3c2-2 5-3 8-3s6 1 8 3l3-3c-3-2.5-7-4-11-4z" fill="currentColor" opacity=".7"/><path d="M60 17c-2.5 0-5 1-7 3l3 3c1-1 2.5-2 4-2s3 1 4 2l3-3c-2-2-4.5-3-7-3z" fill="currentColor"/></svg>
</div>
<p><strong>Step 1:</strong> On the device Home screen, navigate to <strong>WiFi Portal</strong> and press SELECT to confirm.</p>
<p><strong>Step 2:</strong> The OLED display will show the WiFi network name and IP address. Connect your phone or computer to the displayed WiFi network.</p>
<p><strong>Step 3:</strong> Open any web browser and go to <strong>192.168.4.1</strong></p>
<p style="padding-bottom:0"><strong>Tip:</strong> The portal will auto-shutdown after 5 minutes of inactivity to conserve battery.</p>
</div>
</div>

<div class="acc-item">
<button class="acc-btn" onclick="toggleAcc(this)">
<span>Uploading &amp; Managing Files</span>
<svg viewBox="0 0 24 24"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="acc-body">
<div class="acc-illust">
<svg viewBox="0 0 120 60" width="120" height="60"><rect x="20" y="10" width="80" height="45" rx="4" fill="none" stroke="currentColor" stroke-width="2" stroke-dasharray="6,3"/><path d="M60 38V20m-8 8l8-8 8 8" stroke="currentColor" stroke-width="2.5" fill="none" stroke-linecap="round" stroke-linejoin="round"/><text x="60" y="50" text-anchor="middle" fill="currentColor" font-size="7" opacity=".6">.txt</text></svg>
</div>
<p>In the <strong>Files</strong> tab, use the Upload section. You can tap to select a file or <strong>drag &amp; drop</strong> a .txt file onto the upload area.</p>
<p>Press <strong>Upload</strong> and watch the progress bar. A toast notification confirms success.</p>
<p>Use the action buttons on each file row to <strong>Edit</strong> (pencil icon), <strong>Rename</strong> (text icon), <strong>Download</strong> (arrow icon), or <strong>Delete</strong> (trash icon).</p>
<p style="padding-bottom:0"><strong>Note:</strong> Only <strong>.txt</strong> files up to <strong>2 MB</strong> are accepted. Filenames are validated server-side to prevent invalid characters.</p>
</div>
</div>

<div class="acc-item">
<button class="acc-btn" onclick="toggleAcc(this)">
<span>Using the Text Editor</span>
<svg viewBox="0 0 24 24"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="acc-body">
<div class="acc-illust">
<svg viewBox="0 0 120 60" width="120" height="60"><rect x="15" y="5" width="90" height="50" rx="4" fill="none" stroke="currentColor" stroke-width="2"/><line x1="30" y1="5" x2="30" y2="55" stroke="currentColor" stroke-width="1" opacity=".3"/><text x="23" y="18" text-anchor="middle" fill="currentColor" font-size="6" opacity=".5">1</text><text x="23" y="28" text-anchor="middle" fill="currentColor" font-size="6" opacity=".5">2</text><text x="23" y="38" text-anchor="middle" fill="currentColor" font-size="6" opacity=".5">3</text><rect x="30" y="21" width="75" height="10" fill="rgba(220,20,60,0.08)"/><line x1="35" y1="16" x2="85" y2="16" stroke="currentColor" stroke-width="1" opacity=".3"/><line x1="35" y1="26" x2="95" y2="26" stroke="currentColor" stroke-width="1" opacity=".5"/><line x1="35" y1="36" x2="75" y2="36" stroke="currentColor" stroke-width="1" opacity=".3"/></svg>
</div>
<p>Click the <strong>pencil icon</strong> next to any file in the Files list. The content loads in the <strong>Editor</strong> tab with a monospace font, line numbers, and active-line highlighting.</p>
<p>The editor shows a live <strong>character and line counter</strong> at the bottom. Click <strong>Save</strong> to write changes to the SD card, or <strong>Cancel</strong> to discard.</p>
<p>Use <strong>Ctrl+S</strong> (or Cmd+S) as a keyboard shortcut to save.</p>
<p style="padding-bottom:0"><strong>Limit:</strong> Web editing supports files up to <strong>64 KB</strong>. For larger files, download, edit locally, and re-upload.</p>
</div>
</div>

<div class="acc-item">
<button class="acc-btn" onclick="toggleAcc(this)">
<span>Hardware Button Controls</span>
<svg viewBox="0 0 24 24"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="acc-body">
<div class="acc-illust">
<svg viewBox="0 0 120 60" width="120" height="60"><rect x="10" y="15" width="30" height="30" rx="15" fill="none" stroke="currentColor" stroke-width="2"/><text x="25" y="34" text-anchor="middle" fill="currentColor" font-size="8" font-weight="bold">UP</text><rect x="45" y="15" width="30" height="30" rx="15" fill="none" stroke="currentColor" stroke-width="2"/><text x="60" y="34" text-anchor="middle" fill="currentColor" font-size="7" font-weight="bold">DOWN</text><rect x="80" y="15" width="30" height="30" rx="15" fill="rgba(220,20,60,0.15)" stroke="currentColor" stroke-width="2"/><text x="95" y="34" text-anchor="middle" fill="currentColor" font-size="6" font-weight="bold">SELECT</text></svg>
</div>
<p><strong>UP Button (GPIO13):</strong> Navigate up through menus. In reading mode, short press scrolls up by one page; hold for continuous line-by-line scrolling.</p>
<p><strong>DOWN Button (GPIO0):</strong> Navigate down through menus. In reading mode, short press scrolls down by one page; hold for continuous scrolling. <em>Do not hold during power-on (boot pin).</em></p>
<p><strong>SELECT Button (GPIO12):</strong> Context-aware action. In menus, it enters the selected item. In reading mode, it acts as Back.</p>
<p style="padding-bottom:0"><strong>Flow:</strong> Home &rarr; WiFi Portal / Files / Settings. Files &rarr; Select file &rarr; Reading mode. SELECT always returns to the previous screen.</p>
</div>
</div>

<div class="acc-item">
<button class="acc-btn" onclick="toggleAcc(this)">
<span>Power Management &amp; Sleep</span>
<svg viewBox="0 0 24 24"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="acc-body">
<div class="acc-illust">
<svg viewBox="0 0 120 60" width="120" height="60"><rect x="40" y="5" width="40" height="50" rx="4" fill="none" stroke="currentColor" stroke-width="2"/><rect x="52" y="2" width="16" height="4" rx="1" fill="currentColor"/><rect x="44" y="22" width="32" height="28" rx="1" fill="rgba(0,230,118,0.15)"/><path d="M60 28v12m-4-6h8" stroke="currentColor" stroke-width="2" stroke-linecap="round" opacity=".6"/><text x="60" y="18" text-anchor="middle" fill="currentColor" font-size="6" opacity=".7">3.7V</text></svg>
</div>
<p><strong>Battery:</strong> 3.7V 1100mAh Li-ion cell with TP4056 USB charging. Red LED = charging, Blue/Green LED = fully charged.</p>
<p><strong>Auto Sleep:</strong> After 5 minutes of inactivity, the device enters light sleep. Display turns off, CPU power draw is minimised.</p>
<p><strong>Wake Up:</strong> Press any button (UP, DOWN, or SELECT) to instantly resume from where you left off.</p>
<p><strong>WiFi Power:</strong> WiFi and Bluetooth radios are fully disabled during reading. They activate only when entering WiFi Portal from the Home menu.</p>
<p style="padding-bottom:0"><strong>Magnetic Switch:</strong> The magnetic reed switch provides hardware power cutoff &mdash; zero standby drain, ideal for long-term storage.</p>
</div>
</div>

</div>
</section>

<!-- ══════════ ABOUT ══════════ -->
<section id="tab-about" class="tab-panel">
<div class="pg-hd">
<h2>
<svg viewBox="0 0 24 24" width="26" height="26"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z" fill="currentColor"/></svg>
About
</h2>
<p>Device documentation and project information</p>
</div>

<div class="about-hero">
<h2>D.E.V_Darshan</h2>
<p>Ultra-Compact Offline TXT Reader &middot; Engineered for Simplicity</p>
<div style="margin-top:14px">
<span class="badge badge-pri">ESP32-CAM</span>&nbsp;
<span class="badge badge-pri">SD Card</span>&nbsp;
<span class="badge badge-pri">OLED</span>&nbsp;
<span class="badge badge-pri">WiFi Portal</span>
</div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M4 8h4V4H4v4zm6 12h4v-4h-4v4zm-6 0h4v-4H4v4zm0-6h4v-4H4v4zm6 0h4v-4h-4v4zm6-10v4h4V4h-4zm-6 4h4V4h-4v4zm6 6h4v-4h-4v4zm0 6h4v-4h-4v4z" fill="currentColor"/></svg>
Technical Specifications
</h3>
<table class="spec-tbl">
<tr><td>Microcontroller</td><td>ESP32-S (Dual-core Xtensa LX6)</td></tr>
<tr><td>Platform</td><td>ESP32-CAM AI Thinker</td></tr>
<tr><td>Display</td><td>0.91" SSD1306 OLED (128&times;32)</td></tr>
<tr><td>Interface</td><td>I2C (GPIO1/GPIO3)</td></tr>
<tr><td>Storage</td><td>SD Card (SD_MMC 1-bit mode)</td></tr>
<tr><td>Battery</td><td>3.7V 1100mAh Li-ion</td></tr>
<tr><td>Charging</td><td>TP4056 USB Module</td></tr>
<tr><td>Power Switch</td><td>Magnetic Reed Switch</td></tr>
<tr><td>Buttons</td><td>3&times; Tactile (UP, DOWN, SELECT)</td></tr>
<tr><td>WiFi</td><td>802.11 b/g/n (AP mode, 2.4 GHz)</td></tr>
<tr><td>CPU Frequency</td><td>80 MHz (reading) / 240 MHz (WiFi)</td></tr>
<tr><td>Firmware</td><td>v1.0 &middot; Arduino Framework</td></tr>
</table>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M22 9V7h-2V5c0-1.1-.9-2-2-2H4c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2v-2h2v-2h-2v-2h2v-2h-2V9h2zm-4 10H4V5h14v14zM6 13h5v4H6v-4zm6-6h4v3h-4V7zM6 7h5v5H6V7zm6 4h4v6h-4v-6z" fill="currentColor"/></svg>
Hardware Overview
</h3>
<p style="font-size:.84em;color:var(--txt2);line-height:1.8;margin-bottom:14px">
The D.E.V_Darshan repurposes the ESP32-CAM module&rsquo;s built-in SD card slot for text file storage, eliminating the need for external SD breakout boards. The camera hardware is unused &mdash; GPIO pins normally reserved for the camera are reassigned to buttons and I2C for the OLED display.
</p>
<div class="info-row"><span class="info-label">GPIO0</span><span class="info-value">BTN_DOWN (internal pull-up)</span></div>
<div class="info-row"><span class="info-label">GPIO1 / GPIO3</span><span class="info-value">I2C SCL / SDA (OLED)</span></div>
<div class="info-row"><span class="info-label">GPIO2 / 14 / 15</span><span class="info-value">SD_MMC (D0, CLK, CMD)</span></div>
<div class="info-row"><span class="info-label">GPIO4</span><span class="info-value">Flash LED (disabled)</span></div>
<div class="info-row"><span class="info-label">GPIO12</span><span class="info-value">BTN_SELECT (internal pull-up)</span></div>
<div class="info-row"><span class="info-label">GPIO13</span><span class="info-value">BTN_UP (internal pull-up)</span></div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M12 17.27L18.18 21l-1.64-7.03L22 9.24l-7.19-.61L12 2 9.19 8.63 2 9.24l5.46 4.73L5.82 21z" fill="currentColor"/></svg>
Features
</h3>
<ul class="feat-list">
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>Offline TXT reading</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>WiFi file management</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>In-browser text editor</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>Drag &amp; drop upload</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>Automatic bookmarking</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>Word-wrap rendering</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>Auto sleep (5 min)</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>Zero standby drain</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>No external resistors</li>
<li><svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/></svg>USB rechargeable</li>
</ul>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M15.67 4H14V2h-4v2H8.33C7.6 4 7 4.6 7 5.33v15.34C7 21.4 7.6 22 8.33 22h7.34c.73 0 1.33-.6 1.33-1.33V5.33C17 4.6 16.4 4 15.67 4z" fill="currentColor"/></svg>
Power Management
</h3>
<div class="info-row"><span class="info-label">Active (Reading)</span><span class="info-value">~80 MHz, WiFi/BT off</span></div>
<div class="info-row"><span class="info-label">WiFi Active</span><span class="info-value">240 MHz, AP mode</span></div>
<div class="info-row"><span class="info-label">Light Sleep</span><span class="info-value">Display off, minimal draw</span></div>
<div class="info-row"><span class="info-label">Power Off</span><span class="info-value">Magnetic switch, 0 &mu;A</span></div>
<div class="info-row"><span class="info-label">Charging Circuit</span><span class="info-value">TP4056 (1A USB input)</span></div>
<div class="info-row"><span class="info-label">Battery Protection</span><span class="info-value">Over-charge / over-discharge</span></div>
</div>

<div class="card">
<div class="author-card">
<div class="author-av">SB</div>
<div class="author-name">Sakshyam Bastakoti</div>
<div class="author-role">Creator &amp; Developer</div>
<div class="author-bio">
Designer and engineer behind D.E.V_Darshan &mdash; a passion project born from the desire to create a truly minimal, distraction-free reading device. Built entirely from accessible components with zero external resistors, this project demonstrates that powerful embedded systems can be both elegant and simple.
</div>
<div class="vision-block">
<strong style="color:var(--pri);">&ldquo;</strong> I believe technology should empower focus, not fragment it. D.E.V_Darshan is my answer to endless scrolling &mdash; a device that does one thing exceptionally well: let you read. Every design decision, from the magnetic power switch to the single-bit SD interface, prioritises simplicity, efficiency, and the pure joy of reading. <strong style="color:var(--pri);">&rdquo;</strong>
</div>
<div style="margin-top:20px;display:flex;flex-wrap:wrap;justify-content:center;gap:10px">
<span class="badge badge-pri">Open Hardware</span>
<span class="badge badge-pri">ESP32 Platform</span>
<span class="badge badge-pri">Arduino Framework</span>
<span class="badge badge-pri">MIT License</span>
</div>
</div>
</div>

<div style="text-align:center;padding:24px 16px;font-size:.72em;color:var(--txt3);line-height:1.8;border-top:1px solid var(--brd);margin-top:8px">
<div style="margin-bottom:8px">
<svg viewBox="0 0 24 24" width="18" height="18" style="color:var(--pri);vertical-align:middle"><path d="M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z" fill="currentColor"/></svg>
</div>
D.E.V_Darshan v1.0 &middot; Designed &amp; Engineered by Sakshyam Bastakoti<br>
Built with precision. Powered by passion.
</div>
</section>

</div><!-- /main -->

<!-- ════════════════════════════════════════════════════════════════════════ -->
<!--  JAVASCRIPT                                                             -->
<!-- ════════════════════════════════════════════════════════════════════════ -->
<script>
/* ═══ Constants ═══ */
var MAX_SIZE=2*1024*1024,MAX_EDIT=65536;

/* ═══ State ═══ */
var curFile='',dirty=false,selUpFile=null,origContent='';
var tabNames=['dashboard','files','editor','settings','guide','about'];

/* ═══ Utility ═══ */
function fmt(b){if(b<1024)return b+' B';if(b<1048576)return(b/1024).toFixed(1)+' KB';return(b/1048576).toFixed(1)+' MB'}
function esc(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;')}
function fmtDate(ts){if(!ts||ts<946684800)return'--';var d=new Date(ts*1000);return d.toLocaleDateString(undefined,{year:'numeric',month:'short',day:'numeric'})}
function fmtUp(s){var d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60),sec=s%60;if(d>0)return d+'d '+h+'h '+m+'m';if(h>0)return h+'h '+m+'m '+sec+'s';return m+'m '+sec+'s'}

/* ═══ Toast ═══ */
function toast(msg,type){
var t=document.createElement('div');t.className='toast '+(type||'info');
var icons={ok:'M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z',err:'M12 2C6.47 2 2 6.47 2 12s4.47 10 10 10 10-4.47 10-10S17.53 2 12 2zm5 13.59L15.59 17 12 13.41 8.41 17 7 15.59 10.59 12 7 8.41 8.41 7 12 10.59 15.59 7 17 8.41 13.41 12 17 15.59z',info:'M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z'};
var ic=icons[type]||icons.info;
t.innerHTML='<svg viewBox="0 0 24 24" width="18" height="18" style="flex-shrink:0"><path d="'+ic+'" fill="currentColor"/></svg>'+esc(msg);
document.getElementById('toastBox').appendChild(t);
setTimeout(function(){t.style.opacity='0';t.style.transition='opacity .35s';setTimeout(function(){t.remove()},400)},3500);
}

/* ═══ Modal ═══ */
function showModal(title,msg,showInp,onOk){
document.getElementById('mTitle').textContent=title;
document.getElementById('mMsg').textContent=msg;
var inp=document.getElementById('mInput');
inp.style.display=showInp?'block':'none';inp.value='';
document.getElementById('modalOv').classList.add('show');
if(showInp)setTimeout(function(){inp.focus()},120);
document.getElementById('mOk').onclick=function(){var v=inp.value;hideModal();onOk(v)};
}
function hideModal(){document.getElementById('modalOv').classList.remove('show')}

/* ═══ Sidebar ═══ */
function toggleSb(){
document.getElementById('sidebar').classList.toggle('open');
document.getElementById('sbBk').classList.toggle('show');
}
function closeSb(){
document.getElementById('sidebar').classList.remove('open');
document.getElementById('sbBk').classList.remove('show');
}

/* ═══ Tab Navigation ═══ */
var navBtns=document.querySelectorAll('.sb-nav button');
navBtns.forEach(function(btn){btn.addEventListener('click',function(){showTab(btn.dataset.tab);closeSb()})});

function showTab(name){
if(dirty&&curFile&&name!=='editor'){if(!confirm('You have unsaved changes. Switch tab anyway?'))return}
document.querySelectorAll('.tab-panel').forEach(function(t){t.classList.remove('active')});
navBtns.forEach(function(b){b.classList.remove('active')});
var idx=tabNames.indexOf(name);if(idx>=0)navBtns[idx].classList.add('active');
var el=document.getElementById('tab-'+name);if(el)el.classList.add('active');
if(name==='dashboard')loadDash();
if(name==='files'){loadFiles();loadUsage()}
if(name==='settings')loadSettings();
}

/* ═══ Dashboard ═══ */
function loadDash(){
fetch('/info').then(function(r){return r.json()}).then(function(d){
document.getElementById('dFw').textContent='v'+d.firmware;
document.getElementById('dFiles').textContent=d.files;
var pct=d.totalBytes>0?Math.round(d.usedBytes/d.totalBytes*100):0;
document.getElementById('dStorage').textContent=pct+'%';
document.getElementById('dSd').innerHTML=d.sdMounted?'<span style="color:var(--ok)">Healthy</span>':'<span style="color:var(--err)">Error</span>';
document.getElementById('dWifi').innerHTML='<span style="color:var(--ok)">Active</span>';
document.getElementById('dUp').textContent=fmtUp(d.uptime||0);

document.getElementById('dsFill').style.width=pct+'%';
document.getElementById('dsUsed').textContent='Used: '+fmt(d.usedBytes)+' ('+pct+'%)';
document.getElementById('dsFree').textContent='Free: '+fmt(d.totalBytes-d.usedBytes);

document.getElementById('dashInfo').innerHTML=
'<div class="info-row"><span class="info-label">Device</span><span class="info-value">'+esc(d.device)+'</span></div>'
+'<div class="info-row"><span class="info-label">Firmware</span><span class="info-value">v'+esc(d.firmware)+'</span></div>'
+'<div class="info-row"><span class="info-label">Author</span><span class="info-value">'+esc(d.author)+'</span></div>'
+'<div class="info-row"><span class="info-label">CPU Frequency</span><span class="info-value">'+d.cpuFreq+' MHz</span></div>'
+'<div class="info-row"><span class="info-label">Free Heap RAM</span><span class="info-value">'+fmt(d.freeHeap)+'</span></div>'
+'<div class="info-row"><span class="info-label">WiFi SSID</span><span class="info-value">'+esc(d.ssid)+'</span></div>'
+'<div class="info-row"><span class="info-label">Portal IP</span><span class="info-value">192.168.4.1</span></div>';
}).catch(function(){
document.getElementById('dashInfo').innerHTML='<p style="color:var(--err);text-align:center;padding:12px">Failed to load device data</p>';
});
}

/* ═══ Storage ═══ */
function loadUsage(){
fetch('/usage').then(function(r){return r.json()}).then(function(d){
var pct=d.total>0?Math.round(d.used/d.total*100):0;
document.getElementById('dsFill').style.width=pct+'%';
document.getElementById('dsUsed').textContent='Used: '+fmt(d.used)+' ('+pct+'%)';
document.getElementById('dsFree').textContent='Free: '+fmt(d.free);
}).catch(function(){});
}

/* ═══ File List ═══ */
function loadFiles(){
fetch('/files').then(function(r){return r.json()}).then(function(files){
var el=document.getElementById('fileList');
if(!files.length){el.innerHTML='<div class="empty-st"><svg viewBox="0 0 24 24" width="44" height="44"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13z" fill="currentColor"/></svg><p style="margin-top:10px;font-size:.88em">No .txt files found on SD card</p></div>';return}
el.innerHTML=files.map(function(f){return '<div class="file-row" data-name="'+esc(f.name)+'">'
+'<div class="file-info">'
+'<div class="fi-icon"><svg viewBox="0 0 24 24" width="18" height="18"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13z" fill="currentColor"/></svg></div>'
+'<div style="min-width:0"><div class="file-name">'+esc(f.name)+'</div>'
+'<div class="file-meta">'+fmt(f.size)+' &middot; '+fmtDate(f.modified)+'</div></div>'
+'</div>'
+'<div class="file-actions">'
+'<button data-action="edit" title="Edit"><svg viewBox="0 0 24 24" width="15" height="15"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 000-1.41l-2.34-2.34a1 1 0 00-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" fill="currentColor"/></svg></button>'
+'<button data-action="rename" title="Rename"><svg viewBox="0 0 24 24" width="15" height="15"><path d="M2.5 4v3h5v12h3V7h5V4h-13zm19 5h-9v3h3v7h3v-7h3V9z" fill="currentColor"/></svg></button>'
+'<button data-action="download" title="Download"><svg viewBox="0 0 24 24" width="15" height="15"><path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z" fill="currentColor"/></svg></button>'
+'<button data-action="delete" class="del" title="Delete"><svg viewBox="0 0 24 24" width="15" height="15"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z" fill="currentColor"/></svg></button>'
+'</div></div>'}).join('');
}).catch(function(){
document.getElementById('fileList').innerHTML='<div class="empty-st"><p style="color:var(--err)">Error loading files</p></div>';
});
}

/* ═══ File Actions (event delegation) ═══ */
document.getElementById('fileList').addEventListener('click',function(e){
var btn=e.target.closest('[data-action]');if(!btn)return;
var row=btn.closest('.file-row');if(!row)return;
var name=row.dataset.name;
switch(btn.dataset.action){
case 'edit':editFile(name);break;
case 'rename':renameFile(name);break;
case 'download':downloadFile(name);break;
case 'delete':confirmDel(name);break;
}
});

/* ═══ Upload ═══ */
var dz=document.getElementById('dropZone');
var fi=document.getElementById('fileInput');
var ub=document.getElementById('uploadBtn');

dz.addEventListener('click',function(){fi.click()});
dz.addEventListener('dragover',function(e){e.preventDefault();dz.classList.add('over')});
dz.addEventListener('dragleave',function(){dz.classList.remove('over')});
dz.addEventListener('drop',function(e){e.preventDefault();dz.classList.remove('over');pickFile(e.dataTransfer.files[0])});
fi.addEventListener('change',function(){pickFile(fi.files[0])});

function pickFile(f){
if(!f)return;
if(!f.name.toLowerCase().endsWith('.txt')){toast('Only .txt files are allowed','err');return}
if(f.size>MAX_SIZE){toast('File too large (max 2 MB)','err');return}
if(f.size===0){toast('File is empty','err');return}
selUpFile=f;
document.getElementById('selFile').textContent=f.name+' ('+fmt(f.size)+')';
ub.disabled=false;
}

ub.addEventListener('click',function(){
if(!selUpFile)return;ub.disabled=true;
var prog=document.getElementById('progress');prog.style.display='block';
var xhr=new XMLHttpRequest();xhr.open('POST','/upload');
xhr.upload.onprogress=function(e){
if(e.lengthComputable){var p=Math.round(e.loaded/e.total*100);document.getElementById('progressFill').style.width=p+'%';document.getElementById('progressText').textContent=p+'%'}
};
xhr.onload=function(){
prog.style.display='none';document.getElementById('progressFill').style.width='0';
if(xhr.status===200){toast('File uploaded successfully!','ok');loadFiles();loadUsage()}
else{toast('Upload failed: '+xhr.responseText,'err')}
selUpFile=null;document.getElementById('selFile').textContent='';fi.value='';ub.disabled=true;
};
xhr.onerror=function(){prog.style.display='none';toast('Network error during upload','err');ub.disabled=false};
var fd=new FormData();fd.append('file',selUpFile);xhr.send(fd);
});

/* ═══ Edit File ═══ */
function editFile(name){
showTab('editor');
document.getElementById('fnTxt').textContent=name;
document.getElementById('edEmpty').style.display='none';
document.getElementById('edBody').style.display='block';
document.getElementById('edActs').style.display='flex';
document.getElementById('edFoot').style.display='flex';
var ta=document.getElementById('edArea');
ta.value='Loading file content...';ta.disabled=true;
curFile=name;dirty=false;
document.getElementById('edStatus').textContent='';
fetch('/read?name='+encodeURIComponent(name)).then(function(r){
if(!r.ok)throw new Error(r.statusText);return r.text()
}).then(function(c){
ta.value=c;ta.disabled=false;origContent=c;
updLines();updCount();updHL();ta.focus();
}).catch(function(e){toast('Failed to load: '+e.message,'err');ta.value='';ta.disabled=false});
}

function saveFile(){
if(!curFile)return;
var c=document.getElementById('edArea').value;
if(c.length>MAX_EDIT){toast('File too large for web editor (max 64 KB)','err');return}
var sb=document.getElementById('saveBtn');sb.disabled=true;
fetch('/save?name='+encodeURIComponent(curFile),{method:'POST',headers:{'Content-Type':'text/plain'},body:c})
.then(function(r){if(!r.ok)throw new Error('Save failed');return r.text()})
.then(function(){toast('File saved successfully!','ok');dirty=false;origContent=c;document.getElementById('edStatus').textContent='Saved';sb.disabled=false})
.catch(function(e){toast(e.message,'err');sb.disabled=false});
}

function cancelEdit(){
if(dirty&&!confirm('Discard unsaved changes?'))return;
curFile='';dirty=false;origContent='';
document.getElementById('fnTxt').textContent='No file open';
document.getElementById('edEmpty').style.display='block';
document.getElementById('edBody').style.display='none';
document.getElementById('edActs').style.display='none';
document.getElementById('edFoot').style.display='none';
}
document.getElementById('saveBtn').addEventListener('click',saveFile);
document.getElementById('cancelBtn').addEventListener('click',cancelEdit);

/* ═══ Editor events ═══ */
var edArea=document.getElementById('edArea');
edArea.addEventListener('input',function(){dirty=true;document.getElementById('edStatus').textContent='Modified';updLines();updCount();updHL()});
edArea.addEventListener('scroll',function(){document.getElementById('lineNums').scrollTop=edArea.scrollTop;updHL()});
edArea.addEventListener('click',function(){updHL()});
edArea.addEventListener('keyup',function(e){if(e.key.indexOf('Arrow')===0||e.key==='Home'||e.key==='End')updHL()});
edArea.addEventListener('keydown',function(e){if((e.ctrlKey||e.metaKey)&&e.key==='s'){e.preventDefault();saveFile()}});

function updLines(){
var n=edArea.value.split('\n').length;var s='';for(var i=1;i<=n;i++)s+=i+'\n';
document.getElementById('lineNums').textContent=s;
}
function updCount(){
var v=edArea.value;
document.getElementById('charCnt').textContent=v.length.toLocaleString()+' chars \u00b7 '+v.split('\n').length+' lines';
}
function updHL(){
var lh=19.5;
var pos=edArea.selectionStart;
var line=edArea.value.substr(0,pos).split('\n').length-1;
var top=line*lh-edArea.scrollTop+14;
var hl=document.getElementById('lineHl');
if(top<0||top>edArea.clientHeight){hl.style.display='none'}
else{hl.style.display='block';hl.style.top=top+'px'}
}

/* ═══ Rename ═══ */
function renameFile(name){
var base=name.toLowerCase().endsWith('.txt')?name.slice(0,-4):name;
showModal('Rename File','Enter a new name for "'+name+'"',true,function(nw){
if(!nw||!nw.trim())return;
nw=nw.trim();if(!nw.toLowerCase().endsWith('.txt'))nw+='.txt';
fetch('/rename?old='+encodeURIComponent(name)+'&new='+encodeURIComponent(nw),{method:'POST'})
.then(function(r){if(r.ok){toast('Renamed to '+nw,'ok');loadFiles();if(curFile===name){curFile=nw;document.getElementById('fnTxt').textContent=nw}}
else return r.text().then(function(t){toast('Rename failed: '+t,'err')})})
.catch(function(){toast('Network error','err')});
});
document.getElementById('mInput').value=base;
}

/* ═══ Delete ═══ */
function confirmDel(name){
showModal('Delete File','Are you sure you want to delete "'+name+'"? This cannot be undone.',false,function(){
fetch('/delete?name='+encodeURIComponent(name),{method:'POST'})
.then(function(r){if(r.ok){toast('File deleted','ok');loadFiles();loadUsage();if(curFile===name)cancelEdit()}
else return r.text().then(function(t){toast('Delete failed: '+t,'err')})})
.catch(function(){toast('Network error','err')});
});
}

/* ═══ Download ═══ */
function downloadFile(name){
var a=document.createElement('a');a.href='/download?name='+encodeURIComponent(name);
a.download=name;document.body.appendChild(a);a.click();a.remove();
}

/* ═══ Create New File ═══ */
document.getElementById('newFileBtn').addEventListener('click',function(){
showModal('Create New File','Enter a name for the new text file:',true,function(name){
if(!name||!name.trim())return;
name=name.trim();if(!name.toLowerCase().endsWith('.txt'))name+='.txt';
fetch('/create?name='+encodeURIComponent(name),{method:'POST'})
.then(function(r){if(r.ok){toast('"'+name+'" created!','ok');loadFiles();editFile(name)}
else return r.text().then(function(t){toast('Create failed: '+t,'err')})})
.catch(function(){toast('Network error','err')});
});
});

/* ═══ Settings ═══ */
function loadSettings(){
fetch('/info').then(function(r){return r.json()}).then(function(d){
document.getElementById('wifiInfo').innerHTML=
'<div class="info-row"><span class="info-label">Network SSID</span><span class="info-value">'+esc(d.ssid)+'</span></div>'
+'<div class="info-row"><span class="info-label">Portal IP</span><span class="info-value">192.168.4.1</span></div>'
+'<div class="info-row"><span class="info-label">WiFi Channel</span><span class="info-value">6</span></div>'
+'<div class="info-row"><span class="info-label">Max Upload Size</span><span class="info-value">'+fmt(d.maxUpload)+'</span></div>'
+'<div class="info-row"><span class="info-label">Auto Shutdown</span><span class="info-value">'+Math.round(d.wifiTimeout/60000)+' min inactivity</span></div>';

var totalB=d.totalBytes,usedB=d.usedBytes,freeB=totalB-usedB;
var pct=totalB>0?Math.round(usedB/totalB*100):0;
document.getElementById('storageInfo').innerHTML=
'<div class="info-row"><span class="info-label">Total Capacity</span><span class="info-value">'+fmt(totalB)+'</span></div>'
+'<div class="info-row"><span class="info-label">Used Space</span><span class="info-value">'+fmt(usedB)+' ('+pct+'%)</span></div>'
+'<div class="info-row"><span class="info-label">Available</span><span class="info-value">'+fmt(freeB)+'</span></div>'
+'<div class="storage-bar" style="margin-top:8px"><div class="storage-fill" style="width:'+pct+'%"></div></div>';

document.getElementById('sysInfo').innerHTML=
'<div class="info-row"><span class="info-label">Free Heap RAM</span><span class="info-value">'+fmt(d.freeHeap)+'</span></div>'
+'<div class="info-row"><span class="info-label">CPU Frequency</span><span class="info-value">'+d.cpuFreq+' MHz</span></div>'
+'<div class="info-row"><span class="info-label">SD Card</span><span class="info-value">'+(d.sdMounted?'<span style="color:var(--ok)">Mounted</span>':'<span style="color:var(--err)">Not Found</span>')+'</span></div>'
+'<div class="info-row"><span class="info-label">Uptime</span><span class="info-value">'+fmtUp(d.uptime||0)+'</span></div>';
}).catch(function(){
document.getElementById('wifiInfo').innerHTML='<p style="color:var(--err);text-align:center;padding:12px">Failed to load settings</p>';
});
}

/* ═══ Accordion ═══ */
function toggleAcc(btn){btn.classList.toggle('open');btn.nextElementSibling.classList.toggle('open')}

/* ═══ Protection ═══ */
window.addEventListener('beforeunload',function(e){if(dirty){e.preventDefault();e.returnValue=''}});
document.getElementById('mInput').addEventListener('keydown',function(e){if(e.key==='Enter'){e.preventDefault();document.getElementById('mOk').click()}});

/* ═══ Init ═══ */
loadDash();
</script>
</body>
</html>
)rawliteral";

#endif // PORTAL_H
