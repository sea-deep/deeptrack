import re

with open("DASHBOARD/dashboard.html", "r") as f:
    content = f.read()

# 1. Update MQTT topic and add scan topic
content = content.replace("const MQTT_TOPIC = 'deeptrack/rover/telemetry';", """const MQTT_TOPIC_TELEMETRY = 'rover/telemetry';
const MQTT_TOPIC_SCAN = 'rover/scan';""")
content = content.replace("mqttConn.subscribe(MQTT_TOPIC);", """mqttConn.subscribe(MQTT_TOPIC_TELEMETRY);
      mqttConn.subscribe(MQTT_TOPIC_SCAN);""")

# 2. Modify MQTT message handler to handle both telemetry and scan
mqtt_msg_handler = """    mqttConn.on('message', (topic, payload) => {
      try {
        const d = JSON.parse(payload.toString());
        if (topic === MQTT_TOPIC_TELEMETRY) {
           handleTelemetry(d);
        } else if (topic === MQTT_TOPIC_SCAN) {
           handleScan(d);
        }
      } catch(e) {}
    });"""
content = re.sub(r'    mqttConn\.on\(\'message\', \(topic, payload\) => \{.*?    \}\);', mqtt_msg_handler, content, flags=re.DOTALL)


# 3. Modify Web Serial message handler to handle both
serial_msg_handler = """        try{
          const d = JSON.parse(line);
          if (d.type === 'scan') {
              handleScan(d);
          } else {
              handleTelemetry(d);
          }
        }catch(e){ /* partial/garbled line, skip */ }"""
content = re.sub(r'        try\{\s*const d = JSON\.parse\(line\);\s*handleTelemetry\(d\);\s*\}catch\(e\)\{.*?\}', serial_msg_handler, content, flags=re.DOTALL)


# 4. Add the Radar UI element to the DOM
radar_html = """
    <div class="panel map-panel">
      <h2><span class="dot live"></span> 2D Environment Scan</h2>
      <div class="canvas-container" style="position:relative; width:400px; height:400px; margin:0 auto; background:#000; border:1px solid #333; border-radius:4px; overflow:hidden;">
         <canvas id="radarCanvas" width="400" height="400"></canvas>
      </div>
    </div>
"""
# Find a place to insert the radar (e.g. before the logs panel)
content = content.replace('<div class="panel log-panel">', radar_html + '\n    <div class="panel log-panel">')


# 5. Add Radar Drawing and `handleScan` function to JavaScript
radar_js = """
// ---------- Radar Canvas (VL53L0X) ----------
const rCanvas = document.getElementById('radarCanvas');
const rCtx = rCanvas.getContext('2d');
const scanPoints = []; 
const MAX_POINTS = 200;
const SCALE = 0.25; 

function drawRadar() {
    rCtx.clearRect(0, 0, rCanvas.width, rCanvas.height);
    
    // Draw grid
    rCtx.strokeStyle = '#333';
    rCtx.beginPath();
    rCtx.arc(200, 380, 100, 0, Math.PI, true);
    rCtx.arc(200, 380, 200, 0, Math.PI, true);
    rCtx.stroke();
    
    // Draw rover
    rCtx.fillStyle = '#00ff00';
    rCtx.fillRect(195, 375, 10, 10);
    
    // Draw points
    const now = Date.now();
    for (let i = scanPoints.length - 1; i >= 0; i--) {
        const pt = scanPoints[i];
        const age = now - pt.ts;
        if (age > 5000) {
            scanPoints.splice(i, 1);
            continue;
        }
        
        const alpha = 1.0 - (age / 5000);
        rCtx.fillStyle = `rgba(0, 255, 255, ${alpha})`;
        rCtx.beginPath();
        rCtx.arc(pt.x, pt.y, 3, 0, Math.PI * 2);
        rCtx.fill();
    }
    requestAnimationFrame(drawRadar);
}
drawRadar();

function handleScan(data) {
    if (data.valid) {
        const theta = ((data.angle_deg - 90) * Math.PI) / 180;
        const lateral = data.distance_mm * Math.cos(theta); 
        const forward = data.distance_mm * Math.sin(theta); 
        
        const canvasX = 200 - (lateral * SCALE);
        const canvasY = 380 - (forward * SCALE);
        
        scanPoints.push({ x: canvasX, y: canvasY, ts: Date.now() });
        if (scanPoints.length > MAX_POINTS) scanPoints.shift();
    }
}
"""
content = content.replace('// ---------- Odometry map ----------', radar_js + '\n// ---------- Odometry map ----------')

# 6. Update `handleTelemetry` to support new fields properly
# Specifically: danger logic (1 vs 0), and Pitch/Roll from ax, ay, az
telemetry_fix = """function handleTelemetry(raw){
  const d = {
    tempC: raw.t ?? raw.tempC,
    hum: raw.h ?? raw.hum,
    gas: raw.gas,
    frontCm: raw.dist ?? raw.frontCm ?? 'N/A',
    water: raw.water,
    x: raw.x ?? 0,
    y: raw.y ?? 0,
    heading: raw.hd ?? raw.heading ?? 0,
    state: (raw.danger === 1) ? 'DANGER' : 'NORMAL',
    dangerCause: (raw.danger === 1) ? 'EMERGENCY' : 'NONE',
    encL: raw.encL ?? 0,
    encR: raw.encR ?? 0
  };
  if (raw.ax !== undefined) {
      d.pitch = Math.atan2(-raw.ax, Math.sqrt(raw.ay * raw.ay + raw.az * raw.az)) * 180 / Math.PI;
      d.roll = Math.atan2(raw.ay, raw.az) * 180 / Math.PI;
  }
  latest = d;

  const danger = d.state === 'DANGER';
"""
content = re.sub(r'function handleTelemetry\(raw\)\{.*?const danger = d\.state === \'DANGER\';', telemetry_fix, content, flags=re.DOTALL)


# 7. Update the UI bindings in `handleTelemetry` to show Pitch/Roll instead of Odometry coords if missing
ui_update = """
  document.getElementById('s-pos').textContent = d.pitch !== undefined ? `P:${d.pitch.toFixed(1)}° R:${d.roll.toFixed(1)}°` : `X:${d.x.toFixed(1)} Y:${d.y.toFixed(1)}`;
"""
content = re.sub(r'  document\.getElementById\(\'s-pos\'\)\.textContent = `X:\$\{d\.x\.toFixed\(1\)\} Y:\$\{d\.y\.toFixed\(1\)\}`;', ui_update, content)


with open("DASHBOARD/dashboard.html", "w") as f:
    f.write(content)

