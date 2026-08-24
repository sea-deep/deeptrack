import re

with open("DASHBOARD/dashboard.html", "r") as f:
    content = f.read()

# 1. Update MQTT topic to telemetry and add scan topic
content = content.replace("const MQTT_TOPIC = 'deeptrack/rover/telemetry';", """const MQTT_TOPIC_TELEMETRY = 'rover/telemetry';
const MQTT_TOPIC_SCAN = 'rover/scan';""")
content = content.replace("mqttConn.subscribe(MQTT_TOPIC);", """mqttConn.subscribe(MQTT_TOPIC_TELEMETRY);
      mqttConn.subscribe(MQTT_TOPIC_SCAN);""")

# 2. Modify MQTT message handler
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

# 3. Modify Web Serial message handler
serial_msg_handler = """        try{
          const d = JSON.parse(line);
          if (d.type === 'scan') {
              handleScan(d);
          } else {
              handleTelemetry(d);
          }
        }catch(e){ /* partial/garbled line, skip */ }"""
content = re.sub(r'        try\{\s*const d = JSON\.parse\(line\);\s*handleTelemetry\(d\);\s*\}catch\(e\)\{.*?\}', serial_msg_handler, content, flags=re.DOTALL)


# 4. Replace `handleTelemetry` to support Pitch/Roll instead of x/y
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

# 5. Fix UI bindings for positions (X/Y -> Pitch/Roll)
ui_update = """  document.getElementById('s-pos').textContent = d.pitch !== undefined ? `P:${d.pitch.toFixed(1)}° R:${d.roll.toFixed(1)}°` : `X:${d.x.toFixed(1)} Y:${d.y.toFixed(1)}`;"""
content = re.sub(r'  document\.getElementById\(\'s-pos\'\)\.textContent = `X:\$\{d\.x\.toFixed\(1\)\} Y:\$\{d\.y\.toFixed\(1\)\}`;', ui_update, content)


# 6. Replace Odometry Map logic with Radar Canvas logic
radar_logic = """
// ---------- Radar Scan Map ----------
const scanPoints = []; 
const MAX_POINTS = 200;
const SCALE = 0.25; 

function handleScan(data) {
    if (data.valid) {
        const theta = ((data.angle_deg - 90) * Math.PI) / 180;
        const lateral = data.distance_mm * Math.cos(theta); 
        const forward = data.distance_mm * Math.sin(theta); 
        
        // Map to canvas (origin at center bottom)
        const w = canvas.width;
        const h = canvas.height;
        const canvasX = (w / 2) - (lateral * SCALE);
        const canvasY = (h - 20) - (forward * SCALE);
        
        scanPoints.push({ x: canvasX, y: canvasY, ts: Date.now() });
        if (scanPoints.length > MAX_POINTS) scanPoints.shift();
    }
}

function drawMap() {
    const w = canvas.width, h = canvas.height;
    ctx.clearRect(0,0,w,h);
    ctx.fillStyle = '#0f1512';
    ctx.fillRect(0,0,w,h);
    
    // Draw grid
    ctx.strokeStyle = '#25332c';
    ctx.beginPath();
    ctx.arc(w/2, h-20, 100, 0, Math.PI, true);
    ctx.arc(w/2, h-20, 200, 0, Math.PI, true);
    ctx.stroke();
    
    // Draw rover
    ctx.fillStyle = '#4cc27a';
    ctx.fillRect(w/2 - 5, h-25, 10, 10);
    
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
        ctx.fillStyle = `rgba(0, 255, 255, ${alpha})`;
        ctx.beginPath();
        ctx.arc(pt.x, pt.y, 3, 0, Math.PI * 2);
        ctx.fill();
    }
    requestAnimationFrame(drawMap);
}
drawMap();
"""

# Completely replace the old drawMap
content = re.sub(r'function drawMap\(\)\{.*?requestAnimationFrame\(drawMap\);\s*\}\s*drawMap\(\);', radar_logic, content, flags=re.DOTALL)
# Strip out the trail pushing logic in handleTelemetry
content = re.sub(r'  // Push trail.*?  requestAnimationFrame\(drawMap\);', '', content, flags=re.DOTALL)


with open("DASHBOARD/dashboard.html", "w") as f:
    f.write(content)
