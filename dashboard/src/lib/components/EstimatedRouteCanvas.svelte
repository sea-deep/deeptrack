<script>
  import { onMount } from 'svelte';
  import { getTheme } from '$lib/utils/theme.js';
  import { roverCalibration } from '$lib/config/roverCalibration.js';

  const pixelsPerMeter = roverCalibration.map.pixelsPerMeter;

  /**
   * @typedef {Object} ScanPoint
   * @property {number} angle_deg
   * @property {number} distance_mm
   * @property {boolean} valid
   */

  let {
    mode = 'MANUAL', // 'MANUAL', 'AUTO_EXPLORE'
    isRecording = false,
    onToggleRecord = () => {},
    scanPoints = /** @type {ScanPoint[]} */ ([]),
    roverPose = { x: 0, y: 0, headingDeg: 90 },
    isConnected = true,
    isDemo = false,
    dataSource = 'UNKNOWN',
    hasEstimatedPose = false,
    mapEvidence = null,
    hazardZones = /** @type {Array<{x:number,y:number,score:number,state:string,simulated?:boolean}>} */ ([]),
    waypointEnabled = false,
    missionWaypoint = null,
    onSetWaypoint = (/** @type {{x: number, y: number}} */ _) => {}
  } = $props();

  /** @type {HTMLCanvasElement | undefined} */
  let canvas = $state();
  let zoom = $state(1.0);
  let panX = $state(0);
  let panY = $state(0);
  let isDragging = $state(false);
  let dragStart = { x: 0, y: 0 };
  let isDarkMode = $state(true);
  let followRover = $state(true);
  let hasPoseEvidence = $derived(isDemo || hasEstimatedPose);
  let validLocalScanCount = $derived(
    scanPoints.filter((/** @type {ScanPoint} */ point) => point.valid === true).length
  );
  let hasLocalScanEvidence = $derived(validLocalScanCount > 0);

  // Layer Toggles (Subdued quiet states)
  let showTrajectory = $state(true);
  let showObstacles = $state(true);
  let showTofRays = $state(true);
  let showInflation = $state(true);
  let showFrontiers = $state(true);

  /** @type {{x: number, y: number} | null} */
  let currentWaypoint = $state(null);
  let drawFrame = 0;

  function scheduleDraw() {
    if (typeof requestAnimationFrame === 'undefined') return;
    cancelAnimationFrame(drawFrame);
    drawFrame = requestAnimationFrame(drawMap);
  }

  $effect(() => {
    if (missionWaypoint && Number.isFinite(missionWaypoint.x_m) &&
        Number.isFinite(missionWaypoint.y_m)) {
      currentWaypoint = { x: missionWaypoint.x_m * pixelsPerMeter,
        y: missionWaypoint.y_m * pixelsPerMeter };
    } else if (!missionWaypoint && !isDemo) currentWaypoint = null;
  });

  // Repaint only when visible map inputs change. This replaces the previous
  // unconditional 10 FPS full-canvas redraw while preserving live motion.
  $effect(() => {
    void [mode, isRecording, isConnected, dataSource, hasPoseEvidence,
      roverPose.x, roverPose.y, roverPose.headingDeg, zoom, panX, panY,
      showTrajectory, showObstacles, showTofRays, showInflation,
      showFrontiers, scanPoints.length, obstaclePoints.length,
      inflatedPoints.length, frontierPoints.length, exploredTrail.length,
      hazardZones.length, currentWaypoint?.x, currentWaypoint?.y, isDarkMode];
    scheduleDraw();
  });

  // Seeded demo geometry is isolated from real calibrated dead-reckoning and
  // sparse occupancy evidence.
  /** @type {Array<{x: number, y: number, intensity: number}>} */
  const demoObstaclePoints = [
    { x: -50, y: -90, intensity: 1 }, { x: 0, y: -95, intensity: 1 }, { x: 50, y: -90, intensity: 1 },
    { x: -60, y: -45, intensity: 1 }, { x: 60, y: -45, intensity: 1 },
    { x: -65, y: 0, intensity: 1 }, { x: 65, y: 0, intensity: 1 },
    { x: -60, y: 45, intensity: 1 }, { x: 60, y: 45, intensity: 1 },
    { x: -50, y: 90, intensity: 1 }, { x: 50, y: 90, intensity: 1 },
    { x: 90, y: 95, intensity: 1 }, { x: 130, y: 100, intensity: 1 }, { x: 170, y: 105, intensity: 1 },
    { x: 90, y: 135, intensity: 1 }, { x: 130, y: 140, intensity: 1 }, { x: 170, y: 145, intensity: 1 }
  ];
  let obstaclePoints = $state(/** @type {Array<{x: number, y: number, intensity: number}>} */ ([]));
  let inflatedPoints = $state(/** @type {Array<{x: number, y: number}>} */ ([]));
  let frontierPoints = $state(/** @type {Array<{x: number, y: number}>} */ ([]));

  /** @type {Array<{x: number, y: number}>} */
  let exploredTrail = $state(/** @type {Array<{x: number, y: number}>} */ ([]));

  $effect(() => {
    obstaclePoints = isDemo
      ? demoObstaclePoints.map((point) => ({ ...point }))
      : (mapEvidence?.occupied || []).map((/** @type {any} */ cell) => ({
          x: cell.x * mapEvidence.cellSizeM * pixelsPerMeter,
          y: cell.y * mapEvidence.cellSizeM * pixelsPerMeter,
          intensity: Math.min(1, Math.max(0, cell.log_odds / 3))
        }));
    inflatedPoints = isDemo ? [] : (mapEvidence?.inflated || []).map(
      (/** @type {any} */ cell) => ({
        x: cell.x * mapEvidence.cellSizeM * pixelsPerMeter,
        y: cell.y * mapEvidence.cellSizeM * pixelsPerMeter
      })
    );
    frontierPoints = isDemo ? [] : (mapEvidence?.frontiers || []).map(
      (/** @type {any} */ cell) => ({
        x: cell.x * mapEvidence.cellSizeM * pixelsPerMeter,
        y: cell.y * mapEvidence.cellSizeM * pixelsPerMeter
      })
    );
    exploredTrail = isDemo
      ? [{ x: 0, y: pixelsPerMeter },
          { x: 0, y: pixelsPerMeter / 2 }, { x: 0, y: 0 }]
      : (mapEvidence?.trajectory || []).map((/** @type {any} */ point) => ({
          x: point.x_m * pixelsPerMeter,
          y: point.y_m * pixelsPerMeter
        }));
  });

  function updateThemeState() {
    isDarkMode = getTheme() === 'dark';
  }

  function drawMap() {
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const w = canvas.width;
    const h = canvas.height;

    if (followRover && hasPoseEvidence) {
      panX = -roverPose.x * zoom;
      panY = -roverPose.y * zoom;
    }

    const cx = w / 2 + panX;
    const cy = h / 2 + panY;

    // Theme Palette
    const bgFill = isDarkMode ? '#0c0e10' : '#f8f9fc';
    const gridStroke = isDarkMode ? 'rgba(141, 145, 153, 0.12)' : 'rgba(100, 116, 139, 0.18)';
    const corridorFill = isDarkMode ? 'rgba(0, 97, 164, 0.14)' : 'rgba(14, 165, 233, 0.12)';
    const trailStroke = isDarkMode ? 'rgba(89, 219, 199, 0.7)' : 'rgba(2, 132, 199, 0.8)';
    const obstacleFill = isDarkMode ? '#ffb4ab' : '#dc2626';
    const inflationFill = isDarkMode ? 'rgba(255, 210, 113, 0.20)' : 'rgba(217, 119, 6, 0.18)';
    const frontierFill = isDarkMode ? 'rgba(89, 219, 199, 0.75)' : 'rgba(2, 132, 199, 0.75)';
    const waypointColor = isDarkMode ? '#ffd271' : '#d97706';
    const tofRayColor = isDarkMode ? 'rgba(89, 219, 199, 0.35)' : 'rgba(2, 132, 199, 0.4)';
    const tofHitColor = isDarkMode ? '#59dbc7' : '#0284c7';
    const roverBodyFill = isDarkMode ? '#9fcaff' : '#0061a4';

    // Clear canvas
    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = bgFill;
    ctx.fillRect(0, 0, w, h);

    // 1. Grid (1m equivalent)
    const gridSize = pixelsPerMeter * zoom;
    ctx.strokeStyle = gridStroke;
    ctx.lineWidth = 1;

    const startX = cx % gridSize;
    for (let x = startX; x < w; x += gridSize) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, h);
      ctx.stroke();
    }

    const startY = cy % gridSize;
    for (let y = startY; y < h; y += gridSize) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(w, y);
      ctx.stroke();
    }

    // 2. Explored Corridor Region & Trajectory
    if (showTrajectory) {
      ctx.fillStyle = corridorFill;
      ctx.beginPath();
      exploredTrail.forEach((pt) => {
        const sx = cx + pt.x * zoom;
        const sy = cy + pt.y * zoom;
        const corridorR = 36 * zoom;
        ctx.moveTo(sx + corridorR, sy);
        ctx.arc(sx, sy, corridorR, 0, Math.PI * 2);
      });
      ctx.fill();

      // Trajectory Line
      if (exploredTrail.length > 1) {
        ctx.strokeStyle = trailStroke;
        ctx.lineWidth = 2 * zoom;
        ctx.setLineDash([4 * zoom, 4 * zoom]);
        ctx.beginPath();
        exploredTrail.forEach((pt, i) => {
          const sx = cx + pt.x * zoom;
          const sy = cy + pt.y * zoom;
          if (i === 0) ctx.moveTo(sx, sy);
          else ctx.lineTo(sx, sy);
        });
        ctx.stroke();
        ctx.setLineDash([]);
      }
    }

    // 3. Inflated footprint exclusion and persistent obstacle points.
    if (showInflation) {
      ctx.fillStyle = inflationFill;
      inflatedPoints.forEach((pt) => {
        const sx = cx + pt.x * zoom;
        const sy = cy + pt.y * zoom;
        ctx.fillRect(sx - 1.5 * zoom, sy - 1.5 * zoom,
          3 * zoom, 3 * zoom);
      });
    }
    if (showObstacles) {
      obstaclePoints.forEach(pt => {
        const sx = cx + pt.x * zoom;
        const sy = cy + pt.y * zoom;
        ctx.beginPath();
        ctx.arc(sx, sy, 2, 0, Math.PI * 2);
        ctx.fillStyle = obstacleFill;
        ctx.fill();
      });
    }

    if (showFrontiers) {
      ctx.fillStyle = frontierFill;
      frontierPoints.forEach((pt) => {
        const sx = cx + pt.x * zoom;
        const sy = cy + pt.y * zoom;
        ctx.fillRect(sx - 1.5 * zoom, sy - 1.5 * zoom,
          3 * zoom, 3 * zoom);
      });
    }

    // Sentinel zones are advisory map annotations only. They never enter the
    // occupancy grid or planner cost map.
    if (hazardZones.length) {
      ctx.save();
      hazardZones.forEach((
        /** @type {{x:number,y:number,score:number,state:string,simulated?:boolean}} */ zone,
        /** @type {number} */ index
      ) => {
        const zx = cx + zone.x * zoom;
        const zy = cy + zone.y * zoom;
        const critical = zone.state === 'CRITICAL';
        const color = critical ? '#dc2626' : '#d97706';
        const radius = (10 + Math.min(8, zone.score / 12)) * zoom;
        ctx.beginPath();
        ctx.arc(zx, zy, radius, 0, Math.PI * 2);
        ctx.fillStyle = critical
          ? 'rgba(220,38,38,0.13)' : 'rgba(217,119,6,0.11)';
        ctx.fill();
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.setLineDash(zone.simulated ? [5, 4] : []);
        ctx.stroke();
        ctx.setLineDash([]);
        if (index === hazardZones.length - 1) {
          ctx.fillStyle = color;
          ctx.font = '700 10px system-ui, sans-serif';
          ctx.textAlign = 'center';
          ctx.fillText(`AI ${zone.score}`, zx, zy + 3);
        }
      });
      ctx.restore();
    }

    // 4. Target Waypoint
    if (currentWaypoint && hasPoseEvidence) {
      const wx = cx + currentWaypoint.x * zoom;
      const wy = cy + currentWaypoint.y * zoom;
      const rx = cx + roverPose.x * zoom;
      const ry = cy + roverPose.y * zoom;

      ctx.strokeStyle = waypointColor;
      ctx.lineWidth = 2 * zoom;
      ctx.setLineDash([6 * zoom, 3 * zoom]);
      ctx.beginPath();
      ctx.moveTo(rx, ry);
      ctx.lineTo(wx, wy);
      ctx.stroke();
      ctx.setLineDash([]);

      ctx.beginPath();
      ctx.arc(wx, wy, 5, 0, Math.PI * 2);
      ctx.strokeStyle = waypointColor;
      ctx.lineWidth = 2;
      ctx.stroke();
    }

    // 5. Single-point VL53L0X rays mounted on the SG90 servo.
    const rx = cx + roverPose.x * zoom;
    const ry = cy + roverPose.y * zoom;
    const rad = (roverPose.headingDeg -
      roverCalibration.scanner.forwardAngleDeg) * (Math.PI / 180);

    // With no calibrated pose, keep the sweep in a clearly labelled local
    // rover frame instead of hiding real scan evidence or inventing a world pose.
    if (showTofRays && (hasPoseEvidence || hasLocalScanEvidence)) {
      scanPoints.forEach((/** @type {ScanPoint} */ p) => {
        if (!p.valid) return;
        const rayAngle = (roverPose.headingDeg -
          roverCalibration.scanner.forwardAngleDeg +
          (p.angle_deg - roverCalibration.scanner.forwardAngleDeg) +
          roverCalibration.scanner.bearingOffsetDeg) * (Math.PI / 180);
        const distPx = (p.distance_mm / 20) * zoom;
        const lx = rx + Math.cos(rayAngle) * distPx;
        const ly = ry + Math.sin(rayAngle) * distPx;

        ctx.strokeStyle = tofRayColor;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(rx, ry);
        ctx.lineTo(lx, ly);
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(lx, ly, 1.5, 0, Math.PI * 2);
        ctx.fillStyle = tofHitColor;
        ctx.fill();

      });
    }

    // 6. Top-view rover. With an unknown pose it stays at the local origin,
    // visibly translucent; this shows the connected rover without inventing
    // world coordinates.
    if (hasPoseEvidence || hasLocalScanEvidence || isConnected) {
      ctx.save();
      ctx.translate(rx, ry);
      ctx.rotate(rad + Math.PI / 2);
      ctx.globalAlpha = hasPoseEvidence ? 1 : 0.55;

      // Four wheels.
      ctx.fillStyle = isDarkMode ? '#30343b' : '#1f2937';
      for (const [x, y] of [[-10, -8], [10, -8], [-10, 8], [10, 8]])
        ctx.fillRect((x - 3) * zoom, (y - 5) * zoom, 6 * zoom, 10 * zoom);

      // Chassis, nose, and centered top ToF turret.
      ctx.fillStyle = roverBodyFill;
      ctx.beginPath();
      ctx.roundRect(-8 * zoom, -13 * zoom, 16 * zoom, 26 * zoom, 4 * zoom);
      ctx.fill();
      ctx.beginPath();
      ctx.moveTo(-6 * zoom, -13 * zoom);
      ctx.lineTo(0, -18 * zoom);
      ctx.lineTo(6 * zoom, -13 * zoom);
      ctx.closePath();
      ctx.fill();
      ctx.fillStyle = tofHitColor;
      ctx.beginPath();
      ctx.arc(0, -7 * zoom, 3.2 * zoom, 0, Math.PI * 2);
      ctx.fill();
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 1.5 * zoom;
      ctx.beginPath();
      ctx.moveTo(0, -9 * zoom);
      ctx.lineTo(0, -23 * zoom);
      ctx.stroke();

      ctx.restore();
    }
  }

  // Pointer drag handling
  /** @param {PointerEvent} e */
  function handlePointerDown(e) {
    if (e.button !== 0) return;
    followRover = false;
    isDragging = true;
    dragStart = { x: e.clientX - panX, y: e.clientY - panY };
  }

  /** @param {PointerEvent} e */
  function handlePointerMove(e) {
    if (!isDragging) return;
    panX = e.clientX - dragStart.x;
    panY = e.clientY - dragStart.y;
    drawMap();
  }

  function handlePointerUp() {
    isDragging = false;
  }

  /** @param {WheelEvent} e */
  function handleWheel(e) {
    e.preventDefault();
    const factor = e.deltaY < 0 ? 1.12 : 0.88;
    zoom = Math.max(0.3, Math.min(3.5, zoom * factor));
    drawMap();
  }

  /** @param {MouseEvent} e */
  function handleCanvasClick(e) {
    if (!canvas || !(isDemo || waypointEnabled) ||
        mode !== 'AUTO_EXPLORE' || !hasPoseEvidence) return;
    const rect = canvas.getBoundingClientRect();
    const clickX = e.clientX - rect.left;
    const clickY = e.clientY - rect.top;
    const cx = canvas.width / 2 + panX;
    const cy = canvas.height / 2 + panY;

    const mapX = +((clickX - cx) / zoom).toFixed(1);
    const mapY = +((clickY - cy) / zoom).toFixed(1);

    currentWaypoint = { x: mapX, y: mapY };
    onSetWaypoint(isDemo ? currentWaypoint : {
      x: Math.floor((mapX / pixelsPerMeter) /
        (mapEvidence?.cellSizeM || roverCalibration.map.cellSizeM)),
      y: Math.floor((mapY / pixelsPerMeter) /
        (mapEvidence?.cellSizeM || roverCalibration.map.cellSizeM))
    });
    drawMap();
  }

  function zoomIn() {
    zoom = Math.min(3.5, zoom * 1.2);
    drawMap();
  }

  function zoomOut() {
    zoom = Math.max(0.3, zoom / 1.2);
    drawMap();
  }

  function resetView() {
    zoom = 1.0;
    followRover = true;
    panX = hasPoseEvidence ? -roverPose.x * zoom : 0;
    panY = hasPoseEvidence ? -roverPose.y * zoom : 0;
    drawMap();
  }

  function clearMap() {
    obstaclePoints = [];
    exploredTrail = [{ x: roverPose.x, y: roverPose.y }];
    drawMap();
  }

  onMount(() => {
    updateThemeState();
    const handleThemeChange = () => {
      updateThemeState();
      scheduleDraw();
    };
    window.addEventListener('ui:themechange', handleThemeChange);

    const observer = new MutationObserver(() => {
      updateThemeState();
      scheduleDraw();
    });
    observer.observe(document.documentElement, { attributes: true, attributeFilter: ['class', 'data-theme'] });

    const resize = () => {
      if (!canvas?.parentElement) return;
      const width = canvas.parentElement.clientWidth;
      const height = canvas.parentElement.clientHeight || 460;
      if (canvas.width === width && canvas.height === height) return;
      canvas.width = width;
      canvas.height = height;
      scheduleDraw();
    };
    resize();

    const trailInterval = setInterval(() => {
      if (isRecording && isConnected) {
        const lastPt = exploredTrail[exploredTrail.length - 1];
        if (!lastPt || Math.hypot(lastPt.x - roverPose.x, lastPt.y - roverPose.y) > 6) {
          exploredTrail = [...exploredTrail,
            { x: roverPose.x, y: roverPose.y }];
          scheduleDraw();
        }
      }
    }, 250);

    const resizeObserver = typeof ResizeObserver === 'undefined'
      ? null : new ResizeObserver(resize);
    if (canvas?.parentElement) resizeObserver?.observe(canvas.parentElement);
    if (!resizeObserver) window.addEventListener('resize', resize);
    const handleVisibility = () => {
      if (!document.hidden) scheduleDraw();
    };
    document.addEventListener('visibilitychange', handleVisibility);
    scheduleDraw();

    return () => {
      window.removeEventListener('ui:themechange', handleThemeChange);
      observer.disconnect();
      clearInterval(trailInterval);
      resizeObserver?.disconnect();
      if (!resizeObserver) window.removeEventListener('resize', resize);
      document.removeEventListener('visibilitychange', handleVisibility);
      cancelAnimationFrame(drawFrame);
    };
  });
</script>

<div class="relative w-full h-full bg-[var(--md-sys-color-surface-container-lowest)] rounded-2xl border border-[var(--md-sys-color-outline-variant)] shadow-sm overflow-hidden flex flex-col select-none">
  
  <!-- Canvas Surface with Native Pointer Dragging & Wheel Zooming -->
  <!-- svelte-ignore a11y_click_events_have_key_events, a11y_no_noninteractive_element_interactions -->
  <canvas
    bind:this={canvas}
    class="w-full h-full block {isDragging ? 'cursor-grabbing' : 'cursor-grab'} touch-none"
    onclick={handleCanvasClick}
    onpointerdown={handlePointerDown}
    onpointermove={handlePointerMove}
    onpointerup={handlePointerUp}
    onpointercancel={handlePointerUp}
    onwheel={handleWheel}
  ></canvas>

  <div class="absolute top-3 left-3 px-2.5 py-1 rounded-md bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)] text-xs font-bold tracking-wide pointer-events-none">
    {isDemo ? 'SIMULATED · ESTIMATED VIEW' : hasPoseEvidence ? `REAL · ${dataSource} · DEAD-RECKONING ESTIMATE` : hasLocalScanEvidence ? `REAL · ${dataSource} · LOCAL SCAN / POSE UNKNOWN` : `REAL · ${dataSource} · POSE UNKNOWN`}
  </div>

  <!-- Top Right: Quiet Layer Toggles -->
  <div class="absolute top-3 right-3 flex items-center gap-1.5 pointer-events-auto">
    <button
      type="button"
      class="px-2.5 py-1 rounded-md text-sm font-medium border transition-colors duration-150 {showTrajectory ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] border-[var(--md-sys-color-primary-container)] shadow-sm' : 'bg-[var(--md-sys-color-surface-container)] text-[var(--md-sys-color-on-surface-variant)] border-transparent hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)]'}"
      onclick={() => { showTrajectory = !showTrajectory; drawMap(); }}
      title="Toggle Trajectory Layer"
    >
      Trajectory
    </button>
    <button
      type="button"
      class="px-2.5 py-1 rounded-md text-sm font-medium border transition-colors duration-150 {showObstacles ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] border-[var(--md-sys-color-primary-container)] shadow-sm' : 'bg-[var(--md-sys-color-surface-container)] text-[var(--md-sys-color-on-surface-variant)] border-transparent hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)]'}"
      onclick={() => { showObstacles = !showObstacles; drawMap(); }}
      title="Toggle Obstacle Hits"
    >
      Obstacles
    </button>
    <button
      type="button"
      class="px-2.5 py-1 rounded-md text-sm font-medium border transition-colors duration-150 {showTofRays ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] border-[var(--md-sys-color-primary-container)] shadow-sm' : 'bg-[var(--md-sys-color-surface-container)] text-[var(--md-sys-color-on-surface-variant)] border-transparent hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)]'}"
      onclick={() => { showTofRays = !showTofRays; drawMap(); }}
      title="Toggle ToF rays"
    >
      ToF rays
    </button>
    <button
      type="button"
      class="px-2.5 py-1 rounded-md text-sm font-medium border transition-colors duration-150 {showInflation ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] border-[var(--md-sys-color-primary-container)] shadow-sm' : 'bg-[var(--md-sys-color-surface-container)] text-[var(--md-sys-color-on-surface-variant)] border-transparent hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)]'}"
      onclick={() => { showInflation = !showInflation; drawMap(); }}
      title="Toggle inflated no-go cells"
    >
      Inflation
    </button>
    <button
      type="button"
      class="px-2.5 py-1 rounded-md text-sm font-medium border transition-colors duration-150 {showFrontiers ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] border-[var(--md-sys-color-primary-container)] shadow-sm' : 'bg-[var(--md-sys-color-surface-container)] text-[var(--md-sys-color-on-surface-variant)] border-transparent hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)]'}"
      onclick={() => { showFrontiers = !showFrontiers; drawMap(); }}
      title="Toggle exploration boundaries"
    >
      Frontiers
    </button>
  </div>

  <!-- Bottom Non-Wrapping Status Footer & Actions Toolbar -->
  <div class="absolute bottom-2.5 inset-x-2.5 flex items-center justify-between pointer-events-none gap-2">
    <!-- Non-Wrapping Single-Line Statistics Bar -->
    <div class="px-3 py-1.5 rounded-xl bg-[var(--md-sys-color-surface-container)]/95 backdrop-blur-md border border-[var(--md-sys-color-outline-variant)] text-[13px] text-[var(--md-sys-color-on-surface-variant)] shadow-sm flex items-center gap-1.5 whitespace-nowrap overflow-hidden text-ellipsis min-w-0 shrink flex-1 max-w-fit">
      <span>Pts: <strong class="telemetry text-[var(--md-sys-color-on-surface)]">{obstaclePoints.length}</strong></span>
      <span class="opacity-40">·</span>
      <span>Scan hits: <strong class="telemetry text-[var(--md-sys-color-on-surface)]">{validLocalScanCount}</strong></span>
      <span class="opacity-40">·</span>
      <span>Frontiers: <strong class="telemetry text-[var(--md-sys-color-on-surface)]">{frontierPoints.length}</strong></span>
      <span class="opacity-40">·</span>
      <span>Zoom: <strong class="telemetry text-[var(--ui-brand-cyan)]">{(zoom * 100).toFixed(0)}%</strong></span>
      <span class="opacity-40">·</span>
      <span class="truncate">Pose: <strong class="telemetry text-[var(--md-sys-color-on-surface)]">{hasPoseEvidence ? `x:${(roverPose.x / pixelsPerMeter).toFixed(2)}m y:${(roverPose.y / pixelsPerMeter).toFixed(2)}m` : 'UNKNOWN'}</strong></span>
    </div>

    <!-- Grouped Action Buttons -->
    <div class="flex items-center gap-1.5 pointer-events-auto shrink-0">
      <!-- Single Unified Recording Action -->
      <button
        type="button"
        class="ui-button !h-8 !px-3 text-sm font-medium transition-all {isRecording ? 'bg-[var(--md-sys-color-error)] text-[var(--md-sys-color-on-error)] animate-pulse' : 'ui-button--tonal'}"
        onclick={() => onToggleRecord()}
        disabled={!isDemo}
        title={isRecording ? 'Stop recording estimated trail' : 'Start recording estimated trail'}
      >
        <span class="material-symbols-rounded text-sm filled">{isRecording ? 'stop_circle' : 'fiber_manual_record'}</span>
        <span>{isRecording ? 'Stop' : 'Record'}</span>
      </button>

      <!-- Recenter / Follow Rover -->
      <button
        type="button"
        class="w-8 h-8 rounded-lg border flex items-center justify-center shadow-sm transition-all active:scale-95 {followRover ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] border-[var(--md-sys-color-primary)]' : 'bg-[var(--md-sys-color-surface-container)] text-[var(--md-sys-color-on-surface)] border-[var(--md-sys-color-outline-variant)]'}"
        title="Follow Rover"
        aria-label="Follow Rover"
        onclick={resetView}
        disabled={!hasPoseEvidence}
      >
        <span class="material-symbols-rounded text-base">my_location</span>
      </button>

      <!-- Zoom In -->
      <button
        type="button"
        class="w-8 h-8 rounded-lg bg-[var(--md-sys-color-surface-container)] hover:bg-[var(--md-sys-color-surface-container-highest)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center text-[var(--md-sys-color-on-surface)] shadow-sm transition-transform active:scale-95"
        title="Zoom In"
        aria-label="Zoom in"
        onclick={zoomIn}
      >
        <span class="material-symbols-rounded text-base">zoom_in</span>
      </button>

      <!-- Zoom Out -->
      <button
        type="button"
        class="w-8 h-8 rounded-lg bg-[var(--md-sys-color-surface-container)] hover:bg-[var(--md-sys-color-surface-container-highest)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center text-[var(--md-sys-color-on-surface)] shadow-sm transition-transform active:scale-95"
        title="Zoom Out"
        aria-label="Zoom out"
        onclick={zoomOut}
      >
        <span class="material-symbols-rounded text-base">zoom_out</span>
      </button>

      {#if isDemo}
        <!-- Clear simulated points -->
        <button
          type="button"
          class="w-8 h-8 rounded-lg bg-[var(--md-sys-color-surface-container)] hover:bg-[var(--md-sys-color-surface-container-highest)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center text-[var(--md-sys-color-on-surface)] shadow-sm transition-transform active:scale-95"
          title="Clear demo points"
          aria-label="Clear demo points"
          onclick={clearMap}
        >
          <span class="material-symbols-rounded text-base">delete_sweep</span>
        </button>
      {/if}
    </div>
  </div>
</div>
