<script>
  import { onMount } from 'svelte';
  import { getTheme } from '$lib/utils/theme.js';

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
    distanceMeters = 0,
    isConnected = true,
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

  // Layer Toggles (Subdued quiet states)
  let showTrajectory = $state(true);
  let showObstacles = $state(true);
  let showLaserRays = $state(true);

  /** @type {{x: number, y: number} | null} */
  let currentWaypoint = $state(null);

  // Persistent Obstacle Point Cloud & Explored Trail
  /** @type {Array<{x: number, y: number, intensity: number}>} */
  let obstaclePoints = $state([
    { x: -50, y: -90, intensity: 1 }, { x: 0, y: -95, intensity: 1 }, { x: 50, y: -90, intensity: 1 },
    { x: -60, y: -45, intensity: 1 }, { x: 60, y: -45, intensity: 1 },
    { x: -65, y: 0, intensity: 1 }, { x: 65, y: 0, intensity: 1 },
    { x: -60, y: 45, intensity: 1 }, { x: 60, y: 45, intensity: 1 },
    { x: -50, y: 90, intensity: 1 }, { x: 50, y: 90, intensity: 1 },
    { x: 90, y: 95, intensity: 1 }, { x: 130, y: 100, intensity: 1 }, { x: 170, y: 105, intensity: 1 },
    { x: 90, y: 135, intensity: 1 }, { x: 130, y: 140, intensity: 1 }, { x: 170, y: 145, intensity: 1 }
  ]);

  /** @type {Array<{x: number, y: number}>} */
  let exploredTrail = $state([
    { x: 0, y: 40 }, { x: 0, y: 20 }, { x: 0, y: 0 }
  ]);

  function updateThemeState() {
    isDarkMode = getTheme() === 'dark';
  }

  function drawMap() {
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const w = canvas.width;
    const h = canvas.height;

    if (followRover) {
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
    const waypointColor = isDarkMode ? '#ffd271' : '#d97706';
    const laserRayColor = isDarkMode ? 'rgba(89, 219, 199, 0.35)' : 'rgba(2, 132, 199, 0.4)';
    const laserHitColor = isDarkMode ? '#59dbc7' : '#0284c7';
    const roverBodyFill = isDarkMode ? '#9fcaff' : '#0061a4';

    // Clear canvas
    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = bgFill;
    ctx.fillRect(0, 0, w, h);

    // 1. Grid (1m equivalent)
    const gridSize = 40 * zoom;
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

    // 3. Persistent Obstacle Points
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

    // 4. Target Waypoint
    if (currentWaypoint) {
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

    // 5. Live SG90 Servo Laser Sweep Rays (VL53L0X)
    const rx = cx + roverPose.x * zoom;
    const ry = cy + roverPose.y * zoom;
    const rad = (roverPose.headingDeg - 90) * (Math.PI / 180);

    if (showLaserRays) {
      scanPoints.forEach((/** @type {ScanPoint} */ p) => {
        if (!p.valid) return;
        const rayAngle = (roverPose.headingDeg - 90 + (p.angle_deg - 90)) * (Math.PI / 180);
        const distPx = (p.distance_mm / 20) * zoom;
        const lx = rx + Math.cos(rayAngle) * distPx;
        const ly = ry + Math.sin(rayAngle) * distPx;

        ctx.strokeStyle = laserRayColor;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(rx, ry);
        ctx.lineTo(lx, ly);
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(lx, ly, 1.5, 0, Math.PI * 2);
        ctx.fillStyle = laserHitColor;
        ctx.fill();

        // Integrate hit points if recording
        if (isRecording && Math.random() < 0.08) {
          const mapX = roverPose.x + Math.cos(rayAngle) * (p.distance_mm / 20);
          const mapY = roverPose.y + Math.sin(rayAngle) * (p.distance_mm / 20);
          if (!obstaclePoints.some(pt => Math.hypot(pt.x - mapX, pt.y - mapY) < 6)) {
            obstaclePoints.push({ x: mapX, y: mapY, intensity: 1 });
          }
        }
      });
    }

    // 6. Rover Body Avatar
    ctx.save();
    ctx.translate(rx, ry);
    ctx.rotate(rad + Math.PI / 2);

    ctx.fillStyle = roverBodyFill;
    ctx.beginPath();
    ctx.moveTo(0, -14 * zoom);
    ctx.lineTo(10 * zoom, 12 * zoom);
    ctx.lineTo(0, 7 * zoom);
    ctx.lineTo(-10 * zoom, 12 * zoom);
    ctx.closePath();
    ctx.fill();

    // Nose heading line
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(0, -5 * zoom);
    ctx.lineTo(0, -18 * zoom);
    ctx.stroke();

    ctx.restore();
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
    if (!canvas || mode !== 'AUTO_EXPLORE') return;
    const rect = canvas.getBoundingClientRect();
    const clickX = e.clientX - rect.left;
    const clickY = e.clientY - rect.top;
    const cx = canvas.width / 2 + panX;
    const cy = canvas.height / 2 + panY;

    const mapX = +((clickX - cx) / zoom).toFixed(1);
    const mapY = +((clickY - cy) / zoom).toFixed(1);

    currentWaypoint = { x: mapX, y: mapY };
    onSetWaypoint(currentWaypoint);
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
    panX = -roverPose.x * zoom;
    panY = -roverPose.y * zoom;
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
      drawMap();
    };
    window.addEventListener('ui:themechange', handleThemeChange);

    const observer = new MutationObserver(() => {
      updateThemeState();
      drawMap();
    });
    observer.observe(document.documentElement, { attributes: true, attributeFilter: ['class', 'data-theme'] });

    if (canvas && canvas.parentElement) {
      canvas.width = canvas.parentElement.clientWidth;
      canvas.height = canvas.parentElement.clientHeight || 460;
    }

    const interval = setInterval(() => {
      const lastPt = exploredTrail[exploredTrail.length - 1];
      if (!lastPt || Math.hypot(lastPt.x - roverPose.x, lastPt.y - roverPose.y) > 6) {
        exploredTrail.push({ x: roverPose.x, y: roverPose.y });
      }
      drawMap();
    }, 100);

    const handleResize = () => {
      if (canvas && canvas.parentElement) {
        canvas.width = canvas.parentElement.clientWidth;
        canvas.height = canvas.parentElement.clientHeight || 460;
        drawMap();
      }
    };
    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('ui:themechange', handleThemeChange);
      observer.disconnect();
      clearInterval(interval);
      window.removeEventListener('resize', handleResize);
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
      class="px-2.5 py-1 rounded-md text-sm font-medium border transition-colors duration-150 {showLaserRays ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] border-[var(--md-sys-color-primary-container)] shadow-sm' : 'bg-[var(--md-sys-color-surface-container)] text-[var(--md-sys-color-on-surface-variant)] border-transparent hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)]'}"
      onclick={() => { showLaserRays = !showLaserRays; drawMap(); }}
      title="Toggle Laser Rays"
    >
      Laser rays
    </button>
  </div>

  <!-- Bottom Non-Wrapping Status Footer & Actions Toolbar -->
  <div class="absolute bottom-2.5 inset-x-2.5 flex items-center justify-between pointer-events-none gap-2">
    <!-- Non-Wrapping Single-Line Statistics Bar -->
    <div class="px-3 py-1.5 rounded-xl bg-[var(--md-sys-color-surface-container)]/95 backdrop-blur-md border border-[var(--md-sys-color-outline-variant)] text-[13px] text-[var(--md-sys-color-on-surface-variant)] shadow-sm flex items-center gap-1.5 whitespace-nowrap overflow-hidden text-ellipsis min-w-0 shrink flex-1 max-w-fit">
      <span>Pts: <strong class="telemetry text-[var(--md-sys-color-on-surface)]">{obstaclePoints.length}</strong></span>
      <span class="opacity-40">·</span>
      <span>Zoom: <strong class="telemetry text-[var(--ui-brand-cyan)]">{(zoom * 100).toFixed(0)}%</strong></span>
      <span class="opacity-40">·</span>
      <span class="truncate">Pose: <strong class="telemetry text-[var(--md-sys-color-on-surface)]">x:{roverPose.x.toFixed(1)} y:{roverPose.y.toFixed(1)}</strong></span>
    </div>

    <!-- Grouped Action Buttons -->
    <div class="flex items-center gap-1.5 pointer-events-auto shrink-0">
      <!-- Single Unified Recording Action -->
      <button
        type="button"
        class="ui-button !h-8 !px-3 text-sm font-medium transition-all {isRecording ? 'bg-[var(--md-sys-color-error)] text-[var(--md-sys-color-on-error)] animate-pulse' : 'ui-button--tonal'}"
        onclick={() => onToggleRecord()}
        title={isRecording ? 'Stop Recording Map' : 'Start Recording Map'}
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

      <!-- Clear Points -->
      <button
        type="button"
        class="w-8 h-8 rounded-lg bg-[var(--md-sys-color-surface-container)] hover:bg-[var(--md-sys-color-surface-container-highest)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center text-[var(--md-sys-color-on-surface)] shadow-sm transition-transform active:scale-95"
        title="Clear Point Cloud"
        aria-label="Clear point cloud"
        onclick={clearMap}
      >
        <span class="material-symbols-rounded text-base">delete_sweep</span>
      </button>
    </div>
  </div>
</div>
