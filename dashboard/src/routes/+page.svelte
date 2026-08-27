<script>
  import Navbar from '$lib/components/Navbar.svelte';
  import Footer from '$lib/components/Footer.svelte';
  import { onMount } from 'svelte';
  import { fade } from 'svelte/transition';

  const roverViews = [
    { src: '/images/rover/front.png', angle: 'Front view' },
    { src: '/images/rover/right.png', angle: 'Right view' },
    { src: '/images/rover/rear.png', angle: 'Rear view' },
    { src: '/images/rover/left.png', angle: 'Left view' }
  ];

  const features = [
    { icon: 'air', eyebrow: 'Gas sensing', title: 'Methane activity', copy: 'The MQ-4 watches for changes in nearby gas levels and sends the raw reading to the dashboard.' },
    { icon: 'device_thermostat', eyebrow: 'Environment', title: 'Temperature & humidity', copy: 'A DHT22 keeps track of heat and moisture so the operator has more context about the route.' },
    { icon: 'route', eyebrow: 'Movement', title: 'Assisted self-drive', copy: 'Distance sensors help the rover spot obstacles, choose open space and stop before a collision.' },
    { icon: 'wifi_off', eyebrow: 'Connection', title: 'Works offline', copy: 'The rover and gateway talk directly over ESP-NOW. The core controls do not need internet access.' }
  ];

  const hardware = [
    { image: '/images/components/esp32.webp', name: 'ESP32 controller', tag: 'Control', copy: 'Reads the sensors, runs the rover logic and sends updates to the gateway.' },
    { image: '/images/components/mq4.webp', name: 'MQ-4 gas sensor', tag: 'Air', copy: 'Reports raw methane-sensitive activity. It is useful for trends, not certified safety readings.' },
    { image: '/images/components/dht.webp', name: 'DHT22 sensor', tag: 'Climate', copy: 'Measures the temperature and relative humidity around the rover.' },
    { image: '/images/components/hypersonic.webp', name: 'HC-SR04 sonar', tag: 'Distance', copy: 'Checks the space directly ahead and helps stop the rover before it reaches an obstacle.' },
    { image: '/images/components/encoder.webp', name: 'Optical encoder', tag: 'Motion', copy: 'Measures wheel rotation to estimate speed and distance traveled.' },
    { image: '/images/components/mpu.webp', name: 'MPU6050 IMU', tag: 'Tilt', copy: 'Adds tilt and movement context when the rover travels over uneven ground.' },
    { image: '/images/components/vl53l0x.webp', name: 'VL53L0X ToF', tag: 'Distance', copy: 'High-precision Time-of-Flight laser sensor for millimeter-accurate obstacle detection.' },
    { image: '/images/components/water-sensor.png', name: 'Water contact probe', tag: 'Liquid', copy: 'Detects the presence of water or puddles under the rover chassis.' },
    { image: '/images/components/chasis.webp', name: '4WD drive system', tag: 'Drive', copy: 'Four geared motors and two motor drivers provide simple skid-steer movement.' }
  ];

  let activeView = $state(0);

  function showNextView() {
    activeView = (activeView + 1) % roverViews.length;
  }

  /** @param {number} index */
  function showView(index) {
    activeView = index;
  }

  /** @param {KeyboardEvent} event */
  function handleCarouselKeydown(event) {
    if (event.key === 'ArrowRight' || event.key === 'Enter' || event.key === ' ') {
      event.preventDefault();
      showNextView();
    }

    if (event.key === 'ArrowLeft') {
      event.preventDefault();
      activeView = (activeView - 1 + roverViews.length) % roverViews.length;
    }
  }

  onMount(() => {
    for (const { src } of roverViews.slice(1)) {
      const image = new Image();
      image.decoding = 'async';
      image.src = src;
    }
    const interval = window.setInterval(showNextView, 4200);
    return () => window.clearInterval(interval);
  });
</script>

<svelte:head>
  <title>DeepTrack — Mine rescue rover prototype</title>
  <meta name="description" content="DeepTrack is a v0 mine rescue rover prototype for offline sensing, assisted driving and operator control." />
</svelte:head>

<div class="min-h-screen flex flex-col bg-[var(--md-sys-color-surface)] text-[var(--md-sys-color-on-surface)]">
  <Navbar active="home" />

  <main class="flex-1 overflow-hidden">
    <section class="landing-hero" aria-labelledby="hero-title">
      <div class="hero-copy">


        <h1 id="hero-title">A V0 prototype model for a mine rescue rover.</h1>

        <p>DeepTrack is a small four-wheel rover built to explore hard-to-reach spaces, watch its surroundings and send useful readings back to an operator.</p>

        <div class="hero-actions">
          <a href="/dashboard" class="ui-button ui-button--filled !h-12 !px-6 text-sm font-medium shadow-md hover:shadow-lg">
            <span class="material-symbols-rounded text-xl">terminal</span>
            Open dashboard
          </a>
          <a href="#hardware" class="ui-button ui-button--outlined !h-12 !px-6 text-sm font-medium">
            <span class="material-symbols-rounded text-xl">developer_board</span>
            See hardware
          </a>
        </div>
      </div>

      <div class="rover-showcase hero-visual" aria-label="Rover image carousel">
        <button
          type="button"
          class="rover-stage"
          onclick={showNextView}
          onkeydown={handleCarouselKeydown}
          aria-label={`Show next rover view. Current view: ${roverViews[activeView].angle}`}
        >
          <div class="stage-grid"></div>
          {#key activeView}
            <img
              src={roverViews[activeView].src}
              alt={`DeepTrack rover — ${roverViews[activeView].angle}`}
              width="960"
              height="720"
              fetchpriority={activeView === 0 ? 'high' : 'auto'}
              in:fade={{ duration: 480 }}
              out:fade={{ duration: 220 }}
            />
          {/key}

          <span class="next-view">
            Next view
            <span class="material-symbols-rounded">arrow_forward</span>
          </span>
        </button>

        <div class="carousel-dots" aria-label="Choose a rover view">
          {#each roverViews as view, index}
            <button
              type="button"
              class:active={activeView === index}
              onclick={() => showView(index)}
              aria-label={`Show ${view.angle}`}
              aria-current={activeView === index ? 'true' : undefined}
            ></button>
          {/each}
        </div>

      </div>
    </section>

    <section class="feature-section" aria-labelledby="features-title">
      <div class="section-heading section-heading--left">
        <span>What it can do</span>
        <h2 id="features-title">Useful tools for a first prototype.</h2>
      </div>

      <div class="feature-grid">
        {#each features as feature, index}
          <article class="feature-card">
            <div class="feature-number">0{index + 1}</div>
            <span class="material-symbols-rounded feature-icon">{feature.icon}</span>
            <p class="card-eyebrow">{feature.eyebrow}</p>
            <h3>{feature.title}</h3>
            <p class="card-copy">{feature.copy}</p>
          </article>
        {/each}
      </div>
    </section>

    <section id="hardware" class="hardware-section" aria-labelledby="hardware-title">
      <div class="section-heading">
        <span>Hardware</span>
        <h2 id="hardware-title">The main parts and what they add.</h2>
        <p>Real hobby components, chosen to make the rover easy to build, test and repair.</p>
      </div>

      <div class="hardware-grid">
        {#each hardware as part}
          <article class="hardware-card">
            <div class="component-image">
              <img src={part.image} alt={part.name} loading="lazy" decoding="async" width="600" height="600" />
              <span>{part.tag}</span>
            </div>
            <div class="component-copy">
              <h3>{part.name}</h3>
              <p>{part.copy}</p>
            </div>
          </article>
        {/each}
      </div>

    </section>

    <section class="flowchart-section">
      <div class="section-heading">
        <span>Architecture</span>
        <h2>Dual-ESP32 Responsibility Split</h2>
        <p>DeepTrack physically isolates mission-critical safety from high-level interfaces using two distinct microcontrollers linked wirelessly.</p>
      </div>

      <div class="track-container">
        <div class="track-node">
          <div class="node-icon"><span class="material-symbols-rounded">memory</span></div>
          <div class="node-content">
            <h4>1. Rover ESP32</h4>
            <p>Runs the rover's core hardware loop. Reads all physical sensors, enforces local collision protection, actuates the four-wheel chassis, and broadcasts telemetry.</p>
          </div>
        </div>

        <div class="track-link">
          <span class="material-symbols-rounded">wifi</span>
          <small>Wireless ESP-NOW Data Link</small>
        </div>

        <div class="track-node">
          <div class="node-icon"><span class="material-symbols-rounded">router</span></div>
          <div class="node-content">
            <h4>2. Gateway ESP32</h4>
            <p>Stays connected to the laptop. Listens for rover packets, maintains a strict heartbeat watchdog, and bridges the data as a USB NDJSON stream.</p>
          </div>
        </div>
        
        <div class="track-link">
          <span class="material-symbols-rounded">usb</span>
          <small>USB Serial Connection</small>
        </div>

        <div class="track-node">
          <div class="node-icon"><span class="material-symbols-rounded">laptop_mac</span></div>
          <div class="node-content">
            <h4>3. Operator Dashboard</h4>
            <p>The web interface on your computer. Parses the NDJSON stream to provide a live UI, estimated pose mapping, and sends remote drive commands back down the chain.</p>
          </div>
        </div>
      </div>
    </section>
  </main>

  <Footer />
</div>

<style>
  .landing-hero,
  .feature-section,
  .hardware-section {
    width: min(100% - 2rem, 80rem);
    margin-inline: auto;
  }

  .landing-hero {
    min-height: calc(100vh - 4rem);
    display: grid;
    grid-template-columns: minmax(0, 0.88fr) minmax(28rem, 1.12fr);
    align-items: center;
    gap: clamp(3rem, 6vw, 6.5rem);
    padding-block: clamp(4rem, 8vw, 7rem);
  }

  h1 {
    max-width: 12ch;
    margin-top: 1.5rem;
    font-size: clamp(2.9rem, 6vw, 5.8rem);
    font-weight: 650;
    letter-spacing: -0.055em;
    line-height: 0.98;
  }

  .hero-copy > p {
    max-width: 38rem;
    margin-top: 1.75rem;
    color: var(--md-sys-color-on-surface-variant);
    font-size: clamp(1rem, 1.5vw, 1.18rem);
    line-height: 1.7;
  }

  .hero-actions {
    display: flex;
    flex-wrap: wrap;
    gap: 0.85rem;
    margin-top: 2.25rem;
  }

  .rover-showcase { min-width: 0; }

  .rover-stage {
    position: relative;
    width: 100%;
    aspect-ratio: 1 / 0.88;
    overflow: hidden;
    border: 1px solid var(--md-sys-color-outline-variant);
    border-radius: 2rem;
    background: radial-gradient(circle at 60% 34%, color-mix(in srgb, var(--md-sys-color-primary) 15%, transparent), transparent 42%), var(--md-sys-color-surface-container-low);
    color: var(--md-sys-color-on-surface);
    cursor: pointer;
    isolation: isolate;
    box-shadow: 0 2rem 5rem color-mix(in srgb, var(--md-sys-color-on-surface) 10%, transparent);
  }

  .rover-stage:focus-visible {
    outline: 3px solid var(--md-sys-color-primary);
    outline-offset: 4px;
  }

  .stage-grid {
    position: absolute;
    inset: 0;
    z-index: -1;
    opacity: 0.28;
    background-image: linear-gradient(var(--md-sys-color-outline-variant) 1px, transparent 1px), linear-gradient(90deg, var(--md-sys-color-outline-variant) 1px, transparent 1px);
    background-size: 2.5rem 2.5rem;
    mask-image: linear-gradient(to bottom, black, transparent 78%);
  }

  .rover-stage img {
    position: absolute;
    inset: 5% 2% 14%;
    width: 96%;
    height: 81%;
    object-fit: contain;
    filter: drop-shadow(0 1.6rem 1.5rem rgb(0 0 0 / 0.25));
    transition: transform 500ms ease;
  }

  .rover-stage:hover img { transform: scale(1.025) translateY(-0.2rem); }


  .next-view {
    position: absolute;
    top: 1.35rem;
    right: 1.35rem;
    display: inline-flex;
    align-items: center;
    gap: 0.35rem;
    padding: 0.55rem 0.75rem;
    border: 1px solid var(--md-sys-color-outline-variant);
    border-radius: 999px;
    background: color-mix(in srgb, var(--md-sys-color-surface-container-lowest) 82%, transparent);
    color: var(--md-sys-color-on-surface-variant);
    font-size: 0.7rem;
    font-weight: 650;
    backdrop-filter: blur(12px);
  }

  .next-view .material-symbols-rounded { font-size: 1rem; }

  .carousel-dots {
    display: flex;
    justify-content: center;
    gap: 0.5rem;
    margin-top: 1.2rem;
  }

  .carousel-dots button {
    width: 0.55rem;
    height: 0.55rem;
    border-radius: 999px;
    background: var(--md-sys-color-outline-variant);
    transition: width 240ms ease, background 240ms ease;
  }

  .carousel-dots button.active {
    width: 2rem;
    background: var(--md-sys-color-primary);
  }

  .feature-section,
  .hardware-section { padding-block: clamp(5rem, 9vw, 8rem); }

  .feature-section { border-top: 1px solid var(--md-sys-color-outline-variant); }

  .section-heading {
    max-width: 42rem;
    margin: 0 auto clamp(2.5rem, 5vw, 4rem);
    text-align: center;
  }

  .section-heading--left {
    margin-left: 0;
    text-align: left;
  }

  .section-heading > span,
  .card-eyebrow {
    color: var(--md-sys-color-primary);
    font-family: var(--font-mono);
    font-size: 0.72rem;
    font-weight: 700;
    letter-spacing: 0.12em;
    text-transform: uppercase;
  }

  .section-heading h2 {
    margin-top: 0.8rem;
    font-size: clamp(2rem, 4vw, 3.6rem);
    font-weight: 620;
    letter-spacing: -0.045em;
    line-height: 1.05;
  }

  .section-heading > p {
    margin-top: 1rem;
    color: var(--md-sys-color-on-surface-variant);
    line-height: 1.65;
  }

  .feature-grid {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    border-block: 1px solid var(--md-sys-color-outline-variant);
  }

  .feature-card {
    position: relative;
    min-height: 22rem;
    padding: 2rem 1.6rem;
    border-right: 1px solid var(--md-sys-color-outline-variant);
  }

  .feature-card:last-child { border-right: 0; }

  .feature-number {
    color: var(--md-sys-color-outline);
    font-family: var(--font-mono);
    font-size: 0.68rem;
  }

  .feature-icon {
    display: grid;
    width: 3.5rem;
    height: 3.5rem;
    margin: 3.4rem 0 2rem;
    place-items: center;
    border-radius: 1rem;
    background: var(--md-sys-color-primary-container);
    color: var(--md-sys-color-on-primary-container);
  }

  .feature-card h3,
  .hardware-card h3 {
    margin-top: 0.45rem;
    font-size: 1.12rem;
    font-weight: 650;
    letter-spacing: -0.02em;
  }

  .card-copy,
  .hardware-card p {
    margin-top: 0.8rem;
    color: var(--md-sys-color-on-surface-variant);
    font-size: 0.86rem;
    line-height: 1.65;
  }

  .hardware-section {
    width: 100%;
    max-width: none;
    padding-inline: max(1rem, calc((100% - 80rem) / 2));
    background: var(--md-sys-color-surface-container-low);
  }

  .hardware-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 1rem;
  }

  .hardware-card {
    overflow: hidden;
    border: 1px solid var(--md-sys-color-outline-variant);
    border-radius: 1.25rem;
    background: var(--md-sys-color-surface-container-lowest);
    transition: transform 220ms ease, border-color 220ms ease;
  }

  .hardware-card:hover {
    transform: translateY(-0.25rem);
    border-color: var(--md-sys-color-outline);
  }

  .component-image {
    position: relative;
    height: 14rem;
    overflow: hidden;
    background: #e8edee;
  }

  .component-image img {
    width: 100%;
    height: 100%;
    object-fit: cover;
    mix-blend-mode: multiply;
    transition: transform 400ms ease;
  }

  :global(:root[data-theme='dark']) .component-image img {
    mix-blend-mode: normal;
    opacity: 0.88;
  }

  .hardware-card:hover .component-image img { transform: scale(1.035); }

  .component-image span {
    position: absolute;
    top: 0.9rem;
    left: 0.9rem;
    padding: 0.35rem 0.6rem;
    border-radius: 999px;
    background: rgb(0 0 0 / 0.72);
    color: white;
    font-family: var(--font-mono);
    font-size: 0.65rem;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .component-copy {
    min-height: 9.5rem;
    padding: 1.35rem;
  }

  @media (max-width: 1024px) {
    .landing-hero {
      grid-template-columns: 1fr;
      min-height: auto;
      gap: 3.5rem;
    }

    h1 { max-width: 14ch; }

    .rover-showcase {
      width: min(100%, 44rem);
      margin-inline: auto;
    }

    .feature-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    .feature-card:nth-child(2) { border-right: 0; }
    .feature-card:nth-child(-n + 2) { border-bottom: 1px solid var(--md-sys-color-outline-variant); }
  }

  @media (max-width: 760px) {
    .landing-hero,
    .feature-section { width: min(100% - 1.5rem, 80rem); }
    .landing-hero { padding-block: 3.5rem 5rem; }
    h1 { font-size: clamp(2.75rem, 13vw, 4.2rem); }

    .rover-stage {
      aspect-ratio: 0.9;
      border-radius: 1.5rem;
    }

    .next-view {
      top: 0.8rem;
      right: 0.8rem;
    }



    .feature-grid,
    .hardware-grid { grid-template-columns: 1fr; }

    .feature-card {
      min-height: auto;
      padding: 1.6rem 1rem 2rem;
      border-right: 0;
      border-bottom: 1px solid var(--md-sys-color-outline-variant);
    }

    .feature-card:last-child { border-bottom: 0; }
    .feature-icon { margin-block: 2rem 1.5rem; }
    .component-image { height: 13rem; }
  }

  @media (prefers-reduced-motion: no-preference) {
    .hero-copy { animation: enter-copy 700ms cubic-bezier(0.2, 0, 0, 1) backwards; }
    .hero-visual { animation: enter-visual 850ms 120ms cubic-bezier(0.2, 0, 0, 1) backwards; }

    @keyframes enter-copy {
      from { opacity: 0; transform: translateY(1.5rem); }
    }

    @keyframes enter-visual {
      from { opacity: 0; transform: translateY(1.5rem) scale(0.98); }
    }
  }

  .flowchart-section {
    padding-block: clamp(5rem, 9vw, 8rem);
    background: var(--md-sys-color-surface);
    border-top: 1px solid var(--md-sys-color-outline-variant);
  }

  .flowchart-section .section-heading {
    text-align: center;
  }

  .flowchart-section .section-heading span {
    color: var(--md-sys-color-primary);
    font-size: 0.65rem;
    font-weight: 800;
    letter-spacing: 0.15em;
    text-transform: uppercase;
  }

  .flowchart-section .section-heading h2 {
    color: var(--md-sys-color-on-surface);
    font-size: clamp(2rem, 4vw, 3.2rem);
    font-weight: 700;
    margin-top: 0.5rem;
    line-height: 1.1;
  }

  .flowchart-section .section-heading p {
    color: var(--md-sys-color-on-surface-variant);
    margin-top: 1rem;
    font-size: 0.9rem;
    max-width: 36rem;
    margin-inline: auto;
  }

  .track-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    max-width: 48rem;
    margin: 4rem auto 0;
    padding-inline: 1.5rem;
  }

  .track-node {
    display: flex;
    align-items: center;
    gap: 2rem;
    width: 100%;
    padding: 2.5rem;
    background: var(--md-sys-color-surface-container-lowest);
    border: 1px solid var(--md-sys-color-outline-variant);
    border-radius: 1.5rem;
    box-shadow: 0 8px 24px color-mix(in srgb, var(--md-sys-color-on-surface) 4%, transparent);
    position: relative;
    z-index: 2;
  }

  .track-node:nth-child(even) {
    flex-direction: row-reverse;
    text-align: right;
  }

  .node-icon {
    display: grid;
    width: 5rem;
    height: 5rem;
    flex-shrink: 0;
    place-items: center;
    border-radius: 1.25rem;
    background: var(--md-sys-color-primary-container);
    color: var(--md-sys-color-on-primary-container);
  }

  .node-icon .material-symbols-rounded {
    font-size: 2.2rem;
  }

  .node-content h4 {
    font-size: 1.25rem;
    font-weight: 700;
    color: var(--md-sys-color-on-surface);
    margin-bottom: 0.75rem;
  }

  .node-content p {
    font-size: 0.95rem;
    color: var(--md-sys-color-on-surface-variant);
    line-height: 1.6;
    margin: 0;
  }

  .track-link {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    height: 6rem;
    gap: 0.5rem;
    color: var(--md-sys-color-primary);
    position: relative;
    z-index: 1;
  }

  .track-link::before {
    content: '';
    position: absolute;
    top: 0;
    bottom: 0;
    left: 50%;
    width: 2px;
    transform: translateX(-50%);
    z-index: -1;
    border-left: 2px dashed var(--md-sys-color-primary);
    opacity: 0.4;
  }

  .track-link span {
    background: var(--md-sys-color-surface);
    padding: 0.5rem;
    border-radius: 50%;
    font-size: 1.5rem;
    box-sizing: content-box;
  }
  
  .track-link small {
    background: var(--md-sys-color-surface);
    padding: 0.2rem 0.8rem;
    border-radius: 99px;
    font-weight: 600;
    font-size: 0.75rem;
    letter-spacing: 0.05em;
  }
  
  @media (max-width: 760px) {
    .track-node, .track-node:nth-child(even) {
      flex-direction: column;
      text-align: center;
      gap: 1.5rem;
      padding: 2rem 1.5rem;
    }
  }
</style>
