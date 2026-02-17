/*
 * ============================================================================
 *  PocketTXT — WiFi Portal HTML (PROGMEM)
 * ============================================================================
 *  Mobile-responsive upload page served by the ESP32 web server.
 *  Stored in flash (PROGMEM) to minimize RAM usage.
 *
 *  Features:
 *  - Dark theme, mobile-first design
 *  - Drag-and-drop upload area
 *  - .txt type enforcement (client + server)
 *  - File size validation
 *  - Upload progress feedback
 *  - SD card usage display
 *  - Existing file list
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
<title>PocketTXT Upload</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#1a1a2e;color:#e0e0e0;min-height:100vh;padding:16px}
.container{max-width:420px;margin:0 auto}
h1{font-size:1.3em;text-align:center;color:#e94560;margin-bottom:4px}
.subtitle{text-align:center;font-size:.8em;color:#888;margin-bottom:16px}
.card{background:#16213e;border-radius:12px;padding:16px;margin-bottom:12px;border:1px solid #0f3460}
.card h2{font-size:.95em;color:#e94560;margin-bottom:10px}
.drop-zone{border:2px dashed #0f3460;border-radius:10px;padding:32px 16px;text-align:center;cursor:pointer;transition:all .3s}
.drop-zone.over{border-color:#e94560;background:rgba(233,69,96,.1)}
.drop-zone p{font-size:.9em;color:#aaa;margin-bottom:8px}
.drop-zone .icon{font-size:2em;margin-bottom:8px;display:block}
input[type=file]{display:none}
.btn{display:block;width:100%;padding:12px;border:none;border-radius:8px;font-size:1em;font-weight:600;cursor:pointer;margin-top:10px;transition:all .2s}
.btn-upload{background:#e94560;color:#fff}
.btn-upload:hover{background:#c73e54}
.btn-upload:disabled{background:#555;cursor:not-allowed}
.progress{display:none;margin-top:10px}
.progress-bar{height:6px;background:#0f3460;border-radius:3px;overflow:hidden}
.progress-fill{height:100%;width:0;background:#e94560;transition:width .3s;border-radius:3px}
.progress-text{font-size:.75em;color:#888;margin-top:4px;text-align:center}
.toast{display:none;padding:10px;border-radius:8px;text-align:center;font-size:.85em;margin-top:8px;font-weight:600}
.toast.success{display:block;background:rgba(0,200,83,.15);color:#00c853;border:1px solid #00c85355}
.toast.error{display:block;background:rgba(233,69,96,.15);color:#e94560;border:1px solid #e9456055}
.usage-bar{height:8px;background:#0f3460;border-radius:4px;margin:8px 0;overflow:hidden}
.usage-fill{height:100%;background:linear-gradient(90deg,#e94560,#0f3460);border-radius:4px}
.usage-text{display:flex;justify-content:space-between;font-size:.75em;color:#888}
.file-list{max-height:180px;overflow-y:auto;margin-top:8px}
.file-item{padding:6px 8px;border-bottom:1px solid #0f346033;font-size:.8em;display:flex;justify-content:space-between;align-items:center}
.file-item:last-child{border-bottom:none}
.file-name{color:#e0e0e0;word-break:break-all}
.file-size{color:#888;white-space:nowrap;margin-left:8px}
.empty{text-align:center;color:#666;font-size:.8em;padding:16px}
.fname{font-size:.8em;color:#aaa;margin-top:6px;word-break:break-all}
::-webkit-scrollbar{width:4px}
::-webkit-scrollbar-track{background:#16213e}
::-webkit-scrollbar-thumb{background:#0f3460;border-radius:2px}
</style>
</head>
<body>
<div class="container">
<h1>&#128214; PocketTXT</h1>
<p class="subtitle">Upload .txt files to your reader</p>

<div class="card">
<h2>&#128228; Upload File</h2>
<div class="drop-zone" id="dropZone">
<span class="icon">&#128196;</span>
<p>Tap to select or drag a .txt file</p>
<p style="font-size:.7em;color:#666">Max size: 2 MB</p>
</div>
<input type="file" id="fileInput" accept=".txt,text/plain">
<div class="fname" id="fileName"></div>
<button class="btn btn-upload" id="uploadBtn" disabled>Upload</button>
<div class="progress" id="progress">
<div class="progress-bar"><div class="progress-fill" id="progressFill"></div></div>
<div class="progress-text" id="progressText">0%</div>
</div>
<div class="toast" id="toast"></div>
</div>

<div class="card">
<h2>&#128190; Storage</h2>
<div class="usage-bar"><div class="usage-fill" id="usageFill" style="width:0%"></div></div>
<div class="usage-text">
<span id="usedText">--</span>
<span id="freeText">--</span>
</div>
</div>

<div class="card">
<h2>&#128195; Files on SD</h2>
<div class="file-list" id="fileList">
<div class="empty">Loading...</div>
</div>
</div>
</div>

<script>
const dropZone=document.getElementById('dropZone');
const fileInput=document.getElementById('fileInput');
const uploadBtn=document.getElementById('uploadBtn');
const fileName=document.getElementById('fileName');
const progress=document.getElementById('progress');
const progressFill=document.getElementById('progressFill');
const progressText=document.getElementById('progressText');
const toast=document.getElementById('toast');
let selectedFile=null;
const MAX_SIZE=2*1024*1024;

dropZone.addEventListener('click',()=>fileInput.click());
dropZone.addEventListener('dragover',e=>{e.preventDefault();dropZone.classList.add('over')});
dropZone.addEventListener('dragleave',()=>dropZone.classList.remove('over'));
dropZone.addEventListener('drop',e=>{e.preventDefault();dropZone.classList.remove('over');handleFile(e.dataTransfer.files[0])});
fileInput.addEventListener('change',()=>handleFile(fileInput.files[0]));

function handleFile(f){
  toast.className='toast';
  if(!f)return;
  if(!f.name.toLowerCase().endsWith('.txt')){showToast('Only .txt files allowed','error');return}
  if(f.size>MAX_SIZE){showToast('File too large (max 2MB)','error');return}
  if(f.size===0){showToast('File is empty','error');return}
  selectedFile=f;
  fileName.textContent=f.name+' ('+formatSize(f.size)+')';
  uploadBtn.disabled=false;
}

uploadBtn.addEventListener('click',()=>{
  if(!selectedFile)return;
  uploadBtn.disabled=true;
  progress.style.display='block';
  toast.className='toast';
  const xhr=new XMLHttpRequest();
  xhr.open('POST','/upload');
  xhr.upload.onprogress=e=>{
    if(e.lengthComputable){
      const p=Math.round(e.loaded/e.total*100);
      progressFill.style.width=p+'%';
      progressText.textContent=p+'%';
    }
  };
  xhr.onload=()=>{
    progress.style.display='none';
    progressFill.style.width='0';
    if(xhr.status===200){
      showToast('Uploaded successfully!','success');
      loadFiles();loadUsage();
    }else{
      showToast('Upload failed: '+xhr.responseText,'error');
    }
    selectedFile=null;
    fileName.textContent='';
    fileInput.value='';
    uploadBtn.disabled=true;
  };
  xhr.onerror=()=>{
    progress.style.display='none';
    showToast('Network error','error');
    uploadBtn.disabled=false;
  };
  const fd=new FormData();
  fd.append('file',selectedFile);
  xhr.send(fd);
});

function showToast(msg,type){toast.textContent=msg;toast.className='toast '+type}
function formatSize(b){if(b<1024)return b+'B';if(b<1048576)return(b/1024).toFixed(1)+'KB';return(b/1048576).toFixed(1)+'MB'}

function loadUsage(){
  fetch('/usage').then(r=>r.json()).then(d=>{
    const pct=d.total>0?Math.round(d.used/d.total*100):0;
    document.getElementById('usageFill').style.width=pct+'%';
    document.getElementById('usedText').textContent='Used: '+formatSize(d.used)+' ('+pct+'%)';
    document.getElementById('freeText').textContent='Free: '+formatSize(d.free);
  }).catch(()=>{});
}

function loadFiles(){
  fetch('/files').then(r=>r.json()).then(files=>{
    const list=document.getElementById('fileList');
    if(!files.length){list.innerHTML='<div class="empty">No .txt files found</div>';return}
    list.innerHTML=files.map(f=>'<div class="file-item"><span class="file-name">'+f.name+'</span><span class="file-size">'+formatSize(f.size)+'</span></div>').join('');
  }).catch(()=>{document.getElementById('fileList').innerHTML='<div class="empty">Error loading files</div>'});
}

loadUsage();loadFiles();
</script>
</body>
</html>
)rawliteral";

#endif // PORTAL_H
