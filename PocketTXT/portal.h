/*
 * ============================================================================
 *  D.E.V_Darshan — WiFi Portal HTML (PROGMEM)
 * ============================================================================
 *  Full-featured, mobile-responsive web portal for file management,
 *  editing, device settings, and user guide.
 *
 *  Stored in flash (PROGMEM) to minimize RAM usage.
 *
 *  Features:
 *    - Professional re-themed UI (Indigo/Cyan palette)
 *    - Tabbed navigation: Files, Editor, Settings, Guide
 *    - File management: Upload, Rename, Delete, Download, Create
 *    - Web-based text editor with line numbers
 *    - SD card storage visualization
 *    - Device info & settings display
 *    - Collapsible user guide
 *    - Toast notifications & confirmation modals
 *    - Fully responsive (mobile + desktop)
 *    - All inline SVG icons — no external assets
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
<title>D.E.V_Darshan</title>
<style>
:root{--bg:#F5F5F5;--card:#FFF;--pri:#3F51B5;--pri-dk:#303F9F;--sec:#00BCD4;--txt:#212121;--txt2:#757575;--brd:#E0E0E0;--ok:#4CAF50;--err:#F44336;--warn:#FF9800;--shd:0 2px 8px rgba(0,0,0,.1)}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',-apple-system,BlinkMacSystemFont,sans-serif;background:var(--bg);color:var(--txt);min-height:100vh}
header{background:linear-gradient(135deg,var(--pri),var(--pri-dk));color:#fff;padding:18px 16px 14px;text-align:center}
header h1{font-size:1.4em;margin-bottom:2px;display:flex;align-items:center;justify-content:center;gap:8px}
header p{font-size:.78em;opacity:.8;letter-spacing:.5px}
nav{display:flex;background:#fff;border-bottom:2px solid var(--brd);position:sticky;top:0;z-index:100;box-shadow:0 2px 4px rgba(0,0,0,.06)}
nav button{flex:1;padding:11px 6px;border:none;background:var(--brd);color:var(--txt);font-size:.78em;font-weight:600;cursor:pointer;transition:all .25s;display:flex;align-items:center;justify-content:center;gap:5px;border-bottom:3px solid transparent}
nav button.active{background:var(--pri);color:#fff;border-bottom-color:var(--sec)}
nav button:hover:not(.active){background:var(--sec);color:#fff}
nav button svg{flex-shrink:0}
.card{background:var(--card);border-radius:12px;padding:16px;margin:12px;box-shadow:var(--shd);transition:box-shadow .2s}
.card:hover{box-shadow:0 4px 16px rgba(0,0,0,.12)}
.card h3{font-size:.95em;color:var(--pri);margin-bottom:12px;display:flex;align-items:center;gap:8px}
.tab-content{display:none;animation:fadeIn .35s ease}
.tab-content.active{display:block}
@keyframes fadeIn{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:translateY(0)}}
.storage-bar{height:10px;background:var(--brd);border-radius:5px;overflow:hidden;margin:8px 0}
.storage-fill{height:100%;background:linear-gradient(90deg,var(--pri),var(--sec));border-radius:5px;transition:width .6s ease}
.storage-text{display:flex;justify-content:space-between;font-size:.75em;color:var(--txt2)}
.drop-zone{border:2px dashed var(--brd);border-radius:10px;padding:28px 16px;text-align:center;cursor:pointer;transition:all .3s}
.drop-zone:hover,.drop-zone.over{border-color:var(--sec);background:rgba(0,188,212,.05)}
.drop-zone p{font-size:.85em;color:var(--txt2)}
.btn{padding:10px 18px;border:none;border-radius:8px;font-size:.88em;font-weight:600;cursor:pointer;transition:all .2s;display:inline-flex;align-items:center;gap:6px}
.btn:active{transform:scale(.97)}
.btn-pri{background:var(--pri);color:#fff}
.btn-pri:hover{background:var(--pri-dk);transform:translateY(-1px);box-shadow:0 4px 12px rgba(63,81,181,.3)}
.btn-sec{background:var(--brd);color:var(--txt)}
.btn-sec:hover{background:var(--sec);color:#fff}
.btn-ok{background:var(--ok);color:#fff}
.btn-ok:hover{background:#388E3C}
.btn-err{background:var(--err);color:#fff}
.btn:disabled{opacity:.5;cursor:not-allowed;transform:none!important;box-shadow:none!important}
.progress{display:none;margin-top:10px}
.progress-bar{height:6px;background:var(--brd);border-radius:3px;overflow:hidden}
.progress-fill{height:100%;width:0;background:linear-gradient(90deg,var(--pri),var(--sec));transition:width .3s;border-radius:3px}
.progress-text{font-size:.75em;color:var(--txt2);margin-top:4px;text-align:center}
.file-row{display:flex;justify-content:space-between;align-items:center;padding:10px 12px;border-bottom:1px solid #f0f0f0;transition:all .2s;border-radius:6px;margin:2px 0}
.file-row:last-child{border-bottom:none}
.file-row:hover{background:rgba(63,81,181,.04);transform:translateY(-1px);box-shadow:0 2px 6px rgba(0,0,0,.06)}
.file-info{display:flex;align-items:center;gap:10px;flex:1;min-width:0}
.file-name{font-size:.88em;font-weight:500;word-break:break-all;color:var(--txt)}
.file-meta{font-size:.72em;color:var(--txt2);margin-top:2px}
.file-actions{display:flex;gap:3px;flex-shrink:0}
.file-actions button{width:34px;height:34px;border:none;border-radius:8px;cursor:pointer;display:flex;align-items:center;justify-content:center;background:transparent;color:var(--txt2);transition:all .2s}
.file-actions button:hover{background:var(--sec);color:#fff;transform:scale(1.1);box-shadow:0 2px 8px rgba(0,188,212,.3)}
.file-actions button.del:hover{background:var(--err);box-shadow:0 2px 8px rgba(244,67,54,.3)}
.editor-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;flex-wrap:wrap;gap:8px}
.editor-fname{font-size:.92em;font-weight:600;color:var(--pri);display:flex;align-items:center;gap:6px}
.editor-actions{display:flex;gap:8px}
.editor-wrap{display:flex;height:55vh;min-height:220px;border:1px solid var(--brd);border-radius:8px;overflow:hidden}
.line-nums{background:#ECEFF1;color:var(--txt2);padding:12px 8px;font:13px/1.5 'Courier New',Consolas,monospace;text-align:right;user-select:none;overflow:hidden;min-width:44px;border-right:1px solid var(--brd)}
.editor-area{flex:1;resize:none;border:none;padding:12px;font:13px/1.5 'Courier New',Consolas,monospace;outline:none;background:#FAFAFA;color:var(--txt);tab-size:4}
.editor-area:focus{background:#fff}
.editor-footer{display:flex;justify-content:space-between;margin-top:8px;font-size:.75em;color:var(--txt2)}
.info-row{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #f5f5f5;font-size:.88em}
.info-row:last-child{border-bottom:none}
.info-label{color:var(--txt2)}
.info-value{font-weight:600;color:var(--txt)}
.accordion-item{border-bottom:1px solid var(--brd)}
.accordion-item:last-child{border-bottom:none}
.accordion-btn{width:100%;padding:14px 4px;border:none;background:none;text-align:left;font-size:.9em;font-weight:600;color:var(--txt);cursor:pointer;display:flex;justify-content:space-between;align-items:center;transition:color .2s}
.accordion-btn:hover{color:var(--sec)}
.accordion-btn svg{transition:transform .3s;flex-shrink:0}
.accordion-btn.open svg{transform:rotate(180deg)}
.accordion-body{max-height:0;overflow:hidden;transition:max-height .4s ease;font-size:.84em;line-height:1.7;color:var(--txt2)}
.accordion-body.open{max-height:600px}
.accordion-body p{padding:0 4px 10px}
.accordion-body strong{color:var(--txt)}
.toast-box{position:fixed;top:16px;right:16px;z-index:300;display:flex;flex-direction:column;gap:8px;pointer-events:none}
.toast{padding:12px 18px;border-radius:10px;font-size:.84em;font-weight:500;box-shadow:0 4px 16px rgba(0,0,0,.15);animation:toastIn .35s ease;display:flex;align-items:center;gap:8px;max-width:320px;pointer-events:auto}
.toast.ok{background:#E8F5E9;color:#2E7D32;border:1px solid #A5D6A7}
.toast.err{background:#FFEBEE;color:#C62828;border:1px solid #EF9A9A}
.toast.info{background:#E3F2FD;color:#1565C0;border:1px solid #90CAF9}
@keyframes toastIn{from{transform:translateX(120%);opacity:0}to{transform:translateX(0);opacity:1}}
.modal-overlay{display:none;position:fixed;inset:0;background:rgba(0,0,0,.45);z-index:200;align-items:center;justify-content:center;padding:16px}
.modal-overlay.show{display:flex}
.modal{background:#fff;border-radius:14px;padding:24px;max-width:400px;width:100%;box-shadow:0 12px 40px rgba(0,0,0,.2);animation:modalIn .25s ease}
@keyframes modalIn{from{transform:scale(.92);opacity:0}to{transform:scale(1);opacity:1}}
.modal h3{margin-bottom:8px;color:var(--txt);font-size:1.05em}
.modal p{color:var(--txt2);margin-bottom:16px;font-size:.88em;line-height:1.5}
.modal input{width:100%;padding:10px 12px;border:2px solid var(--brd);border-radius:8px;font-size:.9em;margin-bottom:16px;outline:none;transition:border-color .2s}
.modal input:focus{border-color:var(--sec)}
.modal-btns{display:flex;gap:8px;justify-content:flex-end}
.empty-state{text-align:center;color:var(--txt2);padding:32px 16px}
.empty-state svg{margin-bottom:12px}
::-webkit-scrollbar{width:6px;height:6px}
::-webkit-scrollbar-track{background:transparent}
::-webkit-scrollbar-thumb{background:var(--brd);border-radius:3px}
::-webkit-scrollbar-thumb:hover{background:var(--txt2)}
footer{text-align:center;padding:20px 16px;font-size:.72em;color:var(--txt2);letter-spacing:.3px}
@media(max-width:480px){
nav button{font-size:.7em;padding:10px 3px;gap:3px}
nav button svg{width:15px;height:15px}
.card{margin:8px}
.file-row{flex-wrap:wrap;gap:6px}
.file-actions{width:100%;justify-content:flex-end}
.editor-wrap{height:45vh;min-height:180px}
.toast{max-width:260px;font-size:.78em;padding:10px 14px}
.modal{padding:18px}
}
</style>
</head>
<body>

<div class="toast-box" id="toastBox"></div>

<div class="modal-overlay" id="modalOverlay" onclick="if(event.target===this)hideModal()">
<div class="modal" id="modal">
<h3 id="modalTitle"></h3>
<p id="modalMsg"></p>
<input id="modalInput" style="display:none" placeholder="Enter filename">
<div class="modal-btns">
<button class="btn btn-sec" onclick="hideModal()">Cancel</button>
<button class="btn btn-pri" id="modalConfirm">Confirm</button>
</div>
</div>
</div>

<header>
<h1>
<svg viewBox="0 0 24 24" width="26" height="26"><path d="M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z" fill="currentColor"/></svg>
D.E.V_Darshan
</h1>
<p>TXT Reader &middot; WiFi Portal</p>
</header>

<nav id="nav">
<button class="active" data-tab="files">
<svg viewBox="0 0 24 24" width="17" height="17"><path d="M10 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z" fill="currentColor"/></svg>
Files
</button>
<button data-tab="editor">
<svg viewBox="0 0 24 24" width="17" height="17"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 000-1.41l-2.34-2.34a1 1 0 00-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" fill="currentColor"/></svg>
Editor
</button>
<button data-tab="settings">
<svg viewBox="0 0 24 24" width="17" height="17"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58a.49.49 0 00.12-.61l-1.92-3.32a.49.49 0 00-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54a.48.48 0 00-.48-.41h-3.84a.48.48 0 00-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96a.49.49 0 00-.59.22L2.74 8.87a.48.48 0 00.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58a.49.49 0 00-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6A3.6 3.6 0 1112 8.4a3.6 3.6 0 010 7.2z" fill="currentColor"/></svg>
Settings
</button>
<button data-tab="guide">
<svg viewBox="0 0 24 24" width="17" height="17"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z" fill="currentColor"/></svg>
Guide
</button>
</nav>

<main>
<!-- ═══════════════ FILES TAB ═══════════════ -->
<section id="tab-files" class="tab-content active">

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M18 2h-8L4.02 8 4 20c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 18H6v-8h12v8zm-6-2l4-4h-3V9h-2v5H8l4 4z" fill="currentColor"/></svg>
Storage
</h3>
<div class="storage-bar"><div class="storage-fill" id="storageFill" style="width:0"></div></div>
<div class="storage-text">
<span id="usedInfo">Calculating...</span>
<span id="freeInfo">&nbsp;</span>
</div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M19.35 10.04C18.67 6.59 15.64 4 12 4 9.11 4 6.6 5.64 5.35 8.04 2.34 8.36 0 10.91 0 14c0 3.31 2.69 6 6 6h13c2.76 0 5-2.24 5-5 0-2.64-2.05-4.78-4.65-4.96zM14 13v4h-4v-4H7l5-5 5 5h-3z" fill="currentColor"/></svg>
Upload File
</h3>
<div class="drop-zone" id="dropZone">
<svg viewBox="0 0 24 24" width="42" height="42" style="color:var(--sec);margin-bottom:8px"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm4 18H6V4h7v5h5v11zM8 15.01l1.41 1.41L11 14.84V19h2v-4.16l1.59 1.59L16 15.01 12.01 11 8 15.01z" fill="currentColor"/></svg>
<p>Tap to select or drag a .txt file</p>
<p style="font-size:.7em;color:#999;margin-top:4px">Max size: 2 MB &middot; .txt only</p>
</div>
<input type="file" id="fileInput" accept=".txt,text/plain" hidden>
<div id="selFile" style="font-size:.8em;color:var(--txt2);margin-top:8px;word-break:break-all"></div>
<button class="btn btn-pri" id="uploadBtn" disabled style="width:100%;margin-top:10px;justify-content:center">
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
<span style="display:flex;align-items:center;gap:8px">
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm2 16H8v-2h8v2zm0-4H8v-2h8v2zM13 9V3.5L18.5 9H13z" fill="currentColor"/></svg>
Files on SD Card
</span>
<button class="btn btn-sec" id="newFileBtn" style="padding:6px 12px;font-size:.8em">
<svg viewBox="0 0 24 24" width="14" height="14"><path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" fill="currentColor"/></svg>
New
</button>
</h3>
<div id="fileList" style="max-height:420px;overflow-y:auto">
<div class="empty-state"><p>Loading files...</p></div>
</div>
</div>

</section>

<!-- ═══════════════ EDITOR TAB ═══════════════ -->
<section id="tab-editor" class="tab-content">
<div class="card">
<div class="editor-header">
<span class="editor-fname" id="editorFname">
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13z" fill="currentColor"/></svg>
<span id="fnameText">No file open</span>
</span>
<div class="editor-actions" id="editorActions" style="display:none">
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
<div id="editorBody" style="display:none">
<div class="editor-wrap">
<div class="line-nums" id="lineNums">1</div>
<textarea class="editor-area" id="editorArea" spellcheck="false" placeholder="Start typing..."></textarea>
</div>
</div>
<div class="editor-footer" id="editorFooter" style="display:none">
<span id="charCount">0 characters</span>
<span id="editStatus"></span>
</div>
<div id="editorEmpty" class="empty-state">
<svg viewBox="0 0 24 24" width="52" height="52" style="color:var(--brd)"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 000-1.41l-2.34-2.34a1 1 0 00-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" fill="currentColor"/></svg>
<p style="margin-top:8px;font-size:.9em">Select a file from the <strong>Files</strong> tab to begin editing</p>
<p style="font-size:.78em;margin-top:4px">Or create a new file using the <strong>New</strong> button</p>
</div>
</div>
</section>

<!-- ═══════════════ SETTINGS TAB ═══════════════ -->
<section id="tab-settings" class="tab-content">
<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z" fill="currentColor"/></svg>
Device Information
</h3>
<div id="deviceInfo"><div class="empty-state"><p>Loading device info...</p></div></div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3a4.24 4.24 0 00-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z" fill="currentColor"/></svg>
WiFi &amp; Portal Settings
</h3>
<div id="wifiInfo"><div class="empty-state"><p>Loading...</p></div></div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M18 2h-8L4.02 8 4 20c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 18H6v-8h12v8z" fill="currentColor"/></svg>
Storage Details
</h3>
<div id="storageInfo"><div class="empty-state"><p>Loading...</p></div></div>
</div>

<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58a.49.49 0 00.12-.61l-1.92-3.32a.49.49 0 00-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54a.48.48 0 00-.48-.41h-3.84a.48.48 0 00-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96a.49.49 0 00-.59.22L2.74 8.87a.48.48 0 00.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58a.49.49 0 00-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6A3.6 3.6 0 1112 8.4a3.6 3.6 0 010 7.2z" fill="currentColor"/></svg>
Reading Settings
</h3>
<div id="readingInfo">
<div class="info-row"><span class="info-label">Chars per Line</span><span class="info-value">21</span></div>
<div class="info-row"><span class="info-label">Display Lines</span><span class="info-value">4</span></div>
<div class="info-row"><span class="info-label">Bookmark Auto-save</span><span class="info-value">Every 10 lines</span></div>
<div class="info-row"><span class="info-label">Sleep Timeout</span><span class="info-value">5 minutes</span></div>
</div>
</div>
</section>

<!-- ═══════════════ GUIDE TAB ═══════════════ -->
<section id="tab-guide" class="tab-content">
<div class="card">
<h3>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z" fill="currentColor"/></svg>
User Guide
</h3>

<div class="accordion-item">
<button class="accordion-btn" onclick="toggleAcc(this)">
<span>About Device</span>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="accordion-body">
<p><strong>D.E.V_Darshan</strong> is an ultra-compact offline TXT reader built on the ESP32-CAM (AI Thinker) platform. It repurposes the camera module's SD card slot for file storage and uses a tiny 0.91" SSD1306 OLED (128&times;32) for display.</p>
<p>The device is powered by a 3.7V 1100mAh Li-ion battery with TP4056 USB charging. A magnetic reed switch provides hardware power control for zero standby drain.</p>
<p style="padding-bottom:0"><strong>Designed &amp; developed by Sakshyam Bastakoti.</strong></p>
</div>
</div>

<div class="accordion-item">
<button class="accordion-btn" onclick="toggleAcc(this)">
<span>How to Upload Files</span>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="accordion-body">
<p><strong>Step 1:</strong> On the device Home screen, select <strong>WiFi Portal</strong> and confirm.</p>
<p><strong>Step 2:</strong> The OLED will show the WiFi name and IP address. Connect your phone or computer to the displayed WiFi network.</p>
<p><strong>Step 3:</strong> Open any web browser and navigate to <strong>192.168.4.1</strong></p>
<p><strong>Step 4:</strong> In the <strong>Files</strong> tab, use the Upload section. You can tap to select a file or drag &amp; drop a <strong>.txt</strong> file onto the upload area.</p>
<p><strong>Step 5:</strong> Press <strong>Upload</strong> and wait for the progress bar to complete. A success notification will appear.</p>
<p style="padding-bottom:0"><strong>Note:</strong> Only <strong>.txt</strong> files up to <strong>2 MB</strong> are accepted. The portal will auto-shutdown after 5 minutes of inactivity.</p>
</div>
</div>

<div class="accordion-item">
<button class="accordion-btn" onclick="toggleAcc(this)">
<span>How to Edit Files</span>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="accordion-body">
<p>Click the <strong>pencil icon</strong> next to any file in the Files list. The file content will load in the <strong>Editor</strong> tab.</p>
<p>The editor provides a monospace text area with line numbers and a character counter. Edit your text as needed.</p>
<p>Click <strong>Save</strong> to write changes directly to the SD card. Click <strong>Cancel</strong> to discard changes. You can also use <strong>Ctrl+S</strong> as a keyboard shortcut to save.</p>
<p>To create a new file, click the <strong>New</strong> button in the Files tab. Enter a filename and the editor will open with a blank document.</p>
<p style="padding-bottom:0"><strong>Note:</strong> Web editing supports files up to <strong>64 KB</strong>. For larger files, download, edit on your computer, and re-upload.</p>
</div>
</div>

<div class="accordion-item">
<button class="accordion-btn" onclick="toggleAcc(this)">
<span>Button Controls</span>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="accordion-body">
<p><strong>UP Button (GPIO13):</strong> Navigate up through menus. In reading mode, short press scrolls up by one page; hold for continuous line-by-line scrolling.</p>
<p><strong>DOWN Button (GPIO0):</strong> Navigate down through menus. In reading mode, short press scrolls down by one page; hold for continuous line-by-line scrolling. <em>Do not hold during power-on (boot pin).</em></p>
<p><strong>SELECT Button (GPIO12):</strong> Context-aware action. In menus, it enters the selected item. In sub-screens and reading mode, it acts as Back.</p>
<p style="padding-bottom:0"><strong>Navigation Flow:</strong> Home &rarr; WiFi Portal / Files / Settings. Files &rarr; Select file &rarr; Reading mode. SELECT always returns to the previous screen.</p>
</div>
</div>

<div class="accordion-item">
<button class="accordion-btn" onclick="toggleAcc(this)">
<span>Power &amp; Sleep</span>
<svg viewBox="0 0 24 24" width="18" height="18"><path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z" fill="currentColor"/></svg>
</button>
<div class="accordion-body">
<p><strong>Battery:</strong> 3.7V 1100mAh Li-ion cell. Charging via TP4056 USB module. Red LED = charging, Blue/Green LED = fully charged.</p>
<p><strong>Auto Sleep:</strong> After 5 minutes of inactivity, the device enters light sleep mode. The display turns off and the CPU reduces power consumption dramatically.</p>
<p><strong>Wake Up:</strong> Press any button (UP, DOWN, or SELECT) to instantly wake the device. It resumes from where you left off.</p>
<p><strong>WiFi Power:</strong> WiFi and Bluetooth radios are completely disabled during normal reading to maximize battery life. They are only activated when you enter the WiFi Portal from the Home menu.</p>
<p style="padding-bottom:0"><strong>Power Switch:</strong> The magnetic reed switch provides a hardware power cutoff. When the magnet is in the OFF position, the device draws zero current &mdash; perfect for long-term storage.</p>
</div>
</div>

</div>
</section>
</main>

<footer>D.E.V_Darshan v1.0 &middot; Designed by Sakshyam Bastakoti</footer>

<script>
/* ════════════════════════════════════════════════════════════════════════════
   D.E.V_Darshan Portal — Client-Side Logic
   ════════════════════════════════════════════════════════════════════════════ */

// ── Constants ──
var MAX_SIZE=2*1024*1024,MAX_EDIT=65536;

// ── State ──
var currentEditFile='',editDirty=false,selectedUpFile=null,origContent='';

// ── Utility ──
function fmt(b){if(b<1024)return b+' B';if(b<1048576)return(b/1024).toFixed(1)+' KB';return(b/1048576).toFixed(1)+' MB'}
function esc(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;')}
function fmtDate(ts){if(!ts||ts<946684800)return'--';var d=new Date(ts*1000);return d.toLocaleDateString(undefined,{year:'numeric',month:'short',day:'numeric'})}

// ── Toast ──
function toast(msg,type){
var t=document.createElement('div');t.className='toast '+(type||'info');
t.textContent=msg;document.getElementById('toastBox').appendChild(t);
setTimeout(function(){t.style.opacity='0';t.style.transition='opacity .3s';setTimeout(function(){t.remove()},350)},3200);
}

// ── Modal ──
function showModal(title,msg,showInput,onConfirm){
document.getElementById('modalTitle').textContent=title;
document.getElementById('modalMsg').textContent=msg;
var inp=document.getElementById('modalInput');
inp.style.display=showInput?'block':'none';inp.value='';
document.getElementById('modalOverlay').classList.add('show');
if(showInput)setTimeout(function(){inp.focus()},100);
document.getElementById('modalConfirm').onclick=function(){var v=inp.value;hideModal();onConfirm(v)};
}
function hideModal(){document.getElementById('modalOverlay').classList.remove('show')}

// ── Tab Navigation ──
var tabOrder=['files','editor','settings','guide'];
var navBtns=document.querySelectorAll('nav button');
navBtns.forEach(function(btn){btn.addEventListener('click',function(){showTab(btn.dataset.tab)})});

function showTab(name){
if(editDirty&&currentEditFile&&name!=='editor'){
if(!confirm('You have unsaved changes. Switch tab anyway?'))return;
}
document.querySelectorAll('.tab-content').forEach(function(t){t.classList.remove('active')});
navBtns.forEach(function(b){b.classList.remove('active')});
var idx=tabOrder.indexOf(name);
if(idx>=0)navBtns[idx].classList.add('active');
var el=document.getElementById('tab-'+name);
if(el)el.classList.add('active');
if(name==='settings')loadInfo();
if(name==='files'){loadFiles();loadUsage()}
}

// ── Storage ──
function loadUsage(){
fetch('/usage').then(function(r){return r.json()}).then(function(d){
var pct=d.total>0?Math.round(d.used/d.total*100):0;
document.getElementById('storageFill').style.width=pct+'%';
document.getElementById('usedInfo').textContent='Used: '+fmt(d.used)+' ('+pct+'%)';
document.getElementById('freeInfo').textContent='Free: '+fmt(d.free);
}).catch(function(){});
}

// ── File List ──
function loadFiles(){
fetch('/files').then(function(r){return r.json()}).then(function(files){
var el=document.getElementById('fileList');
if(!files.length){el.innerHTML='<div class="empty-state"><svg viewBox="0 0 24 24" width="40" height="40" style="color:var(--brd)"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13z" fill="currentColor"/></svg><p style="margin-top:8px">No .txt files found on SD card</p></div>';return}
el.innerHTML=files.map(function(f){return '<div class="file-row" data-name="'+esc(f.name)+'">'
+'<div class="file-info">'
+'<svg viewBox="0 0 24 24" width="22" height="22" style="color:var(--pri);flex-shrink:0"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13z" fill="currentColor"/></svg>'
+'<div style="min-width:0"><div class="file-name">'+esc(f.name)+'</div>'
+'<div class="file-meta">'+fmt(f.size)+' &middot; '+fmtDate(f.modified)+'</div></div>'
+'</div>'
+'<div class="file-actions">'
+'<button data-action="edit" title="Edit"><svg viewBox="0 0 24 24" width="16" height="16"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 000-1.41l-2.34-2.34a1 1 0 00-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" fill="currentColor"/></svg></button>'
+'<button data-action="rename" title="Rename"><svg viewBox="0 0 24 24" width="16" height="16"><path d="M2.5 4v3h5v12h3V7h5V4h-13zm19 5h-9v3h3v7h3v-7h3V9z" fill="currentColor"/></svg></button>'
+'<button data-action="download" title="Download"><svg viewBox="0 0 24 24" width="16" height="16"><path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z" fill="currentColor"/></svg></button>'
+'<button data-action="delete" class="del" title="Delete"><svg viewBox="0 0 24 24" width="16" height="16"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z" fill="currentColor"/></svg></button>'
+'</div></div>'}).join('');
}).catch(function(){
document.getElementById('fileList').innerHTML='<div class="empty-state"><p style="color:var(--err)">Error loading files</p></div>';
});
}

// ── File List Event Delegation ──
document.getElementById('fileList').addEventListener('click',function(e){
var btn=e.target.closest('[data-action]');
if(!btn)return;
var row=btn.closest('.file-row');
if(!row)return;
var name=row.dataset.name;
switch(btn.dataset.action){
case 'edit':editFile(name);break;
case 'rename':renameFile(name);break;
case 'download':downloadFile(name);break;
case 'delete':confirmDelete(name);break;
}
});

// ── Upload ──
var dropZone=document.getElementById('dropZone');
var fileInput=document.getElementById('fileInput');
var uploadBtn=document.getElementById('uploadBtn');

dropZone.addEventListener('click',function(){fileInput.click()});
dropZone.addEventListener('dragover',function(e){e.preventDefault();dropZone.classList.add('over')});
dropZone.addEventListener('dragleave',function(){dropZone.classList.remove('over')});
dropZone.addEventListener('drop',function(e){e.preventDefault();dropZone.classList.remove('over');pickFile(e.dataTransfer.files[0])});
fileInput.addEventListener('change',function(){pickFile(fileInput.files[0])});

function pickFile(f){
if(!f)return;
if(!f.name.toLowerCase().endsWith('.txt')){toast('Only .txt files are allowed','err');return}
if(f.size>MAX_SIZE){toast('File too large (max 2 MB)','err');return}
if(f.size===0){toast('File is empty','err');return}
selectedUpFile=f;
document.getElementById('selFile').textContent=f.name+' ('+fmt(f.size)+')';
uploadBtn.disabled=false;
}

uploadBtn.addEventListener('click',function(){
if(!selectedUpFile)return;
uploadBtn.disabled=true;
var prog=document.getElementById('progress');prog.style.display='block';
var xhr=new XMLHttpRequest();
xhr.open('POST','/upload');
xhr.upload.onprogress=function(e){
if(e.lengthComputable){var p=Math.round(e.loaded/e.total*100);document.getElementById('progressFill').style.width=p+'%';document.getElementById('progressText').textContent=p+'%'}
};
xhr.onload=function(){
prog.style.display='none';document.getElementById('progressFill').style.width='0';
if(xhr.status===200){toast('File uploaded successfully!','ok');loadFiles();loadUsage()}
else{toast('Upload failed: '+xhr.responseText,'err')}
selectedUpFile=null;document.getElementById('selFile').textContent='';fileInput.value='';uploadBtn.disabled=true;
};
xhr.onerror=function(){prog.style.display='none';toast('Network error during upload','err');uploadBtn.disabled=false};
var fd=new FormData();fd.append('file',selectedUpFile);xhr.send(fd);
});

// ── Edit File ──
function editFile(name){
showTab('editor');
document.getElementById('fnameText').textContent=name;
document.getElementById('editorEmpty').style.display='none';
document.getElementById('editorBody').style.display='block';
document.getElementById('editorActions').style.display='flex';
document.getElementById('editorFooter').style.display='flex';
var ta=document.getElementById('editorArea');
ta.value='Loading file content...';ta.disabled=true;
currentEditFile=name;editDirty=false;
document.getElementById('editStatus').textContent='';
fetch('/read?name='+encodeURIComponent(name)).then(function(r){
if(!r.ok)throw new Error(r.statusText);return r.text()
}).then(function(content){
ta.value=content;ta.disabled=false;origContent=content;
updateLines();updateCount();ta.focus();
}).catch(function(e){toast('Failed to load file: '+e.message,'err');ta.value='';ta.disabled=false});
}

function saveFile(){
if(!currentEditFile)return;
var content=document.getElementById('editorArea').value;
if(content.length>MAX_EDIT){toast('File too large for web editor (max 64 KB). Download and edit locally.','err');return}
var saveB=document.getElementById('saveBtn');saveB.disabled=true;
fetch('/save?name='+encodeURIComponent(currentEditFile),{method:'POST',headers:{'Content-Type':'text/plain'},body:content})
.then(function(r){if(!r.ok)throw new Error('Save failed');return r.text()})
.then(function(){toast('File saved successfully!','ok');editDirty=false;origContent=content;document.getElementById('editStatus').textContent='Saved';saveB.disabled=false})
.catch(function(e){toast(e.message,'err');saveB.disabled=false});
}

function cancelEdit(){
if(editDirty&&!confirm('Discard unsaved changes?'))return;
currentEditFile='';editDirty=false;origContent='';
document.getElementById('fnameText').textContent='No file open';
document.getElementById('editorEmpty').style.display='block';
document.getElementById('editorBody').style.display='none';
document.getElementById('editorActions').style.display='none';
document.getElementById('editorFooter').style.display='none';
}
document.getElementById('saveBtn').addEventListener('click',saveFile);
document.getElementById('cancelBtn').addEventListener('click',cancelEdit);

// ── Editor Events ──
var editorArea=document.getElementById('editorArea');
editorArea.addEventListener('input',function(){
editDirty=true;document.getElementById('editStatus').textContent='Modified';
updateLines();updateCount();
});
editorArea.addEventListener('scroll',function(){document.getElementById('lineNums').scrollTop=editorArea.scrollTop});
editorArea.addEventListener('keydown',function(e){if((e.ctrlKey||e.metaKey)&&e.key==='s'){e.preventDefault();saveFile()}});

function updateLines(){
var n=document.getElementById('editorArea').value.split('\n').length;
var s='';for(var i=1;i<=n;i++)s+=i+'\n';
document.getElementById('lineNums').textContent=s;
}
function updateCount(){
var len=document.getElementById('editorArea').value.length;
var lines=document.getElementById('editorArea').value.split('\n').length;
document.getElementById('charCount').textContent=len.toLocaleString()+' chars &middot; '+lines+' lines';
}

// ── Rename ──
function renameFile(name){
var base=name.toLowerCase().endsWith('.txt')?name.slice(0,-4):name;
showModal('Rename File','Enter a new name for "'+name+'"',true,function(newName){
if(!newName||!newName.trim())return;
newName=newName.trim();if(!newName.toLowerCase().endsWith('.txt'))newName+='.txt';
fetch('/rename?old='+encodeURIComponent(name)+'&new='+encodeURIComponent(newName),{method:'POST'})
.then(function(r){if(r.ok){toast('Renamed to '+newName,'ok');loadFiles();if(currentEditFile===name){currentEditFile=newName;document.getElementById('fnameText').textContent=newName}}
else return r.text().then(function(t){toast('Rename failed: '+t,'err')})})
.catch(function(){toast('Network error','err')});
});
document.getElementById('modalInput').value=base;
}

// ── Delete ──
function confirmDelete(name){
showModal('Delete File','Are you sure you want to delete "'+name+'"? This action cannot be undone.',false,function(){
fetch('/delete?name='+encodeURIComponent(name),{method:'POST'})
.then(function(r){if(r.ok){toast('File deleted','ok');loadFiles();loadUsage();if(currentEditFile===name)cancelEdit()}
else return r.text().then(function(t){toast('Delete failed: '+t,'err')})})
.catch(function(){toast('Network error','err')});
});
}

// ── Download ──
function downloadFile(name){
var a=document.createElement('a');a.href='/download?name='+encodeURIComponent(name);
a.download=name;document.body.appendChild(a);a.click();a.remove();
}

// ── Create New File ──
document.getElementById('newFileBtn').addEventListener('click',function(){
showModal('Create New File','Enter a name for the new text file:',true,function(name){
if(!name||!name.trim())return;
name=name.trim();if(!name.toLowerCase().endsWith('.txt'))name+='.txt';
fetch('/create?name='+encodeURIComponent(name),{method:'POST'})
.then(function(r){if(r.ok){toast('File "'+name+'" created!','ok');loadFiles();editFile(name)}
else return r.text().then(function(t){toast('Create failed: '+t,'err')})})
.catch(function(){toast('Network error','err')});
});
});

// ── Settings / Info ──
function loadInfo(){
fetch('/info').then(function(r){return r.json()}).then(function(d){
document.getElementById('deviceInfo').innerHTML=
'<div class="info-row"><span class="info-label">Device</span><span class="info-value">'+esc(d.device)+'</span></div>'
+'<div class="info-row"><span class="info-label">Firmware</span><span class="info-value">v'+esc(d.firmware)+'</span></div>'
+'<div class="info-row"><span class="info-label">Author</span><span class="info-value">'+esc(d.author)+'</span></div>'
+'<div class="info-row"><span class="info-label">Total Files</span><span class="info-value">'+d.files+'</span></div>'
+'<div class="info-row"><span class="info-label">SD Card</span><span class="info-value">'+(d.sdMounted?'<span style="color:var(--ok)">&#10003; Mounted</span>':'<span style="color:var(--err)">&#10007; Not Found</span>')+'</span></div>'
+'<div class="info-row"><span class="info-label">Free Heap RAM</span><span class="info-value">'+fmt(d.freeHeap)+'</span></div>'
+'<div class="info-row"><span class="info-label">CPU Frequency</span><span class="info-value">'+d.cpuFreq+' MHz</span></div>';

document.getElementById('wifiInfo').innerHTML=
'<div class="info-row"><span class="info-label">Network SSID</span><span class="info-value">'+esc(d.ssid)+'</span></div>'
+'<div class="info-row"><span class="info-label">Portal IP</span><span class="info-value">192.168.4.1</span></div>'
+'<div class="info-row"><span class="info-label">Max Upload Size</span><span class="info-value">'+fmt(d.maxUpload)+'</span></div>'
+'<div class="info-row"><span class="info-label">Auto Shutdown</span><span class="info-value">'+Math.round(d.wifiTimeout/60000)+' min inactivity</span></div>';

var totalB=d.totalBytes,usedB=d.usedBytes,freeB=totalB-usedB;
var pct=totalB>0?Math.round(usedB/totalB*100):0;
document.getElementById('storageInfo').innerHTML=
'<div class="info-row"><span class="info-label">Total Capacity</span><span class="info-value">'+fmt(totalB)+'</span></div>'
+'<div class="info-row"><span class="info-label">Used Space</span><span class="info-value">'+fmt(usedB)+' ('+pct+'%)</span></div>'
+'<div class="info-row"><span class="info-label">Available</span><span class="info-value">'+fmt(freeB)+'</span></div>'
+'<div class="storage-bar" style="margin-top:8px"><div class="storage-fill" style="width:'+pct+'%"></div></div>';
}).catch(function(){
document.getElementById('deviceInfo').innerHTML='<p style="color:var(--err);text-align:center;padding:12px">Failed to load device information</p>';
});
}

// ── Guide Accordion ──
function toggleAcc(btn){
btn.classList.toggle('open');
var body=btn.nextElementSibling;
body.classList.toggle('open');
}

// ── Prevent accidental page close while editing ──
window.addEventListener('beforeunload',function(e){if(editDirty){e.preventDefault();e.returnValue=''}});

// ── Modal keyboard support ──
document.getElementById('modalInput').addEventListener('keydown',function(e){
if(e.key==='Enter'){e.preventDefault();document.getElementById('modalConfirm').click()}
});

// ── Initialize ──
loadFiles();
loadUsage();
</script>
</body>
</html>
)rawliteral";

#endif // PORTAL_H
