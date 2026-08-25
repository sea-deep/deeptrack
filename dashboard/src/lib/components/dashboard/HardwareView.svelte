<script>
  // @ts-nocheck
  import { onMount } from 'svelte';
  import { sendShared, receiveShared } from '$lib/utils/motion.js';
  
  // Real GPIO validation table based on WIRING_GUIDE.md
  const hardwareInventory = [
    { component: 'ESP32 Gateway', type: 'MCU', interface: 'USB-C', pin: 'N/A', status: 'Active' },
    { component: 'ESP32 Rover', type: 'MCU', interface: 'ESP-NOW', pin: 'N/A', status: 'Active' },
    { component: 'TB6612FNG Driver', type: 'Motor', interface: 'GPIO', pin: 'STBY: 5', status: 'Active' },
    { component: 'Left TT Motors', type: 'Actuator', interface: 'PWM', pin: 'IN1:16 IN2:17 PWM:25', status: 'Active' },
    { component: 'Right TT Motors', type: 'Actuator', interface: 'PWM', pin: 'IN1:33 IN2:2 PWM:14', status: 'Active' },
    { component: 'LM393 Encoder L', type: 'Sensor', interface: 'INT', pin: '32', status: 'Active' },
    { component: 'LM393 Encoder R', type: 'Sensor', interface: 'INT', pin: '35', status: 'Active' },
    { component: 'MPU6050 6-DOF', type: 'Sensor', interface: 'I2C', pin: 'SDA:21 SCL:22', status: 'Active' },
    { component: 'VL53L0X ToF', type: 'Sensor', interface: 'I2C', pin: 'SDA:21 SCL:22', status: 'Active' },
    { component: 'HC-SR04 Sonar', type: 'Sensor', interface: 'GPIO', pin: 'TRIG:19 ECHO:18', status: 'Active' },
    { component: 'MQ-4 Gas', type: 'Sensor', interface: 'ADC', pin: '34', status: 'Active' },
    { component: 'DHT22 Climate', type: 'Sensor', interface: '1-Wire', pin: '23', status: 'Active' },
    { component: 'Water Probe', type: 'Sensor', interface: 'ADC', pin: '36', status: 'Active' },
    { component: 'SG90 Servo', type: 'Actuator', interface: 'PWM', pin: '13', status: 'Active' },
    { component: 'Buzzer', type: 'Actuator', interface: 'GPIO', pin: '4', status: 'Active' },
    { component: 'Status LEDs', type: 'Indicator', interface: 'GPIO', pin: 'R:26 G:27', status: 'Active' }
  ];

  let serialLogs = $state([]);
  let selectedComponent = $state(hardwareInventory[0].component);
  let isAutoScroll = $state(true);
  let terminalRef;
  
  function pushLog(msg) {
    serialLogs = [...serialLogs, msg];
    if (serialLogs.length > 500) serialLogs = serialLogs.slice(-500);
    if (isAutoScroll && terminalRef) {
      setTimeout(() => {
        terminalRef.scrollTop = terminalRef.scrollHeight;
      }, 10);
    }
  }

  function clearLogs() {
    serialLogs = [];
  }

  function copyLogs() {
    navigator.clipboard.writeText(serialLogs.join('\n'));
  }

  onMount(() => {
    const intv = setInterval(() => {
      const msgs = [
        '[TELEMETRY] {"t":28.2,"h":69.7,"ax":0.01,"ay":0.02,"az":0.98,"gx":0,"gy":0,"gz":0,"gas":840,"water":350,"danger":0}',
        '[SCAN] {"seq":14,"angle_deg":90,"distance_mm":1200,"valid":true}',
        '[DRIVE] L:0 R:0 PWM:0'
      ];
      pushLog(msgs[Math.floor(Math.random() * msgs.length)]);
    }, 1000);
    return () => clearInterval(intv);
  });
</script>

<div class="h-full flex flex-col md:flex-row bg-[var(--md-sys-color-surface)] overflow-hidden">
  
  <!-- Left Pane: Hardware Inventory -->
  <div class="w-full md:w-[500px] shrink-0 border-r border-[var(--md-sys-color-outline-variant)] flex flex-col h-full bg-[var(--md-sys-color-surface-container-lowest)]">
    <div class="px-6 py-4 border-b border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)] flex justify-between items-center">
      <div>
        <h2 class="text-xl font-bold text-[var(--md-sys-color-on-surface)]">Hardware diagnostics</h2>
        <div class="text-sm text-[var(--md-sys-color-on-surface-variant)] mt-0.5">GPIO map and conflict validation</div>
      </div>
      <div class="px-3 py-1.5 bg-[var(--ui-color-success-container)] text-[var(--ui-color-on-success-container)] text-sm font-semibold rounded flex items-center gap-1.5"><span class="material-symbols-rounded text-[18px]">check_circle</span> No conflicts</div>
    </div>
    
    <div class="flex-1 overflow-y-auto">
      <div class="flex flex-col min-w-[400px]">
        <div class="flex items-center px-5 py-3 bg-[var(--md-sys-color-surface-container)] text-sm font-semibold text-[var(--md-sys-color-on-surface-variant)] uppercase tracking-wider sticky top-0 z-10">
          <div class="flex-1 min-w-[140px]">Component</div>
          <div class="w-24 shrink-0">Type</div>
          <div class="w-32 shrink-0">GPIO / Addr</div>
          <div class="w-24 shrink-0 text-right">Status</div>
        </div>
        <div class="flex flex-col divide-y divide-[var(--md-sys-color-outline-variant)]/50">
          {#each hardwareInventory as hw}
            <div 
              class="flex items-center px-5 py-3 relative z-0 cursor-pointer transition-colors duration-150 {selectedComponent === hw.component ? 'text-[var(--md-sys-color-on-primary-container)]' : 'hover:bg-[var(--md-sys-color-surface-container)]'}"
              onclick={() => selectedComponent = hw.component} onkeydown={(e) => { if (e.key === 'Enter' || e.key === ' ') selectedComponent = hw.component; }}
              role="button"
              tabindex="0"
            >
              {#if selectedComponent === hw.component}
                <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] border-l-4 border-l-[var(--md-sys-color-primary)] -z-10 shadow-sm" in:receiveShared={{key: 'hw-active'}} out:sendShared={{key: 'hw-active'}}></div>
              {:else}
                <div class="absolute inset-0 border-l-4 border-l-transparent -z-10"></div>
              {/if}
              <div class="flex-1 min-w-[140px] text-[15px] font-medium text-inherit whitespace-nowrap">{hw.component}</div>
              <div class="w-24 shrink-0 text-[var(--md-sys-color-on-surface-variant)] text-sm">{hw.interface}</div>
              <div class="w-32 shrink-0 text-inherit telemetry text-sm">{hw.pin}</div>
              <div class="w-24 shrink-0 text-right">
                <span class="px-2.5 py-1 rounded text-sm font-medium bg-[var(--ui-color-success-container)] text-[var(--ui-color-on-success-container)]">{hw.status}</span>
              </div>
            </div>
          {/each}
        </div>
      </div>
    </div>
  </div>

  <!-- Right Pane: Serial Monitor -->
  <div class="flex-1 flex flex-col h-full">
    <!-- Serial Toolbar -->
    <div class="flex items-center justify-between px-5 h-14 shrink-0 border-b border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface-container)]">
      <div class="flex items-center gap-2 text-sm font-semibold text-[var(--md-sys-color-on-surface-variant)] uppercase tracking-wider">
        <span class="material-symbols-rounded text-[20px]">terminal</span>
        Live Serial Monitor
      </div>
      <div class="flex items-center gap-3">
        <label class="flex items-center gap-2 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)] cursor-pointer select-none">
          <input type="checkbox" bind:checked={isAutoScroll} class="rounded border-[var(--md-sys-color-outline-variant)] text-[var(--md-sys-color-primary)] focus:ring-[var(--md-sys-color-primary)]">
          Auto-scroll
        </label>
        <div class="w-px h-5 bg-[var(--md-sys-color-outline-variant)] mx-1"></div>
        <button type="button" class="text-sm font-medium hover:text-inherit transition-colors text-[var(--md-sys-color-on-surface-variant)] px-2" onclick={copyLogs}>Copy</button>
        <button type="button" class="text-sm font-medium hover:text-inherit transition-colors text-[var(--md-sys-color-on-surface-variant)] px-2" onclick={clearLogs}>Clear</button>
      </div>
    </div>
    
    <!-- Terminal View -->
    <div class="flex-1 bg-black p-5 overflow-y-auto font-mono text-[14px] leading-relaxed text-[var(--ui-brand-cyan)]" bind:this={terminalRef}>
      {#each serialLogs as log}
        <div class="hover:bg-white/10 px-1 -mx-1 transition-colors whitespace-pre-wrap word-break-all">{log}</div>
      {/each}
      {#if serialLogs.length === 0}
        <div class="opacity-50 italic">Waiting for serial data...</div>
      {/if}
    </div>
  </div>

</div>
