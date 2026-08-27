<script>
  let { size = 32, class: className = '' } = $props();
</script>

<span
  class="inline-flex select-none pointer-events-none rounded-xl shrink-0 {className}"
  style="width: {size}px; height: {size}px"
>
  <svg
    viewBox="0 0 64 64"
    role="img"
    aria-label="DeepTrack Rover"
    width={size}
    height={size}
    class="block w-full h-full rounded-xl"
  >
    <rect width="64" height="64" rx="16" fill="#d1e4ff" />
    <rect x="10" y="38" width="16" height="16" rx="5" fill="#001d36" stroke="#006b5f" stroke-width="2.5" />
    <path d="M10 46h16M38 46h16" stroke="#59dbc7" stroke-width="2" />
    <rect x="38" y="38" width="16" height="16" rx="5" fill="#001d36" stroke="#006b5f" stroke-width="2.5" />
    <path d="M16 26l7-8h18l7 8-3 16H19z" fill="#0061a4" stroke="#fff" stroke-width="2.5" stroke-linejoin="round" />
    <circle cx="26" cy="31" r="3.5" fill="#59dbc7" />
    <circle cx="38" cy="31" r="3.5" fill="#59dbc7" />
    <rect x="28" y="35" width="8" height="2" rx="1" fill="#fff" />
    <rect x="29" y="14" width="6" height="4" rx="1" fill="#001d36" />
    <circle cx="32" cy="12" r="5" fill="#ba1a1a" stroke="#fff" stroke-width="1.5" />
    <path d="M36 10l12-5" stroke="#ba1a1a" stroke-width="2.5" stroke-linecap="round" stroke-dasharray="2 3" />
  </svg>
</span>
