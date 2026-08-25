<script>
  let { size = 32, class: className = '' } = $props();
</script>

<img
  src="/favicon.svg"
  alt="DeepTrack Rover"
  width={size}
  height={size}
  draggable="false"
  class="select-none pointer-events-none object-contain rounded-xl shrink-0 {className}"
  style="width: {size}px; height: {size}px; user-select: none; -webkit-user-drag: none; -moz-user-select: none;"
/>
