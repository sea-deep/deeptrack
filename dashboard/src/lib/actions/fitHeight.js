/**
 * Scale a complete panel to its available height without adding an internal
 * scrollbar. The first child remains the semantic interactive content.
 * @param {HTMLElement} node
 * @param {unknown} [_trigger]
 */
export function fitHeight(node, _trigger) {
  /** @type {HTMLElement | null} */
  const content = node.firstElementChild instanceof HTMLElement
    ? node.firstElementChild : null;
  let frame = 0;

  const measure = () => {
    cancelAnimationFrame(frame);
    frame = requestAnimationFrame(() => {
      if (!content || !node.isConnected) return;

      content.style.transform = 'none';
      content.style.width = '100%';
      const availableHeight = node.clientHeight;
      node.dataset.fitDensity = 'comfortable';
      let naturalHeight = content.scrollHeight;

      // Preserve normal typography whenever possible. If the panel is too
      // tall, first tighten whitespace and control geometry through CSS; only
      // then scale the complete panel as a last resort for short viewports.
      if (availableHeight > 0 && naturalHeight > availableHeight) {
        node.dataset.fitDensity = 'compact';
        naturalHeight = content.scrollHeight;
      }
      const scale = availableHeight > 0 && naturalHeight > availableHeight
        ? Math.max(0.5, availableHeight / naturalHeight)
        : 1;

      node.dataset.fitScale = scale.toFixed(3);
      // The inverse width keeps the transformed visual width flush with the
      // pane instead of leaving an empty strip beside scaled content.
      content.style.width = scale < 1 ? `${100 / scale}%` : '100%';
      content.style.transform = scale < 1 ? `scale(${scale})` : 'none';
      content.style.willChange = scale < 1 ? 'transform' : 'auto';
    });
  };

  /** @type {ResizeObserver | null} */
  const resizeObserver = typeof ResizeObserver === 'undefined'
    ? null
    : new ResizeObserver(measure);
  resizeObserver?.observe(node);
  if (!resizeObserver && typeof window !== 'undefined')
    window.addEventListener('resize', measure);
  measure();

  return {
    update(/** @type {unknown} */ _nextTrigger) { measure(); },
    destroy() {
      cancelAnimationFrame(frame);
      resizeObserver?.disconnect();
      if (!resizeObserver && typeof window !== 'undefined')
        window.removeEventListener('resize', measure);
      if (content) content.style.willChange = 'auto';
    }
  };
}
