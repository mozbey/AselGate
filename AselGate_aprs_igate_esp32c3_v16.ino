/*
  ESP32-C3 Super Mini  –  APRS iGate  (ASELSAN 4021 E  ->  APRS-IS)
  ==========================================================

  Donanım (şemaya göre):
    ASELSAN Pin 7 (Tx veri) -> MAX3232 RIN1 -> MAX3232 ROUT1 -> ESP32-C3 GPIO5 (RX)
    GPS (NEO-6M) verisi MAX3232 üzerinden telsize gider, ESP32 sadece dinler (TX kullanılmaz).

  Arduino IDE ayarları:
    Kart            : "ESP32C3 Dev Module"  (esp32 by Espressif, 2.0.x veya 3.x)
    USB CDC On Boot : Enabled  (ZORUNLU: USB seri monitör için)
    Ek kütüphane    : GEREKMEZ (WiFi, WebServer, DNSServer, ESPmDNS, Preferences hazır gelir)

  İlk kullanım:
    1) Telefon/PC ile "AselGate" Wi-Fi ağına bağlan (şifre: 12345678), http://192.168.1.1 aç.
    2) "Wi-Fi" bölümünden ağları tara, kendi ağını ekle.
    3) "Ayarlar" bölümünden çağrı işareti, passcode ve konumu gir, kaydet.
    4) Cihaz ağa bağlanınca http://AselGate.local veya durum kartındaki IP ile de erişebilirsin.
       Ağda ayrıca "AselGate" adıyla görünür (mDNS + NetBIOS + DHCP hostname):
         - Windows : ping AselGate   /  http://AselGate/   (Ağ listesinde de çıkabilir)
         - Mac/iOS/Linux/Android : http://AselGate.local
         - Modem arayüzündeki "bağlı cihazlar" listesinde "AselGate" adıyla görünür.
       IP adresi ayrıca USB seri monitörde (9600) "# " ile başlayan satır olarak yazdırılır.

  v1.7: - ESP32-C3 Super Mini'ye uyarlandı (tek çekirdek RISC-V, yerleşik USB seri).
          Bağlantı aynı: MAX3232 ROUT1 -> GPIO5. USB seri PC'ye bağlı değilken beklemez.
          Bazı C3 Super Mini kartlarında anten zayıf olduğundan Wi-Fi TX gücü düşürülebilir
          (bkz. WIFI_TXPOWER_FIX).

  v1.6: - Program adı "AselGate" (AP ağı, arayüz, hostname: AselGate / AselGate.local).
        - Sürekli APRS-IS bağlantısı: kopunca 0,5 sn içinde yeniden dener (en çok 15 sn'ye kadar
          kademeli bekleme), 30 sn'de bir keepalive, sunucudan 60 sn veri gelmezse bağlantı yenilenir.
        - Bağlantı yokken duyulan paketler (12'ye kadar, 60 sn'den eski değilse) bağlantı gelince
          sırayla iletilir; paket kaybı azalır. Bağlanma zaman aşımı 2 sn, seri tampon 4 KB.
        - Telsiz verisi varken web isteği işlenmez (paket gecikmesi en aza iner).
        - WIDE1-1,WIDE2-2 / WIDE2-2 gibi yollarla duyulan paketler zaten süzülmeden iletilir;
          yalnızca TCPIP/TCPXX/NOGATE/RFONLY/q-yapısı taşıyanlar iletilmez (iGate kuralı).

  v1.5: - Cihaz adı "AselGate" (AP ağı, arayüz başlığı); ağ adı (hostname) "AselGate".
        - Arayüzden GPS konum kartı ve "GPS konumunu kullan" düğmeleri kaldırıldı.
        - Hızlı iletim: paket önce APRS-IS'e gönderilir, USB kopyası sonra yapılır;
          ana döngüde seri okuma / APRS-IS işleme web sunucusundan önce çalışır.

  v1.4: - Telsizden duyulan TÜM APRS paketleri (iGate'lensin ya da süzülsün) TNC2 metin biçiminde
          USB seri porta (9600 baud) da kopyalanır:  CAGRI>DEST,PATH:veri
          KISS modunda da çerçeveler çözülüp aynı biçimde basılır. GPS NMEA satırları basılmaz.
          Bilgi satırları "# " ile başlar (APRS yazılımları bunları yorum olarak yok sayar).
          PC bağlı değilse / okumuyorsa veri atılır, iGate döngüsü beklemez.
          Not: C3 Super Mini'de USB seri için "USB CDC On Boot: Enabled" seçilmelidir.

  v1.3: - "Seri Hat (GPIO5)" kartı: veri geliyor mu, toplam bayt, bayt/sn, bozuk bayt, son ham satır.
        - "GPS" kartı: telsizin verdiği NMEA ($GPGGA/RMC/GLL) çözülür; konum, uydu, yükseklik, hız, saat.
        - "GPS konumunu kullan" düğmesi: son geçerli GPS konumunu iGate konumu olarak kaydeder.

  v1.2.1: Telsizin seri çıkışındaki GPS NMEA satırları ($GP...) artık yok sayılır
          ("Geçersiz TNC2 biçimi" kayıtları kalktı). Durum metni: "ASELSAN iGate - yAtolye /TA2BI".

  v1.2 değişiklikleri:
    - Kurulum Wi-Fi ağı adı "AselGate"; web arayüzü başlığı da AselGate.
    - "Engellenen çağrı işaretleri" alanı (en çok 5): bu istasyonların paketleri APRS-IS'e iletilmez.
    - Her konum işaretçisinden hemen sonra sabit DURUM paketi gönderilir:
        TA2BI-10>AP2BI,TCPIP*:>ASELSAN iGate istasyonu / yAtolye (TA2BI)
    - Wi-Fi bağlantısı yeniden yazıldı: hostname doğru sırada ayarlanır, Wi-Fi uyku modu kapalı
      (ağda ping/mDNS yanıtı kesilmez), bağlantı kopunca önce aynı ağa dönülür, tarama sırasında
      bağlantı denemesi yapılmaz, mDNS her bağlantıda yeniden başlatılır, NetBIOS eklendi,
      bağlanamama nedeni (şifre hatalı / ağ bulunamadı) arayüzde gösterilir.

  Çalışma mantığı:
    Telsizden gelen paket  CAGRI>APRS,PATH:veri  biçimindeyse (TNC2 metin) veya KISS ise
    şu şekilde APRS-IS'e gönderilir:
        CAGRI>APRS,PATH,qAR,IGATE_CAGRISI:veri
    Böylece paketin bu iGate üzerinden geçtiği (qAR) aprs.fi gibi sitelerde görünür.
    TCPIP / TCPXX / NOGATE / RFONLY içeren ve q-yapısı taşıyan paketler iletilmez.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <NetBIOS.h>                   // Windows ağında "AselGate" adıyla görünmek için (core içinde hazır)
#include <Preferences.h>
#include <time.h>

// ======================= AYARLAR (derleme zamanı) =======================
#define RX_PIN        5                // MAX3232 ROUT1 (pin 12) -> GPIO5
#define TX_PIN        -1               // kullanılmıyor
#define FW_VERSION    "1.7"
// Wi-Fi bağlanmıyor / sık kopuyorsa (bazı C3 Super Mini kartlarında bilinen sorun) 1 yapın:
#define WIFI_TXPOWER_FIX 0
#define HOSTNAME      "AselGate"      // ağda görünen ad: AselGate.local / modem cihaz listesi / NetBIOS
#define TOCALL        "APATAR"          // cihaz (tocall) adı: konum paketinde hedef alanı olarak gider
#define STATUS_TEXT   "ASELSAN iGate - yAtolye /TA2BI"   // her konum paketinden sonra gider
#define MAX_NETS      8
#define LOG_N         30
#define TZ_OFFSET_SEC (3 * 3600)       // Türkiye UTC+3
#define USB_BAUD      9600             // duyulan paketlerin kopyalandığı USB seri hız

const char* AP_SSID = "AselGate";
const char* AP_PASS = "12345678";      // en az 8 karakter; değiştirmeniz önerilir

// AP (kurulum ağı) adresi: açılışta 192.168.1.1. Modem de 192.168.1.x ise iki ağ çakışır;
// bu durumda cihaz modeme bağlandıktan 30 sn sonra AP adresini otomatik başka bloğa taşır.
IPAddress apIP(192, 168, 1, 1);
bool      apCheck = false;
uint32_t  staUpMs = 0;

// ======================= GLOBALLER =======================
Preferences prefs;
WebServer   server(80);
DNSServer   dns;
WiFiClient  isClient;

struct Config {
  String   call, pass, comment, srv, uipw, block;   // block: engellenen çağrı işaretleri (boşlukla ayrılmış, en çok 5)
  double   lat, lon;
  char     symTab, symCode;
  uint16_t port;
  uint16_t beaconMin;
  uint32_t baud;
  uint8_t  fmt;                        // 0 = TNC2 metin, 1 = KISS
} cfg;

String netSsid[MAX_NETS], netPass[MAX_NETS];
int    netCount = 0;

// Wi-Fi durum makinesi
int      wifiIdx = -1, wifiTry = -1;
uint32_t wifiTryMs = 0;
bool     wasSta = false, mdnsOn = false;
volatile int wifiReason = 0;           // son bağlantı kopma/hata kodu (olay işleyicisinden)
String   wifiMsg = "";                 // arayüzde gösterilen bağlantı durumu / hata nedeni

// APRS-IS durumu
bool     isConnected = false, isVerified = false, beaconDue = false;
String   isStatus = "Başlatılıyor", isSrvName = "", isLine = "";
uint32_t lastIsTry = 0, lastIsRx = 0, lastKeep = 0, lastBeacon = 0, isRetryMs = 500;

// Sayaçlar
uint32_t cntRx = 0, cntGated = 0, cntSkip = 0, cntBeacon = 0, bytesRx = 0, lastRxMs = 0, cntNmea = 0;
uint32_t cntBad = 0, lastByteMs = 0;   // cntBad: yazdırılamayan bayt sayısı (baud hızı yanlışsa artar)
String   lastRaw = "";                 // seri hattan gelen son ham satır

// Telsizin verdiği GPS (NMEA) verisi
struct GpsData { bool have, fix; double lat, lon, alt, spd; uint8_t sats; char tm[12]; uint32_t fixMs; } gps;
uint32_t restartAt = 0;

// Paket günlüğü
struct LogEntry { uint32_t seq, t, up; uint8_t state; char reason[48]; char text[240]; };
LogEntry logs[LOG_N];
uint32_t logSeq = 0;

// Tekrar engelleme
struct Seen { uint32_t h; uint32_t ms; } seen[24];
uint8_t seenIdx = 0;

// Seri ayrıştırıcı
char   rxBuf[512]; size_t rxLen = 0;
uint8_t kbuf[420]; size_t klen = 0; bool kesc = false;

// ======================= HTML =======================
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="tr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AselGate</title>
<style>
:root{--bg:#0f172a;--card:#1e293b;--fg:#e2e8f0;--mut:#94a3b8;--ac:#38bdf8;--ok:#22c55e;--bad:#ef4444;--warn:#f59e0b}
*{box-sizing:border-box}
body{margin:0;font:15px system-ui,Segoe UI,Arial,sans-serif;background:var(--bg);color:var(--fg)}
header{padding:14px 18px;background:#0b1220;border-bottom:1px solid #334155;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:8px}
h1{font-size:18px;margin:0}
main{max-width:1000px;margin:auto;padding:14px;display:grid;gap:14px}
.card{background:var(--card);border-radius:12px;padding:14px}
.card h2{margin:0 0 10px;font-size:16px;color:var(--ac)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:10px}
.stat{background:#0f172a;border-radius:8px;padding:10px}
.stat b{display:block;font-size:12px;color:var(--mut);font-weight:500}
.stat span{font-size:16px;word-break:break-all}
label{display:block;font-size:12px;color:var(--mut);margin:8px 0 3px}
input,select,button{font:inherit;padding:8px 10px;border-radius:8px;border:1px solid #475569;background:#0f172a;color:var(--fg);width:100%}
button{background:var(--ac);color:#04121f;border:0;font-weight:600;cursor:pointer;width:auto}
button.sec{background:#334155;color:var(--fg)}
button.del{background:var(--bad);color:#fff;padding:4px 10px}
button:disabled{opacity:.5}
.row{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:10px}
.btns{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}
#log{max-height:430px;overflow:auto;font:12px ui-monospace,Consolas,monospace}
.pk{padding:6px 8px;border-left:3px solid var(--ok);background:#0f172a;margin-bottom:5px;border-radius:4px;word-break:break-all}
.pk.skip{border-color:var(--warn)}.pk.me{border-color:var(--ac)}
.pk small{display:block;color:var(--mut);margin-bottom:2px}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:var(--bad);margin-right:6px}
.dot.on{background:var(--ok)}
#map{height:270px;border-radius:8px;margin-top:8px;background:#0f172a;display:flex;align-items:center;justify-content:center;color:var(--mut);text-align:center;padding:8px}
.net{display:flex;justify-content:space-between;align-items:center;gap:8px;padding:7px 0;border-bottom:1px solid #334155}
.net span.m{color:var(--mut);font-size:12px}
#toast{position:fixed;left:50%;bottom:18px;transform:translateX(-50%);background:#0b1220;border:1px solid #475569;padding:10px 16px;border-radius:10px;display:none;z-index:9999}
.hint{font-size:12px;color:var(--mut);margin-top:6px}
footer{text-align:center;font-size:14px;letter-spacing:.6px;color:var(--mut);padding:10px 10px 28px}
footer b{color:var(--ac);font-weight:600}
</style></head><body>
<header>
  <h1>📡 AselGate <span id="hcall" style="color:var(--ac)"></span></h1>
  <div><span class="dot" id="d_is"></span><span id="st_is_h">APRS-IS</span></div>
</header>
<main>

<section class="card"><h2>Durum</h2>
 <div class="grid">
  <div class="stat"><b>Wi-Fi</b><span id="st_wifi">-</span></div>
  <div class="stat"><b>IP adresi</b><span id="st_ip">-</span></div>
  <div class="stat"><b>APRS-IS</b><span id="st_is">-</span></div>
  <div class="stat"><b>Sunucu</b><span id="st_srv">-</span></div>
  <div class="stat"><b>Telsizden alınan</b><span id="st_rx">0</span></div>
  <div class="stat"><b>APRS-IS'e iletilen</b><span id="st_gated">0</span></div>
  <div class="stat"><b>Atlanan</b><span id="st_skip">0</span></div>
  <div class="stat"><b>Seri hattan gelen bayt</b><span id="st_bytes">0</span></div>
  <div class="stat"><b>Son paket</b><span id="st_last">-</span></div>
  <div class="stat"><b>Çalışma süresi</b><span id="st_up">-</span></div>
  <div class="stat"><b>Boş bellek</b><span id="st_heap">-</span></div>
  <div class="stat"><b>Kayıtlı konum</b><span id="st_pos">-</span></div>
 </div>
</section>

<section class="card"><h2>Seri Hat (telsiz → GPIO5)</h2>
 <div class="grid">
  <div class="stat"><b>Veri durumu</b><span><span class="dot" id="d_ser"></span><span id="ser_st">-</span></span></div>
  <div class="stat"><b>Toplam bayt</b><span id="ser_bytes">0</span></div>
  <div class="stat"><b>Hız</b><span id="ser_rate">0 bayt/sn</span></div>
  <div class="stat"><b>Son veriden beri</b><span id="ser_age">-</span></div>
  <div class="stat"><b>Baud</b><span id="ser_baud">-</span></div>
  <div class="stat"><b>Bozuk bayt</b><span id="ser_bad">0</span></div>
 </div>
 <div class="hint">Seri hattan gelen son ham satır:</div>
 <div class="pk" id="ser_raw" style="margin-top:4px;font:12px ui-monospace,Consolas,monospace">-</div>
 <div class="hint" id="ser_hint"></div>
</section>

<section class="card"><h2>Canlı Paket Akışı (telsiz → iGate → APRS-IS)</h2>
 <div id="log"><div class="hint" id="logempty">Telsizden paket bekleniyor…</div></div>
</section>

<section class="card"><h2>Ayarlar</h2>
 <div class="row">
  <div><label>iGate çağrı işareti (ör. TA1ABC-10)</label><input id="f_call" maxlength="9" autocapitalize="characters"></div>
  <div><label>APRS-IS passcode</label><input id="f_pass" type="password" inputmode="numeric" placeholder="(kayıtlı değer korunur)"></div>
 </div>
 <div class="row">
  <div><label>Enlem</label><input id="f_lat" inputmode="decimal"></div>
  <div><label>Boylam</label><input id="f_lon" inputmode="decimal"></div>
 </div>
 <div class="row">
  <div><label>Koordinat yapıştır: Google Haritalar'da konuma basılı tutup üstteki sayıları kopyalayın (ör. 40.8438, 31.1565)</label>
   <input id="f_paste" placeholder="40.8438, 31.1565" oninput="pasteCoord()"></div>
 </div>
 <div class="btns"><button class="sec" onclick="geo()">📍 Tarayıcı konumumu kullan</button></div>
 <div id="map">Harita yükleniyor… (internet gerekir; koordinatları elle de girebilirsiniz)</div>
 <div class="hint">Haritaya tıklayarak konum seçebilirsiniz.</div>
 <div class="row">
  <div><label>Konum açıklaması (en çok 43 karakter)</label><input id="f_cmt" maxlength="43"></div>
  <div><label>Sembol tablosu (/ \ veya overlay harfi)</label><input id="f_tab" maxlength="1"></div>
  <div><label>Sembol kodu (iGate için &amp;)</label><input id="f_code" maxlength="1"></div>
 </div>
 <div class="row">
  <div><label>APRS-IS sunucusu</label><input id="f_srv"></div>
  <div><label>Port (14580)</label><input id="f_port" inputmode="numeric"></div>
  <div><label>Konum bildirimi aralığı (dk, 0=kapalı, en az 10)</label><input id="f_bint" inputmode="numeric"></div>
 </div>
 <div class="row">
  <div><label>Telsiz seri hızı (baud)</label>
   <select id="f_baud"><option>1200</option><option>2400</option><option>4800</option><option>9600</option><option>19200</option><option>38400</option><option>57600</option><option>115200</option></select></div>
  <div><label>Telsiz veri biçimi</label>
   <select id="f_fmt"><option value="0">TNC2 metin (CALL&gt;APRS,PATH:veri)</option><option value="1">KISS (ikili)</option></select></div>
  <div><label>Arayüz şifresi (kullanıcı: admin)</label><input id="f_uipw" type="password" placeholder="boş=değişmez, - = kaldır"></div>
 </div>
 <div class="row">
  <div><label>Engellenen çağrı işaretleri (en çok 5; boşluk veya virgülle ayırın, ör. TA1XYZ TA2ABC-7)</label>
   <input id="f_block" maxlength="60" autocapitalize="characters" placeholder="boş = engel yok">
   <div class="hint">SSID yazılmazsa o çağrı işaretinin tüm SSID'leri engellenir. Bu istasyonların paketleri APRS-IS'e iletilmez.</div></div>
 </div>
 <div class="btns">
  <button id="savebtn" onclick="save()" disabled>💾 Kaydet</button>
  <button class="sec" onclick="beacon()">📍 Konum işaretçisini şimdi gönder</button>
  <button class="sec" onclick="restart()">⟳ Yeniden başlat</button>
 </div>
</section>

<section class="card"><h2>Wi-Fi Ağları</h2>
 <div class="btns" style="margin-top:0"><button id="scanbtn" onclick="scan()">🔍 Ağları tara</button></div>
 <div id="nets" class="hint">Taramak için düğmeye basın.</div>
 <div class="row">
  <div><label>Ağ adı (SSID)</label><input id="w_ssid" maxlength="32"></div>
  <div><label>Şifre</label><input id="w_pass" type="password" maxlength="63"></div>
 </div>
 <div class="btns"><button onclick="addNet()">➕ Ağı ekle / güncelle</button></div>
 <div class="hint">Tarama sırasında cihazın kendi ağı (AselGate) birkaç saniye kesilebilir; sayfa yeniden bağlanır.</div>
 <h2 style="margin-top:16px">Kayıtlı ağlar</h2>
 <div id="saved" class="hint">-</div>
</section>
</main>
<footer>yATÖLYE / <b>TA2BI-MURAT</b></footer>
<div id="toast"></div>

<script>
const $=id=>document.getElementById(id);
async function api(p,o){
  const c=new AbortController();const t=setTimeout(()=>c.abort(),8000);
  try{const r=await fetch(p,Object.assign({signal:c.signal},o||{}));return await r.json();}
  finally{clearTimeout(t);}
}
function post(p,d){return api(p,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(d||{})});}
function toast(m,ok){const t=$('toast');t.textContent=m;t.style.borderColor=ok===false?'var(--bad)':'var(--ok)';t.style.display='block';clearTimeout(toast.h);toast.h=setTimeout(()=>t.style.display='none',ok===false?8000:3500);}
function fmtUp(s){const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return (d?d+'g ':'')+h+'sa '+m+'dk';}
function fmtT(t,u){return t?new Date(t*1000).toLocaleTimeString('tr-TR'):'+'+u+' sn';}

let myCall='',prevB=null,prevT=0;
async function refresh(){
  try{
    const s=await api('/api/status');
    myCall=s.call||'';
    $('hcall').textContent=myCall||'(çağrı işareti girilmedi)';
    $('d_is').className='dot'+(s.is.ok?' on':'');
    $('st_wifi').textContent=s.wifi.sta?(s.wifi.ssid+' ('+s.wifi.rssi+' dBm)'):('Bağlı değil'+(s.wifi.msg?' – '+s.wifi.msg:''));
    $('st_ip').textContent=(s.wifi.sta?(s.wifi.ip+'  |  '+s.wifi.host):'—')+'  |  AP: '+s.wifi.ap+' ('+s.wifi.apc+')';
    $('st_is').textContent=s.is.st;
    $('st_srv').textContent=s.is.srv?(s.is.srv+' ('+s.is.host+')'):s.is.host;
    $('st_rx').textContent=s.cnt.rx;
    $('st_gated').textContent=s.cnt.gated;
    $('st_skip').textContent=s.cnt.skip;
    $('st_bytes').textContent=s.cnt.bytes;
    $('st_last').textContent=s.last<0?'henüz yok':(s.last+' sn önce');
    $('st_up').textContent=fmtUp(s.up);
    $('st_heap').textContent=Math.round(s.heap/1024)+' KB';
    $('st_pos').textContent=s.pos||'girilmedi';

    const r=s.ser, now=Date.now();
    const live=r.age>=0&&r.age<3000;
    let rate=0;
    if(prevB!==null&&now>prevT&&r.bytes>=prevB)rate=Math.round((r.bytes-prevB)*1000/(now-prevT));
    prevB=r.bytes;prevT=now;
    $('d_ser').className='dot'+(live?' on':'');
    $('ser_st').textContent=r.bytes===0?'Hiç veri gelmedi':(live?'Veri geliyor':'Veri kesildi');
    $('ser_bytes').textContent=r.bytes;
    $('ser_rate').textContent=rate+' bayt/sn';
    $('ser_age').textContent=r.age<0?'—':(r.age<1000?'şimdi':Math.round(r.age/1000)+' sn önce');
    $('ser_baud').textContent=r.baud;
    $('ser_bad').textContent=r.bad;
    $('ser_raw').textContent=r.raw||'-';
    let h='';
    if(r.bytes===0)h='GPIO5\'e hiç veri gelmiyor: MAX3232 beslemesini, ROUT1→GPIO5 bağlantısını ve ortak GND\'yi kontrol edin.';
    else if(r.bytes>50&&r.bad>r.bytes/5)h='Çok sayıda bozuk bayt var: telsiz seri hızı (baud) yanlış olabilir.';
    else if(!live)h='Veri bir süredir gelmiyor.';
    $('ser_hint').textContent=h;
  }catch(e){$('st_is').textContent='Cihaza ulaşılamıyor';}
}

let lastSeq=0;
function addPk(p){
  const d=document.createElement('div');
  const h=document.createElement('small');
  const x=document.createElement('div');
  if(p.s===0){d.className='pk';h.textContent=fmtT(p.t,p.u)+'  ✔ '+(myCall||'iGate')+' iGate üzerinden APRS-IS\'e gönderildi';}
  else if(p.s===1){d.className='pk skip';h.textContent=fmtT(p.t,p.u)+'  ⏭ Atlandı: '+p.r;}
  else if(p.s===2){d.className='pk me';h.textContent=fmtT(p.t,p.u)+'  📍 iGate konum işaretçisi APRS-IS\'e gönderildi';}
  else{d.className='pk me';h.textContent=fmtT(p.t,p.u)+'  💬 iGate durum mesajı APRS-IS\'e gönderildi';}
  x.textContent=p.x;           // textContent: RF'den gelen veri HTML olarak yorumlanmaz
  d.appendChild(h);d.appendChild(x);
  const L=$('log');const e=$('logempty');if(e)e.remove();
  L.prepend(d);
  while(L.children.length>150)L.lastChild.remove();
}
async function logs(){
  try{
    const d=await api('/api/log?after='+lastSeq);
    if(d.seq<lastSeq){lastSeq=0;$('log').innerHTML='';}
    d.p.forEach(p=>{addPk(p);lastSeq=Math.max(lastSeq,p.q);});
    if(d.seq>lastSeq&&d.p.length===0)lastSeq=d.seq;
  }catch(e){}
}

let cfgOk=false;
async function loadCfg(){
  try{
    const c=await api('/api/config');
    if(!c||c.lat===undefined)return false;
    $('f_call').value=c.call;$('f_lat').value=c.lat;$('f_lon').value=c.lon;
    $('f_cmt').value=c.cmt;$('f_tab').value=c.tab;$('f_code').value=c.code;
    $('f_srv').value=c.srv;$('f_port').value=c.port;$('f_bint').value=c.bint;
    $('f_baud').value=c.baud;$('f_fmt').value=c.fmt;$('f_block').value=c.blk;
    $('f_pass').placeholder=c.hasPass?'(kayıtlı - değiştirmek için yeni değer girin)':'passcode';
    $('f_uipw').placeholder=c.hasUi?'(ayarlı - değiştir / - ile kaldır)':'boş=şifresiz';
    if(window.setMapPos)setMapPos(parseFloat(c.lat),parseFloat(c.lon),true);
    cfgOk=true;$('savebtn').disabled=false;
    return true;
  }catch(e){return false;}
}
async function save(){
  if(!cfgOk){toast('Ayarlar henüz yüklenmedi, birkaç saniye bekleyin',false);return;}
  const d={call:$('f_call').value,pass:$('f_pass').value,lat:$('f_lat').value,lon:$('f_lon').value,
    cmt:$('f_cmt').value,tab:$('f_tab').value,code:$('f_code').value,srv:$('f_srv').value,
    port:$('f_port').value,bint:$('f_bint').value,baud:$('f_baud').value,fmt:$('f_fmt').value,uipw:$('f_uipw').value,blk:$('f_block').value};
  try{const r=await post('/api/config',d);toast(r.msg,r.ok);if(r.ok){$('f_pass').value='';$('f_uipw').value='';loadCfg();refresh();}}
  catch(e){toast('Kaydedilemedi',false);}
}
async function beacon(){try{const r=await post('/api/beacon');toast(r.msg,r.ok);}catch(e){toast('Hata',false);}}
async function restart(){if(!confirm('Cihaz yeniden başlatılsın mı?'))return;try{await post('/api/restart');toast('Yeniden başlatılıyor…');}catch(e){}}

/* ---- harita ---- */
let map=null,marker=null;
function setMapPos(la,lo,center){
  if(!map||isNaN(la)||isNaN(lo)||(la===0&&lo===0))return;
  if(!marker)marker=L.marker([la,lo]).addTo(map);else marker.setLatLng([la,lo]);
  if(center)map.setView([la,lo],12);
}
function initMap(){
  const l=document.createElement('link');l.rel='stylesheet';l.href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css';document.head.appendChild(l);
  const s=document.createElement('script');s.src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js';
  s.onload=()=>{
    const m=$('map');m.innerHTML='';m.style.display='block';
    map=L.map('map').setView([39,35],5);
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png',{maxZoom:18,attribution:'© OpenStreetMap'}).addTo(map);
    map.on('click',e=>{$('f_lat').value=e.latlng.lat.toFixed(6);$('f_lon').value=e.latlng.lng.toFixed(6);setMapPos(e.latlng.lat,e.latlng.lng,false);});
    ['f_lat','f_lon'].forEach(i=>$(i).addEventListener('change',()=>setMapPos(parseFloat($('f_lat').value.replace(',','.')),parseFloat($('f_lon').value.replace(',','.')),true)));
    setMapPos(parseFloat($('f_lat').value),parseFloat($('f_lon').value),true);
  };
  s.onerror=()=>{$('map').textContent='Harita yüklenemedi (internet yok). Koordinatları elle girebilirsiniz.';};
  document.head.appendChild(s);
}
function pasteCoord(){
  const m=$('f_paste').value.match(/(-?\d+(?:[.]\d+)?)\s*[,;\s]\s*(-?\d+(?:[.]\d+)?)/);
  if(!m)return;
  const la=parseFloat(m[1]),lo=parseFloat(m[2]);
  if(Math.abs(la)>90||Math.abs(lo)>180)return;
  $('f_lat').value=la.toFixed(6);$('f_lon').value=lo.toFixed(6);
  setMapPos(la,lo,true);toast('Koordinat alındı, Kaydet\'e basın');
}
function geo(){
  if(!window.isSecureContext||!navigator.geolocation){
    toast('Tarayıcılar konumu yalnızca https sayfalarda verir; bu sayfa http olduğu için engelleniyor. "Koordinat yapıştır" alanını veya haritayı kullanın.',false);return;}
  navigator.geolocation.getCurrentPosition(p=>{
    $('f_lat').value=p.coords.latitude.toFixed(6);$('f_lon').value=p.coords.longitude.toFixed(6);
    setMapPos(p.coords.latitude,p.coords.longitude,true);toast('Konum alındı');
  },e=>toast('Konum alınamadı (http üzerinde tarayıcı engelleyebilir): '+e.message,false),{enableHighAccuracy:true,timeout:15000});
}

/* ---- Wi-Fi ---- */
async function scan(){
  const b=$('scanbtn');b.disabled=true;$('nets').textContent='Taranıyor…';
  try{await api('/api/scan?start=1');}catch(e){}
  let done=false;
  for(let i=0;i<25&&!done;i++){
    await new Promise(r=>setTimeout(r,1500));
    try{const d=await api('/api/scan');if(d.st==='done'){showNets(d.n);done=true;}}catch(e){}
  }
  if(!done)$('nets').textContent='Tarama sonucu alınamadı, tekrar deneyin.';
  b.disabled=false;
}
function showNets(n){
  const best={};n.forEach(x=>{if(x.s&&(!best[x.s]||x.r>best[x.s].r))best[x.s]=x;});
  const a=Object.values(best).sort((p,q)=>q.r-p.r);
  const box=$('nets');box.innerHTML='';
  if(!a.length){box.textContent='Ağ bulunamadı.';return;}
  a.forEach(x=>{
    const d=document.createElement('div');d.className='net';
    const l=document.createElement('div');l.textContent=(x.e?'🔒 ':'🔓 ')+x.s;
    const m=document.createElement('span');m.className='m';m.textContent='  '+x.r+' dBm · kanal '+x.c;l.appendChild(m);
    const bt=document.createElement('button');bt.className='sec';bt.textContent='Seç';
    bt.onclick=()=>{$('w_ssid').value=x.s;$('w_pass').focus();};
    d.appendChild(l);d.appendChild(bt);box.appendChild(d);
  });
}
async function addNet(){
  const s=$('w_ssid').value.trim();if(!s){toast('Ağ adı girin',false);return;}
  try{const r=await post('/api/wifi/add',{ssid:s,pass:$('w_pass').value});toast(r.msg,r.ok);if(r.ok){$('w_pass').value='';loadNets();}}
  catch(e){toast('Hata',false);}
}
async function delNet(i){
  if(!confirm('Bu ağ silinsin mi?'))return;
  try{const r=await post('/api/wifi/del',{idx:i});toast(r.msg,r.ok);loadNets();}catch(e){}
}
async function loadNets(){
  try{
    const d=await api('/api/wifi');const box=$('saved');box.innerHTML='';
    if(!d.nets.length){box.textContent='Kayıtlı ağ yok.';return;}
    d.nets.forEach((s,i)=>{
      const r=document.createElement('div');r.className='net';
      const l=document.createElement('div');l.textContent=s+(s===d.cur?'  ✅ bağlı':'');
      const b=document.createElement('button');b.className='del';b.textContent='Sil';b.onclick=()=>delNet(i);
      r.appendChild(l);r.appendChild(b);box.appendChild(r);
    });
  }catch(e){}
}

/* İstekler sırayla yapılır: ESP32 aynı anda tek istemciye hizmet verir */
(async function(){
  while(!(await loadCfg())){await new Promise(r=>setTimeout(r,1500));}
  await loadNets();
  initMap();
  let n=0;
  (async function tick(){
    await refresh();await logs();
    if(n++%6===0)await loadNets();
    setTimeout(tick,2500);
  })();
})();
</script></body></html>
)rawliteral";

// ======================= YARDIMCILAR =======================
const char* B(bool b) { return b ? "true" : "false"; }

String jEsc(const String& s) {
  String o; o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += (char)c; }
    else if (c < 0x20) { o += '.'; }
    else o += (char)c;
  }
  return o;
}

uint32_t nowEpoch() { time_t t = time(nullptr); return t > 1700000000 ? (uint32_t)t : 0; }

uint32_t fnv(const String& s) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < s.length(); i++) { h ^= (uint8_t)s[i]; h *= 16777619u; }
  return h;
}

bool hasPos() { return !(cfg.lat == 0.0 && cfg.lon == 0.0); }

void sendJson(int code, const String& s) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", s);
}

bool authOk() {
  if (cfg.uipw.length() == 0) return true;
  if (server.authenticate("admin", cfg.uipw.c_str())) return true;
  server.requestAuthentication(BASIC_AUTH, "AselGate", "Giris gerekli");
  return false;
}

// ======================= AYAR SAKLAMA =======================
void loadConfig() {
  cfg.call    = prefs.getString("call", "");
  cfg.pass    = prefs.getString("pass", "");
  cfg.comment = prefs.getString("cmt", "ESP32-C3 iGate");
  cfg.srv     = prefs.getString("srv", "rotate.aprs2.net");
  cfg.uipw    = prefs.getString("uipw", "");
  cfg.block   = prefs.getString("blk", "");
  cfg.lat     = prefs.getDouble("lat", 0.0);
  cfg.lon     = prefs.getDouble("lon", 0.0);
  cfg.symTab  = (char)prefs.getUChar("stab", 'I');
  cfg.symCode = (char)prefs.getUChar("scod", '&');
  cfg.port    = prefs.getUShort("port", 14580);
  cfg.beaconMin = prefs.getUShort("bint", 30);
  cfg.baud    = prefs.getUInt("baud", 9600);
  cfg.fmt     = prefs.getUChar("fmt", 0);
}

void saveConfig() {
  prefs.putString("call", cfg.call);
  prefs.putString("pass", cfg.pass);
  prefs.putString("cmt", cfg.comment);
  prefs.putString("srv", cfg.srv);
  prefs.putString("uipw", cfg.uipw);
  prefs.putString("blk", cfg.block);
  prefs.putDouble("lat", cfg.lat);
  prefs.putDouble("lon", cfg.lon);
  prefs.putUChar("stab", (uint8_t)cfg.symTab);
  prefs.putUChar("scod", (uint8_t)cfg.symCode);
  prefs.putUShort("port", cfg.port);
  prefs.putUShort("bint", cfg.beaconMin);
  prefs.putUInt("baud", cfg.baud);
  prefs.putUChar("fmt", cfg.fmt);
}

void loadNets() {
  netCount = 0;
  for (int i = 0; i < MAX_NETS; i++) {
    String s = prefs.getString(("ws" + String(i)).c_str(), "");
    if (s.length()) {
      netSsid[netCount] = s;
      netPass[netCount] = prefs.getString(("wp" + String(i)).c_str(), "");
      netCount++;
    }
  }
}

void saveNets() {
  for (int i = 0; i < MAX_NETS; i++) {
    String k1 = "ws" + String(i), k2 = "wp" + String(i);
    prefs.putString(k1.c_str(), i < netCount ? netSsid[i] : String(""));
    prefs.putString(k2.c_str(), i < netCount ? netPass[i] : String(""));
  }
}

// ======================= GÜNLÜK =======================
void addLog(uint8_t state, const char* reason, const String& text) {
  LogEntry& e = logs[logSeq % LOG_N];
  logSeq++;
  e.seq = logSeq; e.t = nowEpoch(); e.up = millis() / 1000; e.state = state;
  strlcpy(e.reason, reason, sizeof(e.reason));
  size_t n = 0;
  for (size_t i = 0; i < text.length() && n < sizeof(e.text) - 1; i++) {
    unsigned char c = text[i];
    e.text[n++] = (c < 0x20 || c > 0x7E) ? '.' : (char)c;
  }
  e.text[n] = 0;
}

// ======================= APRS KONUM BİÇİMİ =======================
String aprsLat(double lat) {
  char h = lat >= 0 ? 'N' : 'S'; lat = fabs(lat);
  int d = (int)lat; double m = (lat - d) * 60.0;
  char b[16]; snprintf(b, sizeof(b), "%02d%05.2f%c", d, m, h);
  return String(b);
}
String aprsLon(double lon) {
  char h = lon >= 0 ? 'E' : 'W'; lon = fabs(lon);
  int d = (int)lon; double m = (lon - d) * 60.0;
  char b[16]; snprintf(b, sizeof(b), "%03d%05.2f%c", d, m, h);
  return String(b);
}

void sendBeacon() {
  beaconDue = false;
  if (!isVerified || !isClient.connected() || !hasPos()) return;
  String p = cfg.call + ">" TOCALL ",TCPIP*:!" + aprsLat(cfg.lat) + String(cfg.symTab) +
             aprsLon(cfg.lon) + String(cfg.symCode) + cfg.comment;
  isClient.print(p + "\r\n");
  lastBeacon = millis(); cntBeacon++;
  addLog(2, "", p);

  // Her konum paketinden sonra sabit DURUM paketi:  CAGRI>AP2BI,TCPIP*:>durum metni
  String st = cfg.call + ">" TOCALL ",TCPIP*:>" STATUS_TEXT;
  isClient.print(st + "\r\n");
  addLog(3, "", st);
}

// ======================= ENGELLENEN ÇAĞRI İŞARETLERİ =======================
// Engel listesi: "TA1XYZ TA2ABC-7". SSID'siz giriş o çağrı işaretinin tüm SSID'lerini engeller.
bool isBlocked(const String& src) {
  const String& b = cfg.block;
  size_t i = 0;
  while (i < b.length()) {
    while (i < b.length() && b[i] == ' ') i++;
    size_t j = i;
    while (j < b.length() && b[j] != ' ') j++;
    if (j > i) {
      String t = b.substring(i, j);
      if (src == t) return true;
      if (t.indexOf('-') < 0 && src.startsWith(t + "-")) return true;
    }
    i = j;
  }
  return false;
}

// ======================= GPS (NMEA) ÇÖZÜMLEME =======================
// $GPGGA / $GPRMC / $GPGLL (GP, GN, GL... fark etmez) cümlelerinden konum alınır.
static String nmeaField(const String& s, int n) {
  int start = 0;
  for (int i = 0; i < n; i++) { start = s.indexOf(',', start); if (start < 0) return ""; start++; }
  int end = s.indexOf(',', start);
  int star = s.indexOf('*', start);
  if (end < 0 || (star >= 0 && star < end)) end = star;
  if (end < 0) end = s.length();
  return s.substring(start, end);
}

static bool nmeaChecksumOk(const String& s) {
  int star = s.indexOf('*');
  if (star < 1 || star + 3 > (int)s.length()) return false;
  uint8_t x = 0;
  for (int i = 1; i < star; i++) x ^= (uint8_t)s[i];
  char b[3] = { s[star + 1], s[star + 2], 0 };
  return x == (uint8_t)strtol(b, nullptr, 16);
}

static double nmeaDeg(const String& v, const String& hemi) {      // ddmm.mmmm -> ondalık derece
  double raw = v.toDouble();
  int deg = (int)(raw / 100);
  double d = deg + (raw - deg * 100) / 60.0;
  if (hemi == "S" || hemi == "W") d = -d;
  return d;
}

static void setGpsPos(const String& la, const String& ns, const String& lo, const String& ew) {
  if (la.length() < 4 || lo.length() < 5) return;
  double a = nmeaDeg(la, ns), b = nmeaDeg(lo, ew);
  if (fabs(a) > 90 || fabs(b) > 180 || (a == 0 && b == 0)) return;
  gps.lat = a; gps.lon = b; gps.have = true;
  gps.fixMs = millis(); if (!gps.fixMs) gps.fixMs = 1;
}

static void setGpsTime(const String& t) {
  if (t.length() >= 6) snprintf(gps.tm, sizeof(gps.tm), "%c%c:%c%c:%c%c", t[0], t[1], t[2], t[3], t[4], t[5]);
}

void parseNmea(const String& s) {
  if (s.length() < 12 || !nmeaChecksumOk(s)) return;
  String type = s.substring(3, 6);
  if (type == "GGA") {
    gps.fix  = nmeaField(s, 6).toInt() > 0;
    gps.sats = (uint8_t)nmeaField(s, 7).toInt();
    String alt = nmeaField(s, 9); if (alt.length()) gps.alt = alt.toDouble();
    setGpsTime(nmeaField(s, 1));
    if (gps.fix) setGpsPos(nmeaField(s, 2), nmeaField(s, 3), nmeaField(s, 4), nmeaField(s, 5));
  } else if (type == "RMC") {
    bool a = (nmeaField(s, 2) == "A");
    gps.fix = a;
    setGpsTime(nmeaField(s, 1));
    if (a) {
      setGpsPos(nmeaField(s, 3), nmeaField(s, 4), nmeaField(s, 5), nmeaField(s, 6));
      gps.spd = nmeaField(s, 7).toDouble() * 1.852;               // knot -> km/sa
    }
  } else if (type == "GLL") {
    if (nmeaField(s, 6) == "A") setGpsPos(nmeaField(s, 1), nmeaField(s, 2), nmeaField(s, 3), nmeaField(s, 4));
  }
}

// ======================= PAKET İŞLEME (RF -> APRS-IS) =======================
// Duyulan paketi USB seri porta kopyalar. Tampon doluysa (PC okumuyor) satır atılır, döngü beklemez.
void usbCopy(const String& s) {
  if ((size_t)Serial.availableForWrite() < s.length() + 2) return;
  Serial.print(s);
  Serial.print("\r\n");
}

void gatePacket(const String& s);

// Bağlantı yokken duyulan paketler: kısa süre saklanır, giriş doğrulanınca sırayla gönderilir
#define QN 12
String   qBuf[QN]; uint32_t qMs[QN]; uint8_t qHead = 0, qCount = 0;
void qPush(const String& o) {
  if (qCount == QN) { qBuf[qHead] = ""; qHead = (qHead + 1) % QN; qCount--; }
  uint8_t i = (qHead + qCount) % QN;
  qBuf[i] = o; qMs[i] = millis() ? millis() : 1; qCount++;
}
void qFlush() {
  while (qCount) {
    String& o = qBuf[qHead];
    if (millis() - qMs[qHead] < 60000UL) { isClient.print(o + "\r\n"); cntGated++; addLog(0, "", o); }
    o = ""; qHead = (qHead + 1) % QN; qCount--;
  }
}

void skipPkt(const char* reason, const String& raw) {
  cntSkip++;
  addLog(1, reason, raw);
}

void handlePacket(const String& sIn) {
  // Telsiz GPS'ten gelen NMEA cümlelerini ($GPGGA, $GPGLL, $GPGSV...) de seri çıkıştan verir.
  // Bunlar APRS paketi değildir: sayılmaz, günlüğe yazılmaz, sessizce yok sayılır.
  String s = sIn;
  s.trim();
  if (!s.length()) return;
  int dl = s.indexOf('$');
  if (dl == 0 || (dl > 0 && s.indexOf(':') < 0)) {     // NMEA (başında artık karakter olabilir)
    cntNmea++;
    parseNmea(dl > 0 ? s.substring(dl) : s);
    return;
  }

  cntRx++; lastRxMs = millis(); if (lastRxMs == 0) lastRxMs = 1;
  gatePacket(s);                                       // önce APRS-IS'e gönder (en düşük gecikme)
  usbCopy(s);                                          // duyulan her paket sonra USB'ye de gider
}

void gatePacket(const String& s) {

  int c = s.indexOf(':'), g = s.indexOf('>');
  if (c < 0 || g < 1 || g > c || c + 1 >= (int)s.length()) { skipPkt("Geçersiz TNC2 biçimi", s); return; }

  String hdr = s.substring(0, c);
  String pay = s.substring(c + 1);
  while (pay.length() && (pay[pay.length() - 1] == '\r' || pay[pay.length() - 1] == '\n')) pay.remove(pay.length() - 1);
  if (!pay.length()) { skipPkt("Boş veri", s); return; }

  String src = hdr.substring(0, g);
  src.toUpperCase();
  bool okSrc = src.length() >= 3 && src.length() <= 9;
  for (size_t i = 0; okSrc && i < src.length(); i++) if (!(isalnum((unsigned char)src[i]) || src[i] == '-')) okSrc = false;
  if (!okSrc) { skipPkt("Geçersiz kaynak çağrı işareti", s); return; }
  if (isBlocked(src)) { skipPkt("Engellenen çağrı işareti", s); return; }

  String hu = hdr; hu.toUpperCase();
  if (hu.indexOf("TCPIP") >= 0 || hu.indexOf("TCPXX") >= 0) { skipPkt("TCPIP/TCPXX yolu", s); return; }
  if (hu.indexOf("NOGATE") >= 0 || hu.indexOf("RFONLY") >= 0) { skipPkt("NOGATE/RFONLY", s); return; }
  if (hu.indexOf(",QA") >= 0) { skipPkt("q-yapısı mevcut", s); return; }
  if (pay[0] == '}') { skipPkt("Üçüncü taraf paketi", s); return; }
  if (pay[0] == '?') { skipPkt("Sorgu paketi", s); return; }

  // Tekrar engelleme (30 sn)
  uint32_t h = fnv(src + ":" + pay);
  uint32_t now = millis();
  for (int i = 0; i < 24; i++)
    if (seen[i].h == h && seen[i].ms && now - seen[i].ms < 30000) { skipPkt("Tekrar (30 sn içinde)", s); return; }

  bool live = isConnected && isClient.connected() && isVerified;
  if (!live && (cfg.call.length() == 0 || cfg.pass.length() == 0)) { skipPkt("Çağrı işareti / passcode girilmedi", s); return; }

  seen[seenIdx].h = h; seen[seenIdx].ms = now ? now : 1; seenIdx = (seenIdx + 1) % 24;

  // CAGRI>DEST,PATH,qAR,IGATE:veri
  String out = hdr + ",qAR," + cfg.call + ":" + pay;
  if (!live) { qPush(out); addLog(1, "Kuyruğa alındı (APRS-IS bağlı değil)", s); return; }
  isClient.print(out + "\r\n");
  cntGated++;
  addLog(0, "", out);
}

// ======================= KISS / AX.25 =======================
static void ax25Addr(const uint8_t* a, char* dst) {
  int k = 0;
  for (int j = 0; j < 6; j++) { char ch = a[j] >> 1; if (ch != ' ') dst[k++] = ch; }
  int ssid = (a[6] >> 1) & 0x0F;
  if (ssid) k += sprintf(dst + k, "-%d", ssid);
  dst[k] = 0;
}

void kissFrame(const uint8_t* f, size_t n) {
  if (n < 17 || (f[0] & 0x0F) != 0) return;          // sadece veri çerçeveleri
  const uint8_t* p = f + 1; size_t m = n - 1;
  size_t i = 0;
  while (!(p[i + 6] & 1)) { i += 7; if (i + 7 > m) return; }
  size_t alen = i + 7;
  if (alen < 14 || alen > 70 || alen + 2 > m) return;
  if (p[alen] != 0x03 || p[alen + 1] != 0xF0) return;  // UI çerçevesi, PID 0xF0
  char src[16], dst[16], dg[16];
  ax25Addr(p + 7, src); ax25Addr(p, dst);
  String line = String(src) + ">" + dst;
  int lastH = -1;
  for (size_t a = 14; a < alen; a += 7) if (p[a + 6] & 0x80) lastH = (int)a;
  for (size_t a = 14; a < alen; a += 7) {
    ax25Addr(p + a, dg);
    line += ","; line += dg;
    if ((int)a == lastH) line += "*";
  }
  line += ":";
  for (size_t j = alen + 2; j < m; j++) {
    uint8_t ch = p[j];
    if (ch == 0 || ch == '\r' || ch == '\n') continue;
    line += (char)ch;
  }
  handlePacket(line);
}

void kissByte(uint8_t b) {
  if (b == 0xC0) {
    if (klen > 1) { lastRaw = "KISS çerçevesi, " + String((unsigned)klen) + " bayt"; kissFrame(kbuf, klen); }
    klen = 0; kesc = false; return;
  }
  if (b == 0xDB) { kesc = true; return; }
  if (kesc) { kesc = false; if (b == 0xDC) b = 0xC0; else if (b == 0xDD) b = 0xDB; }
  if (klen < sizeof(kbuf)) kbuf[klen++] = b; else klen = 0;
}

void serialInit() {
  Serial1.end();
  Serial1.setRxBufferSize(4096);
  Serial1.begin(cfg.baud, SERIAL_8N1, RX_PIN, TX_PIN);
  rxLen = 0; klen = 0; kesc = false;
}

void pollSerial() {
  while (Serial1.available()) {
    uint8_t b = Serial1.read();
    bytesRx++;
    lastByteMs = millis(); if (!lastByteMs) lastByteMs = 1;
    if (cfg.fmt == 0 && b != '\r' && b != '\n' && (b < 0x20 || b > 0x7E)) cntBad++;
    if (cfg.fmt == 1) { kissByte(b); continue; }
    if (b == '\r' || b == '\n') {
      if (rxLen) {
        rxBuf[rxLen] = 0; String s(rxBuf);
        lastRaw = "";
        for (size_t i = 0; i < rxLen && i < 120; i++) { char ch = rxBuf[i]; lastRaw += (ch < 0x20 || ch > 0x7E) ? '.' : ch; }
        rxLen = 0; handlePacket(s);
      }
    } else if (b != 0) {
      if (rxLen < sizeof(rxBuf) - 1) rxBuf[rxLen++] = b; else rxLen = 0;
    }
  }
}

// ======================= APRS-IS BAĞLANTISI =======================
void handleIsLine(const String& l) {
  lastIsRx = millis();
  if (!l.length() || l[0] != '#') return;
  String low = l; low.toLowerCase();
  if (low.indexOf("logresp") < 0) return;
  int sv = low.indexOf("server ");
  if (sv >= 0) { isSrvName = l.substring(sv + 7); isSrvName.trim(); }
  if (low.indexOf("unverified") >= 0) {
    isVerified = false;
    isStatus = (cfg.pass == "-1") ? "Bağlı (yalnızca alma, passcode -1)" : "Bağlı ama passcode HATALI";
  } else if (low.indexOf("verified") >= 0) {
    isVerified = true;
    isStatus = "Bağlı ve doğrulandı";
    beaconDue = true;
  }
}

void isDisconnect(const char* why) {
  if (isClient.connected()) isClient.stop();
  isConnected = false; isVerified = false; isLine = "";
  isStatus = why;
}

void isLoop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (isConnected || isClient.connected()) isDisconnect("Wi-Fi bağlantısı yok");
    else isStatus = "Wi-Fi bağlantısı yok";
    return;
  }
  if (cfg.call.length() == 0 || cfg.pass.length() == 0) { isStatus = "Çağrı işareti / passcode girilmedi"; return; }

  if (!isClient.connected()) {
    if (isConnected) isDisconnect("Bağlantı koptu, yeniden denenecek");
    if (lastIsTry && millis() - lastIsTry < isRetryMs) return;
    lastIsTry = millis(); if (!lastIsTry) lastIsTry = 1;
    isStatus = "Sunucuya bağlanılıyor…";
    if (isClient.connect(cfg.srv.c_str(), cfg.port, 2000)) {
      isClient.setNoDelay(true);
      isClient.print("user " + cfg.call + " pass " + cfg.pass + " vers AselGate " FW_VERSION "\r\n");
      isConnected = true; isVerified = false; isLine = "";
      lastIsRx = lastKeep = millis();
      isStatus = "Giriş yapılıyor…";
      isRetryMs = 500;
    } else {
      isStatus = "Sunucuya bağlanılamadı";
      isRetryMs = isRetryMs * 2 > 15000 ? 15000 : isRetryMs * 2;
    }
    return;
  }

  while (isClient.available()) {
    char ch = (char)isClient.read();
    if (ch == '\n') { handleIsLine(isLine); isLine = ""; }
    else if (ch != '\r' && isLine.length() < 300) isLine += ch;
  }

  if (millis() - lastIsRx > 60000) { isDisconnect("Sunucudan yanıt yok, yeniden bağlanılıyor"); return; }
  if (millis() - lastKeep > 30000) { isClient.print("# AselGate keepalive\r\n"); lastKeep = millis(); }

  if (isVerified) {
    if (qCount) qFlush();
    if (beaconDue) sendBeacon();
    else if (cfg.beaconMin && millis() - lastBeacon > (uint32_t)cfg.beaconMin * 60000UL) sendBeacon();
  }
}

// ======================= Wi-Fi YÖNETİMİ =======================
void apStart(IPAddress ip) {
  apIP = ip;
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASS);
  dns.stop();
  dns.start(53, "*", apIP);                 // AP'ye bağlanınca ayar sayfası otomatik açılsın
}

static bool subnetsOverlap(IPAddress a, IPAddress am, IPAddress b, IPAddress bm) {
  uint32_t m = (uint32_t)am & (uint32_t)bm;
  return (((uint32_t)a ^ (uint32_t)b) & m) == 0;
}

// Wi-Fi hata kodunu Türkçe açıklamaya çevirir
static String reasonText(int r) {
  switch (r) {
    case 0:   return "yanıt yok (ağ menzilde mi? 2.4 GHz mi?)";
    case 201: return "ağ bulunamadı (adı doğru mu? 2.4 GHz mi? menzilde mi?)";
    case 202: case 204: case 15: case 2: case 203:
              return "şifre hatalı olabilir";
    case 200: return "sinyal çok zayıf / modem yanıt vermedi";
    case 205: return "bağlantı kurulamadı";
    case 8:   return "modem bağlantıyı kapattı";
    default:  return "hata kodu " + String(r);
  }
}

// Ağda görünürlük: mDNS (AselGate.local) + NetBIOS (Windows'ta "AselGate")
static bool nbOn = false;
void startNameServices() {
  if (mdnsOn) { MDNS.end(); mdnsOn = false; }       // IP değişmiş olabilir: her bağlantıda yeniden başlat
  mdnsOn = MDNS.begin(HOSTNAME);
  if (mdnsOn) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "device", "APRS iGate");
  }
  if (!nbOn) nbOn = NBNS.begin(HOSTNAME);
}

uint32_t wifiLostMs = 0;

void wifiLoop() {
  bool nowSta = (WiFi.status() == WL_CONNECTED);
  if (nowSta && !wasSta) {
    wifiTry = -1; wifiReason = 0; wifiLostMs = 0;
    wifiMsg = "";
    startNameServices();
    lastIsTry = 0;
    apCheck = true; staUpMs = millis();
    Serial.println("# Wi-Fi baglandi: " + WiFi.SSID() + "  IP: " + WiFi.localIP().toString() +
                   "  http://" HOSTNAME ".local");
  }
  if (!nowSta && wasSta) {
    wifiLostMs = millis(); if (!wifiLostMs) wifiLostMs = 1;
    wifiMsg = "Bağlantı koptu, yeniden deneniyor…";
  }
  wasSta = nowSta;

  // Modem ağı ile AP ağı aynı bloktaysa (ör. ikisi de 192.168.1.x) AP'yi başka bloğa taşı
  if (apCheck && nowSta && millis() - staUpMs > 30000) {
    apCheck = false;
    IPAddress apMask(255, 255, 255, 0);
    if (subnetsOverlap(WiFi.localIP(), WiFi.subnetMask(), apIP, apMask)) {
      IPAddress cand[3] = { IPAddress(192, 168, 50, 1), IPAddress(172, 30, 50, 1), IPAddress(10, 250, 50, 1) };
      IPAddress pick = cand[0];
      for (int i = 0; i < 3; i++)
        if (!subnetsOverlap(WiFi.localIP(), WiFi.subnetMask(), cand[i], apMask)) { pick = cand[i]; break; }
      WiFi.softAPdisconnect(false);
      delay(200);
      apStart(pick);
    }
  }
  if (nowSta) return;
  if (netCount == 0) { wifiMsg = "Kayıtlı ağ yok"; return; }
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;      // tarama sürerken bağlantı denemesi yapma

  uint32_t now = millis();
  if (wifiTry >= 0) {
    if (wifiTry >= netCount) wifiTry = -1;                   // ağ silinmiş
    else {
      if (now - wifiTryMs < 15000) return;                   // deneme sürüyor
      wifiMsg = netSsid[wifiTry] + ": " + reasonText(wifiReason);
      wifiIdx = (wifiTry + 1) % netCount;                    // başarısız: sıradaki ağı dene
    }
  }
  if (wifiTry < 0) {
    // Bağlantı yeni koptuysa önce sürücünün kendi yeniden bağlanmasına izin ver
    if (wifiLostMs && now - wifiLostMs < 8000) return;
    wifiLostMs = 0;
    if (wifiIdx < 0 || wifiIdx >= netCount) wifiIdx = 0;     // aynı ağa dönmeyi dene
  }
  wifiReason = 0;
  WiFi.setHostname(HOSTNAME);
  WiFi.disconnect();
  WiFi.begin(netSsid[wifiIdx].c_str(), netPass[wifiIdx].c_str());
  wifiTry = wifiIdx; wifiTryMs = now ? now : 1;
  wifiMsg = netSsid[wifiIdx] + " deneniyor…";
}

// ======================= WEB İŞLEYİCİLER =======================
void handleRoot() {
  if (!authOk()) return;
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNotFound() {
  String ip = server.client().localIP().toString();
  server.sendHeader("Location", "http://" + ip + "/");
  server.send(302, "text/plain", "");
}

void handleStatus() {
  if (!authOk()) return;
  bool sta = (WiFi.status() == WL_CONNECTED);
  String ssid = sta ? WiFi.SSID() : String("");
  String ip   = sta ? WiFi.localIP().toString() : String("");
  int rssi    = sta ? WiFi.RSSI() : 0;
  long last   = lastRxMs ? (long)((millis() - lastRxMs) / 1000) : -1;

  String o; o.reserve(800);
  o += "{\"call\":\"" + jEsc(cfg.call) + "\",\"fw\":\"" FW_VERSION "\"";
  o += ",\"wifi\":{\"sta\":"; o += B(sta);
  o += ",\"ssid\":\"" + jEsc(ssid) + "\",\"ip\":\"" + ip + "\",\"rssi\":" + String(rssi);
  o += ",\"ap\":\"" + WiFi.softAPIP().toString() + "\",\"apc\":" + String(WiFi.softAPgetStationNum());
  o += ",\"host\":\"" HOSTNAME ".local\",\"msg\":\"" + jEsc(wifiMsg) + "\"}";
  o += ",\"is\":{\"st\":\"" + jEsc(isStatus) + "\",\"ok\":"; o += B(isVerified);
  o += ",\"conn\":"; o += B(isConnected);
  o += ",\"srv\":\"" + jEsc(isSrvName) + "\",\"host\":\"" + jEsc(cfg.srv) + ":" + String(cfg.port) + "\"}";
  o += ",\"cnt\":{\"rx\":" + String(cntRx) + ",\"gated\":" + String(cntGated) + ",\"skip\":" + String(cntSkip) +
       ",\"beacon\":" + String(cntBeacon) + ",\"bytes\":" + String(bytesRx) + ",\"nmea\":" + String(cntNmea) + "}";
  o += ",\"last\":" + String(last) + ",\"up\":" + String(millis() / 1000) + ",\"heap\":" + String(ESP.getFreeHeap());
  o += ",\"pos\":\"" + (hasPos() ? String(cfg.lat, 5) + ", " + String(cfg.lon, 5) : String("")) + "\"";
  long sAge = lastByteMs ? (long)(millis() - lastByteMs) : -1;
  o += ",\"ser\":{\"bytes\":" + String(bytesRx) + ",\"age\":" + String(sAge) + ",\"baud\":" + String(cfg.baud) +
       ",\"bad\":" + String(cntBad) + ",\"raw\":\"" + jEsc(lastRaw) + "\"}";
  long gAge = gps.fixMs ? (long)((millis() - gps.fixMs) / 1000) : -1;
  o += ",\"gps\":{\"have\":"; o += B(gps.have);
  o += ",\"fix\":"; o += B(gps.fix);
  o += ",\"lat\":" + String(gps.lat, 6) + ",\"lon\":" + String(gps.lon, 6);
  o += ",\"sats\":" + String(gps.sats) + ",\"alt\":" + String(gps.alt, 1) + ",\"spd\":" + String(gps.spd, 1);
  o += ",\"tm\":\"" + String(gps.tm) + "\",\"age\":" + String(gAge) + ",\"n\":" + String(cntNmea) + "}";
  o += ",\"t\":" + String(nowEpoch()) + "}";
  sendJson(200, o);
}

void handleLog() {
  if (!authOk()) return;
  uint32_t after = (uint32_t)server.arg("after").toInt();
  String o; o.reserve(2048);
  o += "{\"seq\":" + String(logSeq) + ",\"p\":[";
  bool first = true;
  uint32_t start = (logSeq > LOG_N) ? logSeq - LOG_N : 0;
  for (uint32_t s0 = start; s0 < logSeq; s0++) {
    LogEntry& e = logs[s0 % LOG_N];
    if (e.seq <= after) continue;
    if (!first) o += ",";
    first = false;
    o += "{\"q\":" + String(e.seq) + ",\"t\":" + String(e.t) + ",\"u\":" + String(e.up) +
         ",\"s\":" + String(e.state) + ",\"r\":\"" + jEsc(String(e.reason)) + "\",\"x\":\"" + jEsc(String(e.text)) + "\"}";
  }
  o += "]}";
  sendJson(200, o);
}

void handleGetCfg() {
  if (!authOk()) return;
  String o;
  o += "{\"call\":\"" + jEsc(cfg.call) + "\",\"hasPass\":"; o += B(cfg.pass.length() > 0);
  o += ",\"lat\":\"" + String(cfg.lat, 6) + "\",\"lon\":\"" + String(cfg.lon, 6) + "\"";
  o += ",\"cmt\":\"" + jEsc(cfg.comment) + "\",\"tab\":\"" + jEsc(String(cfg.symTab)) + "\",\"code\":\"" + jEsc(String(cfg.symCode)) + "\"";
  o += ",\"srv\":\"" + jEsc(cfg.srv) + "\",\"port\":" + String(cfg.port) + ",\"bint\":" + String(cfg.beaconMin);
  o += ",\"blk\":\"" + jEsc(cfg.block) + "\"";
  o += ",\"baud\":" + String(cfg.baud) + ",\"fmt\":" + String(cfg.fmt) + ",\"hasUi\":"; o += B(cfg.uipw.length() > 0);
  o += "}";
  sendJson(200, o);
}

void errJson(const char* msg) { sendJson(400, String("{\"ok\":false,\"msg\":\"") + msg + "\"}"); }

void handleSaveCfg() {
  if (!authOk()) return;

  String call = server.arg("call"); call.trim(); call.toUpperCase();
  if (call.length() > 9) { errJson("Çağrı işareti en fazla 9 karakter olabilir"); return; }
  for (size_t i = 0; i < call.length(); i++)
    if (!(isalnum((unsigned char)call[i]) || call[i] == '-')) { errJson("Çağrı işareti geçersiz karakter içeriyor"); return; }

  String pw = server.arg("pass"); pw.trim();
  if (pw.length()) {
    bool okp = (pw == "-1");
    if (!okp) { okp = pw.length() <= 6; for (size_t i = 0; i < pw.length(); i++) if (!isdigit((unsigned char)pw[i])) okp = false; }
    if (!okp) { errJson("Passcode sayı olmalı (veya -1)"); return; }
  }

  String ls = server.arg("lat"), os = server.arg("lon");
  ls.replace(',', '.'); os.replace(',', '.'); ls.trim(); os.trim();
  // Alan boş gelirse kayıtlı konum korunur (sayfa tam yüklenmeden kaydedilirse silinmesin)
  double lat = ls.length() ? ls.toDouble() : cfg.lat;
  double lon = os.length() ? os.toDouble() : cfg.lon;
  if (lat < -90 || lat > 90 || lon < -180 || lon > 180) { errJson("Konum aralık dışında"); return; }

  String tab = server.arg("tab"), code = server.arg("code");
  char st = tab.length() ? tab[0] : 'I', sc = code.length() ? code[0] : '&';
  if (st <= ' ' || st > '~' || sc <= ' ' || sc > '~') { errJson("Sembol geçersiz"); return; }

  String cmt = server.arg("cmt"); cmt.trim();
  cmt.replace('|', ' '); cmt.replace('~', ' ');
  if (cmt.length() > 43) cmt = cmt.substring(0, 43);

  // Engellenen çağrı işaretleri: en çok 5, boşluk / virgül / noktalı virgül ile ayrılmış
  String bl = server.arg("blk");
  bl.replace(',', ' '); bl.replace(';', ' '); bl.toUpperCase();
  String blk = ""; int blN = 0;
  {
    size_t i = 0;
    while (i < bl.length()) {
      while (i < bl.length() && isspace((unsigned char)bl[i])) i++;
      size_t j = i;
      while (j < bl.length() && !isspace((unsigned char)bl[j])) j++;
      if (j > i) {
        String t = bl.substring(i, j);
        if (t.length() < 3 || t.length() > 9) { errJson("Engellenen çağrı işareti 3-9 karakter olmalı"); return; }
        for (size_t k = 0; k < t.length(); k++)
          if (!(isalnum((unsigned char)t[k]) || t[k] == '-')) { errJson("Engellenen çağrı işaretinde geçersiz karakter var"); return; }
        if (++blN > 5) { errJson("En fazla 5 çağrı işareti engellenebilir"); return; }
        if (blk.length()) blk += ' ';
        blk += t;
      }
      i = j;
    }
  }

  String srv = server.arg("srv"); srv.trim();
  if (!srv.length()) srv = "rotate.aprs2.net";
  long port = server.arg("port").toInt(); if (port <= 0 || port > 65535) port = 14580;
  long bint = server.arg("bint").length() ? server.arg("bint").toInt() : (long)cfg.beaconMin;
  if (bint < 0) bint = 0; if (bint > 0 && bint < 10) bint = 10; if (bint > 1440) bint = 1440;
  long baud = server.arg("baud").length() ? server.arg("baud").toInt() : (long)cfg.baud;
  if (baud < 300 || baud > 921600) baud = 9600;
  uint8_t fmt = server.arg("fmt").toInt() == 1 ? 1 : 0;

  bool serialChanged = (uint32_t)baud != cfg.baud || fmt != cfg.fmt;

  cfg.call = call; if (pw.length()) cfg.pass = pw;
  cfg.lat = lat; cfg.lon = lon; cfg.symTab = st; cfg.symCode = sc; cfg.comment = cmt;
  cfg.srv = srv; cfg.port = (uint16_t)port; cfg.beaconMin = (uint16_t)bint;
  cfg.baud = (uint32_t)baud; cfg.fmt = fmt; cfg.block = blk;
  String u = server.arg("uipw"); u.trim();
  if (u == "-") cfg.uipw = ""; else if (u.length()) cfg.uipw = u;
  saveConfig();

  if (serialChanged) serialInit();
  qCount = 0; qHead = 0;
  isDisconnect("Ayarlar değişti, yeniden bağlanılıyor");
  lastIsTry = 0; beaconDue = true;
  sendJson(200, "{\"ok\":true,\"msg\":\"Ayarlar kaydedildi\"}");
}

void handleScan() {
  if (!authOk()) return;
  if (server.hasArg("start")) {
    WiFi.scanDelete();
    WiFi.scanNetworks(true, false);
    sendJson(200, "{\"st\":\"scanning\"}");
    return;
  }
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) { sendJson(200, "{\"st\":\"scanning\"}"); return; }
  if (n < 0) { sendJson(200, "{\"st\":\"idle\"}"); return; }
  String o = "{\"st\":\"done\",\"n\":[";
  for (int i = 0; i < n; i++) {
    if (i) o += ",";
    o += "{\"s\":\"" + jEsc(WiFi.SSID(i)) + "\",\"r\":" + String(WiFi.RSSI(i)) +
         ",\"c\":" + String(WiFi.channel(i)) + ",\"e\":"; o += B(WiFi.encryptionType(i) != WIFI_AUTH_OPEN); o += "}";
  }
  o += "]}";
  WiFi.scanDelete();
  sendJson(200, o);
}

void handleWifiList() {
  if (!authOk()) return;
  String cur = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : String("");
  String o = "{\"cur\":\"" + jEsc(cur) + "\",\"nets\":[";
  for (int i = 0; i < netCount; i++) { if (i) o += ","; o += "\"" + jEsc(netSsid[i]) + "\""; }
  o += "]}";
  sendJson(200, o);
}

void handleWifiAdd() {
  if (!authOk()) return;
  String s = server.arg("ssid"), p = server.arg("pass");
  if (s.length() < 1 || s.length() > 32) { errJson("Ağ adı 1-32 karakter olmalı"); return; }
  if (p.length() > 0 && (p.length() < 8 || p.length() > 63)) { errJson("Şifre 8-63 karakter olmalı"); return; }
  int idx = -1;
  for (int i = 0; i < netCount; i++) if (netSsid[i] == s) idx = i;
  if (idx < 0) {
    if (netCount >= MAX_NETS) { errJson("En fazla 8 ağ kaydedilebilir"); return; }
    idx = netCount++;
  }
  netSsid[idx] = s; netPass[idx] = p;
  saveNets();
  wifiTry = -1; wifiIdx = idx; wifiLostMs = 0;               // bağlı değilse hemen bu ağı dene
  sendJson(200, "{\"ok\":true,\"msg\":\"Ağ kaydedildi. Bağlı değilse hemen denenir; durum kartından izleyin. Bağlanınca http://AselGate.local\"}");
}

void handleWifiDel() {
  if (!authOk()) return;
  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= netCount) { errJson("Geçersiz ağ"); return; }
  for (int i = idx; i < netCount - 1; i++) { netSsid[i] = netSsid[i + 1]; netPass[i] = netPass[i + 1]; }
  netCount--;
  netSsid[netCount] = ""; netPass[netCount] = "";
  saveNets();
  wifiIdx = -1; wifiTry = -1; wifiLostMs = 0;
  sendJson(200, "{\"ok\":true,\"msg\":\"Ağ silindi\"}");
}

void handleBeacon() {
  if (!authOk()) return;
  if (!hasPos()) { errJson("Önce konum girin"); return; }
  if (!isVerified) { errJson("APRS-IS girişi doğrulanmadı"); return; }
  sendBeacon();
  sendJson(200, "{\"ok\":true,\"msg\":\"Konum işaretçisi gönderildi\"}");
}

void handleUseGps() {
  if (!authOk()) return;
  if (!gps.have) { errJson("GPS'ten henüz geçerli konum alınmadı"); return; }
  uint32_t age = (millis() - gps.fixMs) / 1000;
  if (age > 300) { errJson("GPS konumu eski (5 dk'dan fazla); GPS fix kaybolmuş olabilir"); return; }
  cfg.lat = gps.lat; cfg.lon = gps.lon;
  saveConfig();
  beaconDue = true;                                   // doğrulanmış girişte yeni konum hemen gönderilir
  sendJson(200, "{\"ok\":true,\"msg\":\"GPS konumu kaydedildi\",\"lat\":\"" + String(cfg.lat, 6) +
                "\",\"lon\":\"" + String(cfg.lon, 6) + "\"}");
}

void handleRestart() {
  if (!authOk()) return;
  sendJson(200, "{\"ok\":true}");
  restartAt = millis() + 800;
}

void setupWeb() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/log", HTTP_GET, handleLog);
  server.on("/api/config", HTTP_GET, handleGetCfg);
  server.on("/api/config", HTTP_POST, handleSaveCfg);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/wifi", HTTP_GET, handleWifiList);
  server.on("/api/wifi/add", HTTP_POST, handleWifiAdd);
  server.on("/api/wifi/del", HTTP_POST, handleWifiDel);
  server.on("/api/beacon", HTTP_POST, handleBeacon);
  server.on("/api/usegps", HTTP_POST, handleUseGps);
  server.on("/api/restart", HTTP_POST, handleRestart);
  server.onNotFound(handleNotFound);
  server.begin();
}

// ======================= SETUP / LOOP =======================
void setup() {
  Serial.begin(USB_BAUD);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);                     // USB'ye PC bağlı değilken yazma beklemesin
#endif
  prefs.begin("igate", false);
  loadConfig();
  loadNets();
  serialInit();

  // Sıra önemli: hostname, modeden ÖNCE ayarlanmalı (aksi halde modem listesinde "espressif" görünür)
  WiFi.persistent(false);                       // her denemede flash'a yazma
  WiFi.setHostname(HOSTNAME);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setSleep(false);                         // Wi-Fi uyku modu kapalı: ping / mDNS / web sayfası kesilmesin
#if WIFI_TXPOWER_FIX
  WiFi.setTxPower(WIFI_POWER_8_5dBm);           // C3 Super Mini anten sorunu için
#endif
  WiFi.setAutoReconnect(true);
  WiFi.onEvent([](arduino_event_id_t ev, arduino_event_info_t info) {
    if (ev == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) wifiReason = info.wifi_sta_disconnected.reason;
    else if (ev == ARDUINO_EVENT_WIFI_STA_GOT_IP)  wifiReason = 0;
  });
  apStart(IPAddress(192, 168, 1, 1));

  configTime(TZ_OFFSET_SEC, 0, "pool.ntp.org", "time.google.com");
  setupWeb();
}

void loop() {
  pollSerial();                 // önce telsiz verisi: paket gelir gelmez APRS-IS'e gider
  isLoop();
  pollSerial();
  dns.processNextRequest();
  if (!Serial1.available()) server.handleClient();   // telsiz verisi varken web isteği bekler
  pollSerial();
  wifiLoop();
  if (restartAt && millis() > restartAt) ESP.restart();
}
