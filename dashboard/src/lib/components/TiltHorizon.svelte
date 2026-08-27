<script>
  import { roverCalibration } from '$lib/config/roverCalibration.js';

  let {
    pitchDeg = null,
    rollDeg = null,
    headingDeg = null,
    isStale = false,
    isCalibrating = false,
    maxTiltThreshold = 25,
    cautionThreshold = 12
  } = $props();

  let hasOrientation = $derived(Number.isFinite(pitchDeg) && Number.isFinite(rollDeg));
  let chassisPitch = $derived(hasOrientation
    ? (roverCalibration.imuMount.chassisPitchFrom === 'roll'
        ? Number(rollDeg) : Number(pitchDeg)) *
        roverCalibration.imuMount.chassisPitchSign +
        roverCalibration.imuMount.chassisPitchOffsetDeg : 0);
  let chassisRoll = $derived(hasOrientation
    ? (roverCalibration.imuMount.chassisRollFrom === 'pitch'
        ? Number(pitchDeg) : Number(rollDeg)) *
        roverCalibration.imuMount.chassisRollSign +
        roverCalibration.imuMount.chassisRollOffsetDeg : 0);
  let chassisHeading = $derived(Number.isFinite(headingDeg)
    ? ((Number(headingDeg) * roverCalibration.imuMount.chassisYawSign +
        roverCalibration.imuMount.chassisYawOffsetDeg) % 360 + 360) % 360
    : 0);
  let maxInclination = $derived(Math.max(Math.abs(chassisPitch), Math.abs(chassisRoll)));
  let stabilityStatus = $derived(
    !hasOrientation ? 'unknown'
      : isCalibrating ? 'calibrating'
        : isStale ? 'stale'
          : maxInclination >= maxTiltThreshold ? 'critical'
            : maxInclination >= cautionThreshold ? 'caution'
              : 'level'
  );
  // Keep the isometric rover in a stable camera projection. Rotating that
  // already-projected drawing made quarter turns look as if the rover had
  // stood on its nose. Relative yaw belongs to the ground-plane compass.
  let modelTransform = $derived(
    `translate(160 ${116 + Math.max(-3, Math.min(3, chassisPitch * 0.1))}) skewX(${Math.max(-3, Math.min(3, chassisRoll * 0.08))})`
  );
  let headingTransform = $derived(`translate(160 125) rotate(${-chassisHeading})`);

  /** @param {number} value */
  const signed = (value) => `${value > 0 ? '+' : ''}${value.toFixed(1)}°`;
  /** @param {string} status */
  const statusLabel = (status) => /** @type {Record<string, string>} */ ({
    unknown: 'Unknown', calibrating: 'Calibrating', stale: 'Frozen',
    critical: 'Critical tilt', caution: 'Caution', level: 'Level'
  })[status];
</script>

<div data-tilt-horizon class="flex h-full min-h-[280px] w-full flex-col select-none">
  <div class="relative min-h-[190px] flex-1 overflow-hidden rounded-xl border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface-container-lowest)]">
    <svg
      viewBox="0 0 320 220"
      class="block h-full w-full transition-opacity duration-150"
      class:opacity-45={isStale || !hasOrientation}
      role="img"
      aria-label="Top perspective rover orientation"
    >
      <defs>
        <pattern id="orientation-grid" width="20" height="20" patternUnits="userSpaceOnUse" patternTransform="skewX(-28)">
          <path d="M 20 0 L 0 0 0 20" fill="none" stroke="var(--md-sys-color-outline-variant)" stroke-width="0.7" opacity="0.48" />
        </pattern>
        <linearGradient id="orientation-body" x1="0" y1="0" x2="1" y2="1">
          <stop offset="0" stop-color="#43b6f4" />
          <stop offset="1" stop-color="#1769c2" />
        </linearGradient>
        <linearGradient id="orientation-deck" x1="0" y1="0" x2="1" y2="1">
          <stop offset="0" stop-color="#8be5ff" />
          <stop offset="1" stop-color="#48a9e8" />
        </linearGradient>
        <filter id="orientation-shadow" x="-30%" y="-30%" width="160%" height="180%">
          <feDropShadow dx="0" dy="8" stdDeviation="7" flood-color="#001f25" flood-opacity="0.26" />
        </filter>
      </defs>

      <rect width="320" height="220" fill="url(#orientation-grid)" />
      <ellipse cx="160" cy="125" rx="112" ry="67" fill="var(--md-sys-color-surface-container-high)" opacity="0.72" />
      <ellipse cx="160" cy="125" rx="112" ry="67" fill="none" stroke="var(--md-sys-color-outline-variant)" />

      <g transform={headingTransform} class="heading-vector" aria-hidden="true">
        <path d="M0 -82 L0 -58" stroke="var(--ui-brand-cyan)" stroke-width="3" stroke-linecap="round" />
        <path d="M0 -91 l-7 13 h14 z" fill="var(--ui-brand-cyan)" />
        <circle cx="0" cy="0" r="3" fill="var(--ui-brand-cyan)" />
      </g>

      <g transform={modelTransform} class="orientation-rover" filter="url(#orientation-shadow)">
        <g fill="#172235" stroke="#07111f" stroke-width="2">
          <ellipse cx="-61" cy="-32" rx="17" ry="11" />
          <ellipse cx="61" cy="-32" rx="17" ry="11" />
          <ellipse cx="-61" cy="33" rx="17" ry="11" />
          <ellipse cx="61" cy="33" rx="17" ry="11" />
        </g>
        <g fill="#f5ce57" stroke="#fff4b6" stroke-width="1.5">
          <ellipse cx="-61" cy="-32" rx="7" ry="5" />
          <ellipse cx="61" cy="-32" rx="7" ry="5" />
          <ellipse cx="-61" cy="33" rx="7" ry="5" />
          <ellipse cx="61" cy="33" rx="7" ry="5" />
        </g>

        <path d="M-48 -47 L40 -47 L58 34 L-39 34 L-55 17 Z" fill="url(#orientation-body)" stroke="#0a579f" stroke-width="2" />
        <path d="M-48 -47 L40 -47 L28 -58 L-37 -58 Z" fill="#78d4ff" opacity="0.82" />
        <path d="M-39 34 L58 34 L50 43 L-30 43 Z" fill="#0e58a8" opacity="0.9" />
        <path d="M-33 -30 L29 -30 L38 10 L-26 10 Z" fill="url(#orientation-deck)" stroke="#a7ebff" stroke-width="1.5" />

        <path d="M0 -48 L0 -76" stroke="#54dcca" stroke-width="5" stroke-linecap="round" />
        <ellipse cx="0" cy="-80" rx="12" ry="7" fill="#101c2d" stroke="#58ddec" stroke-width="2" />
        <circle cx="0" cy="-84" r="2.5" fill="#9bf4ff" />
        <path d="M0 -49 l-7 12 h14 z" fill="#66e6d3" />
      </g>
    </svg>

    <div class="pointer-events-none absolute left-3 top-3 flex items-center gap-2 rounded-full border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)]/90 px-2.5 py-1 text-xs font-bold uppercase tracking-wide backdrop-blur">
      <span class="h-2 w-2 rounded-full" class:bg-[var(--ui-color-success)]={stabilityStatus === 'level'} class:bg-[var(--ui-color-warning)]={stabilityStatus === 'caution'} class:bg-[var(--md-sys-color-error)]={stabilityStatus === 'critical'} class:bg-[var(--md-sys-color-outline)]={['unknown','stale','calibrating'].includes(stabilityStatus)}></span>
      {statusLabel(stabilityStatus)}
    </div>
    <div class="pointer-events-none absolute right-3 top-3 rounded-full border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)]/90 px-2.5 py-1 text-xs font-semibold text-[var(--md-sys-color-on-surface-variant)] backdrop-blur">Heading <span class="text-[var(--ui-brand-cyan)]">▲</span></div>
    {#if !hasOrientation || isStale || isCalibrating}
      <div class="pointer-events-none absolute inset-0 grid place-items-center">
        <div class="rounded-lg border border-white/15 bg-black/75 px-3 py-1.5 text-sm font-medium text-white shadow-lg backdrop-blur">{statusLabel(stabilityStatus)}</div>
      </div>
    {/if}
  </div>

  <div class="mt-3 grid grid-cols-3 gap-2">
    <div class="rounded-lg bg-[var(--md-sys-color-surface-container)] px-2.5 py-2">
      <div class="text-xs font-bold uppercase tracking-wide text-[var(--md-sys-color-on-surface-variant)]">Pitch</div>
      <div class="telemetry mt-0.5 text-sm font-bold">{hasOrientation ? signed(chassisPitch) : '—'}</div>
    </div>
    <div class="rounded-lg bg-[var(--md-sys-color-surface-container)] px-2.5 py-2 text-center">
      <div class="text-xs font-bold uppercase tracking-wide text-[var(--md-sys-color-on-surface-variant)]">Relative yaw</div>
      <div class="telemetry mt-0.5 text-sm font-bold">{Number.isFinite(headingDeg) ? `${Math.round(chassisHeading)}°` : '—'}</div>
    </div>
    <div class="rounded-lg bg-[var(--md-sys-color-surface-container)] px-2.5 py-2 text-right">
      <div class="text-xs font-bold uppercase tracking-wide text-[var(--md-sys-color-on-surface-variant)]">Roll</div>
      <div class="telemetry mt-0.5 text-sm font-bold">{hasOrientation ? signed(chassisRoll) : '—'}</div>
    </div>
  </div>
  <div class="mt-1.5 text-center text-[11px] leading-tight text-[var(--md-sys-color-on-surface-variant)]">90° MPU mount correction + reversed yaw axis · yaw is gyro-relative</div>
</div>

<style>
  .orientation-rover {
    transform-box: fill-box;
    transform-origin: center;
    transition: transform 140ms ease-out;
  }

  .heading-vector {
    transform-box: fill-box;
    transform-origin: center;
    transition: transform 140ms ease-out;
  }

  @media (prefers-reduced-motion: reduce) {
    .orientation-rover,
    .heading-vector { transition: none; }
  }
</style>
