<script>
  import Navbar from '$lib/components/Navbar.svelte';
  import Footer from '$lib/components/Footer.svelte';
  import RoverLogo from '$lib/components/RoverLogo.svelte';
  import { initialScanPoints } from '$lib/mocks/telemetryMock.js';
  import { getTheme } from '$lib/utils/theme.js';
  import { onMount } from 'svelte';

  // Live simulation for Hero Radar ToF Laser Sweep
  let servoAngle = $state(90);
  let servoDir = $state(4);
  let currentScanMm = $state(1240);
  let liveMethane = $state(450); // ppm
  let liveLel = $state(0.9); // % LEL
  let liveTemp = $state(29.1); // °C
  let isDarkMode = $state(true);

  /** @type {HTMLCanvasElement | undefined} */
  let radarCanvas = $state();
  let scanPoints = $state([...initialScanPoints]);

  function updateThemeState() {
    isDarkMode = getTheme() === 'dark';
  }

  function drawPolarRadar() {
    if (!radarCanvas) return;
    const ctx = radarCanvas.getContext('2d');
    if (!ctx) return;

    const w = radarCanvas.width;
    const h = radarCanvas.height;
    const cx = w / 2;
    const cy = h - 25;
    const maxRadius = Math.min(cx - 20, cy - 20);

    ctx.clearRect(0, 0, w, h);

    // Theme-Aware Colors
    const arcStroke = isDarkMode ? 'rgba(141, 145, 153, 0.22)' : 'rgba(100, 116, 139, 0.3)';
    const textFill = isDarkMode ? 'rgba(141, 145, 153, 0.8)' : 'rgba(71, 85, 105, 0.9)';
    const hitColor = isDarkMode ? '#59dbc7' : '#0284c7';
    const hitCloseColor = isDarkMode ? '#ffb4ab' : '#dc2626';
    const laserStroke = isDarkMode ? '#59dbc7' : '#0284c7';

    // Draw Polar Range Arcs (0.5m, 1.0m, 1.5m, 2.0m)
    ctx.strokeStyle = arcStroke;
    ctx.lineWidth = 1;
    [0.25, 0.5, 0.75, 1.0].forEach((ratio) => {
      const r = maxRadius * ratio;
      ctx.beginPath();
      ctx.arc(cx, cy, r, Math.PI, 0, false);
      ctx.stroke();

      // Range label
      ctx.fillStyle = textFill;
      ctx.font = '10px "Google Sans Code", monospace';
      ctx.fillText(`${(ratio * 2.0).toFixed(1)}m`, cx + r - 25, cy - 4);
    });

    // Draw Radial Angle Lines (30°, 60°, 90°, 120°, 150°)
    [30, 60, 90, 120, 150].forEach(deg => {
      const rad = (180 - deg) * (Math.PI / 180);
      const x = cx + Math.cos(rad) * maxRadius;
      const y = cy - Math.sin(rad) * maxRadius;
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(x, y);
      ctx.stroke();

      ctx.fillStyle = textFill;
      ctx.font = '10px "Google Sans Code", monospace';
      ctx.fillText(`${deg}°`, x + (deg < 90 ? 4 : -24), y - 4);
    });

    // Draw Point Cloud from VL53L0X Sweep
    scanPoints.forEach(p => {
      if (!p.valid) return;
      const rad = (180 - p.angle_deg) * (Math.PI / 180);
      const r = (Math.min(2000, p.distance_mm) / 2000) * maxRadius;
      const px = cx + Math.cos(rad) * r;
      const py = cy - Math.sin(rad) * r;

      ctx.beginPath();
      ctx.arc(px, py, 4, 0, Math.PI * 2);
      ctx.fillStyle = p.distance_mm < 500 ? hitCloseColor : hitColor;
      ctx.fill();
    });

    // Draw Current Servo Laser Beam with Glow
    const beamRad = (180 - servoAngle) * (Math.PI / 180);
    const beamLen = (Math.min(2000, currentScanMm) / 2000) * maxRadius;
    const bx = cx + Math.cos(beamRad) * beamLen;
    const by = cy - Math.sin(beamRad) * beamLen;

    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(bx, by);
    ctx.strokeStyle = laserStroke;
    ctx.lineWidth = 2;
    ctx.stroke();

    // Rover Pivot Indicator at Center
    ctx.beginPath();
    ctx.arc(cx, cy, 6, 0, Math.PI * 2);
    ctx.fillStyle = hitColor;
    ctx.fill();
  }

  onMount(() => {
    updateThemeState();
    const handleThemeChange = () => {
      updateThemeState();
      drawPolarRadar();
    };
    window.addEventListener('ui:themechange', handleThemeChange);

    if (radarCanvas) {
      const rect = radarCanvas.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      radarCanvas.width = rect.width * dpr;
      radarCanvas.height = rect.height * dpr;
      const ctx = radarCanvas.getContext('2d');
      if (ctx) ctx.scale(dpr, dpr);
      drawPolarRadar();
    }

    const interval = setInterval(() => {
      servoAngle += servoDir;
      if (servoAngle >= 150) {
        servoAngle = 150;
        servoDir = -4;
      } else if (servoAngle <= 30) {
        servoAngle = 30;
        servoDir = 4;
      }

      const wallDist = 700 + Math.sin(servoAngle * (Math.PI / 180)) * 900 + (Math.random() - 0.5) * 35;
      currentScanMm = Math.round(wallDist);

      scanPoints = scanPoints.map(p => 
        Math.abs(p.angle_deg - servoAngle) < 4 ? { angle_deg: servoAngle, distance_mm: currentScanMm, valid: true } : p
      );

      liveMethane = +(420 + Math.sin(Date.now() / 4000) * 80).toFixed(0);
      liveLel = +(liveMethane / 500.0).toFixed(2);
      liveTemp = +(28.8 + Math.cos(Date.now() / 6000) * 1.2).toFixed(1);

      drawPolarRadar();
    }, 100);

    return () => {
      window.removeEventListener('ui:themechange', handleThemeChange);
      clearInterval(interval);
    };
  });
</script>

<svelte:head>
  <title>DeepTrack — Subterranean Mine Rescue Rover System</title>
</svelte:head>

<div class="min-h-screen flex flex-col bg-[var(--md-sys-color-surface)] text-[var(--md-sys-color-on-surface)] select-none">
  <Navbar active="home" />

  <main class="flex-1">
    <!-- Hero Section with Expressive Entrance Animation -->
    <section class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12 lg:py-20 grid grid-cols-1 lg:grid-cols-12 gap-12 items-center" aria-labelledby="hero-title">
      
      <!-- Left 7 cols: Direct Headline & Actions -->
      <div class="lg:col-span-7 flex flex-col items-start hero-text-layer">
        <h1 id="hero-title" class="tracking-tight text-3xl sm:text-5xl lg:text-6xl font-bold leading-tight text-[var(--md-sys-color-on-surface)]">
          Robotic Reconnaissance for Hazardous Coal Mines
        </h1>

        <p class="text-base sm:text-lg text-[var(--md-sys-color-on-surface-variant)] leading-relaxed mt-5 max-w-2xl">
          An expendable, rugged 4WD robotic rover engineered to enter post-explosion coal mine workings, map toxic firedamp (CH₄) gas pockets with 2D SLAM, and assess shaft stability before rescue personnel commit human lives underground.
        </p>

        <div class="flex flex-wrap items-center gap-3.5 mt-8">
          <a href="/dashboard" class="ui-button ui-button--filled !h-12 !px-6 text-sm font-medium shadow-md hover:shadow-lg transition-all duration-300 active:scale-95">
            <span class="material-symbols-rounded text-xl">terminal</span>
            Open mission console
          </a>
          <a href="#hardware" class="ui-button ui-button--outlined !h-12 !px-6 text-sm font-medium transition-all duration-300 active:scale-95">
            <span class="material-symbols-rounded text-xl">developer_board</span>
            Hardware architecture
          </a>
        </div>

        <!-- 4-Column Technical Metrics Strip with Staggered Entrance -->
        <div class="grid grid-cols-2 sm:grid-cols-4 gap-4 mt-12 pt-8 border-t border-[var(--md-sys-color-outline-variant)] w-full hero-metrics-layer">
          <div class="p-3 rounded-xl bg-[var(--md-sys-color-surface-container-low)] border border-[var(--md-sys-color-outline-variant)]/50">
            <div class="text-xl font-semibold telemetry text-[var(--ui-brand-cyan)]">10 Hz</div>
            <div class="text-xs text-[var(--md-sys-color-on-surface-variant)] mt-0.5">LiDAR sweep</div>
          </div>
          <div class="p-3 rounded-xl bg-[var(--md-sys-color-surface-container-low)] border border-[var(--md-sys-color-outline-variant)]/50">
            <div class="text-xl font-semibold telemetry text-[var(--ui-brand-cyan)]">1.0% LEL</div>
            <div class="text-xs text-[var(--md-sys-color-on-surface-variant)] mt-0.5">Safety cutoff</div>
          </div>
          <div class="p-3 rounded-xl bg-[var(--md-sys-color-surface-container-low)] border border-[var(--md-sys-color-outline-variant)]/50">
            <div class="text-xl font-semibold telemetry text-[var(--ui-brand-cyan)]">240 MHz</div>
            <div class="text-xs text-[var(--md-sys-color-on-surface-variant)] mt-0.5">ESP32 dual core</div>
          </div>
          <div class="p-3 rounded-xl bg-[var(--md-sys-color-surface-container-low)] border border-[var(--md-sys-color-outline-variant)]/50">
            <div class="text-xl font-semibold telemetry text-[var(--ui-brand-cyan)]">25° Limit</div>
            <div class="text-xs text-[var(--md-sys-color-on-surface-variant)] mt-0.5">Rollover E-stop</div>
          </div>
        </div>
      </div>

      <!-- Right 5 cols: Live Polar ToF Laser Radar Visualizer Card -->
      <div class="lg:col-span-5 ui-card !p-5 flex flex-col justify-between shadow-xl bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] m3-card-interactive hero-radar-layer">
        <!-- Visualizer Header -->
        <div class="flex items-center justify-between border-b border-[var(--md-sys-color-outline-variant)] pb-3">
          <div class="flex items-center gap-2">
            <RoverLogo size={20} />
            <div class="section-title text-[var(--md-sys-color-on-surface)]">
              VL53L0X LiDAR sweep (30° - 150°)
            </div>
          </div>
          <div class="telemetry text-xs text-[var(--ui-brand-cyan)]">
            Servo: {servoAngle}° • Range: {currentScanMm} mm
          </div>
        </div>

        <!-- Polar Canvas -->
        <div class="my-3 relative w-full h-[240px] bg-[var(--md-sys-color-surface-container-lowest)] rounded-xl border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center overflow-hidden">
          <canvas bind:this={radarCanvas} class="w-full h-full block"></canvas>
        </div>

        <!-- Telemetry Ticker -->
        <div class="grid grid-cols-2 gap-3 pt-2 text-xs">
          <div class="p-2.5 rounded-lg bg-[var(--md-sys-color-surface-container-highest)] border border-[var(--md-sys-color-outline-variant)]">
            <div class="metric-label text-[10px] text-[var(--md-sys-color-on-surface-variant)]">Methane · CH₄</div>
            <div class="font-semibold text-sm text-[var(--ui-brand-cyan)] telemetry mt-0.5">
              {liveMethane} ppm <span class="text-xs font-normal text-[var(--md-sys-color-on-surface-variant)]">({liveLel}% LEL)</span>
            </div>
          </div>
          <div class="p-2.5 rounded-lg bg-[var(--md-sys-color-surface-container-highest)] border border-[var(--md-sys-color-outline-variant)]">
            <div class="metric-label text-[10px] text-[var(--md-sys-color-on-surface-variant)]">Shaft climate</div>
            <div class="font-semibold text-sm text-[var(--ui-brand-cyan)] telemetry mt-0.5">
              {liveTemp}°C <span class="text-xs font-normal text-[var(--md-sys-color-on-surface-variant)]">(RH: 68%)</span>
            </div>
          </div>
        </div>
      </div>

    </section>

    <!-- Hardware Architecture Section (#hardware) -->
    <section id="hardware" class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-16 border-t border-[var(--md-sys-color-outline-variant)] hardware-section relative z-10">
      <div class="text-center max-w-3xl mx-auto mb-12">
        <h2 class="text-2xl sm:text-3xl font-bold text-[var(--md-sys-color-on-surface)]">
          Subterranean Sensor & Actuator Architecture
        </h2>
        <p class="text-[var(--md-sys-color-on-surface-variant)] text-sm sm:text-base mt-2">
          Modular avionics and intrinsic sensor hardware designed for extreme coal mine environments.
        </p>
      </div>

      <div class="grid grid-cols-1 md:grid-cols-3 gap-6">
        <!-- Card 1: MQ-4 -->
        <div class="ui-card flex flex-col justify-between !p-5 m3-card-interactive">
          <div>
            <div class="w-10 h-10 rounded-xl bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] flex items-center justify-center mb-4">
              <span class="material-symbols-rounded text-xl filled text-[var(--ui-brand-cyan)]">air</span>
            </div>
            <h3 class="text-base font-semibold text-[var(--md-sys-color-on-surface)] mb-1.5">
              MQ-4 Methane Sensor Array
            </h3>
            <p class="text-xs text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">
              Continuous sampling of combustible hydrocarbon gases (200-10000 ppm CH₄) with voltage-divider protection on GPIO 34 for real-time explosion threshold detection.
            </p>
          </div>
          <div class="mt-4 pt-3 border-t border-[var(--md-sys-color-outline-variant)] text-xs telemetry text-[var(--md-sys-color-on-surface-variant)]">
            Explosion cutoff: 1.0% CH₄
          </div>
        </div>

        <!-- Card 2: SLAM LiDAR -->
        <div class="ui-card flex flex-col justify-between !p-5 m3-card-interactive">
          <div>
            <div class="w-10 h-10 rounded-xl bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] flex items-center justify-center mb-4">
              <span class="material-symbols-rounded text-xl filled text-[var(--ui-brand-cyan)]">explore</span>
            </div>
            <h3 class="text-base font-semibold text-[var(--md-sys-color-on-surface)] mb-1.5">
              2D SLAM Occupancy Mapping
            </h3>
            <p class="text-xs text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">
              Laser Time-of-Flight obstacle profiling combined with autonomous frontier discovery to generate topological maps of uncharted mine galleries and collapsed voids.
            </p>
          </div>
          <div class="mt-4 pt-3 border-t border-[var(--md-sys-color-outline-variant)] text-xs telemetry text-[var(--md-sys-color-on-surface-variant)]">
            Autonomous & manual modes
          </div>
        </div>

        <!-- Card 3: Incline & IMU -->
        <div class="ui-card flex flex-col justify-between !p-5 m3-card-interactive">
          <div>
            <div class="w-10 h-10 rounded-xl bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] flex items-center justify-center mb-4">
              <span class="material-symbols-rounded text-xl filled text-[var(--ui-brand-cyan)]">screen_rotation</span>
            </div>
            <h3 class="text-base font-semibold text-[var(--md-sys-color-on-surface)] mb-1.5">
              6-DOF IMU Dynamic Stability
            </h3>
            <p class="text-xs text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">
              MPU6050 accelerometer and gyroscope fusion computing real-time pitch and roll angles with hardware automatic emergency brake triggering at ±25° inclination.
            </p>
          </div>
          <div class="mt-4 pt-3 border-t border-[var(--md-sys-color-outline-variant)] text-xs telemetry text-[var(--md-sys-color-on-surface-variant)]">
            Dynamic cutoff: ±25° pitch/roll
          </div>
        </div>
      </div>
    </section>
  </main>

  <Footer />
</div>


<style>
  /* --- PAGE LOAD CHOREOGRAPHY --- */
  
  @media (prefers-reduced-motion: no-preference) {
    /* 1. Primary Visual (Radar) establishes the spatial background */
    .hero-radar-layer {
      animation: resolve-radar 1.1s cubic-bezier(0.2, 0.0, 0, 1.0) backwards;
      animation-delay: 0.1s;
    }
    
    /* 2. Headline & Copy sweep in, overlapping the radar */
    .hero-text-layer {
      animation: resolve-text 0.9s cubic-bezier(0.2, 0.0, 0, 1.0) backwards;
      animation-delay: 0.25s;
    }

    /* 3. Secondary Metadata / Metrics settle in last */
    .hero-metrics-layer {
      animation: resolve-metrics 0.8s cubic-bezier(0.2, 0.0, 0, 1.0) backwards;
      animation-delay: 0.4s;
    }

    @keyframes resolve-radar {
      0% { opacity: 0; transform: scale(0.97) translateY(12px); filter: blur(2px); }
      100% { opacity: 1; transform: scale(1) translateY(0); filter: blur(0); }
    }

    @keyframes resolve-text {
      0% { opacity: 0; transform: translateY(24px); }
      100% { opacity: 1; transform: translateY(0); }
    }

    @keyframes resolve-metrics {
      0% { opacity: 0; transform: translateY(16px); }
      100% { opacity: 1; transform: translateY(0); }
    }

    /* --- SCROLL CHOREOGRAPHY (Continuous Spatial Transition) --- */
    /* Modern CSS scroll-driven animations with graceful fallback */
    @supports (animation-timeline: view()) {
      .hero-text-layer {
        /* Text lifts out faster than the background scrolls, dissolving */
        animation: hero-text-scroll linear both;
        animation-timeline: view();
        animation-range: exit 0% exit 100%;
        will-change: transform, opacity;
      }

      .hero-radar-layer {
        /* Radar visually anchors and sinks backward to create parallax depth */
        animation: hero-radar-scroll linear both;
        animation-timeline: view();
        animation-range: exit 0% exit 100%;
        transform-origin: center top;
        will-change: transform, opacity;
      }

      .hardware-section {
        /* Next section rises slightly into the fading hero */
        animation: section-enter linear both;
        animation-timeline: view();
        animation-range: entry 10% cover 30%;
        will-change: transform, opacity;
      }

      @keyframes hero-text-scroll {
        to { opacity: 0; transform: translateY(-70px); }
      }

      @keyframes hero-radar-scroll {
        to { opacity: 0.15; transform: translateY(45px) scale(0.96); }
      }

      @keyframes section-enter {
        from { opacity: 0; transform: translateY(50px); }
        to { opacity: 1; transform: translateY(0); }
      }
    }
  }
</style>
