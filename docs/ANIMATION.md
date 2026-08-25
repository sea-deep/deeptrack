# Material 3 Expressive Motion and Animation Specification

**Scope:** Animation, transitions, motion choreography, gesture response, motion accessibility, and implementation only.  
**Reference model:** Google Material 3 Expressive, Android predictive back, and current first-party Android motion tokens.  
**Research date:** August 25, 2026.

---

## 1. Scope and Evidence Model

This specification describes how an interface should move. It intentionally excludes unrelated guidance about branding, static layout, typography, color palettes, information architecture, and product features.

Every numerical recommendation falls into one of three categories:

- **Verified:** Directly supported by Google's published documentation or first-party source code.
- **Derived:** Calculated from verified spring constants using the documented spring model.
- **Recommended:** An implementation decision for applying Material-like behavior where Google does not publish a universal number.

Recommended values are deliberately identified as such. They are useful defaults, not claims about a secret or universal Android specification.

The source of truth for numerical spring values is Google's first-party Material Components Android [`tokens.xml`](https://github.com/material-components/material-components-android/blob/master/lib/java/com/google/android/material/motion/res/values/tokens.xml). The corresponding [`styles.xml`](https://github.com/material-components/material-components-android/blob/master/lib/java/com/google/android/material/motion/res/values/styles.xml) binds those values into named spring styles, and [`themes_base.xml`](https://github.com/material-components/material-components-android/blob/master/lib/java/com/google/android/material/theme/res/values/themes_base.xml) assigns expressive springs to expressive themes and standard springs to standard themes. [S01–S03]

## 2. Motion Direction

Material 3 Expressive is not defined by adding bounce to every visible element. Its defining characteristic is a coherent, physics-based motion system in which position, scale, size, and shape respond to interaction naturally while visual effects remain controlled.

The system must satisfy these rules:

1. Motion explains where content came from, where it is going, and how two states relate.
2. User input has priority over any animation already in progress.
3. A new target continues from the current visual position and velocity; it does not reset to the original start state.
4. The same interaction receives the same motion treatment across the entire interface.
5. Prominent spatial transformations may overshoot. Opacity and color channels never overshoot.
6. Larger distances and larger surfaces use slower spring families than small controls.
7. Entering content receives more readable deceleration than exiting content.
8. Persistent surfaces remain visually stable across navigation.
9. Motion never delays interaction, disguises loading, or obscures important information.
10. Reduced-motion preferences remove decorative movement while preserving usable state changes.

Google's own research describes Material 3 Expressive as an outcome of 46 studies involving more than 18,000 participants. The practical conclusion for this document is that expressiveness is a usability mechanism, not permission for indiscriminate movement. [S04]

## 3. The Two Motion Systems

### 3.1 Primary System: Physics-Based Springs

The current Material expressive motion model is built around springs. A spring is described by:

- **Stiffness `k`:** How strongly the animated value is pulled toward its destination.
- **Damping ratio `ζ`:** How quickly oscillation decays.
- **Initial velocity `v₀`:** The current movement carried into the next animation.
- **Target:** The destination value, which may change while the animation is running.

A spring is not fundamentally defined by a fixed duration. Its visible completion time depends on stiffness, damping, displacement, initial velocity, and the implementation's resting thresholds.

For unit mass, the governing equation is:

```text
x'' + 2ζ√k · x' + k(x - target) = 0
```

Where:

```text
natural angular frequency: ωₙ = √k
damping coefficient:       c  = 2ζ√k
```

Interpretation:

- `ζ < 1`: Underdamped; the value can overshoot and oscillate.
- `ζ = 1`: Critically damped; the value approaches its target without overshoot.
- `ζ > 1`: Overdamped; the value approaches without overshoot but responds more slowly.

Google exposes six semantic spring slots: three speed families, each split into spatial and effects variants. [S01, S05]

### 3.2 Compatibility System: Easing and Duration

Traditional easing curves and fixed durations still matter for:

- CSS transitions that cannot use a live spring solver.
- Web Animations API fallbacks.
- Existing Android view transitions.
- Strictly timed choreography.
- Non-interruptible visual effects.
- Integration with third-party animation systems.

These values are compatibility primitives. They should not replace physics-based springs for interactive, interruptible movement when a real spring is available.

## 4. Verified Expressive Spring Tokens

The following values are copied as factual identifiers and numerical constants from Google's first-party Material token source. They are the actual expressive values, not estimates. [S01]

| Semantic slot | First-party token prefix | Damping ratio `ζ` | Stiffness `k` | Appropriate role |
| --- | --- | ---: | ---: | --- |
| Fast spatial | `m3_sys_motion_expressive_spring_fast_spatial` | `0.6` | `800` | Small controls, shape response, selected indicators, compact movement |
| Fast effects | `m3_sys_motion_expressive_spring_fast_effects` | `1.0` | `3800` | Small-control opacity, color, icon visibility |
| Default spatial | `m3_sys_motion_expressive_spring_default_spatial` | `0.8` | `380` | Sheets, panels, medium surfaces, layout movement |
| Default effects | `m3_sys_motion_expressive_spring_default_effects` | `1.0` | `1600` | Medium-surface opacity, scrims, content replacement |
| Slow spatial | `m3_sys_motion_expressive_spring_slow_spatial` | `0.8` | `200` | Full-screen transitions, large surfaces, major navigation |
| Slow effects | `m3_sys_motion_expressive_spring_slow_effects` | `1.0` | `800` | Full-screen opacity and other large-surface effects |

The important distinctions are:

- Fast expressive spatial motion is substantially bouncier than the default and slow expressive spatial families.
- Default and slow expressive spatial motion both use `ζ = 0.8`; they differ primarily in stiffness.
- Every expressive effects spring uses `ζ = 1.0`.
- Effects stiffness is higher than the corresponding spatial stiffness so visual effects resolve promptly while spatial motion continues settling.

## 5. Verified Standard Spring Tokens

The standard scheme is useful for repetitive, utilitarian, or information-dense interactions where expressive bounce would become fatiguing. [S01, S05]

| Semantic slot | Damping ratio `ζ` | Stiffness `k` | Difference from expressive |
| --- | ---: | ---: | --- |
| Fast spatial | `0.9` | `1400` | Faster settling and much less overshoot than expressive fast spatial |
| Fast effects | `1.0` | `3800` | Identical to expressive fast effects |
| Default spatial | `0.9` | `700` | Faster and more restrained than expressive default spatial |
| Default effects | `1.0` | `1600` | Identical to expressive default effects |
| Slow spatial | `0.9` | `300` | Faster and more restrained than expressive slow spatial |
| Slow effects | `1.0` | `800` | Identical to expressive slow effects |

Only the spatial springs change between standard and expressive schemes. The corresponding effects springs have the same numerical constants.

Use expressive spatial motion for navigation landmarks, important controls, surface transformations, and meaningful state changes. Use standard spatial motion for repeated low-attention interactions, dense lists, rapidly changing interfaces, and cases where oscillation undermines legibility.

## 6. Derived Spring Behavior

For an underdamped unit step with zero initial velocity, theoretical overshoot is:

```text
overshoot = exp(-ζπ / √(1 - ζ²))
```

A common rough 2% settling-envelope estimate is:

```text
settling time ≈ 4 / (ζ√k)
```

These values are mathematical approximations. They are not fixed animation durations, and real implementations may use different termination thresholds.

| Spring | Damping coefficient `c` for unit mass | Approximate peak overshoot | Approximate 2% settling envelope |
| --- | ---: | ---: | ---: |
| Expressive fast spatial | `33.94` | `9.48%` | `236 ms` |
| Expressive default spatial | `31.19` | `1.52%` | `257 ms` |
| Expressive slow spatial | `22.63` | `1.52%` | `354 ms` |
| Standard fast spatial | `67.35` | `0.15%` | `119 ms` |
| Standard default spatial | `47.62` | `0.15%` | `168 ms` |
| Standard slow spatial | `31.18` | `0.15%` | `257 ms` |
| Fast effects | `123.29` | `0%` | Depends on critically damped resting criteria |
| Default effects | `80.00` | `0%` | Depends on critically damped resting criteria |
| Slow effects | `56.57` | `0%` | Depends on critically damped resting criteria |

Consequences:

- A small expressive control can visibly overshoot; a large page should not wobble in the same way.
- The fast expressive spring's approximate `9.48%` overshoot applies to its animated displacement, not necessarily to the entire element's size.
- A `1 → 0.96` scale animation with `9.48%` displacement overshoot reaches approximately `0.9562`; it does not shrink by an extra `9.48%` of its original size.
- Effects must remain bounded even when spatial movement overshoots.
- Initial gesture velocity can make the actual trajectory differ from the zero-velocity examples above.

## 7. Property-to-Spring Mapping

| Animated property | Spring family | Overshoot allowed? | Implementation note |
| --- | --- | --- | --- |
| Horizontal position | Spatial | Yes, when visually safe | Respect container clipping and screen bounds |
| Vertical position | Spatial | Yes, when visually safe | Avoid motion that resembles unintended dismissal |
| Scale | Spatial | Yes, within a tight visual envelope | Keep interactive hit targets stable |
| Rotation | Spatial | Occasionally | Prefer small angles and meaningful direction |
| Corner radius | Spatial | Yes, with valid bounds | Clamp radius to valid geometry |
| Container width | Spatial | Yes, with layout precautions | Prefer transform-based layout projection where possible |
| Container height | Spatial | Yes, with layout precautions | Avoid repeated layout work during long transitions |
| Clip geometry | Spatial | Sometimes | Check paint cost and clipping artifacts |
| Shared-element bounds | Spatial | Usually subtle | Use default or slow spatial based on travel distance |
| Opacity | Effects | No | Clamp to `[0, 1]` |
| Background color | Effects | No | Interpolate channels without bounce |
| Foreground color | Effects | No | Preserve contrast throughout the transition |
| Scrim opacity | Effects | No | Coordinate with the foreground surface |
| Blur intensity | Effects | No | Use sparingly; large-area blur can be expensive |
| Shadow opacity | Effects | No | Prefer animating opacity of a prepared shadow layer |
| Icon visibility | Effects | No | Sequence replacements to avoid double exposure |
| Progress value | Effects or bounded spatial | No for determinate progress | Progress must not move backward unless the underlying value does |

Never apply an underdamped spatial spring directly to opacity, RGB components, alpha channels, or bounded progress values.

## 8. Choosing Fast, Default, or Slow

Google's documented selection rule is based on the size of the animated component and the distance it travels. Small controls use fast springs, partial-screen surfaces use default springs, and full-screen movement uses slow springs. [S05]

The following cutoffs are recommended implementation heuristics, not Google-published thresholds:

| Motion footprint | Recommended spatial family | Typical examples |
| --- | --- | --- |
| Small control; movement under approximately `32 px` | Fast | Switch thumb, selected tab indicator, button compression, checkbox |
| Medium surface; movement approximately `32–320 px` | Default | Drawer reveal, popover, filter panel, sheet snap, card rearrangement |
| Full-screen surface or movement above approximately `320 px` | Slow | Route change, immersive surface, major shared-element transform |

Select the effects spring from the same speed family unless the effect must finish earlier for readability.

Example:

```text
Button press:
  shape + scale  → expressive.fast.spatial
  color + icon   → expressive.fast.effects

Modal sheet:
  translation    → expressive.default.spatial
  scrim opacity  → expressive.default.effects

Page transition:
  bounds + scale → expressive.slow.spatial
  page opacity   → expressive.slow.effects
```

## 9. Verified Legacy Duration Tokens

Google's first-party token file defines the following fixed-duration values. These are verified compatibility tokens, not durations intrinsically attached to spring animations. [S01, S05]

| Token | Duration | Appropriate fallback role |
| --- | ---: | --- |
| `short1` | `50 ms` | Immediate acknowledgment, tiny effect correction |
| `short2` | `100 ms` | Fast exit, tiny icon effect |
| `short3` | `150 ms` | Press feedback, small fade |
| `short4` | `200 ms` | Small control transformation |
| `medium1` | `250 ms` | Compact surface entry |
| `medium2` | `300 ms` | Typical content transition |
| `medium3` | `350 ms` | Emphasized medium-surface transformation |
| `medium4` | `400 ms` | Larger coordinated transition |
| `long1` | `450 ms` | Large shared-element transition |
| `long2` | `500 ms` | Large deliberate transformation |
| `long3` | `550 ms` | Full-screen choreography |
| `long4` | `600 ms` | Large complex transition |
| `extraLong1` | `700 ms` | Exceptional choreography |
| `extraLong2` | `800 ms` | Exceptional choreography |
| `extraLong3` | `900 ms` | Exceptional choreography |
| `extraLong4` | `1000 ms` | Exceptional choreography |

Normal interactive navigation should rarely need the extra-long values. The presence of a duration token is not a recommendation to use it indiscriminately.

Important versioning caveat: Some older Material transition documentation associates component-specific defaults with similarly named tokens but lists historical values that do not match the current token table. Resolve the installed package's actual values instead of mixing historical component examples with current token definitions. [S01, S05]

## 10. Verified Easing Tokens and Source Discrepancies

The current first-party token XML contains these curves: [S01]

| Curve | Current token value | Intended fallback behavior |
| --- | --- | --- |
| Standard | `cubic-bezier(0.2, 0, 0, 1)` | Ordinary movement that begins and ends on screen |
| Standard decelerate | `cubic-bezier(0, 0, 0, 1)` | Incoming movement and gesture feedback |
| Standard accelerate | `cubic-bezier(0.3, 0, 1, 1)` | Exiting movement |
| Emphasized decelerate | `cubic-bezier(0.1, 0.7, 0.1, 1)` | Expressive incoming movement |
| Emphasized accelerate | `cubic-bezier(0.3, 0, 0.8, 0.2)` | Expressive outgoing movement |
| Legacy | `cubic-bezier(0.4, 0, 0.2, 1)` | Compatibility with older Material transitions |
| Linear | `linear` | Mechanical progress, bounded effects, deterministic interpolation |

The full emphasized curve is not a single cubic Bézier. Its current first-party path is:

```text
M 0,0
C 0.05,0 0.133333,0.06 0.166666,0.4
C 0.208333,0.82 0.25,1 1,1
```

The Material Components motion guide currently lists different emphasized-entry and emphasized-exit control points in one documentation table: `cubic-bezier(0.05, 0.7, 0.1, 1)` and `cubic-bezier(0.3, 0, 0.8, 0.15)`. The live token XML lists `0.1` and `0.2` at those respective positions. Treat this as a documentation/source-version discrepancy, not interchangeable evidence. For a current source-faithful implementation, prefer the installed token source. [S01, S05]

An approximation of the full emphasized path using CSS `linear()` is:

```css
--motion-emphasized-path: linear(
  0 0%,
  0.005 2.5%,
  0.021 5%,
  0.049 7.5%,
  0.093 10%,
  0.162 12.5%,
  0.273 15%,
  0.4 16.67%,
  0.52 18%,
  0.636 20%,
  0.719 22.5%,
  0.773 25%,
  0.84 30%,
  0.883 35%,
  0.912 40%,
  0.951 50%,
  0.973 60%,
  0.987 70%,
  0.995 80%,
  0.999 90%,
  1 100%
);
```

This curve is derived by sampling the first-party path; the sampled representation itself is not an official Google token.

## 11. Motion Architecture

An interface should be organized into persistent motion layers:

1. **Application shell:** Navigation, persistent framing, and surfaces that remain present across routes.
2. **Destination surface:** The active screen or primary content area.
3. **Shared elements:** Objects that visually connect one destination to another.
4. **Transient surfaces:** Menus, dialogs, sheets, drawers, and popovers.
5. **Interaction feedback:** Press, focus, selection, drag, and gesture response.
6. **Status feedback:** Loading, success, error, and background activity.

Motion rules:

- Never animate every layer simply because a route changed.
- Preserve the application shell unless the shell itself changes hierarchy.
- Keep shared elements visually above outgoing and incoming content during a transform.
- Scrims belong behind their associated transient surface and above the underlying destination.
- Interaction feedback must never appear below the surface being interacted with.
- Loading motion should not restart whenever an unrelated component rerenders.

## 12. Transition Selection Matrix

| Relationship between states | Primary pattern | Spatial family | Effects family | Core behavior |
| --- | --- | --- | --- | --- |
| Two unrelated top-level destinations | Fade through | Slow or default | Slow or default | Exit old content, then reveal new content without inventing direction |
| Parent to child destination | Shared axis Z | Slow | Slow | Incoming content appears closer; outgoing content recedes |
| Child back to parent | Reverse shared axis Z | Slow | Slow | Incoming parent returns while the child recedes |
| Ordered horizontal steps | Shared axis X | Default or slow | Default | Movement follows sequence order and reverses on back |
| Ordered vertical steps | Shared axis Y | Default | Default | Movement follows vertical hierarchy |
| Card or list item to detail | Container transform | Default or slow | Default or slow | Source container becomes the destination surface |
| Image to image detail | Shared element | Default or slow | Default | The same image preserves identity across destinations |
| Search field to search surface | Container transform | Default | Default | Search field expands into its focused surface |
| Button to contextual panel | Anchored expansion | Default | Default | Panel appears from the initiating control |
| Dialog presentation | Fade plus scale | Default | Default | Dialog appears above the current destination |
| Bottom sheet presentation | Vertical translation | Default | Default | Sheet rises while the scrim fades in |
| Side panel presentation | Horizontal translation | Default | Default | Panel enters from its anchored edge |
| Tab selection | Indicator translation | Fast | Fast | Indicator moves; content uses restrained replacement |
| Menu or popover | Anchored scale plus fade | Fast or default | Fast | Surface grows from its trigger |
| Inline content expansion | Size transform | Default | Default | Container expands while neighboring content adjusts |
| Item reorder | Layout projection | Fast or default | Fast | Items move to their new spatial positions |

The first four named transition patterns—container transform, shared axis, fade through, and fade—are documented in Google's Material Components motion guidance. [S05]

## 13. Transition Grammar

### 13.1 Entering Versus Exiting

Entering content must become readable before its spatial movement fully settles. Exiting content should release attention promptly.

Recommended fallback ratios:

```text
Exit effects:  30–45% of the overall transition window.
Enter effects: Start around 25–40% through the window.
Enter spatial: Continue until the destination is visually stable.
```

These ratios are choreography defaults, not universal Material tokens. Predictive-back fade-through uses a separately documented and verified `35%` threshold; do not generalize that exact value to every animation.

### 13.2 Origin and Anchor

Set the transform origin to the point that explains the interaction:

- Context menu: The triggering button or pointer location.
- Dropdown: The top edge adjacent to its field.
- Dialog: Usually the surface center, unless initiated by a clearly associated card.
- Bottom sheet: The bottom edge of the viewport.
- Side sheet: Its attachment edge.
- Shared element: The actual source element bounds.
- Floating action: The action's on-screen position.

A centered scale on every overlay destroys the relationship between trigger and result.

### 13.3 Direction

- Forward horizontal sequence: Current content moves toward the leading edge; incoming content enters from the trailing edge.
- Backward horizontal sequence: Reverse the movement.
- Forward vertical sequence: Current content moves upward; incoming content arrives from below.
- Backward vertical sequence: Reverse the movement.
- Deeper hierarchy: Use depth or containment, not arbitrary lateral movement.
- Unrelated destinations: Avoid directional claims; use fade through.

Direction must honor writing direction. In right-to-left layouts, semantic “forward” and “backward” should map to the appropriate logical inline direction.

### 13.4 Distance

Recommended screen transition travel:

```text
Compact inline replacement:  8–16 px
Medium panel or tab content: 16–32 px
Major destination shift:     24–48 px
Full sheet or drawer:        Its actual surface travel distance
```

Avoid moving an entire page by its full viewport width unless the destination is genuinely arranged as a horizontal pager or the gesture is directly manipulating that surface.

### 13.5 Scale

Recommended local defaults:

```text
Small control press:     1.00 → 0.96–0.98
Large control press:     1.00 → 0.98–0.99
Popover entry:           0.92–0.97 → 1.00
Dialog entry:            0.94–0.98 → 1.00
Parent surface recession: 1.00 → 0.96–0.98
```

Android predictive back has its own verified full-screen values: exiting content `100% → 90%`, entering content `110% → 100%`. Those values should be used for that documented predictive-back pattern, not indiscriminately for every modal or page transition. [S06]

## 14. Top-Level Destination Transition

Use fade through when moving between destinations without a clear spatial relationship.

Behavior:

1. Persistent navigation remains fixed.
2. Existing content fades out with the appropriate effects spring.
3. The incoming destination appears after the outgoing content has meaningfully cleared.
4. Incoming content may travel upward by a recommended `12–24 px`.
5. Incoming content may scale from a recommended `0.985–0.995` to `1`.
6. Interactive controls are available as soon as the new destination is mounted.
7. The transition must not rerun when the same destination merely receives updated data.

Recommended fallback duration: `250–350 ms` total.

Do not slide unrelated destinations left and right as though they were sequential pages.

## 15. Hierarchical Navigation Transition

For parent-to-child navigation without a distinct shared object, use shared axis Z.

Recommended forward choreography:

```text
Parent:
  opacity: 1 → 0
  scale:   1 → 0.97

Child:
  opacity: 0 → 1
  scale:   1.025 → 1
```

Recommended backward choreography:

```text
Child:
  opacity: 1 → 0
  scale:   1 → 1.025

Parent:
  opacity: 0 → 1
  scale:   0.97 → 1
```

Use slow spatial and slow effects for full-screen destinations. Preserve focus, scroll position, and the visual identity of persistent shell elements.

If a specific card, image, or list item initiated the navigation, prefer a shared-element or container transform over generic depth movement.

## 16. Container Transform

A container transform treats the initiating surface and resulting surface as two states of the same object. Google's documented examples include card-to-detail, list-item-to-detail, action-to-detail, and search-field expansion. [S05]

### 16.1 Required Animated Properties

- Container position.
- Container width and height.
- Corner radius.
- Optional elevation or shadow intensity.
- Outgoing content opacity.
- Incoming content opacity.
- Optional scrim opacity.

### 16.2 Choreography

1. Record source and destination bounds.
2. Promote the shared container into an overlay or transition layer.
3. Hold the outgoing destination in place while the shared surface moves.
4. Animate bounds and corner radius with the selected spatial spring.
5. Fade old internal content out before new internal content becomes dominant.
6. Keep the destination's final text and controls stable as soon as they become legible.
7. Return the transformed element to its normal layout once the spring settles.

Recommended fallback timing:

```text
Small card → medium panel: 300–400 ms
Card → full-screen detail: 400–550 ms
Reverse transformation:    250–450 ms
```

### 16.3 Shape Continuity

Corner radius must interpolate continuously:

```text
rounded card → less-rounded expanded surface → destination radius
```

Avoid abruptly replacing a rounded source with a square destination halfway through the animation.

### 16.4 Content Handoff

When outgoing and incoming content differ significantly, prefer sequential fading:

```text
0.00–0.35: Fade outgoing content.
0.30–0.65: Grow and reposition the shared surface.
0.35–0.80: Fade incoming content.
0.80–1.00: Finish spatial settling.
```

These are recommended choreography ranges. The `0.35` handoff here is a useful design choice, not a claim that every container transform inherits predictive-back's official threshold.

### 16.5 Failure Cases

- Duplicate shared-element identifiers.
- Source element removed before its bounds are captured.
- Destination content mounted too late.
- Text stretched by scaling its container without a content handoff.
- Source and destination using incompatible clipping.
- Scroll restoration changing source bounds during the animation.
- Unrelated background surfaces animating simultaneously.

## 17. Shared Element Transition

A shared element keeps the same visual identity across two states. In Jetpack Compose, Google distinguishes between `sharedElement()` for matching content and `sharedBounds()` for visually different content occupying corresponding containers. [S07]

Use a shared element when:

- The same image exists in both states.
- The same avatar moves into an expanded header.
- A selected thumbnail becomes a primary visual.
- A persistent label or icon meaningfully connects two screens.

Use shared bounds when:

- A compact card becomes a large detail surface.
- The container persists but its internal content changes.
- The starting and ending shapes differ significantly.

Implementation rules:

- Shared identity must be stable and unique.
- Animate only elements that genuinely represent the same thing.
- Do not assign a shared transition to every list item.
- Avoid animating selectable text through a distorted scale.
- Preserve aspect ratio unless changing it communicates a meaningful state change.
- Clip overlays deliberately so they neither disappear under parents nor cover unrelated system UI.

## 18. Ordered Navigation and Shared Axes

Google's Material motion guidance defines shared axes for X, Y, and Z relationships. Its documented direction table maps forward X movement left, forward Y movement up, and forward Z movement deeper into the interface. [S05]

### 18.1 Horizontal Ordered Sequence

Use for a pager, a multistep workflow, or ordered sibling content.

Recommended forward motion:

```text
Outgoing: translateX(0 → -24 px), opacity(1 → 0)
Incoming: translateX(24 px → 0), opacity(0 → 1)
```

Reverse both spatial directions for backward navigation.

Use logical direction in bidirectional layouts rather than hard-coded left/right assumptions.

### 18.2 Vertical Ordered Sequence

Use for a vertical stepper, content stack, or a state that clearly moves upward or downward.

Recommended forward motion:

```text
Outgoing: translateY(0 → -20 px), opacity(1 → 0)
Incoming: translateY(20 px → 0), opacity(0 → 1)
```

### 18.3 Depth-Based Sequence

Use when a destination is nested inside its parent but no source container deserves a full transform.

Keep scale changes small. An aggressive full-screen zoom makes ordinary navigation feel unstable.

## 19. Dialog Motion

Dialog presentation should read as a transient surface appearing above an existing destination.

Recommended entry:

```text
Dialog position: translateY(12–20 px → 0)
Dialog scale:    0.95–0.98 → 1
Dialog opacity:  0 → 1
Scrim opacity:   0 → configured resting value
```

Recommended exit:

```text
Dialog position: translateY(0 → 8–12 px)
Dialog scale:    1 → 0.97–0.99
Dialog opacity:  1 → 0
Scrim opacity:   resting value → 0
```

Use default spatial for translation and scale, and default effects for dialog and scrim opacity.

Rules:

- Keep the underlying destination mounted and visually stable.
- Move focus into the dialog only once it exists in the accessibility tree.
- Restore focus to the triggering control on dismissal.
- Keep the surface present until the exit transition completes.
- Escape/back dismissal must interrupt an in-progress entrance gracefully.
- Do not bounce the scrim.
- Do not wait for the entry animation to finish before allowing dismissal.

## 20. Bottom Sheet Motion

Bottom sheets should move as directly manipulable surfaces attached to the lower edge.

### 20.1 Presentation

```text
Surface: translateY(sheetHeight → restingPosition)
Scrim:   opacity(0 → restingOpacity)
Shape:   larger top corner radius → resting top corner radius
```

Use default spatial for translation and shape; default effects for the scrim.

### 20.2 Dragging

While the pointer is down:

- The sheet follows pointer movement directly.
- The finger-to-surface relationship does not lag behind a decorative easing curve.
- Resistance increases near invalid bounds.
- Scrim opacity follows normalized sheet openness without overshoot.
- Nearby content stays fixed unless deliberately coordinated.

### 20.3 Release

Choose the destination from position and velocity:

```text
if projectedPosition passes dismissal threshold:
    dismiss
else if projectedPosition passes expansion threshold:
    expand
else:
    return to the nearest resting state
```

Recommended initial projected-position heuristic:

```text
projectedPosition = currentPosition + velocity × 0.18 s
```

The `0.18 s` projection horizon is a tunable implementation heuristic, not an Android platform constant.

Feed the actual release velocity into the settling spring. A fast flick should not restart from zero velocity.

### 20.4 Snap States

Possible sheet states:

```text
hidden → partially expanded → fully expanded
```

Support only states that match the surface's actual content and available viewport height. Google's Compose guidance documents partially expanded sheets and user-controlled drag behavior. [S08]

### 20.5 Nested Scroll

- If sheet content can still scroll upward, content scrolling takes priority.
- When the content reaches its top and the user continues dragging downward, control transfers to the sheet.
- The handoff preserves pointer continuity and, when feasible, release velocity.
- Avoid simultaneous content scroll and sheet drag unless the interaction explicitly requires it.

## 21. Side Sheets, Drawers, and Panels

Use spatial movement from the actual attachment edge.

Recommended behavior:

```text
Leading-side drawer:
  translateX(-surfaceWidth → 0)

Trailing-side panel:
  translateX(surfaceWidth → 0)

Scrim:
  opacity(0 → restingOpacity)
```

Rules:

- The surface should not scale from the viewport center.
- Persistent navigation may remain visible if the layout treats the panel as nonmodal.
- Modal panels must trap focus and close on Escape/back.
- Reverse motion on dismissal.
- In right-to-left layouts, map leading and trailing edges logically.
- When a permanent side panel appears because the viewport changes, use a restrained layout transition instead of a modal drawer animation.

## 22. Menus, Popovers, Tooltips, and Dropdowns

### 22.1 Anchored Menu

Recommended entry:

```text
transform-origin: nearest corner to the trigger
scale:            0.94–0.98 → 1
translateY:       -4–8 px → 0, depending on anchor edge
opacity:          0 → 1
```

Use fast or default spatial based on surface size; use fast effects for opacity.

### 22.2 Exit

- Exit more quickly than entry.
- Keep the origin at the trigger attachment point.
- Preserve focus transfer and selection events.
- Do not leave an invisible surface intercepting pointer events.

### 22.3 Tooltip

Keep movement minimal:

```text
opacity:    0 → 1
translateY: 2–6 px → 0
```

Tooltips should never wobble, bounce aggressively, or delay keyboard access.

### 22.4 Cascading Menus

- Open child menus from the edge nearest their parent item.
- Keep parent menus mounted while descendants are open.
- Stagger only enough to communicate hierarchy; recommended `20–40 ms` between surfaces.
- Cancel pending child entry when pointer intent changes.

## 23. Search Expansion

When a compact search field opens a larger search interface, treat the field as the starting container.

Animate:

- Bounds from compact field to expanded surface.
- Corner radius from compact to expanded geometry.
- Leading icon into its destination state.
- Placeholder into input content without stretching text.
- Background content into an appropriate dimmed or inactive state.
- Suggestion items after the surface establishes its new bounds.

Recommended choreography:

```text
0–35%:   Expand container and begin shape change.
20–55%:  Fade or replace compact-field content.
35–75%:  Reveal input and supporting controls.
55–100%: Reveal suggestions with minimal stagger.
```

Place keyboard focus in the input promptly. Do not postpone typing until the entire animation completes.

## 24. Navigation Indicator Motion

Selection indicators should move as persistent objects rather than disappearing and reappearing under the next destination.

### 24.1 Navigation Bar or Rail

Animate:

```text
Indicator bounds: old item → new item
Indicator shape:  old selected shape → new selected shape
Old icon:         selected treatment → unselected treatment
New icon:         unselected treatment → selected treatment
Labels:           bounded effects without oscillation
```

Use fast expressive spatial for the indicator when the travel distance is short; use default spatial if it travels across a wide rail or bar.

### 24.2 Tab Indicator

- Maintain one continuous indicator.
- Interpolate its leading and trailing bounds.
- Allow a slight asymmetric stretch when the indicator changes width.
- Avoid stretching the text label.
- Animate the indicator independently of slow data loading in the destination panel.
- Keep keyboard focus visible throughout the movement.

Recommended fallback duration: `150–250 ms`.

### 24.3 Content Under Tabs

Use restrained shared-axis movement only when tabs represent an ordered sequence. For independent content categories, a small fade or fade-through treatment is usually more truthful.

## 25. Button and Press Motion

### 25.1 Interaction Lifecycle

```text
idle → pressed → released → idle
idle → pressed → canceled → idle
idle → pressed → selected
```

The pressed state starts on pointer down or equivalent keyboard activation, not after the click handler completes.

### 25.2 Recommended Visual Response

```text
Scale:         1 → 0.96–0.98
Corner radius: resting radius → slightly more expressive pressed shape
State layer:   transparent → bounded visible state
Icon:          optional rotation or morph when semantically justified
```

Use fast spatial for scale and shape. Use fast effects for state-layer opacity and color.

The official Androidify example uses the themed `MaterialTheme.motionScheme.defaultSpatialSpec<Float>()` for a camera button scale effect. This demonstrates that the semantic family should be selected by the interaction's actual importance and footprint; “button” does not require a rigid one-size-fits-all token. [S09]

### 25.3 Release

- Preserve the velocity generated by the press.
- Use a slightly expressive rebound when appropriate.
- If the action opens another surface, visually coordinate the release with that surface's entry.
- If the pointer leaves the control, cancel cleanly without triggering the action.
- Prevent repeated clicks from spawning competing animation instances.

### 25.4 Disabled Controls

- Do not play active press motion for disabled controls.
- A state change from enabled to disabled may fade using fast effects.
- Do not use motion as the sole indication that a control became unavailable.

### 25.5 Icon Morphs and Animated Vectors

Icons may animate between semantically related states:

```text
play → pause
menu → close
expand → collapse
unchecked → checked
muted → unmuted
```

Rules:

- Morph an icon only when the starting and ending symbols describe states of the same control.
- Prefer vector-path interpolation when both paths share compatible topology.
- If the paths are incompatible, coordinate a short outgoing fade with an incoming fade and a restrained scale or rotation.
- Keep the icon center and optical alignment stable.
- Use fast spatial for rotation or geometric deformation and fast effects for opacity.
- Avoid spinning an icon through a large angle simply to indicate a binary state.
- Preserve the accessible name and checked/expanded semantics independently from the animation.
- Do not restart an icon morph if the target state has not changed.

For path interpolation, source and destination paths must have compatible command structures or be normalized by a reliable vector-morphing tool. Android's animated-vector guidance covers native vector-based animation; on the web, use SVG paths or a framework-native vector representation rather than animated raster replacements. [S32]

## 26. Connected Button Groups

Expressive button groups can redistribute shape and width across adjacent controls when a button is pressed or selected. Google's Compose release notes explicitly document `ButtonGroupScope` and an `animateWidth` modifier for coordinating child-width animation. [S10]

Recommended behavior:

1. The pressed item gains a modest amount of visual emphasis.
2. Adjacent items compress proportionally when space is constrained.
3. Internal adjoining corners adjust continuously.
4. The outer silhouette remains coherent.
5. Selection persists after the transient press shape settles.
6. Label readability and minimum hit targets remain intact.

Never stretch text glyphs to create the width change. Animate the container or its projected layout while rendering text at its normal scale.

## 27. Switches, Checkboxes, and Radio Controls

### 27.1 Switch

Animate separately:

```text
Thumb position:       fast spatial
Thumb size or shape:  fast spatial
Track color:          fast effects
Optional icon:        fast effects or a bounded icon morph
```

The thumb may overshoot slightly inside a safe travel envelope. It must not visually escape the track.

### 27.2 Checkbox

Animate:

- Container state using fast effects.
- Checkmark appearance using a short path reveal or opacity transition.
- Optional shape response using fast spatial.

Do not repeatedly replay the checkmark drawing when a parent rerenders.

### 27.3 Radio Control

Animate the selection indicator's scale or size using fast spatial and its visibility or fill using fast effects. Keep group updates coordinated so the outgoing and incoming selected states do not appear simultaneously selected for an extended period.

## 28. Chips, Filters, and Segmented Controls

- Selection shape: Fast spatial.
- Leading checkmark appearance: Fast effects.
- Label movement after adding or removing an icon: Fast spatial or a small layout projection.
- Background-state change: Fast effects.
- Filter-list changes: Default spatial for item rearrangement; bounded effects for entry and exit.

If a selected chip causes a large list to update, do not replay entrance animation for every existing list item. Animate only inserted, removed, and moved items.

## 29. Sliders and Scrubbing Controls

While dragging:

- The thumb tracks the pointer directly.
- The active track updates continuously.
- Value text updates without restarting a transition on every input event.
- Haptic feedback, if available, occurs at meaningful ticks rather than every rendered frame.

On release:

- Snap to the nearest valid tick when appropriate.
- Carry release velocity into the settling animation when the control's behavior supports it.
- Use fast spatial for thumb settling.
- Use fast effects for track and label visibility.
- Avoid animating discrete values backward after commitment.

## 30. Text Fields and Inline Validation

Motion should explain focus and validation without disrupting typing.

Animate:

- Label position with fast spatial or a short emphasized curve.
- Supporting-text reveal with fast effects.
- Error-message container expansion with default spatial.
- Focus-indicator geometry with fast spatial.
- Validation color with fast effects.

Avoid:

- Moving the caret unexpectedly.
- Shaking the entire form for ordinary validation.
- Reanimating labels on each keystroke.
- Introducing layout jumps when helper text appears.
- Hiding errors before assistive technology can announce them.

If an explicit error shake is necessary, restrict it to a small local displacement, make it non-looping, and remove it completely for reduced-motion users.

## 31. Cards and Surface Expansion

Cards may animate between compact, focused, selected, and expanded states.

Recommended property mapping:

```text
Position:      default spatial
Bounds:        default spatial
Corner radius: default spatial
Shadow alpha:  default effects
Content fade:  default effects
```

Use transform-based layout projection when a card's new geometry would otherwise force a large number of sibling layouts on every frame.

If selecting a card does not change its hierarchy, avoid launching a full container transform. A small shape, state-layer, or bounds adjustment is sufficient.

### 31.1 Expressive Shape Morphing

Shape morphing is one of the recognizable motion behaviors in Material 3 Expressive. Google's Wear OS guidance identifies fluid movement and shape transformation as key interaction behaviors, while its Compose graphics documentation describes constructing a `Morph` between two `RoundedPolygon` shapes. [S29, S30]

Typical uses:

```text
Resting button shape → compressed pressed shape.
Unselected control → selected control silhouette.
Compact rounded card → expanded container.
Loading indicator shape A → shape B → shape C.
Connected button segment → redistributed group shape.
```

The animated shape should remain recognizable throughout its trajectory. A morph that changes the entire silhouette without a corresponding state change communicates noise rather than continuity.

### 31.2 Corner-Radius Morph

The simplest shape transition interpolates the corner radius:

```ts
const radius = createSpringController(
  24,
  motionTokens.expressive.fast.spatial,
  (value) => {
    surface.style.setProperty('--corner-radius', `${value}px`);
  },
);

radius.setTarget(pressed ? 16 : 24);
```

Use the spring controller from Section 49 or a framework-native equivalent.

Keep corner radii physically valid:

```text
0 ≤ cornerRadius ≤ min(width, height) / 2
```

For asymmetric surfaces, animate each corner deliberately rather than replacing one complete border-radius declaration with an unrelated one.

### 31.3 Path and Polygon Morph

For more expressive geometry:

1. Normalize both shapes to a common coordinate space.
2. Ensure their path representations have compatible correspondence.
3. Animate a normalized progress value with a spatial spring.
4. Generate the intermediate path from that progress.
5. Cache reusable path and polygon data.
6. Clamp or otherwise handle spring overshoot if the shape generator only accepts progress in `[0, 1]`.

Compose concept:

```kotlin
val firstShape = RoundedPolygon(
    numVertices = 6,
    rounding = CornerRounding(radius = 0.15f)
)

val secondShape = RoundedPolygon(
    numVertices = 4,
    rounding = CornerRounding(radius = 0.25f)
)

val morph = remember(firstShape, secondShape) {
    Morph(firstShape, secondShape)
}

val shapeProgress by animateFloatAsState(
    targetValue = if (selected) 1f else 0f,
    animationSpec = MaterialTheme.motionScheme.fastSpatialSpec(),
    label = "shape-morph"
)

val path = morph.toPath(
    progress = shapeProgress.coerceIn(0f, 1f)
)
```

Check the installed `androidx.graphics:graphics-shapes` and Compose versions for exact constructor/import/API details. The shape's visual container can still preserve expressive rebound even if path-generator progress itself must be clamped.

### 31.4 Shape-Animation Performance

- Prefer animating a small vector or one isolated surface.
- Cache polygon construction, path correspondence, and expensive derived geometry.
- Avoid recomputing every sibling's clip path on each frame.
- Profile large-area `clip-path`, `border-radius`, and `filter` combinations.
- Use simpler corner-radius interpolation when a complex morph adds no useful information.
- Provide a direct state change or bounded fade under reduced motion.

## 32. List Entry, Exit, Reorder, and Filtering

### 32.1 Initial Entrance

Recommended:

```text
Item opacity:    0 → 1
Item translateY: 8–16 px → 0
Stagger:         20–35 ms per visible group
Maximum stagger: 120–160 ms across the visible sequence
```

These values are implementation recommendations.

Animate only the initially visible items. Virtualized items entering the viewport during ordinary scrolling should not repeatedly execute a theatrical entrance.

### 32.2 Insertion

- Existing items move to their new positions with default or fast spatial motion.
- The inserted item enters with bounded effects and a small spatial offset.
- Preserve scroll position when insertion happens above the viewport.
- Do not shift keyboard focus unexpectedly.

### 32.3 Removal

- Fade the removed item quickly.
- Collapse its occupied space without delaying user input.
- Move remaining items into their new positions.
- Provide undo where the interaction model requires it; motion does not replace undo.

### 32.4 Reordering

- The dragged item follows the pointer.
- Neighboring items spring into their projected positions.
- The dragged item may gain slight scale or shadow emphasis.
- On release, it settles with its current velocity.
- Stable item identifiers are mandatory.

### 32.5 Filtering

Keep surviving items visually continuous. Animate removed items out, inserted items in, and surviving items into their new locations. Never remount the entire list simply to obtain an animation.

## 33. Accordions and Disclosure Regions

Animate:

```text
Indicator rotation: fast spatial
Container extent:   default spatial
Body opacity:       default effects
Neighbor movement:  default spatial
```

Rules:

- Keep content readable; do not vertically squash glyphs.
- Animate between measured heights or use supported intrinsic-size animation.
- Preserve semantic expanded/collapsed state immediately.
- Support rapid repeated toggles without snapping to the previous start height.
- If several disclosure items change simultaneously, coordinate neighboring layout movement.

Modern CSS can interpolate intrinsic sizes with `interpolate-size: allow-keywords` in supporting browsers. Provide a measured-height or transform-based fallback where unavailable. [S11]

## 34. Floating Actions and Floating Toolbars

Floating surfaces should respond to scrolling and context without competing with content.

Recommended behaviors:

- Hide or compress after sustained scrolling in the direction that benefits content visibility.
- Reappear promptly when scrolling reverses or focus enters the relevant region.
- Use default spatial for translation and expansion.
- Use fast or default effects for icon and label changes.
- Preserve the action's identity when switching between compact and extended forms.
- Avoid toggling visibility on every tiny scroll-direction change.

Recommended hysteresis:

```text
Minimum accumulated scroll before hiding: 24–48 px
Minimum reverse movement before showing: 12–24 px
```

These are tunable heuristics, not official Material thresholds. Google's Compose release notes document floating-toolbar scroll behavior that responds after crossing a threshold, supporting the principle of hysteresis without publishing a universal distance. [S10]

## 35. App Bar and Scroll-Coupled Motion

An app bar may collapse, expand, or adjust its visual prominence in response to scroll.

Behavior:

- Direct manipulation follows scroll progress.
- Programmatic settling uses default spatial.
- Background or state-layer opacity uses default effects.
- Title handoff occurs without duplicate readable titles lingering simultaneously.
- Toolbars do not flash between expanded and collapsed states near a boundary.
- Scroll-linked movement is clamped to valid bounds.

For web implementations, prefer declarative scroll-driven animations where supported and appropriate. They can coordinate animation progress with scroll timelines without manual per-scroll-event DOM mutation. [S12]

## 36. Pull-to-Refresh and Overscroll

### 36.1 Pull-to-Refresh

States:

```text
idle → pulling → armed → refreshing → completing → idle
```

Behavior:

- Pull distance controls indicator position directly.
- Resistance increases as the user approaches the maximum pull extent.
- Crossing the activation threshold changes the indicator state once.
- Release below the threshold returns to idle.
- Release above the threshold enters refreshing.
- Completion briefly confirms success before the indicator exits.

Do not claim a universal activation distance; tune the threshold to the surface and gesture model.

### 36.2 Overscroll

Material Compose uses stretch-based overscroll by default for supported scrolling containers. Treat overscroll as elastic resistance rather than an excuse to shift every unrelated element. [S13]

Recommended model:

```text
effectivePull = limit × (1 - exp(-rawPull / limit))
```

On release, return to rest with a spatial spring. Disable decorative stretch for reduced-motion users.

## 37. Progress and Loading Motion

### 37.1 Determinate Progress

- Progress reflects actual work.
- Increasing progress moves smoothly toward the newest value.
- Do not overshoot 100%.
- Do not animate backward unless the underlying process genuinely moved backward.
- Do not restart from zero when a component rerenders.
- Coalesce high-frequency updates before rendering them.

### 37.2 Indeterminate Progress

- Use a continuous loop only while work is actually active.
- Pause when the surface is hidden or the tab becomes inactive.
- Keep the loop visually bounded and predictable.
- Use a static or reduced alternative for motion-sensitive users.
- Avoid multiple simultaneous independent loading loops on one surface.

### 37.3 Expressive Wavy Indicators

Google's Compose release notes document wavy progress indicators and parameters such as amplitude and wave speed. Use this style for meaningful waiting states, not as a permanently animated decoration. [S10]

Rules:

- Keep amplitude proportional to track thickness.
- Preserve a clear direction of progress.
- Prevent wave motion from implying false determinate progress.
- Reduce amplitude or stop wave travel when reduced motion is enabled.
- Avoid animating large SVG paths at a rate that causes repeated expensive repaints.

### 37.4 Skeletons

- Prefer a subtle bounded opacity effect over a fast sweeping shimmer.
- Do not shimmer every placeholder independently.
- Stop the effect immediately when content is available.
- Maintain stable geometry between placeholder and final content.
- Use static placeholders for reduced motion.

## 38. Data Changes and Frequently Updating Content

### 38.1 Numeric Updates

For infrequent changes, a short bounded transition can clarify that a value changed:

```text
Old value: opacity(1 → 0), translateY(0 → -4 px)
New value: opacity(0 → 1), translateY(4 px → 0)
```

Use fast effects and minimal spatial movement.

For rapid updates, replace the value directly or interpolate it without rerunning a full entrance animation.

### 38.2 Charts and Indicators

When chart data changes:

- Interpolate the underlying geometry rather than replacing the entire chart.
- Keep axes and labels stable.
- Avoid bouncing quantitative marks past their actual values.
- Pause nonessential motion outside the viewport.
- Keep alerts and threshold markers visible immediately.

### 38.3 Update Rate

Recommended initial UI coordination:

```text
Display updates: Batch to animation frames.
Expensive visual interpolation: Coalesce to approximately 8–15 updates/second.
Direct pointer response: Track the active frame rate.
Critical state changes: Apply immediately.
```

These are implementation heuristics. Data ingestion should remain independent from visual animation frequency.

## 39. Responsive Layout Transitions

When a surface changes because its available space changes:

- Keep the user's current destination intact.
- Preserve scroll position and focus.
- Animate surfaces that change spatial role.
- Avoid rerunning initial page-entry choreography.
- Use default spatial for pane expansion and contraction.
- Use slow spatial only when the entire destination changes composition.
- Do not animate continuously during every resize event; settle after a meaningful breakpoint or gesture pause.

Examples:

```text
Navigation rail → wider side panel:
  Animate rail width and label visibility.

Single pane → list/detail split:
  Keep the selected item visually connected to the detail pane.

Bottom sheet → side sheet:
  Crossfade or transform the surface once after the layout mode changes.
```

## 40. Theme and Appearance Transitions

Only the motion behavior is specified here.

When a global appearance mode changes:

- Animate bounded visual channels with effects springs or short monotonic fades.
- Keep layout geometry, scroll position, and interaction state unchanged.
- Do not transition every element through independently scheduled timers.
- Apply a coordinated root-level transition or a small number of grouped layers.
- Preserve text legibility at intermediate states.
- Avoid full-screen radial reveals unless they are directly tied to an explicit user action and disabled for reduced motion.
- Never delay the actual preference change solely for choreography.

## 41. Interruptibility and Retargeting

Google's Compose gesture guidance explicitly describes interrupting an active animation and continuing toward a new target while preserving the interrupted animation's velocity. This is a core expressive-motion requirement. [S14]

### 41.1 Incorrect Behavior

```text
Current position: 72
Original start:    0
Old target:      100
New target:       30

Incorrect: restart at 0, then animate to 30.
```

### 41.2 Correct Behavior

```text
Current position: 72
Current velocity: +310 units/second
New target:       30

Correct: keep position 72, keep velocity +310, retarget the spring to 30.
```

The object may briefly continue in its old direction before spring force reverses it. That continuity is expected when it remains visually safe.

### 41.3 Interaction Priority

```text
Active pointer or keyboard input
  > accessibility and focus changes
  > explicit navigation changes
  > active surface transition
  > background loading motion
  > decorative ambient motion
```

An incoming user gesture must be able to interrupt lower-priority motion immediately.

## 42. Gesture Tracking and Velocity

### 42.1 Direct Manipulation

During an active drag:

```text
visualPosition = constrained(pointerPosition - initialPointerOffset)
```

Do not place an easing curve between the user's finger and the element they are actively dragging.

### 42.2 Velocity Estimation

Track timestamped pointer samples and estimate release velocity using recent movement rather than only the final two samples.

Recommended starting window:

```text
Recent pointer history: approximately 60–120 ms
```

This is an implementation heuristic. Platform-provided velocity trackers should be preferred over custom estimators when available.

### 42.3 Release and Fling

- Project current position forward using release velocity.
- Select the nearest valid resting state after accounting for intent.
- Pass release velocity to the settling spring.
- Clamp visually invalid overshoot.
- If canceled, return to the original state using the current position and velocity.

Compose's advanced gesture guidance documents `VelocityTracker`, direct `snapTo` updates during dragging, and velocity-aware settling or decay after release. [S14]

## 43. Android Predictive Back: Verified Full-Screen Behavior

Android's predictive-back design guidance publishes explicit motion values for full-screen back preview. These numbers are verified platform guidance, not generic recommendations. [S06]

| Property | Initial value | Target value | Verified rule |
| --- | ---: | ---: | --- |
| Exiting surface scale | `100%` | `90%` | Shrink the outgoing full-screen surface during the preview |
| Entering surface scale | `110%` | `100%` | Reveal the destination surface from a slightly larger scale |
| Exiting surface opacity | `100%` | `0%` | Finish fading by `35%` progress |
| Entering surface opacity | `0%` | `100%` | Begin fading in at `35%` progress |
| Full-screen exit interpolator | — | — | `cubic-bezier(0.1, 0.1, 0, 1)` |
| Gesture-progress interpolation | — | — | Standard decelerate: `cubic-bezier(0, 0, 0, 1)` |

At the `35%` handoff threshold, the outgoing surface has finished fading and the incoming surface begins appearing.

### 43.1 Shared-Surface Geometry

Android also documents the following geometry for a predictive-back shared surface:

```text
Minimum surface scale: 90%
Minimum visible edge gap: 8 dp

Maximum horizontal shift:
  (screenWidth / 20) - 8 dp

Maximum vertical shift:
  (availableScreenHeight / 20) - 8 dp
```

Example from the platform guidance:

```text
screenWidth = 1280 dp
maximumXShift = (1280 / 20) - 8
maximumXShift = 56 dp
```

Preserve the visible `8 dp` margin and decelerate movement toward the boundary.

### 43.2 Preview, Commit, and Cancel

```text
idle → previewing → committed
idle → previewing → canceled
```

Preview:

- The user controls progress continuously.
- The destination becomes understandable before commitment.
- Progress is mapped through a decelerating response rather than displayed as raw linear finger distance.

Commit:

- Incorporate the release velocity.
- Complete the destination change without a visual discontinuity.
- Absorb excessive fling energy before the final commit animation when necessary.

Cancel:

- Restore the original surface from its current scale, position, and velocity.
- Revert preview-only state changes.
- Keep the original screen interactive.

### 43.3 Platform Limitation

Google's predictive-back guidance notes that the documented custom shared-element preview behavior applies to specific same-screen/custom-back-stack scenarios. It explicitly warns against applying the same `90%` full-screen scaling blindly with `FragmentManager`, Navigation Component, or Navigation Compose. Follow the supported integration model for the actual navigation stack. [S06]

## 44. Choreography and Stagger

### 44.1 Group Before Staggering

Animate coherent groups, not every leaf node:

```text
Primary surface
  → section headings
  → primary interactive content
  → secondary details
```

### 44.2 Recommended Limits

```text
Related items:      20–35 ms between items or groups
Major surface handoff: 40–80 ms between layers
Maximum visible list cascade: 120–160 ms
Maximum independent motion layers: approximately 2–3 per interaction
```

These are practical defaults, not official Google constants.

### 44.3 When Not to Stagger

- Reduced-motion mode.
- High-frequency content updates.
- Already-visible lists being filtered.
- Critical alerts.
- Keyboard-focused interactions.
- A route transition that is already performing a major shared-element transform.
- Virtualized items entering the viewport through normal scrolling.

## 45. Motion State Machines

### 45.1 Surface Lifecycle

```text
closed
  → entering
  → open
  → exiting
  → closed

entering + close request → exiting from the current visual state
exiting + open request   → entering from the current visual state
```

### 45.2 Gesture Lifecycle

```text
idle
  → pointer-down
  → dragging
  → released
  → settling
  → idle

dragging + cancel → settling toward the original state
settling + pointer-down → dragging from the current state
```

### 45.3 Navigation Lifecycle

```text
stable
  → destination requested
  → old state captured
  → destination mounted
  → transition running
  → stable

transition running + new request → retarget or replace safely
```

Avoid coupling logical state exclusively to animation-completion callbacks. A skipped, interrupted, or reduced-motion transition must still result in the correct logical state.

## 46. Recommended Component Motion Inventory

| Component or behavior | Spatial motion | Effects motion | Reduced-motion behavior |
| --- | --- | --- | --- |
| Page entry | Slow spatial | Slow effects | Immediate swap or short fade |
| Top-level navigation | Default/slow spatial when needed | Default/slow effects | Short fade |
| Hierarchical navigation | Slow spatial | Slow effects | Short fade |
| Shared card/detail transition | Default/slow spatial | Default/slow effects | Direct state change or short crossfade |
| Modal dialog | Default spatial | Default effects | Short fade without scale |
| Bottom sheet | Default spatial | Default effects | Immediate placement or short fade |
| Side sheet | Default spatial | Default effects | Immediate placement or short fade |
| Navigation indicator | Fast spatial | Fast effects | Immediate movement or short state fade |
| Tab indicator | Fast spatial | Fast effects | Immediate selection |
| Button press | Fast spatial | Fast effects | Bounded state-color change |
| Connected button group | Fast/default spatial | Fast effects | Immediate shape and selection |
| Switch thumb | Fast spatial | Fast effects | Immediate position and color |
| Checkbox | Fast spatial when needed | Fast effects | Immediate checked state |
| Radio control | Fast spatial when needed | Fast effects | Immediate selection |
| Chip selection | Fast spatial | Fast effects | Immediate selection |
| Slider thumb | Direct manipulation; fast spatial settle | Fast effects | Direct manipulation without decorative rebound |
| Text field label | Fast spatial | Fast effects | Immediate label placement |
| Error message | Default spatial | Fast/default effects | Immediate reveal without shake |
| Card expansion | Default spatial | Default effects | Immediate layout with short content fade |
| Accordion | Default spatial | Default effects | Immediate expansion |
| List insertion | Fast/default spatial | Fast effects | Immediate insertion |
| List reorder | Fast/default spatial | Fast effects | Immediate placement; preserve drag semantics |
| Menu | Fast/default spatial | Fast effects | Short fade |
| Tooltip | Minimal spatial | Fast effects | Short fade or immediate reveal |
| Floating action | Default spatial | Fast/default effects | Immediate resting state |
| Floating toolbar | Default spatial | Fast/default effects | Immediate resting state |
| App bar collapse | Direct scroll; default spatial settle | Default effects | Direct layout without rebound |
| Pull-to-refresh | Direct gesture; default spatial settle | Fast/default effects | Static indicator and immediate state changes |
| Overscroll | Elastic spatial | None | Disable decorative stretch |
| Determinate progress | Bounded interpolation | Effects | Direct or minimal bounded update |
| Indeterminate progress | Optional restrained loop | Effects | Static activity indication |
| Skeleton | None or minimal | Slow bounded effects | Static skeleton |
| Theme transition | None | Effects | Immediate appearance change |
| Focus ring | Minimal spatial if useful | Fast effects | Immediate visible focus |
| Toast/snackbar | Default spatial | Fast/default effects | Short fade; preserve announcements |
| Badge/count update | Fast spatial if infrequent | Fast effects | Immediate value change |

## 47. Web Motion Token Architecture

Centralize motion values instead of scattering durations, easing functions, and spring parameters across components.

```ts
export type MotionSpeed = 'fast' | 'default' | 'slow';
export type MotionProperty = 'spatial' | 'effects';
export type MotionScheme = 'expressive' | 'standard';

export interface SpringToken {
  stiffness: number;
  dampingRatio: number;
}

export const motionTokens = {
  expressive: {
    fast: {
      spatial: { stiffness: 800, dampingRatio: 0.6 },
      effects: { stiffness: 3800, dampingRatio: 1 },
    },
    default: {
      spatial: { stiffness: 380, dampingRatio: 0.8 },
      effects: { stiffness: 1600, dampingRatio: 1 },
    },
    slow: {
      spatial: { stiffness: 200, dampingRatio: 0.8 },
      effects: { stiffness: 800, dampingRatio: 1 },
    },
  },
  standard: {
    fast: {
      spatial: { stiffness: 1400, dampingRatio: 0.9 },
      effects: { stiffness: 3800, dampingRatio: 1 },
    },
    default: {
      spatial: { stiffness: 700, dampingRatio: 0.9 },
      effects: { stiffness: 1600, dampingRatio: 1 },
    },
    slow: {
      spatial: { stiffness: 300, dampingRatio: 0.9 },
      effects: { stiffness: 800, dampingRatio: 1 },
    },
  },
} as const satisfies Record<
  MotionScheme,
  Record<MotionSpeed, Record<MotionProperty, SpringToken>>
>;

export function getSpringToken(
  scheme: MotionScheme,
  speed: MotionSpeed,
  property: MotionProperty,
): SpringToken {
  return motionTokens[scheme][speed][property];
}
```

Do not treat `dampingRatio` as interchangeable with every animation library's `damping` parameter.

## 48. Adapting Material Springs to Motion for React/JavaScript

The Motion library's physics-based spring accepts `stiffness`, a damping coefficient, `mass`, and optional initial `velocity`. Its `damping` field is not the same quantity as Android's damping ratio. [S15]

For a unit-mass spring:

```ts
import type { SpringToken } from './motion-tokens';

export function toMotionSpring(token: SpringToken, mass = 1) {
  return {
    type: 'spring' as const,
    stiffness: token.stiffness,
    damping: 2 * token.dampingRatio * Math.sqrt(token.stiffness * mass),
    mass,
  };
}

export const expressiveFastSpatial = toMotionSpring({
  stiffness: 800,
  dampingRatio: 0.6,
});

// { type: 'spring', stiffness: 800, damping: 33.94..., mass: 1 }
```

Example:

```tsx
import { motion } from 'motion/react';

<motion.button
  whileTap={{ scale: 0.97 }}
  transition={toMotionSpring(motionTokens.expressive.fast.spatial)}
/>
```

Shared surface:

```tsx
<motion.div
  layoutId="selected-surface"
  transition={toMotionSpring(motionTokens.expressive.default.spatial)}
/>
```

Google's numeric stiffness values and the damping-coefficient conversion provide a physically grounded starting point. Visual equivalence still depends on the library's integration behavior, velocity units, resting thresholds, layout-projection implementation, and the property being animated.

Never pass `damping: 0.6` to Motion and assume it reproduces Android's `dampingRatio: 0.6`; that produces a radically different system.

## 49. Exact JavaScript Spring Integrator

The following implementation solves the unit-mass damped spring analytically for underdamped, critically damped, and overdamped cases. Unlike a simplistic frame-dependent linear interpolation, it accepts the actual elapsed time and preserves the current velocity when the target changes.

```ts
export interface SpringState {
  value: number;
  velocity: number;
}

export interface SpringConfig {
  target: number;
  stiffness: number;
  dampingRatio: number;
}

export function stepSpring(
  state: SpringState,
  config: SpringConfig,
  deltaSeconds: number,
): SpringState {
  if (!Number.isFinite(deltaSeconds) || deltaSeconds <= 0) {
    return state;
  }

  if (config.stiffness <= 0 || config.dampingRatio < 0) {
    throw new RangeError('Invalid spring stiffness or damping ratio');
  }

  const omega = Math.sqrt(config.stiffness);
  const zeta = config.dampingRatio;
  const displacement = state.value - config.target;
  const velocity = state.velocity;

  if (Math.abs(zeta - 1) < 1e-6) {
    const coefficient = velocity + omega * displacement;
    const decay = Math.exp(-omega * deltaSeconds);

    return {
      value:
        config.target +
        (displacement + coefficient * deltaSeconds) * decay,
      velocity:
        (velocity - omega * coefficient * deltaSeconds) * decay,
    };
  }

  if (zeta < 1) {
    const dampedOmega = omega * Math.sqrt(1 - zeta * zeta);
    const decay = Math.exp(-zeta * omega * deltaSeconds);
    const cosine = Math.cos(dampedOmega * deltaSeconds);
    const sine = Math.sin(dampedOmega * deltaSeconds);
    const sineCoefficient =
      (velocity + zeta * omega * displacement) / dampedOmega;
    const positionTerm =
      displacement * cosine + sineCoefficient * sine;
    const derivativeTerm =
      -displacement * dampedOmega * sine +
      sineCoefficient * dampedOmega * cosine;

    return {
      value: config.target + decay * positionTerm,
      velocity:
        decay * (derivativeTerm - zeta * omega * positionTerm),
    };
  }

  const root = Math.sqrt(zeta * zeta - 1);
  const rootA = -omega * (zeta - root);
  const rootB = -omega * (zeta + root);
  const coefficientA =
    (velocity - rootB * displacement) / (rootA - rootB);
  const coefficientB = displacement - coefficientA;
  const termA = coefficientA * Math.exp(rootA * deltaSeconds);
  const termB = coefficientB * Math.exp(rootB * deltaSeconds);

  return {
    value: config.target + termA + termB,
    velocity: rootA * termA + rootB * termB,
  };
}
```

Animation driver:

```ts
export interface SpringController {
  setTarget(target: number): void;
  snap(value: number): void;
  dispose(): void;
}

export function createSpringController(
  initialValue: number,
  token: SpringToken,
  onUpdate: (value: number) => void,
  options: {
    restDelta?: number;
    restVelocity?: number;
  } = {},
): SpringController {
  const restDelta = options.restDelta ?? 0.01;
  const restVelocity = options.restVelocity ?? 0.05;

  let state: SpringState = { value: initialValue, velocity: 0 };
  let target = initialValue;
  let frame = 0;
  let previousTimestamp: number | null = null;

  const tick = (timestamp: number) => {
    const deltaSeconds =
      previousTimestamp === null
        ? 0
        : Math.min((timestamp - previousTimestamp) / 1000, 0.064);

    previousTimestamp = timestamp;

    state = stepSpring(
      state,
      {
        target,
        stiffness: token.stiffness,
        dampingRatio: token.dampingRatio,
      },
      deltaSeconds,
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
    setTarget(nextTarget) {
      target = nextTarget;

      if (!frame) {
        previousTimestamp = null;
        frame = requestAnimationFrame(tick);
      }
    },

    snap(value) {
      cancelAnimationFrame(frame);
      frame = 0;
      previousTimestamp = null;
      target = value;
      state = { value, velocity: 0 };
      onUpdate(value);
    },

    dispose() {
      cancelAnimationFrame(frame);
      frame = 0;
      previousTimestamp = null;
    },
  };
}
```

Choose `restDelta` and `restVelocity` in units appropriate to the animated property. For example, a pixel position and normalized opacity should not necessarily share the same thresholds.

For gestures that require preserving an externally measured release velocity, extend the controller with a method that sets both `target` and `state.velocity` atomically.

## 50. CSS Motion Tokens

```css
:root {
  /* Verified compatibility-duration tokens. */
  --motion-duration-short-1: 50ms;
  --motion-duration-short-2: 100ms;
  --motion-duration-short-3: 150ms;
  --motion-duration-short-4: 200ms;
  --motion-duration-medium-1: 250ms;
  --motion-duration-medium-2: 300ms;
  --motion-duration-medium-3: 350ms;
  --motion-duration-medium-4: 400ms;
  --motion-duration-long-1: 450ms;
  --motion-duration-long-2: 500ms;
  --motion-duration-long-3: 550ms;
  --motion-duration-long-4: 600ms;

  /* Verified curves from the current first-party token source. */
  --motion-ease-standard: cubic-bezier(0.2, 0, 0, 1);
  --motion-ease-standard-decelerate: cubic-bezier(0, 0, 0, 1);
  --motion-ease-standard-accelerate: cubic-bezier(0.3, 0, 1, 1);
  --motion-ease-emphasized-decelerate: cubic-bezier(0.1, 0.7, 0.1, 1);
  --motion-ease-emphasized-accelerate: cubic-bezier(0.3, 0, 0.8, 0.2);

  /* Implementation-level fallback windows; not official spring durations. */
  --motion-spring-fast-window: 400ms;
  --motion-spring-default-window: 420ms;
  --motion-spring-slow-window: 560ms;
}
```

Define property-specific transitions explicitly:

```css
.interactive-surface {
  transition:
    transform var(--motion-duration-short-4)
      var(--motion-ease-emphasized-decelerate),
    opacity var(--motion-duration-short-3)
      var(--motion-ease-standard),
    border-radius var(--motion-duration-short-4)
      var(--motion-ease-emphasized-decelerate);
}
```

Avoid `transition: all`. It silently animates unrelated properties, can trigger layout or paint work, and makes component behavior difficult to predict.

## 51. CSS Spring Approximations With `linear()`

CSS `linear()` can approximate sampled spring trajectories, including overshoot. Chrome's documentation describes this technique for curves that a single cubic Bézier cannot represent. [S16]

The following curves were derived by sampling the verified expressive spring equations using zero initial velocity. The windows are implementation choices, not official Android durations.

```css
:root {
  --motion-spring-expressive-fast: linear(
    0, 0.091, 0.294, 0.527, 0.738, 0.902,
    1.011, 1.071, 1.094, 1.091, 1.074, 1.052,
    1.031, 1.014, 1.002, 0.995, 0.992, 0.991,
    0.992, 0.994, 0.996, 0.998, 1, 1, 1
  );

  --motion-spring-expressive-default: linear(
    0, 0.048, 0.161, 0.301, 0.444, 0.576,
    0.69, 0.784, 0.858, 0.913, 0.953, 0.98,
    0.997, 1.008, 1.013, 1.015, 1.015, 1.013,
    1.011, 1.009, 1.007, 1.005, 1.004, 1.003, 1
  );

  --motion-spring-expressive-slow: linear(
    0, 0.046, 0.153, 0.287, 0.426, 0.556,
    0.67, 0.765, 0.84, 0.898, 0.941, 0.971,
    0.991, 1.004, 1.011, 1.014, 1.015, 1.014,
    1.013, 1.011, 1.008, 1.006, 1.005, 1.003, 1
  );
}

.pressable {
  transition:
    scale var(--motion-spring-fast-window)
      var(--motion-spring-expressive-fast),
    opacity var(--motion-duration-short-3)
      var(--motion-ease-standard);
}

.pressable:active {
  scale: 0.97;
}
```

Limitations:

- A sampled CSS curve does not automatically preserve the original physical velocity when a transition is interrupted.
- The curve is derived for a zero-velocity unit step.
- Actual gesture velocity requires a live spring system.
- Apply overshooting curves only to spatial properties.
- Check `linear()` support before relying on it in a required interaction.

Fallback:

```css
.pressable {
  transition:
    scale 200ms var(--motion-ease-emphasized-decelerate);
}

@supports (animation-timing-function: linear(0, 1)) {
  .pressable {
    transition:
      scale var(--motion-spring-fast-window)
        var(--motion-spring-expressive-fast);
  }
}
```

## 52. Same-Document View Transitions

The View Transition API captures an old visual state, performs a DOM update, captures the new state, and lets CSS animate the transition between them. Chrome's first-party documentation covers same-document transitions, named shared elements, transition types, interruption, and failure handling. [S17]

### 52.1 Robust Navigation Wrapper

```ts
type TransitionKind =
  | 'top-level'
  | 'forward'
  | 'backward'
  | 'shared-surface';

export async function transitionView(
  update: () => void | Promise<void>,
  kind: TransitionKind,
): Promise<void> {
  const reduceMotion = window.matchMedia(
    '(prefers-reduced-motion: reduce)',
  ).matches;

  if (reduceMotion || !document.startViewTransition) {
    await update();
    return;
  }

  const root = document.documentElement;
  root.dataset.motionTransition = kind;

  let transition: ViewTransition;

  try {
    transition = document.startViewTransition(async () => {
      await update();
    });
  } catch {
    delete root.dataset.motionTransition;
    await update();
    return;
  }

  try {
    await transition.finished;
  } finally {
    delete root.dataset.motionTransition;
  }
}
```

The wrapper deliberately uses the callback form for broad compatibility. When the installed browser supports transition types, `document.startViewTransition({ update, types })` and `:active-view-transition-type()` provide a more structured alternative. [S17]

### 52.2 Isolate Persistent Surfaces

```css
.app-shell {
  view-transition-name: app-shell;
}

.destination-surface {
  view-transition-name: destination;
}

::view-transition-group(app-shell) {
  animation-duration: 0ms;
}
```

Name only elements that need independent continuity. Every simultaneously captured `view-transition-name` must be unique. Duplicate names can cause the transition animation to be skipped even though the DOM update succeeds. [S17]

### 52.3 Top-Level Fade Through

```css
html[data-motion-transition='top-level']::view-transition-old(destination) {
  animation: destination-exit 140ms
    var(--motion-ease-emphasized-accelerate) both;
}

html[data-motion-transition='top-level']::view-transition-new(destination) {
  animation: destination-enter 280ms
    var(--motion-ease-emphasized-decelerate) 70ms both;
}

@keyframes destination-exit {
  to {
    opacity: 0;
    transform: translateY(-8px);
  }
}

@keyframes destination-enter {
  from {
    opacity: 0;
    transform: translateY(16px);
  }
}
```

The `140 ms`, `280 ms`, and `70 ms` values are component-level choreography choices, not official Material token values.

### 52.4 Forward and Backward Hierarchy

```css
html[data-motion-transition='forward']::view-transition-old(destination) {
  animation: parent-recede 180ms
    var(--motion-ease-emphasized-accelerate) both;
}

html[data-motion-transition='forward']::view-transition-new(destination) {
  animation: child-approach 340ms
    var(--motion-ease-emphasized-decelerate) both;
}

html[data-motion-transition='backward']::view-transition-old(destination) {
  animation: child-depart 180ms
    var(--motion-ease-emphasized-accelerate) both;
}

html[data-motion-transition='backward']::view-transition-new(destination) {
  animation: parent-return 300ms
    var(--motion-ease-emphasized-decelerate) both;
}

@keyframes parent-recede {
  to { opacity: 0; transform: scale(0.97); }
}

@keyframes child-approach {
  from { opacity: 0; transform: scale(1.025); }
}

@keyframes child-depart {
  to { opacity: 0; transform: scale(1.025); }
}

@keyframes parent-return {
  from { opacity: 0; transform: scale(0.97); }
}
```

### 52.5 Shared Surface Identity

```css
[data-shared-surface='selected'] {
  view-transition-name: selected-surface;
}

::view-transition-group(selected-surface) {
  animation-duration: 420ms;
  animation-timing-function: var(--motion-spring-expressive-default);
}
```

Assign the shared name only to the active source and corresponding destination. Remove it after the transition to prevent collisions during later captures.

### 52.6 Failure Handling

- `transition.ready` can reject when animation capture fails.
- A rejected animation does not necessarily mean the DOM update failed.
- Logical navigation must remain correct if the visual transition is skipped.
- Avoid waiting indefinitely for fonts, images, or data before updating the DOM.
- Do not start a competing transition for every rapid state change.

## 53. Cross-Document View Transitions

For same-origin multi-page navigation, both documents may opt into cross-document transitions: [S18]

```css
@view-transition {
  navigation: auto;
}
```

Requirements:

- Both participating pages must opt in.
- Navigation must satisfy the platform's same-origin requirements.
- Corresponding shared elements need stable names.
- Navigation must remain functional where the feature is unsupported.
- Reduced-motion preferences must still suppress unnecessary movement.

Use progressive enhancement. Do not make application correctness depend on cross-document animation support.

## 54. Dialog Entry and Exit With Modern CSS

```css
.expressive-dialog {
  opacity: 0;
  transform: translateY(16px) scale(0.97);
  transition:
    opacity 180ms var(--motion-ease-standard),
    transform 320ms var(--motion-ease-emphasized-decelerate),
    overlay 320ms allow-discrete,
    display 320ms allow-discrete;
}

.expressive-dialog[open] {
  opacity: 1;
  transform: translateY(0) scale(1);
}

@starting-style {
  .expressive-dialog[open] {
    opacity: 0;
    transform: translateY(16px) scale(0.97);
  }
}

.expressive-dialog::backdrop {
  opacity: 0;
  transition:
    opacity 180ms var(--motion-ease-standard),
    overlay 320ms allow-discrete,
    display 320ms allow-discrete;
}

.expressive-dialog[open]::backdrop {
  opacity: 1;
}

@starting-style {
  .expressive-dialog[open]::backdrop {
    opacity: 0;
  }
}
```

`@starting-style` supplies an entry state for newly visible elements, while discrete transitions can keep `display`/`overlay` coordinated with an exit animation in supporting browsers. Test support and retain a functional immediate-state fallback. [S19]

## 55. Intrinsic Size and Disclosure Animation

Progressive enhancement:

```css
@supports (interpolate-size: allow-keywords) {
  :root {
    interpolate-size: allow-keywords;
  }

  .disclosure-body {
    height: 0;
    overflow: clip;
    transition:
      height 300ms var(--motion-ease-emphasized-decelerate),
      opacity 180ms var(--motion-ease-standard);
    opacity: 0;
  }

  .disclosure-body[data-expanded='true'] {
    height: auto;
    opacity: 1;
  }
}
```

Remember that intrinsic-size animation can still require layout work. Use it for modest disclosure surfaces, not dozens of simultaneously animating containers in a large virtualized list. [S11]

## 56. Svelte 5 Integration

Svelte provides a `Spring` class, `Spring.of`, `preserveMomentum`, and a reactive `prefersReducedMotion` value in `svelte/motion`. The current documentation recommends the `Spring` class over the legacy `spring` store. [S20]

```svelte
<script lang="ts">
  import { Spring, prefersReducedMotion } from 'svelte/motion';

  let pressed = $state(false);

  // Svelte's coefficients are library-specific tuning values.
  // They are not Android damping ratios or Android stiffness units.
  const scale = new Spring(1, {
    stiffness: 0.18,
    damping: 0.58,
    precision: 0.001,
  });

  $effect(() => {
    const target = pressed ? 0.97 : 1;

    if (prefersReducedMotion.current) {
      scale.set(target, { instant: true });
      return;
    }

    scale.target = target;
  });
</script>

<button
  onpointerdown={() => (pressed = true)}
  onpointerup={() => (pressed = false)}
  onpointercancel={() => (pressed = false)}
  onpointerleave={() => (pressed = false)}
  style:transform={`scale(${scale.current})`}
>
  Action
</button>
```

Important:

- The example's Svelte `stiffness: 0.18` and `damping: 0.58` are tuning suggestions, not official Material constants.
- Svelte's spring options do not use the same numeric model as Android `SpringForce`.
- Do not paste Android `stiffness: 800` into `new Spring(...)` and expect an equivalent response.
- Use the exact JavaScript integrator from Section 49 when strict first-party physical constants are required.
- Use `{ preserveMomentum: milliseconds }` only when the interaction calls for intentional velocity continuation.

An exact-token Svelte adapter can store the integrator output in a reactive state value and apply it through a transform style while retaining Svelte's built-in reduced-motion preference.

## 57. Jetpack Compose Integration

Google's Androidify example demonstrates opting into `MaterialExpressiveTheme`, selecting `MotionScheme.expressive()`, and retrieving specs from `MaterialTheme.motionScheme`. Check the installed Material 3 artifact because expressive APIs and opt-in annotations have moved across releases. [S09, S10]

```kotlin
@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
fun ExpressiveApp(content: @Composable () -> Unit) {
    MaterialExpressiveTheme(
        motionScheme = MotionScheme.expressive()
    ) {
        content()
    }
}
```

Use semantic themed specs:

```kotlin
@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
fun ExpressiveScale(pressed: Boolean) {
    val scale by animateFloatAsState(
        targetValue = if (pressed) 0.97f else 1f,
        animationSpec = MaterialTheme.motionScheme.fastSpatialSpec(),
        label = "expressive-scale"
    )

    val alpha by animateFloatAsState(
        targetValue = if (pressed) 0.88f else 1f,
        animationSpec = MaterialTheme.motionScheme.fastEffectsSpec(),
        label = "bounded-alpha"
    )

    Box(
        modifier = Modifier.graphicsLayer {
            scaleX = scale
            scaleY = scale
            this.alpha = alpha.coerceIn(0f, 1f)
        }
    )
}
```

Available semantic methods include: [S21]

```kotlin
MaterialTheme.motionScheme.fastSpatialSpec<Float>()
MaterialTheme.motionScheme.fastEffectsSpec<Float>()
MaterialTheme.motionScheme.defaultSpatialSpec<Float>()
MaterialTheme.motionScheme.defaultEffectsSpec<Float>()
MaterialTheme.motionScheme.slowSpatialSpec<Float>()
MaterialTheme.motionScheme.slowEffectsSpec<Float>()
```

Use a custom spring only when a component genuinely requires a motion identity outside the shared scheme:

```kotlin
val customSpatial = spring<Float>(
    dampingRatio = 0.8f,
    stiffness = 380f
)
```

Prefer theme-derived specs for consistency and future compatibility.

## 58. Compose Shared Elements

Google documents `SharedTransitionLayout`, `sharedElement`, and `sharedBounds` as the key building blocks for shared transitions. [S07]

Conceptual structure:

```kotlin
@OptIn(ExperimentalSharedTransitionApi::class)
@Composable
fun SharedSurfaceHost(selected: Boolean) {
    SharedTransitionLayout {
        AnimatedContent(
            targetState = selected,
            label = "destination"
        ) { isSelected ->
            if (isSelected) {
                DetailSurface(
                    sharedTransitionScope = this@SharedTransitionLayout,
                    animatedVisibilityScope = this@AnimatedContent
                )
            } else {
                ListSurface(
                    sharedTransitionScope = this@SharedTransitionLayout,
                    animatedVisibilityScope = this@AnimatedContent
                )
            }
        }
    }
}
```

Implementation requirements:

- Use the same stable key in source and destination.
- Use `sharedElement` for genuinely matching content.
- Use `sharedBounds` when the visual content changes but the container remains conceptually continuous.
- Account for overlay clipping and ancestor clipping.
- Keep scroll position stable while capturing the starting bounds.
- Avoid combining an unsupported predictive-back full-screen scale with navigation-stack integrations that do not support it.

## 59. Android Views and Material Components

Material Components Android exposes six theme attributes: [S05]

```xml
?attr/motionSpringFastSpatial
?attr/motionSpringFastEffects
?attr/motionSpringDefaultSpatial
?attr/motionSpringDefaultEffects
?attr/motionSpringSlowSpatial
?attr/motionSpringSlowEffects
```

The Material 3 Expressive theme maps these attributes to expressive spring styles. The standard Material 3 theme maps them to standard spring styles. [S02, S03]

Example:

```kotlin
val springForce = MotionUtils.resolveThemeSpringForce(
    context,
    com.google.android.material.R.attr.motionSpringDefaultSpatial
)

SpringAnimation(
    view,
    DynamicAnimation.TRANSLATION_Y,
    targetY
).apply {
    spring = SpringForce(targetY).apply {
        dampingRatio = springForce.dampingRatio
        stiffness = springForce.stiffness
    }
    setStartVelocity(releaseVelocity)
    start()
}
```

The physics motion system is documented as available in Material Components Android `1.13.0` or later. Verify the actual installed dependency and API signatures before implementation. [S05]

## 60. Reduced Motion

The `prefers-reduced-motion` media query detects an operating-system preference to minimize motion. Google's web guidance explicitly notes that applications should honor equivalent platform accessibility settings. [S22]

### 60.1 Required Behavior

Remove or substantially reduce:

- Full-screen translation.
- Large zoom transitions.
- Parallax.
- Elastic overscroll.
- Repeated bouncing.
- Decorative stagger.
- Auto-playing ambient animation.
- Large-area shimmer.
- Continuous wavy progress motion when a static alternative is adequate.
- Radial wipes, spinning surfaces, and rapid perspective changes.

Preserve:

- Immediate state changes.
- Clear focus appearance.
- Relevant loading status.
- Visible validation and error feedback.
- Drag functionality.
- Navigation continuity through a short bounded fade when useful.

### 60.2 CSS Implementation

```css
@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    scroll-behavior: auto !important;
  }

  [data-motion='decorative'] {
    animation: none !important;
    transition: none !important;
  }

  [data-motion='essential'] {
    transition-property: opacity, background-color, color;
    transition-duration: 80ms;
    transition-timing-function: linear;
  }

  ::view-transition-group(*),
  ::view-transition-old(*),
  ::view-transition-new(*) {
    animation-duration: 1ms !important;
  }
}
```

The `80 ms` essential fade and `1 ms` transition suppression are implementation choices, not official Material constants.

Avoid globally setting every animation duration to `0ms` without considering components that rely on completion events, focus coordination, or discrete visibility transitions. Logical state changes must remain correct even when animation is skipped.

### 60.3 JavaScript

```ts
const reducedMotionQuery = window.matchMedia(
  '(prefers-reduced-motion: reduce)',
);

export function shouldReduceMotion(): boolean {
  return reducedMotionQuery.matches;
}

export function observeReducedMotion(
  callback: (reduced: boolean) => void,
): () => void {
  const listener = (event: MediaQueryListEvent) => {
    callback(event.matches);
  };

  reducedMotionQuery.addEventListener('change', listener);

  return () => {
    reducedMotionQuery.removeEventListener('change', listener);
  };
}
```

If the preference changes while an animation is running, snap decorative motion to its destination and preserve application state.

## 61. Accessibility Beyond Reduced Motion

### 61.1 Focus

- Never hide the visible focus indicator during a transition.
- Do not animate keyboard focus to an element that is not yet mounted.
- Move focus according to logical interaction, not visual choreography alone.
- Restore focus after a transient surface closes.

### 61.2 Screen Readers

- Update semantic state promptly.
- Do not announce intermediate animation frames.
- Avoid repeatedly mounting/unmounting live-region content.
- Ensure a visually hidden exiting surface is not simultaneously exposed as active content when inappropriate.

### 61.3 Vestibular Safety

- Avoid large moving backgrounds behind fixed foreground content.
- Avoid unexpected camera-like zoom.
- Avoid rapid alternating or oscillatory page movement.
- Do not use animation as the only way to convey navigation direction.

### 61.4 Flashing

Avoid rapid flashes, high-contrast pulses, and repeated full-screen opacity changes. Treat attention-grabbing error and success motion as a bounded, optional enhancement.

## 62. Performance Requirements

The nominal frame budget depends on refresh rate:

```text
60 Hz  → 16.67 ms per frame
90 Hz  → 11.11 ms per frame
120 Hz →  8.33 ms per frame
144 Hz →  6.94 ms per frame
```

These are mathematical frame intervals, not an assumption that all of the interval is available to application JavaScript.

Google's web performance guidance recommends prioritizing `transform` and `opacity` because they can often avoid layout and paint work. Its Interaction to Next Paint guidance defines `200 ms` or less as a good responsiveness threshold. [S23, S24]

### 62.1 Prefer Compositor-Friendly Properties

Prefer:

```css
transform: translate3d(...);
transform: scale(...);
opacity: ...;
```

Use with caution:

```css
width
height
top
left
margin
box-shadow
filter
backdrop-filter
clip-path
border-radius
```

“Use with caution” does not mean never use. Shape morphing and container transforms sometimes require properties outside the cheapest compositor path. Profile the actual component and contain the cost.

### 62.2 Layout Projection

For major list or surface reflow:

1. Measure initial geometry.
2. Apply the final logical layout.
3. Calculate the visual difference.
4. Temporarily invert that difference with a transform.
5. Animate the transform back to identity.

This first-last-invert-play strategy can preserve the impression of layout movement without recalculating the full layout on every frame.

### 62.3 `will-change`

Use it selectively:

```css
.actively-animating {
  will-change: transform, opacity;
}
```

Remove it after the interaction if the element does not animate frequently. Excessive permanent layer promotion consumes memory and can make performance worse. [S23]

### 62.4 Avoid Layout Thrashing

Incorrect pattern:

```ts
for (const element of elements) {
  const rect = element.getBoundingClientRect();
  element.style.width = `${rect.width + 1}px`;
}
```

Correct pattern:

```ts
const measurements = elements.map((element) => ({
  element,
  rect: element.getBoundingClientRect(),
}));

requestAnimationFrame(() => {
  for (const { element, rect } of measurements) {
    element.style.setProperty('--target-width', `${rect.width + 1}px`);
  }
});
```

Batch layout reads separately from writes.

### 62.5 Compose Rendering

For frequently changing visual values, read animation state as late as possible in the rendering pipeline. Google's Compose performance guidance notes that draw-phase reads can avoid unnecessary recomposition and layout work. [S25]

Prefer:

```kotlin
Modifier.graphicsLayer {
    translationY = animatedOffset
    alpha = animatedAlpha
}
```

when the effect does not require remeasuring the component.

### 62.6 Background Work

- Pause ambient loops when the page is hidden.
- Stop observing elements after they unmount.
- Cancel animation frames on disposal.
- Avoid creating a separate perpetual frame loop for every animated control.
- Coalesce high-frequency data updates.
- Do not block input handlers with expensive synchronous layout or rendering work.

## 63. Web Feature Detection

```ts
export const motionCapabilities = {
  viewTransitions:
    typeof document !== 'undefined' &&
    typeof document.startViewTransition === 'function',

  linearEasing:
    typeof CSS !== 'undefined' &&
    CSS.supports('animation-timing-function', 'linear(0, 1)'),

  intrinsicSizeInterpolation:
    typeof CSS !== 'undefined' &&
    CSS.supports('interpolate-size', 'allow-keywords'),

  scrollDrivenAnimations:
    typeof CSS !== 'undefined' &&
    CSS.supports('animation-timeline', 'scroll()'),
};
```

Use capability checks rather than browser-brand checks. Feature availability can vary by browser version, rendering engine, and deployment environment.

## 64. Route Loading and Asynchronous Data

Navigation animation must remain coherent when destination data is delayed.

Rules:

1. Capture the outgoing state immediately after navigation intent is accepted.
2. Mount a structurally stable destination shell without waiting indefinitely.
3. Keep persistent shell elements mounted.
4. Render placeholders that match the final destination geometry.
5. Fade content into its existing placeholders when data arrives.
6. Never replay the entire route-entry transition for each arriving request.
7. Cancel stale destination transitions when a newer navigation wins.

If the destination is already cached, use the full transition immediately. If it is not cached, prioritize prompt response over a perfectly staged reveal.

## 65. Motion Configuration Contract

```ts
export interface MotionPreferences {
  scheme: 'expressive' | 'standard';
  reducedMotion: boolean;
  allowAmbientMotion: boolean;
  allowSharedElementTransitions: boolean;
  intensity: 'subtle' | 'default' | 'expressive';
}

export const defaultMotionPreferences: MotionPreferences = {
  scheme: 'expressive',
  reducedMotion: false,
  allowAmbientMotion: false,
  allowSharedElementTransitions: true,
  intensity: 'default',
};
```

Recommended intensity policy:

```text
subtle:
  Prefer standard spatial springs.
  Remove decorative stagger and overshoot.

default:
  Use expressive springs for meaningful spatial changes.
  Use standard springs for repetitive dense interactions.

expressive:
  Use expressive spatial springs more broadly.
  Still prohibit unbounded effects and excessive page motion.
```

Reduced motion overrides all three intensity settings.

## 66. Motion Anti-Patterns

Avoid:

1. Applying the fast expressive spring to every full-screen route change.
2. Animating opacity with `ζ = 0.6`.
3. Treating a spring as if it had an official fixed duration.
4. Copying Android damping ratios directly into libraries that expect damping coefficients.
5. Passing Android stiffness values directly into Svelte's differently scaled spring API.
6. Using different hard-coded curves for every component.
7. Restarting list entrance motion whenever data updates.
8. Sliding unrelated top-level destinations horizontally.
9. Animating the persistent shell during every navigation.
10. Scaling text until it becomes blurry or distorted.
11. Delaying focus or interaction until a transition completes.
12. Requiring animation completion for logical state correctness.
13. Assigning duplicate shared-element identifiers.
14. Applying `transition: all` globally.
15. Keeping `will-change` permanently enabled on a large number of elements.
16. Animating layout-heavy properties on dozens of list items simultaneously.
17. Allowing background animation loops to continue when hidden.
18. Using hover as the entire motion strategy while ignoring actual navigation and state transitions.
19. Calling visual interpolation “predictive back” without gesture preview, cancellation, and correct destination semantics.
20. Presenting local choreography recommendations as official Google specifications.

## 67. Verification Checklist

### 67.1 Token Correctness

- [ ] Expressive fast spatial is `ζ = 0.6`, `k = 800`.
- [ ] Expressive default spatial is `ζ = 0.8`, `k = 380`.
- [ ] Expressive slow spatial is `ζ = 0.8`, `k = 200`.
- [ ] All effects springs use `ζ = 1.0`.
- [ ] Fast/default/slow effects stiffness values are `3800`, `1600`, and `800`.
- [ ] Standard spatial springs use `ζ = 0.9`.
- [ ] Standard spatial stiffness values are `1400`, `700`, and `300`.
- [ ] Fixed durations are described as legacy/fallback tokens, not spring durations.
- [ ] Current easing control points are resolved from the installed token source.

### 67.2 Navigation

- [ ] Top-level destinations use a truthful, non-directional transition.
- [ ] Ordered navigation reverses correctly.
- [ ] Parent/child navigation preserves hierarchy.
- [ ] Shared elements maintain stable unique identifiers.
- [ ] Persistent shell elements remain visually stable.
- [ ] Fast repeated navigation does not produce stale screens or duplicate transitions.

### 67.3 Gestures

- [ ] Pointer-driven surfaces follow the pointer directly.
- [ ] Release velocity is preserved.
- [ ] An active pointer interrupts existing animation.
- [ ] Gesture cancellation restores the correct logical and visual state.
- [ ] Sheet and drawer bounds remain valid.
- [ ] Predictive-back values are used only for supported predictive-back patterns.

### 67.4 Components

- [ ] Spatial properties and effects use separate spring families.
- [ ] Opacity, color channels, and determinate progress never overshoot.
- [ ] Labels and text do not stretch during layout transformations.
- [ ] Menus and popovers grow from their actual anchor.
- [ ] Dialogs and sheets remain mounted until their exit is safe to complete.
- [ ] Dynamic lists preserve stable item identity.

### 67.5 Accessibility

- [ ] Reduced-motion preferences are detected and respected.
- [ ] Decorative movement can be skipped entirely.
- [ ] Focus remains visible and logically correct.
- [ ] Screen readers are not flooded with intermediate animation updates.
- [ ] Motion is not the only indication of state.
- [ ] Critical feedback remains immediate.

### 67.6 Performance

- [ ] High-frequency movement primarily uses transform/opacity where practical.
- [ ] Expensive shape, blur, and shadow effects are profiled.
- [ ] Layout reads are batched separately from writes.
- [ ] Background animation stops when hidden.
- [ ] Animation controllers release resources on unmount.
- [ ] Interaction latency is monitored independently from animation duration.
- [ ] Refresh rates of `60 Hz`, `90 Hz`, and `120 Hz` are tested when available.

## 68. Acceptance Criteria

The motion system is acceptable when:

1. Navigation conveys hierarchy and preserves visual continuity.
2. Every major interaction can be traced to one of the six semantic spring slots.
3. Effects are bounded and do not inherit spatial bounce.
4. Large surfaces move with slower, restrained springs.
5. Small controls respond immediately and can show appropriate expressive shape behavior.
6. Rapid interruption never restarts movement from an obsolete position.
7. Gesture release preserves momentum.
8. Shared elements connect genuinely related source and destination surfaces.
9. Reduced-motion users receive the same functionality without unnecessary movement.
10. Performance remains responsive under realistic component counts and data updates.
11. Source-verified constants are not confused with recommended choreography choices.

## 69. Primary Sources

- **[S01]** [Material Components Android: First-party motion tokens, including expressive and standard spring constants, easing curves, and duration values](https://github.com/material-components/material-components-android/blob/master/lib/java/com/google/android/material/motion/res/values/tokens.xml).
- **[S02]** [Material Components Android: Spring-style bindings for standard and expressive motion schemes](https://github.com/material-components/material-components-android/blob/master/lib/java/com/google/android/material/motion/res/values/styles.xml).
- **[S03]** [Material Components Android: Theme mappings for expressive and standard spring attributes](https://github.com/material-components/material-components-android/blob/master/lib/java/com/google/android/material/theme/res/values/themes_base.xml).
- **[S04]** [Google Design: Research behind Material 3 Expressive](https://design.google/library/expressive-material-design-google-research).
- **[S05]** [Material Components Android: Motion theming, spring attributes, duration tokens, and transition patterns](https://github.com/material-components/material-components-android/blob/master/docs/theming/Motion.md).
- **[S06]** [Android Developers: Predictive-back design, scale values, fade threshold, edge margins, and gesture interpolation](https://developer.android.com/design/ui/mobile/guides/patterns/predictive-back).
- **[S07]** [Android Developers: Shared element and shared bounds transitions in Jetpack Compose](https://developer.android.com/develop/ui/compose/animation/shared-elements).
- **[S08]** [Android Developers: Partially expanded modal bottom sheets in Jetpack Compose](https://developer.android.com/develop/ui/compose/components/bottom-sheets-partial).
- **[S09]** [Android Developers Blog: Androidify implementation of MaterialExpressiveTheme and themed motion specs](https://android-developers.googleblog.com/2025/05/androidify-building-delightful-ui-with-compose.html).
- **[S10]** [Android Developers: Compose Material 3 release notes and current expressive-component API changes](https://developer.android.com/jetpack/androidx/releases/compose-material3).
- **[S11]** [Chrome for Developers: Animating intrinsic sizes using interpolate-size and calc-size](https://developer.chrome.com/docs/css-ui/animate-to-height-auto).
- **[S12]** [Chrome for Developers: Scroll-driven animations and declarative animation timelines](https://developer.chrome.com/docs/css-ui/scroll-driven-animations).
- **[S13]** [Android Developers: Material Design 3 in Compose, including ripple and stretch overscroll behavior](https://developer.android.com/develop/ui/compose/designsystems/material3).
- **[S14]** [Android Developers: Advanced Compose gesture animation, interruption, velocity tracking, and animated settling](https://developer.android.com/develop/ui/compose/animation/advanced).
- **[S15]** [Motion: Physics-based spring transitions, stiffness, damping coefficient, mass, velocity, and resting thresholds](https://motion.dev/docs/react-transitions).
- **[S16]** [Chrome for Developers: Approximating complex spring-like curves with CSS linear easing](https://developer.chrome.com/docs/css-ui/css-linear-easing-function).
- **[S17]** [Chrome for Developers: Same-document View Transition API, shared-element naming, transition types, and failure handling](https://developer.chrome.com/docs/web-platform/view-transitions/same-document).
- **[S18]** [Chrome for Developers: Same-origin cross-document view transitions](https://developer.chrome.com/docs/web-platform/view-transitions/cross-document).
- **[S19]** [Chrome for Developers: Modern CSS UI primitives, including starting styles, discrete transitions, and spring-like easing](https://developer.chrome.com/blog/new-in-web-ui-io26).
- **[S20]** [Svelte documentation: Spring, momentum preservation, and reactive reduced-motion preferences](https://svelte.dev/docs/svelte/svelte-motion).
- **[S21]** [Android Developers API reference: MotionScheme, expressive and standard schemes, and six semantic animation specs](https://developer.android.com/reference/kotlin/androidx/compose/material3/MotionScheme).
- **[S22]** [web.dev: Reduced-motion preferences and accessible animation behavior](https://web.dev/articles/prefers-reduced-motion).
- **[S23]** [web.dev: High-performance animation, transform/opacity, layer promotion, and rendering diagnostics](https://web.dev/articles/animations-guide).
- **[S24]** [web.dev: Interaction to Next Paint responsiveness thresholds](https://web.dev/articles/inp).
- **[S25]** [Android Developers: Jetpack Compose performance practices and late-phase state reads](https://developer.android.com/develop/ui/compose/performance/bestpractices).
- **[S26]** [Material Design: Material 3 Expressive motion system overview and semantic motion roles](https://m3.material.io/styles/motion/overview/how-it-works).
- **[S27]** [Material Design: Motion system specifications and spring-token model](https://m3.material.io/styles/motion/overview/specs).
- **[S28]** [Material Design: Motion theming and expressive spring integration with Jetpack Compose](https://m3.material.io/blog/m3-expressive-motion-theming).
- **[S29]** [Android Developers: Morphing rounded polygon shapes in Jetpack Compose](https://developer.android.com/develop/ui/compose/graphics/draw/shapes).
- **[S30]** [Android Developers Blog: Material 3 Expressive shape morphing and fluid motion on Wear OS](https://android-developers.googleblog.com/2025/08/introducing-material-3-expressive-for-wear-os.html).
- **[S31]** [Motion: Layout projection and shared layout identity through layoutId](https://motion.dev/docs/react-layout-animations).
- **[S32]** [Android Developers: Animated vector images and vector-based icon transitions in Jetpack Compose](https://developer.android.com/develop/ui/compose/animation/vectors).
