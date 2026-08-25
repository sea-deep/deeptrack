<script>
  import { onMount } from 'svelte';

  /**
   * @typedef {Object} Props
   * @property {number} pitchDeg - Vehicle pitch in degrees (-90 to +90)
   * @property {number} rollDeg - Vehicle roll in degrees (-180 to +180)
   * @property {boolean} isStale - Whether telemetry is disconnected/stale
   * @property {boolean} [isCalibrating] - True if IMU is undergoing active calibration
   * @property {number} [maxTiltThreshold] - Max safe inclination angle before critical warning
   * @property {number} [cautionThreshold] - Warning inclination angle threshold
   */

  /** @type {Props} */
  let {
    pitchDeg = 0,
    rollDeg = 0,
    isStale = false,
    isCalibrating = false,
    maxTiltThreshold = 25.0,
    cautionThreshold = 12.0
  } = $props();

  let canvas = $state();
  let animationFrameId = 0;

  // Render configuration
  let currentPitch = $state(0);
  let currentRoll = $state(0);
  let prefersReducedMotion = $state(false);
  let isDarkMode = $state(true);

  let stabilityStatus = $derived.by(() => {
    if (isCalibrating) return 'calibrating';
    if (isStale) return 'stale';

    const maxInclination = Math.max(Math.abs(pitchDeg), Math.abs(rollDeg));
    if (maxInclination >= maxTiltThreshold) return 'critical';
    if (maxInclination >= cautionThreshold) return 'caution';
    return 'level';
  });

  function updateThemeState() {
    isDarkMode = document.documentElement.getAttribute('data-theme') === 'dark';
  }

  function drawHorizon() {
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const w = canvas.width;
    const h = canvas.height;
    const cx = w / 2;
    const cy = h / 2;
    const r = Math.min(w, h) / 2 - 4; // Use slightly padded radius for border

    ctx.clearRect(0, 0, w, h);

    ctx.save(); // Save base state

    // Circular clipping mask for the instrument dial
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.clip();

    // The pitch scale translates vertically.
    // At +/- 90 degrees, it shifts +/- r.
    // For small angles, we clamp them visually inside the visible reticle
    const clampedPitch = Math.max(-80, Math.min(80, currentPitch));
    const pitchOffset = cy + (clampedPitch * (r / 50));

    ctx.save(); // Save clip state before transformations

    // Apply vehicle Roll (rotation around center)
    ctx.translate(cx, cy);
    ctx.rotate((currentRoll * Math.PI) / 180);
    ctx.translate(-cx, -cy);

    // 1. Sky Region (Top)
    const skyGrad = ctx.createLinearGradient(0, -r * 2 + pitchOffset, 0, pitchOffset);
    if (isDarkMode) {
      skyGrad.addColorStop(0, '#001933');
      skyGrad.addColorStop(1, '#004c99');
    } else {
      skyGrad.addColorStop(0, '#bae6fd');
      skyGrad.addColorStop(1, '#7dd3fc');
    }
    ctx.fillStyle = skyGrad;
    ctx.fillRect(-w * 1.5, -h * 2 + pitchOffset, w * 3, h * 2);

    // 2. Ground Region (Bottom)
    const groundGrad = ctx.createLinearGradient(0, pitchOffset, 0, r * 2 + pitchOffset);
    if (isDarkMode) {
      groundGrad.addColorStop(0, '#3d1e03');
      groundGrad.addColorStop(1, '#1f0d00');
    } else {
      groundGrad.addColorStop(0, '#78350f');
      groundGrad.addColorStop(1, '#451a03');
    }
    ctx.fillStyle = groundGrad;
    ctx.fillRect(-w * 1.5, pitchOffset, w * 3, h * 2);

    // 3. High-Contrast Horizon Line
    ctx.strokeStyle = isDarkMode ? '#59dbc7' : '#ffffff';
    ctx.lineWidth = 2.5;
    ctx.beginPath();
    ctx.moveTo(-w * 1.5, pitchOffset);
    ctx.lineTo(w * 1.5, pitchOffset);
    ctx.stroke();

    // 4. Pitch Ladder Ticks (+10°, +20°, -10°, -20°)
    const pitchSteps = [20, 10, -10, -20];
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.7)';
    ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
    ctx.font = '9px "Google Sans Code", monospace';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    pitchSteps.forEach(deg => {
      const yPos = pitchOffset - deg * 2.4;
      const lineWidth = deg % 20 === 0 ? 36 : 22;

      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(-lineWidth, yPos);
      ctx.lineTo(-6, yPos);
      ctx.moveTo(6, yPos);
      ctx.lineTo(lineWidth, yPos);
      ctx.stroke();

      ctx.fillText(`${Math.abs(deg)}°`, lineWidth + 10, yPos);
      ctx.fillText(`${Math.abs(deg)}°`, -lineWidth - 10, yPos);
    });

    ctx.restore(); // Restore from roll/pitch transform

    // 5. Roll Angle Arc & Tick Marks (Top Outer Rim)
    const rollAngles = [-45, -30, -20, -10, 0, 10, 20, 30, 45];
    ctx.strokeStyle = isDarkMode ? 'rgba(255, 255, 255, 0.35)' : 'rgba(0, 0, 0, 0.4)';
    ctx.lineWidth = 1.5;

    rollAngles.forEach(deg => {
      const rad = (deg - 90) * (Math.PI / 180);
      const isMajor = deg === 0 || Math.abs(deg) === 30 || Math.abs(deg) === 45;
      const innerR = r - (isMajor ? 10 : 6);
      const outerR = r - 1;

      ctx.beginPath();
      ctx.moveTo(cx + Math.cos(rad) * innerR, cy + Math.sin(rad) * innerR);
      ctx.lineTo(cx + Math.cos(rad) * outerR, cy + Math.sin(rad) * outerR);
      ctx.stroke();
    });

    // 6. Fixed Vehicle Reticle (Center Reference Avatar)
    const reticleColor = isDarkMode ? '#ffd271' : '#f59e0b';
    ctx.strokeStyle = reticleColor;
    ctx.fillStyle = reticleColor;
    ctx.lineWidth = 2.5;

    // Left Wing
    ctx.beginPath();
    ctx.moveTo(cx - 38, cy);
    ctx.lineTo(cx - 14, cy);
    ctx.lineTo(cx - 14, cy + 6);
    ctx.stroke();

    // Right Wing
    ctx.beginPath();
    ctx.moveTo(cx + 38, cy);
    ctx.lineTo(cx + 14, cy);
    ctx.lineTo(cx + 14, cy + 6);
    ctx.stroke();

    // Center Bore Pip
    ctx.beginPath();
    ctx.arc(cx, cy, 3, 0, Math.PI * 2);
    ctx.fill();

    ctx.restore(); // Restore from circular mask clip

    // Outer Ring Border
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.strokeStyle = isDarkMode ? 'var(--md-sys-color-outline-variant)' : '#cbd5e1';
    ctx.lineWidth = 2;
    ctx.stroke();
  }

  function animate() {
    if (prefersReducedMotion || isStale) {
      currentPitch = pitchDeg;
      currentRoll = rollDeg;
    } else {
      // Lightly damped lerp interpolation (0.18)
      currentPitch += (pitchDeg - currentPitch) * 0.18;
      currentRoll += (rollDeg - currentRoll) * 0.18;
    }

    drawHorizon();
    animationFrameId = requestAnimationFrame(animate);
  }

  onMount(() => {
    updateThemeState();
    prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

    const handleThemeChange = () => {
      updateThemeState();
    };
    window.addEventListener('ui:themechange', handleThemeChange);

    if (canvas && canvas.parentElement) {
      const size = Math.min(canvas.parentElement.clientWidth || 180, 180);
      canvas.width = size;
      canvas.height = size;
    }

    const handleResize = () => {
      if (canvas && canvas.parentElement) {
        const size = Math.min(canvas.parentElement.clientWidth || 180, 180);
        canvas.width = size;
        canvas.height = size;
      }
    };
    window.addEventListener('resize', handleResize);

    currentPitch = pitchDeg;
    currentRoll = rollDeg;
    animationFrameId = requestAnimationFrame(animate);

    return () => {
      window.removeEventListener('ui:themechange', handleThemeChange);
      window.removeEventListener('resize', handleResize);
      cancelAnimationFrame(animationFrameId);
    };
  });
</script>

<div class="flex flex-col items-center relative select-none w-full h-full justify-center">
  <!-- Artificial Horizon Canvas Surface -->
  <div class="relative flex items-center justify-center shrink-0 w-full my-1">
    <canvas
      bind:this={canvas}
      class="block rounded-full transition-[filter,opacity] duration-300 shadow-sm {isStale ? 'grayscale opacity-60' : ''}"
    ></canvas>

    <!-- Translucent Overlay for Stale or Calibrating States -->
    {#if isStale || isCalibrating}
      <div class="absolute inset-0 flex items-center justify-center pointer-events-none">
        <div class="px-3 py-1 rounded-lg bg-black/75 backdrop-blur-md text-white text-[11px] font-medium border border-white/20 shadow-lg">
          {isCalibrating ? 'Calibrating...' : 'Frozen'}
        </div>
      </div>
    {/if}
  </div>

  <!-- Single Compact Row for Telemetry and Badge -->
  <div class="w-full flex items-center justify-between mt-3 px-1">
    <div class="flex flex-col items-start w-16">
      <span class="text-[10px] text-[var(--md-sys-color-on-surface-variant)] uppercase tracking-wider font-bold">Pitch</span>
      <strong class="telemetry text-base font-bold leading-tight {Math.abs(pitchDeg) >= maxTiltThreshold ? 'text-[var(--md-sys-color-error)]' : Math.abs(pitchDeg) >= cautionThreshold ? 'text-[var(--ui-color-warning)]' : 'text-[var(--md-sys-color-on-surface)]'}">
        {pitchDeg > 0 ? `+${pitchDeg.toFixed(1)}` : pitchDeg.toFixed(1)}°
      </strong>
    </div>

    <span class="text-[11px] font-bold px-2 py-1 rounded transition-colors duration-150 truncate max-w-[120px] text-center"
      class:bg-[var(--ui-color-success-container)]={stabilityStatus === 'level'}
      class:text-[var(--ui-color-on-success-container)]={stabilityStatus === 'level'}
      class:bg-[var(--ui-color-warning-container)]={stabilityStatus === 'caution'}
      class:text-[var(--ui-color-on-warning-container)]={stabilityStatus === 'caution'}
      class:bg-[var(--md-sys-color-error-container)]={stabilityStatus === 'critical'}
      class:text-[var(--md-sys-color-on-error-container)]={stabilityStatus === 'critical'}
      class:bg-[var(--md-sys-color-surface-container-highest)]={stabilityStatus === 'stale' || stabilityStatus === 'calibrating'}
      class:text-[var(--md-sys-color-on-surface-variant)]={stabilityStatus === 'stale' || stabilityStatus === 'calibrating'}
    >
      {#if stabilityStatus === 'calibrating'} Calibrating
      {:else if stabilityStatus === 'stale'} Stale
      {:else if stabilityStatus === 'critical'} Critical
      {:else if stabilityStatus === 'caution'} Caution
      {:else} Level {/if}
    </span>

    <div class="flex flex-col items-end w-16">
      <span class="text-[10px] text-[var(--md-sys-color-on-surface-variant)] uppercase tracking-wider font-bold">Roll</span>
      <strong class="telemetry text-base font-bold leading-tight {Math.abs(rollDeg) >= maxTiltThreshold ? 'text-[var(--md-sys-color-error)]' : Math.abs(rollDeg) >= cautionThreshold ? 'text-[var(--ui-color-warning)]' : 'text-[var(--md-sys-color-on-surface)]'}">
        {rollDeg > 0 ? `+${rollDeg.toFixed(1)}` : rollDeg.toFixed(1)}°
      </strong>
    </div>
  </div>
</div>
