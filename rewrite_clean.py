import re

with open("DASHBOARD/dashboard.html", "r") as f:
    content = f.read()

# MQTT topic
content = content.replace("const MQTT_TOPIC = 'deeptrack/rover/telemetry';", "const MQTT_TOPIC_TELEMETRY = 'rover/telemetry';\nconst MQTT_TOPIC_SCAN = 'rover/scan';")
content = content.replace("mqttConn.subscribe(MQTT_TOPIC);", "mqttConn.subscribe(MQTT_TOPIC_TELEMETRY);\nmqttConn.subscribe(MQTT_TOPIC_SCAN);")

# MQTT handler
msg_h = """    mqttConn.on('message', (topic, payload) => {
      try {
        const d = JSON.parse(payload.toString());
        if (topic === MQTT_TOPIC_TELEMETRY) { handleTelemetry(d); }
        else if (topic === MQTT_TOPIC_SCAN) { handleScan(d); }
      } catch(e) {}
    });"""
content = re.sub(r'    mqttConn\.on\(\'message\', \(topic, payload\) => \{.*?    \}\);', msg_h, content, flags=re.DOTALL)

# Serial handler
ser_h = """        try{
          const d = JSON.parse(line);
          if (d.type === 'scan') handleScan(d);
          else handleTelemetry(d);
        }catch(e){ }"""
content = re.sub(r'        try\{\s*const d = JSON\.parse\(line\);\s*handleTelemetry\(d\);\s*\}catch\(e\)\{.*?\}', ser_h, content, flags=re.DOTALL)

# UI bindings
ui = """  document.getElementById('s-pos').textContent = d.pitch !== undefined ? `P:${d.pitch.toFixed(1)}° R:${d.roll.toFixed(1)}°` : `X:${d.x.toFixed(1)} Y:${d.y.toFixed(1)}`;"""
content = re.sub(r'  document\.getElementById\(\'s-pos\'\)\.textContent = `X:\$\{d\.x\.toFixed\(1\)\} Y:\$\{d\.y\.toFixed\(1\)\}`;', ui, content)

# handleTelemetry start
tel_start = """function handleTelemetry(raw){
  const d = {
    tempC: raw.t ?? raw.tempC, hum: raw.h ?? raw.hum, gas: raw.gas, frontCm: raw.dist ?? raw.frontCm ?? 'N/A', water: raw.water,
    x: raw.x ?? 0, y: raw.y ?? 0, heading: raw.hd ?? raw.heading ?? 0,
    state: (raw.danger === 1) ? 'DANGER' : 'NORMAL',
    dangerCause: (raw.danger === 1) ? 'EMERGENCY' : 'NONE',
    encL: raw.encL ?? 0, encR: raw.encR ?? 0
  };
  if (raw.ax !== undefined) {
      d.pitch = Math.atan2(-raw.ax, Math.sqrt(raw.ay * raw.ay + raw.az * raw.az)) * 180 / Math.PI;
      d.roll = Math.atan2(raw.ay, raw.az) * 180 / Math.PI;
  }
  latest = d;

  const danger = d.state === 'DANGER';"""
content = re.sub(r'function handleTelemetry\(raw\)\{.*?const danger = d\.state === \'DANGER\';', tel_start, content, flags=re.DOTALL)

# Remove trail logic from handleTelemetry
content = re.sub(r'  // Push trail.*?requestAnimationFrame\(drawMap\);', '', content, flags=re.DOTALL)

# Replace drawMap completely
draw_map_logic = """
const scanPoints = []; 
const MAX_POINTS = 200;
const SCALE = 0.25; 

function handleScan(data) {
    if (data.valid) {
        const theta = ((data.angle_deg - 90) * Math.PI) / 180;
        const lateral = data.distance_mm * Math.cos(theta); 
        const forward = data.distance_mm * Math.sin(theta); 
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
    ctx.strokeStyle = '#25332c';
    ctx.beginPath();
    ctx.arc(w/2, h-20, 100, 0, Math.PI, true);
    ctx.arc(w/2, h-20, 200, 0, Math.PI, true);
    ctx.stroke();
    ctx.fillStyle = '#4cc27a';
    ctx.fillRect(w/2 - 5, h-25, 10, 10);
    const now = Date.now();
    for (let i = scanPoints.length - 1; i >= 0; i--) {
        const pt = scanPoints[i];
        const age = now - pt.ts;
        if (age > 5000) { scanPoints.splice(i, 1); continue; }
        const alpha = 1.0 - (age / 5000);
        ctx.fillStyle = `rgba(0, 255, 255, ${alpha})`;
        ctx.beginPath();
        ctx.arc(pt.x, pt.y, 3, 0, Math.PI * 2);
        ctx.fill();
    }
    requestAnimationFrame(drawMap);
}
"""
content = re.sub(r'function drawMap\(\).*', draw_map_logic + '\ndrawMap();\n</script>\n</body>\n</html>', content, flags=re.DOTALL)

with open("DASHBOARD/dashboard.html", "w") as f:
    f.write(content)
