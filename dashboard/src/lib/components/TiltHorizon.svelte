<script>
  import { onMount } from 'svelte';
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

  /** @type {HTMLDivElement} */
  let host;
  /** @type {HTMLCanvasElement} */
  let canvas;
  let animationFrameId = 0;
  let prefersReducedMotion = false;
  let requestRender = () => {};

  // MPU +X points toward the chassis front in the supplied mounting reference.
  // Convert the 90-degree-rotated sensor frame into chassis pitch and roll.
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

  // Wake the renderer only when orientation-related inputs change. The
  // renderer stops itself once the damped model reaches the new pose.
  $effect(() => {
    void [chassisPitch, chassisRoll, chassisHeading, isStale, isCalibrating];
    requestRender();
  });

  /** @param {number} value */
  const signed = (value) => `${value > 0 ? '+' : ''}${value.toFixed(1)}°`;
  /** @param {string} status */
  const statusLabel = (status) => /** @type {Record<string, string>} */ ({
    unknown: 'Unknown', calibrating: 'Calibrating', stale: 'Frozen',
    critical: 'Critical tilt', caution: 'Caution', level: 'Level'
  })[status];

  onMount(() => {
    let disposed = false;
    let disposeScene = () => {};
    void import('three').then((THREE) => {
    if (disposed) return;
    prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(34, 1, 0.1, 100);
    camera.position.set(5.8, 4.4, 7.2);
    camera.lookAt(0, 0.15, 0);

    const renderer = new THREE.WebGLRenderer({
      canvas,
      alpha: true,
      antialias: (window.devicePixelRatio || 1) <= 1.25,
      powerPreference: 'low-power'
    });
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 1.5));
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 1.1;

    scene.add(new THREE.HemisphereLight(0xd9f7ff, 0x1b2330, 2.4));
    const key = new THREE.DirectionalLight(0xffffff, 3.2);
    key.position.set(4, 7, 5);
    scene.add(key);
    const rim = new THREE.DirectionalLight(0x6fe7ff, 1.5);
    rim.position.set(-5, 2, -4);
    scene.add(rim);

    const rover = new THREE.Group();
    rover.rotation.order = 'YXZ';
    scene.add(rover);

    const blue = new THREE.MeshStandardMaterial({ color: 0x2479df, roughness: 0.55, metalness: 0.08 });
    const blueTop = new THREE.MeshStandardMaterial({ color: 0x45a8ff, roughness: 0.45, metalness: 0.08 });
    const tyre = new THREE.MeshStandardMaterial({ color: 0x1e293b, roughness: 0.82 });
    const hub = new THREE.MeshStandardMaterial({ color: 0xf6c84c, roughness: 0.55, metalness: 0.12 });
    const mast = new THREE.MeshStandardMaterial({ color: 0x55dfcf, roughness: 0.35, metalness: 0.15 });
    const dark = new THREE.MeshStandardMaterial({ color: 0x122333, roughness: 0.5, metalness: 0.2 });

    const body = new THREE.Mesh(new THREE.BoxGeometry(2.7, 0.72, 3.55), blue);
    body.position.y = 0.42;
    rover.add(body);
    const deck = new THREE.Mesh(new THREE.BoxGeometry(2.35, 0.18, 2.45), blueTop);
    deck.position.set(0, 0.91, -0.25);
    rover.add(deck);

    for (const z of [-1.2, 1.2]) {
      for (const x of [-1.62, 1.62]) {
        const wheel = new THREE.Mesh(new THREE.CylinderGeometry(0.58, 0.58, 0.46, 16), tyre);
        wheel.rotation.z = Math.PI / 2;
        wheel.position.set(x, 0, z);
        rover.add(wheel);
        const wheelHub = new THREE.Mesh(new THREE.CylinderGeometry(0.25, 0.25, 0.48, 12), hub);
        wheelHub.rotation.z = Math.PI / 2;
        wheelHub.position.copy(wheel.position);
        rover.add(wheelHub);
      }
    }

    const mastPost = new THREE.Mesh(new THREE.CylinderGeometry(0.09, 0.09, 1.25, 12), mast);
    mastPost.position.set(0, 1.55, -1.16);
    rover.add(mastPost);
    const sensorHead = new THREE.Mesh(new THREE.CapsuleGeometry(0.26, 0.3, 4, 10), dark);
    sensorHead.rotation.z = Math.PI / 2;
    sensorHead.position.set(0, 2.16, -1.16);
    rover.add(sensorHead);
    const lensMaterial = new THREE.MeshStandardMaterial({ color: 0x75e8ff, emissive: 0x164e63, emissiveIntensity: 1.5 });
    const lens = new THREE.Mesh(new THREE.CylinderGeometry(0.09, 0.09, 0.05, 10), lensMaterial);
    lens.rotation.x = Math.PI / 2;
    lens.position.set(0, 2.16, -1.48);
    rover.add(lens);

    const frontMarker = new THREE.Mesh(new THREE.ConeGeometry(0.16, 0.42, 3), mast);
    frontMarker.rotation.x = -Math.PI / 2;
    frontMarker.position.set(0, 0.98, -1.82);
    rover.add(frontMarker);

    const platform = new THREE.Mesh(
      new THREE.CircleGeometry(3.9, 32),
      new THREE.MeshStandardMaterial({ color: 0x2b3947, roughness: 0.95, transparent: true, opacity: 0.34 })
    );
    platform.rotation.x = -Math.PI / 2;
    platform.position.y = -0.61;
    scene.add(platform);
    const grid = new THREE.GridHelper(8, 16, 0x4adcca, 0x536474);
    grid.position.y = -0.59;
    const gridMaterials = Array.isArray(grid.material) ? grid.material : [grid.material];
    gridMaterials.forEach((material) => {
      material.transparent = true;
      material.opacity = 0.34;
    });
    scene.add(grid);

    let shownPitch = chassisPitch;
    let shownRoll = chassisRoll;
    let shownHeading = chassisHeading;
    const resize = () => {
      if (!host) return;
      const width = Math.max(220, host.clientWidth);
      const height = Math.max(190, host.clientHeight);
      renderer.setSize(width, height, false);
      camera.aspect = width / height;
      camera.updateProjectionMatrix();
    };
    const resizeObserver = typeof ResizeObserver === 'undefined'
      ? null : new ResizeObserver(resize);
    resizeObserver?.observe(host);
    if (!resizeObserver) window.addEventListener('resize', resize);

    let isRendering = false;
    const animate = () => {
      if (document.hidden) {
        isRendering = false;
        return;
      }
      const damping = prefersReducedMotion || isStale ? 1 : 0.12;
      shownPitch += (chassisPitch - shownPitch) * damping;
      shownRoll += (chassisRoll - shownRoll) * damping;
      const headingDelta = ((chassisHeading - shownHeading + 540) % 360) - 180;
      shownHeading += headingDelta * damping;
      rover.rotation.set(
        THREE.MathUtils.degToRad(shownPitch),
        THREE.MathUtils.degToRad(-shownHeading),
        THREE.MathUtils.degToRad(shownRoll),
        'YXZ'
      );
      renderer.render(scene, camera);
      const unsettled = !prefersReducedMotion && !isStale && (
        Math.abs(chassisPitch - shownPitch) > 0.05 ||
        Math.abs(chassisRoll - shownRoll) > 0.05 ||
        Math.abs(((chassisHeading - shownHeading + 540) % 360) - 180) > 0.08
      );
      if (unsettled) animationFrameId = requestAnimationFrame(animate);
      else isRendering = false;
    };
    requestRender = () => {
      if (isRendering) return;
      isRendering = true;
      animationFrameId = requestAnimationFrame(animate);
    };
    const handleVisibility = () => {
      if (!document.hidden) requestRender();
    };
    document.addEventListener('visibilitychange', handleVisibility);
    resize();
    requestRender();

    disposeScene = () => {
      requestRender = () => {};
      cancelAnimationFrame(animationFrameId);
      resizeObserver?.disconnect();
      if (!resizeObserver) window.removeEventListener('resize', resize);
      document.removeEventListener('visibilitychange', handleVisibility);
      scene.traverse((item) => {
        if (item instanceof THREE.Mesh) {
          item.geometry?.dispose();
          const materials = Array.isArray(item.material) ? item.material : [item.material];
          materials.forEach((material) => material.dispose());
        }
      });
      renderer.dispose();
    };
    }).catch(() => { requestRender = () => {}; });

    return () => {
      disposed = true;
      disposeScene();
    };
  });
</script>

<div data-tilt-horizon class="flex h-full min-h-[280px] w-full flex-col select-none">
  <div bind:this={host} class="relative min-h-[190px] flex-1 overflow-hidden rounded-xl border border-[var(--md-sys-color-outline-variant)] bg-[radial-gradient(circle_at_50%_28%,rgba(77,208,225,0.14),transparent_58%)]">
    <canvas bind:this={canvas} class:opacity-45={isStale || !hasOrientation} class="block h-full w-full"></canvas>
    <div class="pointer-events-none absolute left-3 top-3 flex items-center gap-2 rounded-full border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)]/85 px-2.5 py-1 text-xs font-bold uppercase tracking-wide backdrop-blur">
      <span class="h-2 w-2 rounded-full" class:bg-[var(--ui-color-success)]={stabilityStatus === 'level'} class:bg-[var(--ui-color-warning)]={stabilityStatus === 'caution'} class:bg-[var(--md-sys-color-error)]={stabilityStatus === 'critical'} class:bg-[var(--md-sys-color-outline)]={['unknown','stale','calibrating'].includes(stabilityStatus)}></span>
      {statusLabel(stabilityStatus)}
    </div>
    <div class="pointer-events-none absolute right-3 top-3 rounded-full border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)]/85 px-2.5 py-1 text-xs font-semibold text-[var(--md-sys-color-on-surface-variant)] backdrop-blur">Front ▲</div>
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
