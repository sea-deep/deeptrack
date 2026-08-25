/**
 * Material 3 Expressive Motion System for DeepTrack
 * Source of Truth: docs/ANIMATION.md
 * 
 * Includes verified first-party spring tokens, exact analytical spring solver,
 * standard/emphasized easing curves, and GPU-accelerated Svelte transition primitives.
 */

import { cubicOut } from 'svelte/easing';
import { crossfade } from 'svelte/transition';

/* ==========================================================================
   1. Verified Spring Tokens (ANIMATION.md Section 4 & 5)
   ========================================================================== */

/**
 * Expressive Spring Scheme
 */
export const SPRING_EXPRESSIVE_FAST_SPATIAL = { dampingRatio: 0.6, stiffness: 800 };
export const SPRING_EXPRESSIVE_FAST_EFFECTS = { dampingRatio: 1.0, stiffness: 3800 };
export const SPRING_EXPRESSIVE_DEFAULT_SPATIAL = { dampingRatio: 0.8, stiffness: 380 };
export const SPRING_EXPRESSIVE_DEFAULT_EFFECTS = { dampingRatio: 1.0, stiffness: 1600 };
export const SPRING_EXPRESSIVE_SLOW_SPATIAL = { dampingRatio: 0.8, stiffness: 200 };
export const SPRING_EXPRESSIVE_SLOW_EFFECTS = { dampingRatio: 1.0, stiffness: 800 };

/**
 * Standard Spring Scheme (For dense or repetitive interactions)
 */
export const SPRING_STANDARD_FAST_SPATIAL = { dampingRatio: 0.9, stiffness: 1400 };
export const SPRING_STANDARD_DEFAULT_SPATIAL = { dampingRatio: 0.9, stiffness: 700 };
export const SPRING_STANDARD_SLOW_SPATIAL = { dampingRatio: 0.9, stiffness: 300 };

/* ==========================================================================
   2. Verified Legacy / Fallback Duration Tokens (ANIMATION.md Section 9)
   ========================================================================== */

export const DUR_SHORT_1 = 50;
export const DUR_SHORT_2 = 100;
export const DUR_SHORT_3 = 150;
export const DUR_SHORT_4 = 200;

export const DUR_MEDIUM_1 = 250;
export const DUR_MEDIUM_2 = 300;
export const DUR_MEDIUM_3 = 350;
export const DUR_MEDIUM_4 = 400;

export const DUR_LONG_1 = 450;
export const DUR_LONG_2 = 500;
export const DUR_LONG_3 = 550;
export const DUR_LONG_4 = 600;

// Aliases for compatibility
export const DUR_SHORT = DUR_SHORT_3;
export const DUR_MEDIUM = DUR_MEDIUM_1;
export const DUR_LONG = DUR_MEDIUM_3;

/* ==========================================================================
   3. Verified First-Party Easing Curves (ANIMATION.md Section 10)
   ========================================================================== */

export const EASING_STANDARD = 'cubic-bezier(0.2, 0, 0, 1)';
export const EASING_STANDARD_DECELERATE = 'cubic-bezier(0, 0, 0, 1)';
export const EASING_STANDARD_ACCELERATE = 'cubic-bezier(0.3, 0, 1, 1)';
export const EASING_EMPHASIZED_DECELERATE = 'cubic-bezier(0.1, 0.7, 0.1, 1)';
export const EASING_EMPHASIZED_ACCELERATE = 'cubic-bezier(0.3, 0, 0.8, 0.2)';

/* ==========================================================================
   4. Accessibility & Reduced Motion (ANIMATION.md Section 60)
   ========================================================================== */

/**
 * Checks if the user prefers reduced motion
 * @returns {boolean}
 */
export function isReducedMotion() {
  if (typeof window === 'undefined') return false;
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches;
}

/* ==========================================================================
   5. Analytical Physics Spring Solver (ANIMATION.md Section 49)
   ========================================================================== */

/**
 * Steps a spring forward by deltaSeconds analytically.
 * @param {{ value: number, velocity: number }} state
 * @param {{ target: number, stiffness: number, dampingRatio: number }} config
 * @param {number} deltaSeconds
 * @returns {{ value: number, velocity: number }}
 */
export function stepSpring(state, config, deltaSeconds) {
  const displacement = state.value - config.target;
  const velocity = state.velocity;
  const omega = Math.sqrt(config.stiffness);
  const zeta = config.dampingRatio;

  if (deltaSeconds <= 0) {
    return state;
  }

  // Underdamped (ζ < 1)
  if (zeta < 1) {
    const dampedOmega = omega * Math.sqrt(1 - zeta * zeta);
    const decay = Math.exp(-zeta * omega * deltaSeconds);
    const cosine = Math.cos(dampedOmega * deltaSeconds);
    const sine = Math.sin(dampedOmega * deltaSeconds);

    const sineCoefficient = (velocity + zeta * omega * displacement) / dampedOmega;
    const positionTerm = displacement * cosine + sineCoefficient * sine;
    const derivativeTerm = -displacement * dampedOmega * sine + sineCoefficient * dampedOmega * cosine;

    return {
      value: config.target + decay * positionTerm,
      velocity: decay * (derivativeTerm - zeta * omega * positionTerm)
    };
  }

  // Critically damped (ζ = 1)
  if (zeta === 1) {
    const decay = Math.exp(-omega * deltaSeconds);
    const positionTerm = displacement + (velocity + omega * displacement) * deltaSeconds;
    const velocityTerm = velocity * (1 - omega * deltaSeconds) - displacement * (omega * omega * deltaSeconds);

    return {
      value: config.target + decay * positionTerm,
      velocity: decay * velocityTerm
    };
  }

  // Overdamped (ζ > 1)
  const root = Math.sqrt(zeta * zeta - 1);
  const rootA = -omega * (zeta - root);
  const rootB = -omega * (zeta + root);
  const coefficientA = (velocity - rootB * displacement) / (rootA - rootB);
  const coefficientB = displacement - coefficientA;
  const termA = coefficientA * Math.exp(rootA * deltaSeconds);
  const termB = coefficientB * Math.exp(rootB * deltaSeconds);

  return {
    value: config.target + termA + termB,
    velocity: rootA * termA + rootB * termB
  };
}

/**
 * Creates an interactive spring animation controller (ANIMATION.md Section 49)
 * @param {number} initialValue
 * @param {{ stiffness: number, dampingRatio: number }} token
 * @param {(value: number) => void} onUpdate
 * @param {{ restDelta?: number, restVelocity?: number }} [options]
 */
export function createSpringController(initialValue, token, onUpdate, options = {}) {
  const restDelta = options.restDelta ?? 0.01;
  const restVelocity = options.restVelocity ?? 0.05;

  let state = { value: initialValue, velocity: 0 };
  let target = initialValue;
  /** @type {number} */
  let frame = 0;
  /** @type {number | null} */
  let previousTimestamp = null;

  /**
   * @param {number} timestamp
   */
  const tick = (timestamp) => {
    const deltaSeconds = previousTimestamp === null
      ? 0
      : Math.min((timestamp - previousTimestamp) / 1000, 0.064);

    previousTimestamp = timestamp;

    state = stepSpring(
      state,
      {
        target,
        stiffness: token.stiffness,
        dampingRatio: token.dampingRatio
      },
      deltaSeconds
    );

    onUpdate(state.value);

    if (
      Math.abs(state.value - target) <= restDelta &&
      Math.abs(state.velocity) <= restVelocity
    ) {
      state = { value: target, velocity: 0 };
      onUpdate(target);
      frame = 0;
      previousTimestamp = null;
      return;
    }

    frame = requestAnimationFrame(tick);
  };

  return {
    /** @param {number} nextTarget */
    setTarget(nextTarget) {
      if (isReducedMotion()) {
        state = { value: nextTarget, velocity: 0 };
        target = nextTarget;
        onUpdate(nextTarget);
        return;
      }
      target = nextTarget;
      if (!frame) {
        previousTimestamp = null;
        frame = requestAnimationFrame(tick);
      }
    },

    /** @param {number} value */
    snap(value) {
      if (frame) cancelAnimationFrame(frame);
      frame = 0;
      previousTimestamp = null;
      target = value;
      state = { value, velocity: 0 };
      onUpdate(value);
    },

    dispose() {
      if (frame) cancelAnimationFrame(frame);
      frame = 0;
      previousTimestamp = null;
    }
  };
}

/* ==========================================================================
   6. Shared-Element Crossfade (ANIMATION.md Section 17 & 24)
   ========================================================================== */

export const [sendShared, receiveShared] = crossfade({
  duration: (/** @type {number} */ d) => (isReducedMotion() ? 0 : Math.min(260, Math.sqrt(d * 180))),
  easing: cubicOut,
  fallback(/** @type {Element} */ node, /** @type {any} */ params) {
    if (isReducedMotion()) {
      return { duration: 0 };
    }
    const style = getComputedStyle(node);
    const transform = style.transform === 'none' ? '' : style.transform;
    const opacity = +style.opacity;

    return {
      duration: params?.duration || DUR_SHORT_4,
      easing: cubicOut,
      css: (/** @type {number} */ t) => `
        transform: ${transform} scale(${0.97 + 0.03 * t});
        opacity: ${t * opacity};
      `
    };
  }
});

/* ==========================================================================
   7. Top-Level Destination Transition: Fade Through (ANIMATION.md Section 14 & 52.3)
   ========================================================================== */

/**
 * Top-Level Destination Fade Through (Non-directional, scale 0.985 -> 1.0, translateY 16px -> 0)
 * @param {HTMLElement} node
 * @param {{ delay?: number, duration?: number }} [params]
 */
export function m3TopLevelFadeThrough(node, params = {}) {
  if (isReducedMotion()) {
    return { duration: 0 };
  }
  const delay = params.delay || 0;
  const duration = params.duration || DUR_MEDIUM_2;

  const style = getComputedStyle(node);
  const transform = style.transform === 'none' ? '' : style.transform;
  const opacity = +style.opacity;

  return {
    delay,
    duration,
    easing: cubicOut,
    css: (/** @type {number} */ t) => `
      transform: ${transform} translateY(${(1 - t) * 16}px) scale(${0.985 + 0.015 * t});
      opacity: ${t * opacity};
      will-change: transform, opacity;
    `
  };
}

/* ==========================================================================
   8. Dialog Presentation: Scale + Fade (ANIMATION.md Section 19 & 54)
   ========================================================================== */

/**
 * Dialog Scale + Translation Transition (translateY 16px -> 0, scale 0.96 -> 1.0)
 * @param {HTMLElement} node
 * @param {{ delay?: number, duration?: number, startScale?: number }} [params]
 */
export function m3DialogScaleFade(node, params = {}) {
  if (isReducedMotion()) {
    return { duration: 0 };
  }
  const delay = params.delay || 0;
  const duration = params.duration || DUR_MEDIUM_1;
  const startScale = params.startScale || 0.96;

  const style = getComputedStyle(node);
  const transform = style.transform === 'none' ? '' : style.transform;
  const opacity = +style.opacity;

  return {
    delay,
    duration,
    easing: cubicOut,
    css: (/** @type {number} */ t) => `
      transform: ${transform} translateY(${(1 - t) * 16}px) scale(${startScale + (1 - startScale) * t});
      opacity: ${t * opacity};
      will-change: transform, opacity;
    `
  };
}

// Alias for backwards compatibility
export const m3ScaleFade = m3DialogScaleFade;

/* ==========================================================================
   9. Direction-Aware Slide + Fade (ANIMATION.md Section 18)
   ========================================================================== */

/**
 * @param {HTMLElement} node
 * @param {{ delay?: number, duration?: number, x?: number, y?: number }} [params]
 */
export function m3SlideFade(node, params = {}) {
  if (isReducedMotion()) {
    return { duration: 0 };
  }
  const delay = params.delay || 0;
  const duration = params.duration || DUR_MEDIUM_1;
  const x = params.x || 0;
  const y = params.y !== undefined ? params.y : 12;

  const style = getComputedStyle(node);
  const transform = style.transform === 'none' ? '' : style.transform;
  const opacity = +style.opacity;

  return {
    delay,
    duration,
    easing: cubicOut,
    css: (/** @type {number} */ t) => `
      transform: ${transform} translate3d(${(1 - t) * x}px, ${(1 - t) * y}px, 0);
      opacity: ${t * opacity};
      will-change: transform, opacity;
    `
  };
}

/* ==========================================================================
   10. Expand / Collapse Transition (ANIMATION.md Section 33 & 55)
   ========================================================================== */

/**
 * @param {HTMLElement} node
 * @param {{ delay?: number, duration?: number }} [params]
 */
export function m3Expand(node, params = {}) {
  if (isReducedMotion()) {
    return { duration: 0 };
  }
  const delay = params.delay || 0;
  const duration = params.duration || DUR_MEDIUM_1;
  const height = node.scrollHeight;

  return {
    delay,
    duration,
    easing: cubicOut,
    css: (/** @type {number} */ t) => `
      max-height: ${t * height}px;
      opacity: ${t};
      overflow: hidden;
    `
  };
}
