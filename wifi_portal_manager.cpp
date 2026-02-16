/*  =========================================================================
 *  DEV_Darshan — WiFi Portal Manager Implementation
 *  Temporary AP with mobile-responsive file upload page
 *  =========================================================================
 */

#include <Arduino.h>
#include "wifi_portal_manager.h"
#include "sd_manager.h"
#include "display_manager.h"
#include "config.h"

#include <WiFi.h>
#include <WebServer.h>
#include <SD_MMC.h>

// ── Private state ──────────────────────────────────────────────────────
static WebServer* _server = nullptr;
static bool       _active = false;

// ── Embedded HTML ──────────────────────────────────────────────────────
// Kept as PROGMEM string to save heap — served directly from flash.

static const char UPLOAD_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>DEV_Darshan Upload</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#0a0a0a;color:#e0e0e0;
     min-height:100vh;display:flex;flex-direction:column;align-items:center;
     justify-content:center;padding:16px}
.card{background:#1a1a2e;border:1px solid #16213e;border-radius:16px;
      padding:28px 24px;max-width:420px;width:100%;box-shadow:0 8px 32px rgba(0,0,0,.6)}
h1{font-size:1.4rem;text-align:center;margin-bottom:4px;color:#e94560}
.sub{text-align:center;font-size:.82rem;color:#888;margin-bottom:20px}
.drop{border:2px dashed #333;border-radius:12px;padding:32px 16px;text-align:center;
      cursor:pointer;transition:.2s;margin-bottom:16px}
.drop.over{border-color:#e94560;background:rgba(233,69,96,.08)}
.drop p{font-size:.95rem;color:#aaa}
.drop .icon{font-size:2rem;margin-bottom:8px}
input[type=file]{display:none}
.btn{display:block;width:100%;padding:14px;border:none;border-radius:10px;
     background:#e94560;color:#fff;font-size:1rem;font-weight:600;cursor:pointer;
     transition:.15s;margin-top:8px}
.btn:disabled{background:#444;cursor:not-allowed}
.btn:hover:not(:disabled){background:#c73652}
.status{margin-top:14px;text-align:center;font-size:.9rem;min-height:22px}
.ok{color:#4ecca3}.err{color:#e94560}
.info{margin-top:18px;padding-top:14px;border-top:1px solid #222;font-size:.8rem;
      color:#666;text-align:center}
.fname{margin-top:10px;text-align:center;font-size:.85rem;color:#ccc;word-break:break-all}
</style>
</head>
<body>
<div class="card">
  <h1>DEV_Darshan</h1>
  <p class="sub">Pocket TXT Reader &mdash; File Upload</p>

  <form id="uf" enctype="multipart/form-data">
    <div class="drop" id="dropzone">
      <div class="icon">&#128195;</div>
      <p>Tap to select or drop a <b>.txt</b> file</p>
    </div>
    <input type="file" id="fi" name="file" accept=".txt">
    <div class="fname" id="fn"></div>
    <button class="btn" type="submit" id="ubtn" disabled>Upload to Reader</button>
  </form>
  <div class="status" id="st"></div>
  <div class="info" id="sdi"></div>
</div>
<script>
const fi=document.getElementById('fi'),fn=document.getElementById('fn'),
      ub=document.getElementById('ubtn'),st=document.getElementById('st'),
      dz=document.getElementById('dropzone'),uf=document.getElementById('uf'),
      si=document.getElementById('sdi');

// Fetch SD info on load
fetch('/sdinfo').then(r=>r.json()).then(d=>{
  si.textContent='SD: '+(d.used/1024/1024).toFixed(1)+' MB used / '
                 +(d.total/1024/1024).toFixed(1)+' MB total';
}).catch(()=>{});

dz.onclick=()=>fi.click();
['dragover','dragenter'].forEach(e=>dz.addEventListener(e,ev=>{ev.preventDefault();dz.classList.add('over')}));
['dragleave','drop'].forEach(e=>dz.addEventListener(e,()=>dz.classList.remove('over')));
dz.addEventListener('drop',ev=>{ev.preventDefault();if(ev.dataTransfer.files.length)handleFile(ev.dataTransfer.files[0])});
fi.addEventListener('change',()=>{if(fi.files.length)handleFile(fi.files[0])});

function handleFile(f){
  if(!f.name.toLowerCase().endsWith('.txt')){st.className='status err';st.textContent='Only .txt files allowed';return}
  fn.textContent=f.name+' ('+( f.size<1024?f.size+' B':(f.size/1024).toFixed(1)+' KB')+')';
  ub.disabled=false;st.textContent='';
}

uf.addEventListener('submit',ev=>{
  ev.preventDefault();
  const f=fi.files[0]||(()=>{const dt=new DataTransfer();return null})();
  if(!f&&!fn.textContent){st.className='status err';st.textContent='No file selected';return}
  const fd=new FormData();fd.append('file',fi.files[0]||document.querySelector('[name=file]').files[0]);
  ub.disabled=true;ub.textContent='Uploading...';st.textContent='';
  fetch('/upload',{method:'POST',body:fd}).then(r=>r.text()).then(t=>{
    st.className='status ok';st.textContent=t;ub.textContent='Upload to Reader';ub.disabled=false;
    // Refresh SD info
    fetch('/sdinfo').then(r=>r.json()).then(d=>{
      si.textContent='SD: '+(d.used/1024/1024).toFixed(1)+' MB used / '
                     +(d.total/1024/1024).toFixed(1)+' MB total';
    }).catch(()=>{});
  }).catch(()=>{
    st.className='status err';st.textContent='Upload failed';ub.textContent='Upload to Reader';ub.disabled=false;
  });
});
</script>
</body>
</html>
)rawliteral";

// ── Route handlers ─────────────────────────────────────────────────────

static void _handleRoot() {
    _server->send_P(200, "text/html", UPLOAD_PAGE);
}

static void _handleUpload() {
    HTTPUpload& upload = _server->upload();

    // We only handle the UPLOAD_FILE_END state — accumulate in a temp file
    // For simplicity and RAM safety, we stream directly to SD.
    static File uploadFile;

    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) filename = "/" + filename;

        // Sanitize: only allow .txt
        if (!filename.endsWith(".txt") && !filename.endsWith(".TXT")) {
            Serial.println("[WIFI] Rejected non-.txt upload");
            return;
        }

        uploadFile = SD_MMC.open(filename, FILE_WRITE);
        if (!uploadFile) {
            Serial.printf("[WIFI] Cannot create %s\n", filename.c_str());
        } else {
            Serial.printf("[WIFI] Upload start: %s\n", filename.c_str());
        }

    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }

    } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            Serial.printf("[WIFI] Upload done: %u bytes\n", (unsigned)upload.totalSize);
            uploadFile.close();
        }
    }
}

static void _handleUploadComplete() {
    _server->send(200, "text/plain", "File uploaded successfully!");
}

static void _handleSdInfo() {
    char json[96];
    snprintf(json, sizeof(json),
             "{\"total\":%llu,\"used\":%llu}",
             (unsigned long long)sd_totalBytes(),
             (unsigned long long)sd_usedBytes());
    _server->send(200, "application/json", json);
}

// ═══════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════

void wifi_portal_start() {
    if (_active) return;

    Serial.println("[WIFI] Starting Access Point...");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, 0, AP_MAX_CONN);

    IPAddress ip = WiFi.softAPIP();
    Serial.printf("[WIFI] AP ready — SSID: %s  IP: %s\n",
                  AP_SSID, ip.toString().c_str());

    // Show WiFi info on OLED
    display_wifiInfo(AP_SSID, ip.toString().c_str());

    // Start web server
    _server = new WebServer(80);
    _server->on("/",          HTTP_GET,  _handleRoot);
    _server->on("/upload",    HTTP_POST, _handleUploadComplete, _handleUpload);
    _server->on("/sdinfo",    HTTP_GET,  _handleSdInfo);
    _server->begin();

    _active = true;
    Serial.println("[WIFI] Web server started on port 80");
}

void wifi_portal_loop() {
    if (_active && _server) {
        _server->handleClient();
    }
}

void wifi_portal_stop() {
    if (!_active) return;

    Serial.println("[WIFI] Stopping portal...");

    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    _active = false;
    Serial.println("[WIFI] Portal stopped, WiFi OFF");
}

bool wifi_portal_isActive() {
    return _active;
}
