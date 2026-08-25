# DESIGN.md — Google-Style Material Design System

> A product-agnostic, implementation-ready specification for interfaces inspired by Google’s Material 3 and Material 3 Expressive design language.

## 1. Scope and source of truth

This document specifies visual foundations, adaptive layout, component dimensions, interaction behavior, accessibility, landing-page composition, dashboard patterns, and configurable light/dark/system themes. It is intentionally independent of application category, business rules, content, and implementation framework.

Apply Google’s public interaction and design principles; do not reproduce Google trademarks, proprietary logos, the multicolor “G,” or imply affiliation.

### 1.1 Provenance labels

| Label | Meaning | How to interpret it |
| --- | --- | --- |
| `[M3]` | Current official Material 3 / Material 3 Expressive guidance. | The authoritative design direction. |
| `[MW]` | Public Google Material Web documentation or component-token source. | A verified implementation default, not necessarily the newest Expressive specification. |
| `[W]` | W3C/WCAG normative or supporting accessibility guidance. | Apply according to the identified conformance level. |
| `[R]` | A practical, framework-neutral implementation recommendation. | A deliberate baseline; not represented as an official Google requirement. |

**Version distinction:** Google identifies `@material/web` as being in maintenance mode. Its stable components and tokens remain useful references, but they do not automatically include every current Material 3 Expressive component, shape, size, or motion behavior. When current Material guidance differs from Material Web, follow current Material guidance and explicitly document compatibility fallbacks.

### 1.2 Measurement conventions

- Material measures most interface dimensions in density-independent pixels, `dp`.
- For browser implementation, use the same numerical value in CSS pixels as the working baseline: `40 dp → 40 px`.
- Text values are documented as CSS pixels for implementation; support browser zoom and user font settings.
- Values below describe minimums or defaults unless marked as fixed.
- Maintain a `48 × 48 px` interaction target even when the visible component is smaller. `[M3]`
- Never confuse the `48 px` Material recommendation with the WCAG 2.2 AA conformance minimum of `24 × 24 CSS px`, which has specified exceptions. `[W]`

## 2. Design language

### 2.1 Visual identity

The interface should feel recognizably Google-like through:

1. Quiet, near-neutral backgrounds with carefully distinguished tonal surface layers.
2. Purposeful accent color, primarily reserved for selected states, primary actions, and meaningful emphasis.
3. Rounded shapes whose radius increases with component size and prominence.
4. Highly legible sans-serif typography with large, relaxed headlines and compact functional labels.
5. Restrained elevation; separate most surfaces using tone, space, and containment rather than heavy shadows.
6. Generous white space on editorial surfaces and efficient, readable density on working surfaces.
7. Clear state changes: hover, focus, press, selection, disabled, loading, error, and success.
8. Familiar controls that remain labeled and discoverable.
9. Subtle, responsive motion that explains state and spatial relationships.
10. Expressive shape, size, color, and motion concentrated on the moments that deserve attention.

Google’s research describes expression through **color, shape, size, motion, and containment**. Expression should make the important action easier to find; it should never obscure navigation, replace labels with ambiguity, or turn dense working screens into decorative compositions.

### 2.2 Non-negotiable composition rules

- One dominant action per visible decision area.
- One page heading and one clear explanation of the current context.
- One presentation of each fact unless the additional instance serves a different interaction.
- One global navigation pattern per breakpoint; do not show two equivalent primary navigations simultaneously.
- One visible focus treatment shared by all interactive controls.
- One consistent semantic meaning for each status color.
- One theme contract across HTML, embedded charts, illustrations, overlays, and system controls.
- Prefer proximity, alignment, and tonal grouping before introducing borders, labels, or additional explanatory copy.
- If removing an element does not reduce understanding, discoverability, or control, remove it.

### 2.3 Expressive intensity

| Surface | Expression level | Recommended treatment |
| --- | --- | --- |
| Landing hero | High | Oversized headline, large-radius containment, one vivid accent, restrained illustration or interface preview. |
| Primary call to action | High | Filled color, clear label, full or strongly rounded shape, direct feedback. |
| Onboarding and empty states | Medium | Friendly illustration, concise instruction, one meaningful next action. |
| Overview dashboard | Medium-low | Tonal cards, strong metric hierarchy, selective accent, subdued charts. |
| Data tables and detailed forms | Low | Neutral surfaces, predictable alignment, dense but accessible controls. |
| Destructive decisions and errors | Low | Direct wording, strong semantic color, minimal decorative motion. |

## 3. Design-token structure

Use three token layers:

1. **Reference tokens:** raw color values, font families, numerical spacing, radii, and durations.
2. **System tokens:** semantic roles such as `--md-sys-color-primary` and `--md-sys-shape-corner-large`.
3. **Component tokens:** optional per-component mappings such as `--md-filled-button-container-color`.

Components must consume system or component tokens. Do not hard-code colors, corner radii, shadows, type styles, or motion values inside isolated components.

### 3.1 Naming conventions

```css
--md-ref-typeface-brand
--md-ref-typeface-plain
--md-sys-color-primary
--md-sys-color-on-primary
--md-sys-color-surface-container
--md-sys-shape-corner-large
--md-sys-typescale-body-medium-size
--md-sys-motion-duration-medium-2
--ui-space-4
--ui-density-standard
--ui-color-success
```

Use `--md-*` for Material-compatible tokens. Use `--ui-*` for application-independent extensions that are not part of the official Material token set.

## 4. Color system

### 4.1 Semantic role model

| Role | Intended use | Paired foreground |
| --- | --- | --- |
| `primary` | Primary action, focus, active navigation, prominent selected state. | `on-primary` |
| `primary-container` | Selected tonal regions and emphasized supportive surfaces. | `on-primary-container` |
| `secondary` | Secondary emphasis and lower-prominence accents. | `on-secondary` |
| `secondary-container` | Tonal buttons, selected chips, and active indicators. | `on-secondary-container` |
| `tertiary` | Carefully limited complementary expression. | `on-tertiary` |
| `tertiary-container` | Decorative support, differentiated data, selected tertiary accents. | `on-tertiary-container` |
| `error` | Errors, invalid fields, destructive emphasis. | `on-error` |
| `error-container` | Error banners, validation regions, and destructive context. | `on-error-container` |
| `surface` | Main page canvas. | `on-surface` |
| `surface-container-*` | Nested panels and levels of visual containment. | `on-surface` |
| `surface-variant` | Secondary neutral regions and selected legacy components. | `on-surface-variant` |
| `outline` | Meaningful boundaries requiring visual presence. | Context-dependent; verify contrast. |
| `outline-variant` | Decorative separators and low-emphasis boundaries. | Not intended to convey information alone. |
| `inverse-surface` | Snackbar and inverse informational surfaces. | `inverse-on-surface` |
| `inverse-primary` | Accent placed on an inverse surface. | Verify against `inverse-surface`. |
| `scrim` | Blocking overlay behind a modal surface. | Not a text background. |

### 4.2 Complete reference palette

The values below are an implementable, blue-led Material-compatible **example theme**. They are not a claim that Google mandates one universal corporate palette. Replace them with a generated Material tonal scheme while preserving all role relationships.

| System role | Light | Dark |
| --- | --- | --- |
| `primary` | `#0061A4` | `#9FCAFF` |
| `on-primary` | `#FFFFFF` | `#003258` |
| `primary-container` | `#D1E4FF` | `#00497D` |
| `on-primary-container` | `#001D36` | `#D1E4FF` |
| `secondary` | `#535F70` | `#BBC7DB` |
| `on-secondary` | `#FFFFFF` | `#253140` |
| `secondary-container` | `#D7E3F7` | `#3B4858` |
| `on-secondary-container` | `#101C2B` | `#D7E3F7` |
| `tertiary` | `#6B5778` | `#D6BEE4` |
| `on-tertiary` | `#FFFFFF` | `#3B2948` |
| `tertiary-container` | `#F2DAFF` | `#523F5F` |
| `on-tertiary-container` | `#251431` | `#F2DAFF` |
| `error` | `#BA1A1A` | `#FFB4AB` |
| `on-error` | `#FFFFFF` | `#690005` |
| `error-container` | `#FFDAD6` | `#93000A` |
| `on-error-container` | `#410002` | `#FFDAD6` |
| `surface` | `#F9F9FC` | `#111318` |
| `on-surface` | `#1A1C1E` | `#E2E2E6` |
| `surface-variant` | `#DFE2EB` | `#43474E` |
| `on-surface-variant` | `#43474E` | `#C3C7CF` |
| `surface-dim` | `#DADADD` | `#111318` |
| `surface-bright` | `#F9F9FC` | `#37393E` |
| `surface-container-lowest` | `#FFFFFF` | `#0C0E13` |
| `surface-container-low` | `#F3F3F6` | `#1A1C21` |
| `surface-container` | `#EDEEF1` | `#1E2025` |
| `surface-container-high` | `#E8E8EB` | `#282A2F` |
| `surface-container-highest` | `#E2E2E6` | `#33353A` |
| `outline` | `#73777F` | `#8D9199` |
| `outline-variant` | `#C3C7CF` | `#43474E` |
| `inverse-surface` | `#2F3033` | `#E2E2E6` |
| `inverse-on-surface` | `#F1F0F4` | `#2F3033` |
| `inverse-primary` | `#9FCAFF` | `#0061A4` |
| `shadow` | `#000000` | `#000000` |
| `scrim` | `#000000` | `#000000` |

### 4.3 Optional semantic extensions

Material defines an error family but does not define universal official success or warning roles. These are useful **custom extensions** and must remain visibly separate from official `--md-sys-color-*` claims. `[R]`

| Extended role | Light | Dark |
| --- | --- | --- |
| `--ui-color-success` | `#006B5F` | `#59DBC7` |
| `--ui-color-on-success` | `#FFFFFF` | `#003731` |
| `--ui-color-success-container` | `#A5F2E3` | `#005047` |
| `--ui-color-on-success-container` | `#00201C` | `#A5F2E3` |
| `--ui-color-warning` | `#8A5100` | `#FFB95C` |
| `--ui-color-on-warning` | `#FFFFFF` | `#482A00` |
| `--ui-color-warning-container` | `#FFDEA8` | `#633B00` |
| `--ui-color-on-warning-container` | `#2B1700` | `#FFDEA8` |
| `--ui-color-information` | Same as `primary`. | Same as `primary`. |

### 4.4 Surface layering

| Layer | Recommended token | Typical use |
| --- | --- | --- |
| Canvas | `surface` | Page background and uncontained content. |
| Quiet inset | `surface-container-lowest` or `surface-container-low` | Hero inset, side navigation, subtle detail regions. |
| Standard container | `surface-container` | Cards, filter regions, grouped dashboard content. |
| Raised container | `surface-container-high` | Menus, sheets, and contextually active regions. |
| Highest neutral container | `surface-container-highest` | Strong containment and legacy filled input backgrounds. |

**Rules:**

- Use tonal differences before shadows.
- Do not stack every surface level in a single section.
- Do not assume dark mode is a simple color inversion; replace every semantic role.
- Avoid pure black and pure white as the universal dark/light canvas.
- Never use semantic color as the sole indicator of status; pair with text, icon, pattern, or placement.
- Use `on-*` pairs; do not manually select a visually similar foreground.
- Decorative `outline-variant` is allowed to be subtle, but any boundary needed to recognize a control must meet non-text contrast requirements.

### 4.5 Reference CSS

```css
:root,
:root[data-theme="light"] {
  color-scheme: light;

  --md-sys-color-primary: #0061a4;
  --md-sys-color-on-primary: #ffffff;
  --md-sys-color-primary-container: #d1e4ff;
  --md-sys-color-on-primary-container: #001d36;
  --md-sys-color-secondary: #535f70;
  --md-sys-color-on-secondary: #ffffff;
  --md-sys-color-secondary-container: #d7e3f7;
  --md-sys-color-on-secondary-container: #101c2b;
  --md-sys-color-tertiary: #6b5778;
  --md-sys-color-on-tertiary: #ffffff;
  --md-sys-color-tertiary-container: #f2daff;
  --md-sys-color-on-tertiary-container: #251431;
  --md-sys-color-error: #ba1a1a;
  --md-sys-color-on-error: #ffffff;
  --md-sys-color-error-container: #ffdad6;
  --md-sys-color-on-error-container: #410002;
  --md-sys-color-surface: #f9f9fc;
  --md-sys-color-on-surface: #1a1c1e;
  --md-sys-color-surface-variant: #dfe2eb;
  --md-sys-color-on-surface-variant: #43474e;
  --md-sys-color-surface-dim: #dadadd;
  --md-sys-color-surface-bright: #f9f9fc;
  --md-sys-color-surface-container-lowest: #ffffff;
  --md-sys-color-surface-container-low: #f3f3f6;
  --md-sys-color-surface-container: #edeef1;
  --md-sys-color-surface-container-high: #e8e8eb;
  --md-sys-color-surface-container-highest: #e2e2e6;
  --md-sys-color-outline: #73777f;
  --md-sys-color-outline-variant: #c3c7cf;
  --md-sys-color-inverse-surface: #2f3033;
  --md-sys-color-inverse-on-surface: #f1f0f4;
  --md-sys-color-inverse-primary: #9fcaff;
  --md-sys-color-shadow: #000000;
  --md-sys-color-scrim: #000000;

  --ui-color-success: #006b5f;
  --ui-color-on-success: #ffffff;
  --ui-color-success-container: #a5f2e3;
  --ui-color-on-success-container: #00201c;
  --ui-color-warning: #8a5100;
  --ui-color-on-warning: #ffffff;
  --ui-color-warning-container: #ffdea8;
  --ui-color-on-warning-container: #2b1700;
}

:root[data-theme="dark"] {
  color-scheme: dark;

  --md-sys-color-primary: #9fcaff;
  --md-sys-color-on-primary: #003258;
  --md-sys-color-primary-container: #00497d;
  --md-sys-color-on-primary-container: #d1e4ff;
  --md-sys-color-secondary: #bbc7db;
  --md-sys-color-on-secondary: #253140;
  --md-sys-color-secondary-container: #3b4858;
  --md-sys-color-on-secondary-container: #d7e3f7;
  --md-sys-color-tertiary: #d6bee4;
  --md-sys-color-on-tertiary: #3b2948;
  --md-sys-color-tertiary-container: #523f5f;
  --md-sys-color-on-tertiary-container: #f2daff;
  --md-sys-color-error: #ffb4ab;
  --md-sys-color-on-error: #690005;
  --md-sys-color-error-container: #93000a;
  --md-sys-color-on-error-container: #ffdad6;
  --md-sys-color-surface: #111318;
  --md-sys-color-on-surface: #e2e2e6;
  --md-sys-color-surface-variant: #43474e;
  --md-sys-color-on-surface-variant: #c3c7cf;
  --md-sys-color-surface-dim: #111318;
  --md-sys-color-surface-bright: #37393e;
  --md-sys-color-surface-container-lowest: #0c0e13;
  --md-sys-color-surface-container-low: #1a1c21;
  --md-sys-color-surface-container: #1e2025;
  --md-sys-color-surface-container-high: #282a2f;
  --md-sys-color-surface-container-highest: #33353a;
  --md-sys-color-outline: #8d9199;
  --md-sys-color-outline-variant: #43474e;
  --md-sys-color-inverse-surface: #e2e2e6;
  --md-sys-color-inverse-on-surface: #2f3033;
  --md-sys-color-inverse-primary: #0061a4;
  --md-sys-color-shadow: #000000;
  --md-sys-color-scrim: #000000;

  --ui-color-success: #59dbc7;
  --ui-color-on-success: #003731;
  --ui-color-success-container: #005047;
  --ui-color-on-success-container: #a5f2e3;
  --ui-color-warning: #ffb95c;
  --ui-color-on-warning: #482a00;
  --ui-color-warning-container: #633b00;
  --ui-color-on-warning-container: #ffdea8;
}
```

## 5. Typography

### 5.1 Typeface system

| Layer | Recommended family | Fallback |
| --- | --- | --- |
| Brand/display | `"Google Sans Flex"` or `"Google Sans"` where validly licensed and available. | `"Roboto Flex"`, `"Roboto"`, `system-ui`, sans-serif. |
| Functional/body | `"Google Sans Flex"`, `"Roboto Flex"`, or `"Roboto"`. | `system-ui`, sans-serif. |
| Numeric-heavy surfaces | Same functional family with tabular numerals enabled. | `system-ui` with `font-variant-numeric: tabular-nums`. |
| Code or fixed-width values | `"Google Sans Code"` where available or an appropriate monospace family. | `ui-monospace`, monospace. |

The public Material Web default is Roboto. A Google-like interface does not depend on reproducing a proprietary typeface; rounded geometry, coherent type hierarchy, and excellent spacing matter more.

### 5.2 Complete baseline Material 3 type scale

| Role | Size | Line height | Weight | Tracking | Typical use |
| --- | ---: | ---: | ---: | ---: | --- |
| Display large | `57 px` | `64 px` | `400` | `-0.25 px` | Exceptional hero headline. |
| Display medium | `45 px` | `52 px` | `400` | `0` | Standard large landing headline. |
| Display small | `36 px` | `44 px` | `400` | `0` | Compact hero or major overview number. |
| Headline large | `32 px` | `40 px` | `400` | `0` | Major section heading. |
| Headline medium | `28 px` | `36 px` | `400` | `0` | Page title or prominent section. |
| Headline small | `24 px` | `32 px` | `400` | `0` | Dashboard section title. |
| Title large | `22 px` | `28 px` | `400` | `0` | Dialog title, large card title. |
| Title medium | `16 px` | `24 px` | `500` | `0.15 px` | List title, form section title. |
| Title small | `14 px` | `20 px` | `500` | `0.10 px` | Dense section and compact card title. |
| Body large | `16 px` | `24 px` | `400` | `0.50 px` | Main readable body copy. |
| Body medium | `14 px` | `20 px` | `400` | `0.25 px` | Standard dashboard content and form support. |
| Body small | `12 px` | `16 px` | `400` | `0.40 px` | Compact supporting information. |
| Label large | `14 px` | `20 px` | `500` | `0.10 px` | Buttons, selected chips, important control labels. |
| Label medium | `12 px` | `16 px` | `500` | `0.50 px` | Metadata and small component labels. |
| Label small | `11 px` | `16 px` | `500` | `0.50 px` | Rare, nonessential annotations. |

Material 3 Expressive also supports emphasized styles. Treat emphasized type as an intentional variant of the same semantic role; do not create an unrelated second scale or use heavy emphasis on every heading.

### 5.3 Type behavior

- Body text defaults to `14–16 px`; reserve `12 px` for supporting information.
- Never make `11 px` labels essential to completing an action.
- Maintain approximately `45–75 characters` per line for long-form readable text. `[R]`
- Limit a hero headline to approximately `10–14 words` and typically `2–4 lines`. `[R]`
- Use sentence case for labels, buttons, tabs, headings, and navigation.
- Avoid full uppercase except short established abbreviations.
- Prefer font weight `400` or `500`; reserve `600–700` for deliberate emphasis.
- Use `font-variant-numeric: tabular-nums` for aligned metrics, timestamps, prices, and numeric tables.
- Preserve text selection on informational surfaces.
- Support text enlargement, browser zoom, user overrides, localization, and right-to-left scripts.

```css
:root {
  --md-ref-typeface-brand:
    "Google Sans Flex", "Roboto Flex", "Roboto", system-ui, sans-serif;
  --md-ref-typeface-plain:
    "Roboto Flex", "Roboto", system-ui, sans-serif;
  --ui-typeface-mono:
    "Google Sans Code", ui-monospace, SFMono-Regular, monospace;

  --md-sys-typescale-display-large-size: 3.5625rem;
  --md-sys-typescale-display-large-line-height: 4rem;
  --md-sys-typescale-headline-medium-size: 1.75rem;
  --md-sys-typescale-headline-medium-line-height: 2.25rem;
  --md-sys-typescale-body-large-size: 1rem;
  --md-sys-typescale-body-large-line-height: 1.5rem;
  --md-sys-typescale-body-medium-size: 0.875rem;
  --md-sys-typescale-body-medium-line-height: 1.25rem;
  --md-sys-typescale-label-large-size: 0.875rem;
  --md-sys-typescale-label-large-line-height: 1.25rem;
}

.ui-numeric {
  font-variant-numeric: tabular-nums lining-nums;
}
```

## 6. Spacing and layout rhythm

### 6.1 Spacing scale

Use a `4 px` base increment, with `8 px` as the dominant visual rhythm.

| Token | Value | Common use |
| --- | ---: | --- |
| `--ui-space-0` | `0 px` | Flush alignment. |
| `--ui-space-1` | `4 px` | Tight icon or metadata separation. |
| `--ui-space-2` | `8 px` | Related inline elements and compact gaps. |
| `--ui-space-3` | `12 px` | Dense control padding and compact stack gaps. |
| `--ui-space-4` | `16 px` | Standard component padding and mobile page margins. |
| `--ui-space-5` | `20 px` | Intermediate card padding and expressive controls. |
| `--ui-space-6` | `24 px` | Standard card padding, section gaps, tablet margins. |
| `--ui-space-8` | `32 px` | Major group separation and desktop content padding. |
| `--ui-space-10` | `40 px` | Spacious grouped layout. |
| `--ui-space-12` | `48 px` | Major section separation and recommended hit target. |
| `--ui-space-16` | `64 px` | Landing sections and large editorial spacing. |
| `--ui-space-20` | `80 px` | Large marketing section spacing. |
| `--ui-space-24` | `96 px` | Desktop hero and major landing-page rhythm. |
| `--ui-space-32` | `128 px` | Exceptional large-screen editorial spacing. |

### 6.2 Spacing relationships

- Icon-to-label gap: `8 px`. `[R]`
- Supporting label-to-control gap: `4–8 px`. `[R]`
- Adjacent chip/control gap: `8 px`. `[R]`
- Form field-to-field gap: `16–24 px`. `[R]`
- Card internal padding: `16 px` compact; `24 px` standard; `32 px` spacious. `[R]`
- Dashboard card gap: `16 px` compact; `24 px` expanded. `[R]`
- Desktop page gutter: `24–40 px`; compact gutter: `16 px`. `[R]`
- Landing section separation: `64–128 px` depending on available width. `[R]`
- Separate unrelated groups using at least twice the spacing used inside each group.

### 6.3 Density modes

| Density | Control height | Table/list row | Card padding | Intended setting |
| --- | ---: | ---: | ---: | --- |
| Comfortable | `48–56 px` | `56–72 px` | `24–32 px` | Touch-heavy or spacious reading surfaces. |
| Standard | `40–48 px` | `48–56 px` | `16–24 px` | Default general-purpose layout. |
| Compact | `32–40 px` visible | `40–48 px` | `12–16 px` | Explicit desktop preference for data-heavy work. |

- Density changes spacing, not typography below legible minimums.
- Keep the actual pointer target at `48 px` wherever feasible, even when the visible control is smaller.
- Never make compact density the default on touch-first layouts.
- Persist a user-selected density consistently across comparable surfaces.
- Changing density must not reset selection, filters, scroll context, or focus.

## 7. Adaptive breakpoints and grid

### 7.1 Official window-width classes

| Window class | Width | Recommended columns | Recommended margins | Navigation baseline |
| --- | --- | ---: | --- | --- |
| Compact | `< 600 px` | `4` | `16 px` | Bottom navigation or compact top-level menu. |
| Medium | `600–839 px` | `8` | `24 px` | Navigation rail or appropriately collapsed navigation. |
| Expanded | `840–1199 px` | `12` | `24–32 px` | Rail, drawer, and optional two-pane layouts. |
| Large | `1200–1599 px` | `12` | `32–40 px` | Persistent navigation and two/three meaningful panes. |
| Extra-large | `≥ 1600 px` | `12–16` | `40–64 px` | Multipane workspace with bounded reading widths. |

Width boundaries are official current Android/Material adaptive size classes. Column counts, gutters, and exact navigation mapping are implementation recommendations. `[M3][R]`

### 7.2 Official height classes

| Height class | Height | Design implication |
| --- | --- | --- |
| Compact height | `< 480 px` | Avoid tall app bars, large hero locks, and stacked persistent chrome. |
| Medium height | `480–899 px` | Standard scrolling and page composition. |
| Expanded height | `≥ 900 px` | Optional persistent supporting panes and more generous vertical rhythm. |

Calculate classes from the actual available window, not the device model. Re-evaluate them when the browser is resized or the viewport changes.

### 7.3 Content widths

| Content type | Recommended maximum |
| --- | ---: |
| Long-form prose | `640–760 px` |
| Centered form | `480–720 px` |
| Single-column settings | `720–960 px` |
| Landing content frame | `1200–1440 px` |
| Standard dashboard frame | `1440–1680 px` |
| Data-heavy workspace | Expand as useful; prevent columns and prose from becoming unreadably wide. |
| Supporting detail pane | `320–420 px` |
| Persistent navigation drawer | `280–360 px` |

### 7.4 Canonical adaptive patterns

1. **Feed/grid:** one column at compact width; multiple appropriately sized columns as space increases.
2. **List–detail:** one visible pane at compact width; list and detail side by side when both remain usable.
3. **Supporting pane:** one primary content area with an optional contextual pane at expanded widths.
4. **Multipane workspace:** three or more panes only when every pane contains independently useful content.

Preserve selection, text input, scroll position, open dialogs, and user context across breakpoint changes. Never reset the interface merely because its layout adapts.

```css
.ui-dashboard-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 16px;
}

@media (min-width: 600px) {
  .ui-dashboard-grid {
    grid-template-columns: repeat(8, minmax(0, 1fr));
    gap: 20px;
  }
}

@media (min-width: 840px) {
  .ui-dashboard-grid {
    grid-template-columns: repeat(12, minmax(0, 1fr));
    gap: 24px;
  }
}
```

### 7.5 Container-aware adaptation

- Use viewport size classes for global navigation and page scaffold decisions.
- Use container width for local cards, charts, toolbars, and filter groups.
- A dashboard card placed in a narrow side pane must use compact internal layout even on a large monitor.
- Prefer wrapping or moving secondary actions to overflow over shrinking labels.
- Account for safe-area insets using `env(safe-area-inset-*)` on fixed navigation and floating surfaces.
- Preserve natural reading order; do not rearrange keyboard order to imitate a purely decorative grid.

## 8. Shape

### 8.1 Material corner-radius scale

| Shape role | Radius | Typical use |
| --- | ---: | --- |
| None | `0 px` | Flush containers and hard-aligned media. |
| Extra small | `4 px` | Baseline Material Web text fields and menus. |
| Small | `8 px` | Chips and compact contained controls. |
| Medium | `12 px` | Standard cards and contained list groups. |
| Large | `16 px` | Prominent cards and larger contained controls. |
| Large increased | `20 px` | Expressive intermediate containers. |
| Extra large | `28 px` | Dialogs, standard large sheets, floating containers. |
| Extra large increased | `32 px` | Expressive feature cards and large floating toolbars. |
| Extra extra large | `48 px` | Large hero containers and highly expressive surfaces. |
| Full | `9999 px` or `50%` | Pill buttons, active indicators, circular controls, avatars. |

The `20 px`, `32 px`, and `48 px` roles reflect expanded current Material shape guidance; older public Material Web examples may expose only the earlier scale.

```css
:root {
  --md-sys-shape-corner-none: 0px;
  --md-sys-shape-corner-extra-small: 4px;
  --md-sys-shape-corner-small: 8px;
  --md-sys-shape-corner-medium: 12px;
  --md-sys-shape-corner-large: 16px;
  --md-sys-shape-corner-large-increased: 20px;
  --md-sys-shape-corner-extra-large: 28px;
  --md-sys-shape-corner-extra-large-increased: 32px;
  --md-sys-shape-corner-extra-extra-large: 48px;
  --md-sys-shape-corner-full: 9999px;
}
```

### 8.2 Shape behavior

- Smaller components use proportionally smaller radii unless they are intentionally pill-shaped.
- Nested radii must remain visually concentric: inner radius ≈ outer radius minus inset, clamped at zero.
- Preserve component identity when customizing shape; a text field should not accidentally resemble a filled button.
- Use full rounding for primary pill actions, icon-button containers, active indicators, and avatars.
- Keep related components on the same shape family within one surface.
- Change corner shape on selection only when it improves state recognition and does not move surrounding content.

## 9. Elevation, borders, and containment

### 9.1 Elevation levels

| Level | Material depth reference | Practical use |
| --- | ---: | --- |
| Level 0 | `0 dp` | Canvas, flush cards, and most dashboard content. |
| Level 1 | `1 dp` | Resting elevated card or subtle top bar. |
| Level 2 | `3 dp` | Active card, resting FAB, or supporting raised region. |
| Level 3 | `6 dp` | Menus, dialogs, and modal sheets. |
| Level 4 | `8 dp` | Significant overlay or elevated interaction state. |
| Level 5 | `12 dp` | Highest temporary layer. |

Depth references do not map literally to one CSS blur radius; use calibrated multi-layer shadows. Prefer tonal surface-container roles, especially in dark mode.

```css
:root {
  --ui-elevation-0: none;
  --ui-elevation-1:
    0 1px 2px rgb(0 0 0 / 0.16),
    0 1px 3px 1px rgb(0 0 0 / 0.08);
  --ui-elevation-2:
    0 1px 2px rgb(0 0 0 / 0.16),
    0 2px 6px 2px rgb(0 0 0 / 0.10);
  --ui-elevation-3:
    0 4px 8px 3px rgb(0 0 0 / 0.12),
    0 1px 3px rgb(0 0 0 / 0.16);
  --ui-elevation-4:
    0 6px 10px 4px rgb(0 0 0 / 0.13),
    0 2px 3px rgb(0 0 0 / 0.16);
  --ui-elevation-5:
    0 8px 12px 6px rgb(0 0 0 / 0.14),
    0 4px 4px rgb(0 0 0 / 0.16);
}
```

### 9.2 Borders

- Standard divider or outline: `1 px`.
- Prominent keyboard focus outline: `2–3 px`, offset `2 px` where practical.
- Current Material Web outlined text-field source overrides its focused outline to `3 px`; do not universally assume a `2 px` Material Web focus width. `[MW]`
- Use `outline-variant` for decorative separators and `outline` for meaningful boundaries.
- Avoid combining a strong outline, a strong shadow, and a strongly contrasting container on the same component.
- Avoid card borders on every dashboard tile when tonal containment is already sufficient.

### 9.3 Overlay stacking order

Use one documented stacking scale instead of arbitrary component-level `z-index` values. `[R]`

| Layer | Suggested value | Examples |
| --- | ---: | --- |
| Base content | `0` | Page, cards, lists, charts. |
| Sticky content | `100` | Sticky headers and pinned table regions. |
| Persistent navigation | `200` | Rail, drawer, bottom navigation. |
| Anchored popup | `300` | Menus, select options, ordinary popovers. |
| Floating action | `400` | FAB and floating contextual toolbar. |
| Modal scrim | `500` | Blocking modal background. |
| Modal surface | `510` | Dialog or modal sheet. |
| Transient status | `600` | Snackbar when it must remain above nonmodal surfaces. |
| Active tooltip | `700` | Tooltip associated with the current foremost context. |

An overlay inside a modal belongs to that modal’s stacking context; background tooltips, menus, and snackbars must not appear visually above an active blocking dialog.

## 10. Interaction states

### 10.1 Shared state model

| State | Visual treatment | Interaction requirement |
| --- | --- | --- |
| Enabled/resting | Base token mapping. | Operable with pointer and keyboard. |
| Hover | Foreground-colored state layer at approximately `8%` opacity. | Apply only to devices that actually support hover. |
| Focus-visible | Distinct focus indicator; optional state layer around `12%` opacity. | Always perceivable when reached by keyboard. |
| Pressed | State layer around `12%` opacity; responsive ripple where appropriate. | Respond immediately; do not shift layout. |
| Dragged | Stronger state layer, around `16%`; optional elevation change. | Offer a non-drag alternative. |
| Selected | Persistent semantic container, indicator, icon, and/or label change. | Communicate state programmatically. |
| Activated/current | Persistent page or location indication. | Use appropriate current-page or selected semantics. |
| Disabled | Content around `38%` opacity; disabled container around `12%` when used. | Clearly unavailable; explain why when discoverability matters. |
| Read-only | Normal legibility with clear noneditable affordance. | Remain selectable and, where appropriate, focusable. |
| Loading | Preserve control dimensions; show a labeled progress state. | Prevent duplicate submission while preserving context. |
| Invalid/error | Error role, descriptive message, and explicit programmatic association. | Explain the correction; do not rely on red alone. |
| Successful | Brief semantic confirmation using text/icon and optional success extension. | Avoid interrupting the next action unnecessarily. |

State-opacity values are established Material-style implementation baselines. Exact token values can differ between component generations and themes. `[MW][R]`

### 10.2 State implementation

```css
:root {
  --ui-state-hover-opacity: 0.08;
  --ui-state-focus-opacity: 0.12;
  --ui-state-pressed-opacity: 0.12;
  --ui-state-dragged-opacity: 0.16;
  --ui-state-disabled-content-opacity: 0.38;
  --ui-state-disabled-container-opacity: 0.12;
}

.ui-interactive:focus-visible {
  outline: 3px solid var(--md-sys-color-primary);
  outline-offset: 2px;
}

@media (hover: hover) and (pointer: fine) {
  .ui-interactive:hover::before {
    opacity: var(--ui-state-hover-opacity);
  }
}
```

Do not apply opacity to an entire container when doing so makes essential labels fail contrast. Use disabled treatment only for genuinely disabled actions. Where explaining an unavailable action matters, a focusable “soft-disabled” pattern may be appropriate if semantics and explanation remain clear.

## 11. Iconography

### 11.1 Material Symbols

Use Google’s Material Symbols family consistently across the interface.

| Property | Baseline |
| --- | --- |
| Default visual size | `24 × 24 px` |
| Dense inline size | `18–20 px` |
| Prominent illustrative size | `32–48 px` |
| Default icon-button container | `40 × 40 px` |
| Recommended interaction target | `48 × 48 px` |
| Fill axis | `0` unselected; `1` selected when the distinction is meaningful. |
| Weight axis | Typically `400`; adjust deliberately for optical consistency. |
| Grade axis | Usually `0`; adjust when a surface or theme requires optical compensation. |
| Optical-size axis | Match the rendered icon size where practical. |

```css
.material-symbols-rounded {
  font-family: "Material Symbols Rounded";
  font-size: 24px;
  font-variation-settings:
    "FILL" 0,
    "wght" 400,
    "GRAD" 0,
    "opsz" 24;
}

.is-selected .material-symbols-rounded {
  font-variation-settings:
    "FILL" 1,
    "wght" 400,
    "GRAD" 0,
    "opsz" 24;
}
```

### 11.2 Icon behavior

- Icon-only controls must have an accessible name.
- Decorative icons must not be announced redundantly by assistive technology.
- Pair unfamiliar actions with visible labels.
- Mirror directional icons in right-to-left layouts when the underlying meaning is directional.
- Do not mirror icons whose meaning depends on a universal physical orientation.
- Keep one icon style and stroke system within a surface.
- Changing icon fill must not be the only indication of selection.
- Subset or self-host only the required glyphs where loading efficiency matters.

## 12. Motion

### 12.1 Current Material expressive direction

Current Material guidance favors a physics-based motion system with two distinct purposes:

- **Spatial motion:** position, scale, shape, and other changes that communicate spatial movement.
- **Effects motion:** opacity, color, and other effects that should remain controlled and readable.

Use spatial spring-like motion for meaningful movement and restrained effects motion for opacity/color. Never add bouncy motion to validation errors, dense tables, or critical reading flows.

### 12.2 Compatibility duration scale

These duration tokens provide a practical CSS fallback where physics-based motion is unavailable.

| Token | Duration | Typical use |
| --- | ---: | --- |
| Short 1 | `50 ms` | Immediate visual acknowledgement. |
| Short 2 | `100 ms` | Hover and small icon feedback. |
| Short 3 | `150 ms` | Compact state changes. |
| Short 4 | `200 ms` | Small menus and active indicators. |
| Medium 1 | `250 ms` | Standard control transitions. |
| Medium 2 | `300 ms` | Cards, tabs, and local surface transitions. |
| Medium 3 | `350 ms` | Sheets, supporting panes, and substantial overlays. |
| Medium 4 | `400 ms` | Larger surface changes. |
| Long 1 | `450 ms` | Large entering surfaces. |
| Long 2 | `500 ms` | Major page-level movement. |
| Long 3 | `550 ms` | Expressive large-surface motion. |
| Long 4 | `600 ms` | Deliberate editorial transitions. |
| Extra long 1 | `700 ms` | Exceptional expressive use only. |
| Extra long 2 | `800 ms` | Exceptional expressive use only. |
| Extra long 3 | `900 ms` | Exceptional expressive use only. |
| Extra long 4 | `1000 ms` | Exceptional expressive use only. |

### 12.3 Easing fallback

| Curve | CSS value | Use |
| --- | --- | --- |
| Standard | `cubic-bezier(0.2, 0, 0, 1)` | Most visible changes. |
| Standard accelerate | `cubic-bezier(0.3, 0, 1, 1)` | Exiting elements. |
| Standard decelerate | `cubic-bezier(0, 0, 0, 1)` | Entering elements. |
| Emphasized fallback | `cubic-bezier(0.2, 0, 0, 1)` | Larger expressive transitions when a spring system is unavailable. |

CSS cubic Bézier curves are compatibility approximations, not equivalent to Material’s current spring system.

### 12.4 Motion contract

- Hover/focus visual response: approximately `100–150 ms`.
- Switch/checkbox/selection change: approximately `150–250 ms`.
- Tab indicator: approximately `200–300 ms`.
- Menu open/close: approximately `150–250 ms`.
- Dialog/sheet enter: approximately `250–400 ms`.
- Toast/snackbar enter/exit: approximately `150–250 ms`.
- Animate `transform` and `opacity` when possible; avoid layout-thrashing width/height animations.
- Preserve the interaction origin so movement explains where an element came from.
- Never delay the availability of a control until a decorative animation completes.
- Stop or simplify repeating effects when the user requests reduced motion.

```css
:root {
  --md-sys-motion-duration-short-2: 100ms;
  --md-sys-motion-duration-short-4: 200ms;
  --md-sys-motion-duration-medium-2: 300ms;
  --md-sys-motion-duration-medium-4: 400ms;
  --md-sys-motion-easing-standard: cubic-bezier(0.2, 0, 0, 1);
}

@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    scroll-behavior: auto !important;
    transition-duration: 0.01ms !important;
  }
}
```

## 13. Component system

### 13.1 Global component contract

Every interactive component must implement:

1. **Anatomy:** container, visible label or accessible name, optional leading/trailing icon, state layer, and focus indicator.
2. **States:** resting, hover when supported, focus-visible, pressed, disabled, and loading/error/selected when relevant.
3. **Input parity:** pointer, keyboard, touch, and assistive-technology operation.
4. **Stable geometry:** state changes must not shift surrounding layout.
5. **Semantic theming:** all foreground and background colors come from paired theme roles.
6. **Bidirectionality:** logical properties and correct right-to-left behavior.
7. **Localization:** flexible width, text wrapping where appropriate, and no text baked into imagery.
8. **Zoom:** no clipping at `200%` browser zoom; critical content must reflow.
9. **Target size:** `48 × 48 px` recommended interaction area even when the visible control is smaller.
10. **Focus restoration:** when a transient surface closes, return focus to its invoker unless the workflow intentionally advances.

### 13.2 Master dimensional registry

Values marked `[R]` are consistent implementation baselines where the public current Material specification is not exposed as a single stable web token.

| Component | Default visible size | Internal spacing / shape | Source |
| --- | --- | --- | --- |
| Common button | `40 px` high | `24 px` horizontal; with leading icon `16 px` leading / `24 px` trailing; full shape; `18 px` icon | `[M3][MW]` |
| Large prominent button | `48–56 px` high | `24–32 px` horizontal; full shape | `[R]` |
| Icon button, current size set | `32 / 40 / 56 / 96 / 136 px` | Extra small / small-default / medium / large / extra large | `[M3]` |
| Material Web icon button | `40 × 40 px` | `24 px` icon; full shape; outlined variant `1 px` | `[MW]` |
| FAB | `56 × 56 px` | `24 px` icon; large shape | `[M3 legacy baseline]` |
| Small FAB | `40 × 40 px` | `24 px` icon | `[M3 legacy baseline]` |
| Large FAB | `96 × 96 px` | `36 px` icon | `[M3 legacy baseline]` |
| Extended FAB | `56 px` high | `16 px` leading, `20 px` trailing, `12 px` icon gap | `[R]` |
| Chip | `32 px` high | `8 px` radius, `18 px` icon, `8 px` group gap | `[MW][R]` |
| Small top app bar | `64 px` high | `16–24 px` horizontal content padding | `[R]` |
| Medium top app bar | `112 px` high | Title in lower region | `[M3 baseline]` |
| Large top app bar | `152 px` high | Title in lower region | `[M3 baseline]` |
| Navigation bar | `80 px` high | `3–5` destinations, `64 px` minimum item width | `[M3 baseline][R]` |
| Navigation rail | `80 px` wide | `3–7` primary destinations | `[M3 baseline]` |
| Navigation drawer | `280–360 px` wide | `56 px` item row, `28 px` end radius when modal | `[R]` |
| Primary tab | `48 px` high | `24 px` horizontal padding, active indicator | `[R]` |
| Icon + label tab | `64–72 px` high | `8 px` icon-label gap | `[R]` |
| Search bar | `56 px` high | `28 px` radius; `16 px` side padding | `[M3 baseline]` |
| Docked toolbar | `56–64 px` high | `8 px` between action targets | `[R]` |
| Floating toolbar | `64–80 px` outer height | Full/large rounding; optional attached FAB | `[M3 direction][R]` |
| Text field | `56 px` minimum single-line height | `16 px` horizontal; `4 px` baseline radius | `[M3][MW]` |
| Textarea | `96 px` minimum | `16 px` padding; resize or auto-grow | `[R]` |
| Select / combobox | `56 px` high | Match text-field geometry | `[MW][R]` |
| Checkbox | `18 × 18 px` visible | `2 px` radius; `48 px` target | `[M3 baseline][R]` |
| Radio | `20 × 20 px` visible | Circular; `48 px` target | `[M3 baseline][R]` |
| Switch | `52 × 32 px` visible | `16 px` off handle, `24 px` on handle; full shape | `[M3 baseline]` |
| Slider | `40–44 px` interaction height | `4 px` legacy track baseline; `20 px` handle baseline | `[R]` |
| One-line list row | `56 px` minimum | `16 px` sides | `[M3 baseline]` |
| Two-line list row | `72 px` minimum | `16 px` sides | `[M3 baseline]` |
| Three-line list row | `88 px` minimum | `16 px` sides | `[M3 baseline]` |
| List avatar | `40 × 40 px` | Full shape | `[MW]` |
| List image | `56 × 56 px` | Appropriate content radius | `[MW]` |
| Menu | `112–280 px` baseline width | `8 px` vertical padding; `4 px` baseline radius | `[M3 baseline][MW]` |
| Menu item | `48 px` minimum high | `12 px` sides, `12 px` icon gap | `[R]` |
| Tooltip | `24 px` minimum high | `8 px` horizontal, `4 px` vertical, `4 px` radius | `[R]` |
| Data-table header/row | `48 / 52 px` standard | `16 px` cells; dense `40–44 px` by opt-in | `[R]` |
| Divider | `1 px` | Inset as content alignment requires | `[MW]` |
| Standard card | Content-driven | `12–16 px` radius; `16–24 px` padding | `[R]` |
| Expressive feature card | Content-driven | `20–32 px` radius; `24–32 px` padding | `[R]` |
| Dialog | `280–560 px` preferred width | `24 px` padding; `28 px` radius | `[M3 baseline][R]` |
| Bottom sheet handle | `32 × 4 px` | Full shape; `22 px` top offset baseline | `[M3 baseline][R]` |
| Side sheet | `320–400 px` wide | `24–28 px` outer corners when floating/modal | `[R]` |
| Snackbar | `48 px` single-line; `68 px` two-line min | `8–16 px` padding; `4–12 px` radius by generation | `[M3 baseline][R]` |
| Linear progress | `4 px` legacy baseline | Full or specified track shape | `[MW/R]` |
| Circular progress | `48 × 48 px` standard | Labeled, determinate or indeterminate | `[MW/R]` |
| Avatar | `24 / 32 / 40 / 56 / 80 px` | Full shape | `[R]` |
| Badge | `6 px` dot or `16 px` numeric min | Full shape; `4 px` horizontal padding | `[R]` |

Do not treat the registry as a license to force fixed height on wrapped text. Interactive rows must grow to preserve text and target size.

## 14. Actions

### 14.1 Common buttons

#### Variants and hierarchy

| Variant | Hierarchy | Use |
| --- | --- | --- |
| Filled | Highest | The most important final action in a context. |
| Filled tonal | Medium-high | Important but not dominant action. |
| Elevated | Medium | Action needing separation from a patterned or tonal surface; use sparingly. |
| Outlined | Medium | Secondary action or choice requiring visible boundary. |
| Text | Lowest | Low-emphasis local action, dialog action, or inline control. |

#### Anatomy

- Container: default `40 px` high and fully rounded.
- Text: Label large, sentence case.
- Icon: `18 px`; leading icons are preferred over trailing unless direction/continuation is being communicated.
- Default no-icon horizontal padding: `24 px` both sides. `[MW]`
- Leading-icon padding: `16 px` leading, `24 px` trailing. `[MW]`
- Trailing-icon padding: `24 px` leading, `16 px` trailing. `[MW]`
- Recommended icon-label gap: `8 px`. `[R]`

#### Behavior

- Use concise verb-first labels.
- Never show two filled buttons competing within one decision group.
- For a link that navigates, use an anchor styled as a button; for an action, use a button.
- Enter or Space activates a focused button.
- A submitting/loading button retains its width, replaces or accompanies content with progress, exposes `aria-busy="true"` at the relevant region, and prevents repeated activation.
- Do not permanently replace a clear label with an icon after first use.
- Disabled buttons are not interactive. If users need to know why an action is unavailable, keep explanatory text nearby or use a carefully implemented focusable soft-disabled state.
- Destructive confirmation uses the error role only where the action is truly destructive or irreversible.

### 14.2 Icon buttons

#### Variants

- Standard/unfilled.
- Filled.
- Filled tonal.
- Outlined.
- Toggle icon button, which may use any compatible visual variant.

#### Size policy

Current Material 3 supports five icon-button sizes: `32, 40, 56, 96, and 136 dp`, with `40 dp` as the small/default size. On the web, keep the interaction target at least `48 × 48 px` even when the visible container is `32` or `40 px`.

#### Behavior

- Every icon-only button has an accessible action label.
- Toggle buttons expose selected state with `aria-pressed` and update the accessible label if the action changes.
- A tooltip supplements but does not replace the accessible name.
- Use filled or tonal icon buttons for selected/emphasized state; do not rely on icon fill alone.
- Preserve control order across breakpoints.
- Avoid more than `3–5` unlabeled icon actions in a row; move overflow actions to a menu.

### 14.3 Split buttons

Use only when one dominant action has closely related alternatives.

- Primary segment performs the current/default action.
- Trailing segment opens a menu of alternatives.
- Both segments are independent `48 px` interaction targets.
- Left/Right Arrow may move between segments; Enter/Space activates the focused segment; Down Arrow on the menu segment opens alternatives.
- A visible divider or shape seam must make the two targets clear.
- Do not use a split button when the alternatives are unrelated or when a simple menu button is clearer.

### 14.4 Floating action buttons

- Use a FAB for one high-priority, frequent creation or transformation action.
- Do not place a FAB on a landing page or every dashboard by habit.
- Sizes: small `40 px`, standard `56 px`, large `96 px` baseline.
- Extended FAB: `56 px` height; visible label; optional leading icon.
- Place with `16 px` compact or `24 px` expanded edge offset. `[R]`
- Account for safe areas, browser UI, bottom navigation, and on-screen keyboard.
- Hide or collapse only when the action remains discoverable and the transition is stable.
- Icon-only FAB requires an accessible label.

### 14.5 Segmented buttons

Use for `2–5` mutually exclusive options or a small multiselect set.

- Standard height: `40 px`; prominent/mobile height: `48 px`. `[R]`
- Group label is visible or programmatic.
- Single-select group uses radio-like behavior; multi-select uses toggle-button behavior.
- Arrow keys move within a single-select group; Space selects.
- Selected state uses container, icon, and/or stronger label contrast.
- Equal widths are preferred for short peer labels; content width is acceptable for longer labels.
- Do not use as page navigation when tabs are the semantically correct pattern.

## 15. Navigation and wayfinding

### 15.1 Top app bar

#### Variants

| Variant | Height | Use |
| --- | ---: | --- |
| Small | `64 px` | Default working surface. |
| Center-aligned | `64 px` | Simple compact views with few actions. |
| Medium | `112 px` | Context where title prominence matters. |
| Large | `152 px` | Content-led entry surface; collapses on scroll. |

#### Anatomy and behavior

- Leading: navigation/back control or brand/home affordance.
- Title: one line in a small bar; may use larger title region in medium/large variants.
- Trailing: up to approximately three top-level actions plus overflow.
- On downward content scroll, medium/large bars may collapse to small.
- Small top bar may gain a tonal background or subtle elevation when content passes beneath.
- Back control returns to the logical prior level, not necessarily browser history when that would be incorrect.
- Preserve stable action order at all sizes.
- Skip repetitive page title text when the same title is already unambiguously present immediately below and the bar does not need it for orientation.

### 15.2 Navigation bar

- Use at compact width for `3–5` top-level destinations.
- Baseline height: `80 px`.
- Each destination has a `24 px` icon and short visible label.
- Selected destination uses an active indicator and strong foreground.
- Destination order is stable and based on importance, not recency.
- Do not place a one-off action as a navigation destination.
- Preserve scroll/navigation state per destination when users switch between peers.
- Hide on text entry only where the on-screen keyboard and task genuinely require it; do not cause layout jump.

### 15.3 Navigation rail

- Use at medium or expanded width for `3–7` primary destinations.
- Standard width baseline: `80 px`.
- A rail may include a menu control or FAB at the top, destinations in the central region, and infrequent settings/help at the bottom.
- Labels may be persistent when space and comprehension benefit; otherwise every destination still has an accessible name and optional tooltip.
- Selected destination uses a tonal, fully rounded active indicator.
- Do not mix primary destinations with transient filters.

### 15.4 Navigation drawer

| Mode | Behavior |
| --- | --- |
| Modal | Overlays content, uses a scrim, traps focus, closes on Escape and outside click unless the workflow requires persistence. |
| Standard | Occupies layout space and remains visible; does not use a scrim. |
| Dismissible | Occupies space while open and can collapse to recover working width. |

- Recommended width: `280–360 px`.
- Destination row: `56 px` minimum, `12–16 px` horizontal inset.
- Group with whitespace and optional section labels; avoid dividers after every item.
- Selected item uses `secondary-container` or an equivalent selected tonal role.
- Drawer header content must provide orientation, not marketing clutter.

### 15.5 Tabs

- Use tabs for peer content views within the same context.
- Use primary tabs for major view changes and secondary tabs inside a contained region.
- Standard text tab: `48 px` high.
- Icon-and-label tab: `64–72 px` high.
- Active indicator: approximately `3 px` high for line-style tabs or a tonal pill for an expressive variant. `[R]`
- Keyboard: Tab enters the tab list; Left/Right Arrow changes focused tab; Home/End moves to first/last; activation may be automatic only when content updates without noticeable delay.
- Use `role="tablist"`, `role="tab"`, `aria-selected`, `aria-controls`, and `role="tabpanel"`.
- If tabs overflow, provide horizontal scrolling with visible affordance or replace the pattern; do not shrink labels below legibility.
- Do not use tabs for sequential steps.

### 15.6 Breadcrumbs

- Use on hierarchies deeper than one level when orientation or direct ancestor navigation helps.
- Show the current page as text with `aria-current="page"`, not a redundant link.
- Use a directional chevron separator that mirrors in RTL.
- Truncate the middle of a long path, preserving first, parent, and current context.
- At compact widths, allow a compact back-to-parent control if full breadcrumbs would dominate.

### 15.7 Search

#### Search bar

- Standard height: `56 px`; full shape with `28 px` radius.
- Leading search icon; optional back/menu control in active mode.
- Placeholder is a prompt, not the only accessible label.
- Trailing actions are limited to clear, voice, filter, or other directly relevant search actions.
- Use `surface-container-high` or a clearly differentiated tonal surface.

#### Behavior

- Activating search moves focus into the field and may expand a suggestion view.
- Show recent items only when useful and privacy-appropriate.
- Debounce remote suggestions; do not delay local text entry.
- Use a combobox/listbox pattern for suggestions.
- Up/Down Arrow navigates suggestions; Enter chooses; Escape closes suggestions first, then exits search mode where appropriate.
- The clear action empties the field and retains focus.
- Search results reflect the submitted or live query clearly; do not repeat the same query in multiple headings and chips.
- Empty query, no results, loading, offline, and error are distinct states.

### 15.8 Toolbars

#### Docked toolbar

- Height: `56–64 px`.
- Contains context actions, filters, view settings, and overflow—not page navigation.
- Keep visible actions to those used most often.
- Use separators only between distinct action groups.

#### Floating toolbar

- Use current Material’s floating toolbar pattern for contextual, reachable actions.
- Place in a high-salience but nonobstructive region; respect safe areas.
- Use large/full rounding and tonal/elevated containment.
- May be horizontal or vertical and may attach to a FAB.
- Collapse labels or hide infrequent controls only if access remains obvious through overflow or context.
- Do not obscure selected content, form submit actions, snackbars, or essential chart labels.
- Current Material redirects legacy bottom-app-bar guidance to its toolbar guidance; treat docked and floating toolbars as the current action-surface family rather than assuming an older bottom app bar remains a separate current pattern.

### 15.9 Pagination and infinite loading

- Use pagination where users need stable location, comparison, or shareable result sets.
- Controls: previous, next, current position, optional first/last, and optional page-size selection.
- Disable unavailable controls programmatically and visually.
- Preserve filter/sort state when changing pages.
- Infinite loading must announce newly added content, preserve focus, and offer a reachable footer or explicit “Load more.”
- Never use endless loading when users need to reach global controls at the page end.

### 15.10 Command menu or quick switcher

This is an optional dashboard convenience, not a core Material component. `[R]`

- Present as a modal search/list surface, typically `560–720 px` wide on desktop.
- Provide a visible entry point; a keyboard shortcut is supplemental.
- Common `Ctrl/⌘+K` behavior must not interfere with editable fields, browser defaults, or assistive technology.
- Search actions and destinations together only when result types are visually labeled.
- Up/Down Arrow moves results; Enter activates; Escape closes.
- Preserve focus and restore it on dismissal.
- Announce loading, no results, and result-count changes without excessive live-region chatter.

### 15.11 Step indicators

Use only for a genuinely ordered, multistep workflow. `[R]`

- Horizontal indicator: approximately `3–5` steps when labels fit.
- Vertical indicator: preferred when steps need descriptions or the viewport is narrow.
- States: upcoming, current, completed, optional, and error.
- Current step is identified with `aria-current="step"`.
- Completed prior steps are navigable only when returning is safe.
- Preserve entered values when the user moves backward.
- Step completion must not depend exclusively on color or an unlabeled icon.
- Do not use a stepper for unrelated peer views; use tabs or navigation.

## 16. Inputs and selection controls

### 16.1 Text fields

#### Variants

- **Filled:** stronger tonal container and bottom active indicator.
- **Outlined:** visible boundary around the field.

Both variants are functionally equivalent.

#### Anatomy and dimensions

- Minimum single-line height: `56 px`.
- Horizontal content padding: `16 px`; leading/trailing icons may reduce adjacent padding to `12 px`.
- Baseline Material Web container radius: `4 px` for both filled and outlined variants. `[MW]`
- Outline: `1 px` resting; current Material Web source uses `3 px` on focus. `[MW]`
- Input text: Body large.
- Floating label: Body small or equivalent.
- Leading/trailing icon: `24 px`.
- Supporting/error text: Body small, starting `4 px` below the field.

#### Behavior

- Always provide a visible or programmatic label; placeholder text is not a substitute.
- Labels float or otherwise remain visible after entry.
- Show required status in the label or supporting text and explain the convention once per form.
- Validate on submit and, when helpful, on blur; avoid error messages while a user is actively entering an incomplete value.
- Associate messages using `aria-describedby`; expose invalid state with `aria-invalid="true"`.
- On failed submit, focus the first invalid field and provide a summary when the form is long.
- Preserve user entry after errors.
- Prefix/suffix text is visually and semantically distinct from user input.
- Password reveal is a toggle button with an accessible state/action label.
- Character count is visible only when a meaningful limit exists.
- Read-only fields remain selectable; disabled fields are not used as a substitute for read-only data.
- Autofill, browser password managers, input types, `autocomplete`, and mobile input modes must work.

### 16.2 Textarea

- Minimum height: `96 px`; prefer `3–6` visible text lines.
- Use auto-grow up to a sensible maximum, then internal scrolling, or provide an explicit resize handle.
- Preserve label, error, supporting text, and count behavior from text fields.
- Avoid placing the submit action inside the text area unless the pattern is a compact composer and keyboard behavior is unambiguous.
- Define Enter behavior: textareas insert a new line; modified Enter submission must be discoverable and optional.

### 16.3 Select, combobox, and autocomplete

Use the correct pattern:

| Pattern | Use |
| --- | --- |
| Native/select-like menu | Small set of known, noneditable options. |
| Combobox | Editable entry with selectable suggestions. |
| Autocomplete | Large or remote option set where filtering materially helps. |

- Match text-field height and label behavior.
- Do not open a native-style select merely on field focus; use expected activation.
- Up/Down Arrow moves options; Enter selects; Escape closes; Home/End follows listbox guidance.
- Expose `aria-expanded`, `aria-controls`, `aria-activedescendant`, and correct combobox/listbox semantics.
- Show loading and no-match states inside the popup without making them selectable.
- Preserve typed text when a remote request fails.
- Multi-select uses chips inside or beneath the field only when removal and keyboard navigation remain clear.

### 16.4 Checkbox

- Visible control: approximately `18 × 18 px`; recommended target: `48 × 48 px`.
- States: unchecked, checked, indeterminate, disabled, error when part of an invalid group.
- Entire associated label row is clickable.
- Space toggles the focused checkbox.
- Indeterminate communicates partial selection and normally becomes checked or unchecked on activation according to the group model.
- Use for independent binary choices or multi-select lists.
- Do not use when the setting must take effect instantly and its on/off status is the central concept; use a switch.

### 16.5 Radio button

- Visible control: approximately `20 × 20 px`; recommended target: `48 × 48 px`.
- Use for one required choice among `2–7` visible alternatives.
- One option should be selected by default only when a safe, meaningful default exists.
- Arrow keys move and select within the radio group; Tab enters/leaves the group.
- Entire label row is clickable.
- Use a select/combobox when options are numerous or space is constrained.
- Never allow every option in a required radio group to appear deselected after a committed choice unless “None” is a valid option.

### 16.6 Switch

- Track: `52 × 32 px`, full shape.
- Handle: approximately `16 px` off and `24 px` on; optional status icon.
- Recommended target: at least `48 px` high.
- Use for an immediate on/off setting, not a form choice requiring a separate submit action.
- Label describes the setting, not the action; use “Notifications,” not “Turn notifications on.”
- State is communicated using position, color, optional icon, and programmatic checked state.
- Space toggles the focused switch.
- If a change cannot apply immediately, use a checkbox or explicitly reveal the required save action.

### 16.7 Slider

- Variants: continuous, discrete, centered, and range.
- The interaction track must be much taller than the visible track; provide at least `44–48 px` target height.
- Expose minimum, maximum, step, current value, and formatted value.
- Arrow keys change one step; Page Up/Down changes a larger increment; Home/End moves to bounds where appropriate.
- Show a value label during interaction when exact value matters.
- Ticks appear only for meaningful discrete stops.
- Range-slider handles must remain individually focusable and clearly labeled.
- Provide a text-field alternative where precision, direct entry, or accessibility requires it.
- Do not encode acceptable and unacceptable values using color alone.

### 16.8 Date and time inputs

- Use locale-aware formatting and first-day-of-week conventions.
- Allow keyboard entry in addition to a visual picker.
- Date grid uses grid semantics and arrow-key navigation.
- Show selected, current, disabled, and out-of-range dates distinctly.
- Range selection clearly distinguishes start, middle, and end.
- Time picker supports direct numeric entry and appropriate step constraints.
- Preserve typed values when switching between text and picker modes.
- Never require users to navigate month-by-month across long date ranges; provide direct month/year movement.

### 16.9 File input

This is a custom extension, not a core Material component. `[R]`

- Provide a visible button and native file input semantics.
- Drop zone is an enhancement, never the only method.
- State accepted types, number, and size limits before selection.
- Show each file with name, size, progress, error, retry, and remove action where relevant.
- A drag interaction has a click/keyboard equivalent.
- Announce upload progress and completion without moving focus.
- Failed files remain available for retry; do not silently discard them.

## 17. Information and containment

### 17.1 Cards

#### Variants

| Variant | Use |
| --- | --- |
| Filled | Default grouping on the page canvas; uses tonal surface. |
| Elevated | Requires separation from a similar background; use sparingly. |
| Outlined | Boundary is useful and tonal contrast would be insufficient. |
| Feature/expressive | Landing feature or key dashboard insight; stronger shape and hierarchy. |

#### Contract

- Standard radius: `12–16 px`; feature radius: `20–32 px`.
- Padding: `16 px` compact, `24 px` standard, `32 px` feature.
- Card title: Title medium or large; body: Body medium/large.
- Use a whole-card click target only when the card represents one clear destination.
- If a card contains multiple actions, the container itself should not also be an ambiguous click target.
- Hover may change surface tone/elevation only for interactive cards.
- Keep card heights content-driven unless alignment materially improves scanning.
- Do not place a card inside a card merely to create another border.
- Avoid more than two simultaneous containment cues: tone, border, elevation, shape, or large padding.

### 17.2 Chips

Material chip types:

| Type | Use |
| --- | --- |
| Assist | A compact, context-related action. |
| Filter | Include or exclude a category or condition. |
| Input | Represent user-supplied or selected information. |
| Suggestion | Present a contextually generated suggestion. |

#### Contract

- Chips appear in a **set**, not as isolated substitutes for buttons. `[MW]`
- Baseline height: `32 px`; shape: `8 px`; icon: `18 px`.
- Label: Label large.
- Gap between chips: `8 px`.
- Filter selected state uses `secondary-container` or equivalent tonal selection.
- Input-chip remove icon is an independent named action; keyboard removal follows expected Delete/Backspace behavior only when focus is on the chip.
- Arrow keys move within a managed chip set when roving focus is implemented.
- Wrap chip sets naturally; do not clip labels.
- Use chips for compact nouns/conditions; use buttons for verbs/actions requiring prominence.

### 17.3 Badges

- Dot badge: approximately `6 px`, no text.
- Numeric badge: `16 px` minimum height, full shape, `4 px` minimum horizontal padding.
- Cap large counts using an understandable representation such as `99+`.
- Badge content supplements the underlying label; assistive text must convey the combined meaning.
- Do not badge every destination; reserve badges for actionable, changed, or unread information.
- A decorative status dot must not be the sole status indication.

### 17.4 Avatars

- Standard sizes: `24, 32, 40, 56, 80 px`.
- Shape: full/circular unless a different identity system is explicitly defined.
- Fallback order: image → initials → generic person/identity icon.
- Initials use one or two grapheme-aware characters, not arbitrary string slicing.
- Status presence indicator requires a textual equivalent.
- Images should be cropped consistently and include appropriate alternative text only when the identity is not already named beside them.

### 17.5 Lists

- Row heights: one-line `56 px`, two-line `72 px`, three-line `88 px` minimum.
- Leading icon: Material Web default `18 px`; use `24 px` where alignment with the broader symbol system is preferable.
- Trailing icon: `24 px`.
- Leading avatar: `40 px`.
- Leading image: `56 × 56 px`.
- Horizontal padding: `16 px` baseline.
- Primary text uses Body large or Title medium; supporting text uses Body medium.
- Lists are for vertically related peers, not generic page layout.
- Make the entire row clickable when it performs one action.
- For multiple row actions, keep the row’s primary target clear and place secondary actions at the trailing edge or in overflow.
- Selection uses a persistent tonal state plus semantic selected/current state.
- Virtualized lists preserve keyboard navigation, accessible position information, and stable focus.

### 17.6 Dividers

- Thickness: `1 px`.
- Use `outline-variant` for decorative separation.
- Full width separates major regions; inset dividers align with text/content starts.
- Dividers are not focusable or announced.
- Prefer whitespace over repeated dividers when grouping remains clear.

### 17.7 Accordions and disclosure

This is a standard web extension consistent with Material behavior. `[R]`

- Header target: at least `48 px` high.
- Header is a button with `aria-expanded` and `aria-controls`.
- Chevron rotates or changes consistently; text label remains stable.
- Enter/Space toggles; Tab moves through headers and contained controls.
- One or many sections may remain open depending on the disclosed model; do not change this behavior unpredictably.
- Do not animate height from unknown content with jank; use grid/clip techniques or a short stable transition.
- Keep critical actions and validation visible; do not collapse a section solely because another one opened.

### 17.8 Tooltips

- Use for short supplementary labels or explanations; do not hide essential instructions in a tooltip.
- Plain tooltip: one or two short lines.
- Rich tooltip: limited title/body/action only when the user explicitly invokes or can interact with it.
- Opens after approximately `500 ms` pointer dwell; opens immediately on keyboard focus when needed. `[R]`
- Closes on Escape, pointer departure after a short grace period, or focus departure.
- Remains visible when the pointer moves over interactive rich-tooltip content.
- Does not cover the trigger.
- Tooltip text should be the action or object name, not “Click to…”.

### 17.9 Menus

- Baseline width: `112–280 px`; allow wider content only when labels/localization require it.
- Vertical padding: `8 px`; baseline Material Web shape: `4 px`; use a larger current shape only as a documented expressive customization.
- Item height: `48 px` minimum.
- Menu surface uses `surface-container` or `surface-container-high` with appropriate elevation.
- Place icons consistently; reserve trailing region for shortcut, selection, submenu arrow, or metadata.
- Up/Down Arrow moves; Home/End jumps; Enter/Space activates; Escape closes; character keys may perform typeahead.
- Focus moves into the opened menu and returns to the invoker on close.
- Submenus open through deliberate hover intent or keyboard Right Arrow, not accidental pointer crossing.
- Context menus appear near the invocation point while remaining within viewport boundaries.
- Avoid nested submenus deeper than one level on touch-oriented layouts.

### 17.10 Data tables

#### Anatomy

- Table title/context, optional search/filter toolbar, column headers, body rows, optional selection, bulk-action toolbar, pagination, and optional density control.
- Standard header: `48 px` high.
- Standard body row: `52 px` high.
- Compact body row: `40–44 px` only as an explicit density preference.
- Cell padding: `16 px`; dense `12 px`.
- Checkbox/select column: `48–56 px`.
- Numeric values align to the end and use tabular numerals.
- Text aligns to the start.
- Status, actions, and short metadata receive stable dedicated columns.

#### Behavior

- Use semantic `table`, `thead`, `tbody`, `th`, and `td` unless the experience genuinely requires an ARIA grid.
- Every header identifies its scope.
- Sort button is inside the column header; expose `aria-sort`.
- One active sort is the default; explicitly communicate multi-sort precedence if supported.
- Sticky header is allowed when it does not obscure focus or content.
- Freeze the first column only when horizontal comparison benefits and the remaining width stays usable.
- At compact widths, prefer horizontal scroll with a persistent first column, priority-column reduction, or a list-card transformation. Never hide essential columns silently.
- Row selection reveals a contextual bulk-action toolbar and selection count.
- Whole-row navigation and embedded controls must not create nested ambiguous click behavior.
- Loading uses skeleton rows of stable height; empty, filtered-empty, error, and offline states are distinct.
- Preserve sort, filters, page, selection, and focus when data refreshes whenever logically possible.

### 17.11 Tree and hierarchy view

This is a generic dashboard extension. `[R]`

- Use only when hierarchy is the primary mental model.
- Parent disclosure and row selection are separate actions.
- Right Arrow expands or enters children; Left Arrow collapses or returns to parent; Up/Down moves rows; Home/End moves to bounds.
- Expose `role="tree"`/`treeitem` and `aria-level`, `aria-expanded`, and `aria-selected` as applicable.
- Indentation step: `16–24 px`.
- Virtualization must preserve semantic level, item count where known, and focus.

### 17.12 Carousel

- Use only when users benefit from comparing a short, related set of visual items.
- Provide visible previous/next controls with `48 px` recommended targets.
- Support touch scroll/swipe as an enhancement, never as the only navigation.
- Announce the region and current position meaningfully.
- Avoid autoplay; if automatic rotation exists, provide visible pause and stop on hover/focus.
- Preserve consistent item height and aspect ratio.
- Ensure partially visible items are clearly intentional, not accidentally clipped.
- Prefer a responsive grid when users need to compare all items simultaneously.

## 18. Feedback, overlays, and progress

### 18.1 Dialogs

#### Types

- Alert dialog: short decision or acknowledgement.
- Confirmation dialog: verifies a consequential action.
- Simple dialog: compact choice list.
- Full-screen dialog: complex editing at compact width; use sparingly on desktop.

#### Anatomy and dimensions

- Preferred width: `280–560 px`; never exceed viewport minus `32 px`.
- Radius: `28 px` baseline.
- Internal padding: `24 px`.
- Icon-to-title gap: `16 px`.
- Title-to-content gap: `16 px`.
- Actions: `8 px` gap, end-aligned on wide layouts; stack when labels would compress.
- Scrim: typically black at approximately `32%` light and higher if required for dark-theme separation; verify result. `[R]`

#### Behavior

- Use native `<dialog>` where feasible.
- On open, move focus to the safest meaningful element: heading/description for informational dialogs, first field for editing, or least destructive action for confirmation.
- Trap focus within a modal dialog.
- Escape closes unless dismissal would discard a critical process; in that case, request confirmation.
- Outside-click dismissal is appropriate only for low-risk, recoverable dialogs.
- Restore focus to the invoker on close.
- Destructive action label names the consequence; avoid vague “Yes.”
- Do not use dialogs for passive success messages.
- Avoid dialog-on-dialog stacking.

### 18.2 Bottom sheets and side sheets

#### Bottom sheet

- Use for compact-width supporting choices, filters, or details.
- Top corners: `28 px` baseline.
- Drag handle: `32 × 4 px`, full shape.
- Support defined states such as hidden, partial, and expanded; do not stop at arbitrary heights.
- Scrim and focus behavior depend on modal vs standard behavior.
- Dragging has buttons/keyboard alternatives.
- Avoid placing a scrollable region inside another same-axis scrolling sheet without clear containment.

#### Side sheet

- Recommended width: `320–400 px`.
- Use for filters, details, or settings that support the main content.
- Modal side sheet overlays with scrim at constrained widths; standard side sheet occupies layout space at expanded widths.
- Preserve the same internal information and state when adapting between modal and standard modes.

### 18.3 Snackbar

- Use for brief feedback about an operation; optional single action.
- One snackbar visible at a time; queue subsequent messages.
- Baseline minimum height: `48 px` one-line, `68 px` two-line.
- Position above bottom navigation, FAB, safe area, and on-screen keyboard.
- Use `inverse-surface` with `inverse-on-surface` and `inverse-primary`.
- Default timeout: approximately `4–6 seconds` for a simple message; `6–10 seconds` when an action is present. `[R]`
- Pause timeout while hovered, focused, or when the page is not visible.
- Persistent or critical information does not auto-dismiss.
- Announce through a noninterruptive live region for status; use assertive announcements only for genuinely urgent errors.
- Action label is a verb and does not duplicate a visible destination.
- Do not include an icon unless it adds meaning.

### 18.4 Banner

This is an application-independent extension for page-level messages. `[R]`

- Appears below top navigation and above affected content.
- Contains concise message, optional leading semantic icon, and at most two actions.
- Use for persistent page-level status, policy, offline state, or required attention.
- Dismissal is available only if safe; remember dismissal appropriately.
- Use error or extended semantic containers only when the meaning matches.
- Avoid showing a banner, inline alert, and snackbar for the same event.

### 18.5 Inline validation and alerts

- Field error appears adjacent to and programmatically associated with the field.
- Page-level error summary links to affected fields for long forms.
- Informational inline alert contains a short title only if it improves scanning.
- Error copy states what happened and how to correct it.
- Do not erase valid fields when any field fails.
- Use `role="alert"` only for dynamic errors requiring immediate announcement; avoid repeated assertive announcements while typing.

### 18.6 Progress indicators

| Indicator | Use |
| --- | --- |
| Linear determinate | Known progress across a broad region or process. |
| Linear indeterminate | Unknown-duration loading tied to a surface. |
| Circular determinate | Compact known progress. |
| Circular indeterminate | Compact unknown wait near the initiating control. |

- Linear legacy baseline: `4 px` high. Current Material visual forms may vary; follow chosen generation consistently.
- Circular standard baseline: `48 × 48 px`; smaller `20–24 px` spinners may appear inside controls. `[R]`
- Give each indicator an accessible name.
- Determinate indicator exposes `aria-valuemin`, `aria-valuemax`, and `aria-valuenow`, or uses native `<progress>`.
- Do not show indeterminate animation for work that is already complete.
- Delay a spinner approximately `150–300 ms` for operations likely to finish immediately, preventing flicker. `[R]`
- For longer waits, show explanatory text, cancellability where possible, and meaningful progress.
- Four-color indeterminate Material Web progress is available, but use it only where multicolor expression does not conflict with semantic color. `[MW]`

### 18.7 Skeletons

Skeletons are a recommended loading pattern, not a core public Material component. `[R]`

- Match final content geometry closely to prevent layout shift.
- Use neutral tonal surfaces; no semantic color.
- Animate a subtle pulse or sheen only when reduced motion is not requested.
- Skeletons are hidden from the accessibility tree; expose one named busy region instead.
- Replace the entire loading region coherently; do not let individual skeleton fragments flash unpredictably.
- Use a progress indicator instead when the user is waiting on an explicit action.

### 18.8 Empty, no-results, error, and offline states

These states are not interchangeable:

| State | Explanation | Primary response |
| --- | --- | --- |
| First-use empty | Nothing has been created or connected yet. | Explain value and offer one setup/create action. |
| Cleared empty | Existing content was intentionally removed. | Confirm state; optional recovery. |
| Filtered no-results | Content may exist but current criteria exclude it. | Show active criteria and clear/adjust action. |
| Search no-results | Query has no match. | Preserve query; offer spelling/filter guidance. |
| Error | Retrieval or operation failed. | Plain explanation, retry, and alternate path where useful. |
| Offline | Network is unavailable. | Show cached state, reconnect behavior, and retry. |
| Permission-limited | User lacks access. | Explain limitation and legitimate request/help route. |

- Use one concise heading, one sentence of support, and one primary action.
- Avoid decorative illustrations in urgent error contexts.
- Preserve prior successful data beneath a nonblocking update error when doing so is accurate.
- Do not claim “No data” while a request is still loading.

### 18.9 Notification center

This is a reusable dashboard pattern, not a standalone core Material component. `[R]`

- Open as an anchored panel, side sheet, or full-screen compact view.
- Recommended desktop panel width: `360–440 px`.
- Each row contains one concise message, optional actor/icon, timestamp, and read/unread state.
- Unread indication uses tone or a badge plus a textual/accessible equivalent.
- Actions such as mark read, dismiss, or open details are individually named.
- “Mark all as read” states its scope.
- Group by time or category only when it materially helps scanning.
- Empty and permission-disabled states are distinct.
- Restoring focus on close is mandatory.
- Do not also surface every nonurgent notification as a blocking dialog or persistent banner.

### 18.10 Teaching popovers

- Show contextual education only when an unfamiliar feature becomes relevant.
- Anchor to the actual control and avoid covering the target.
- Use one short heading, one sentence, and one dismissal/next action.
- Provide keyboard access, Escape dismissal, and appropriate focus handling.
- Remember dismissal; do not repeat the same hint on every visit.
- Do not launch a multistep tour automatically before the user has expressed intent.
- Never use a coach mark to compensate for an unlabeled essential control.

## 19. Landing-page and hero patterns

### 19.1 Landing page structure

Build only the sections justified by actual content. A complete page may use:

1. Compact top navigation.
2. High-impact hero.
3. Optional proof or trust indicators.
4. One concise feature/benefit section.
5. Optional visual demonstration or interface preview.
6. Optional comparison, process, or FAQ when questions genuinely remain.
7. One closing action region.
8. Compact utility footer.

Do not repeat the same headline, explanation, screenshot, or call to action in every section. Repetition is justified only when the user reaches a distinct decision point.

### 19.2 Landing navigation

- Height: `64–72 px`.
- Content width: `1200–1440 px` maximum.
- Horizontal margin: `16 px` compact; `24–40 px` expanded.
- Leading area: a distinct, original identity or wordmark.
- Center/right: no more than `3–5` meaningful links.
- Trailing: one filled primary action and, optionally, one text or outlined action.
- Sticky behavior is optional; if sticky, add tonal containment only after scroll.
- Compact width uses one menu control rather than shrinking links below readable size.
- Theme selection is a labeled button/menu or a clearly discoverable settings entry.

### 19.3 Hero geometry

| Attribute | Compact | Expanded / large |
| --- | --- | --- |
| Content frame | Full width minus `32 px` | `1200–1440 px` maximum |
| Grid | `4` columns, stacked | `12` columns; typically `5/7`, `6/6`, or centered `8` |
| Top/bottom padding | `64–88 px` | `96–144 px` |
| Minimum useful height | Content-driven; never force clipping | Approximately `560–760 px` when viewport height permits |
| Headline | Approximately `40–57 px` | Approximately `57–88 px` expressive extension |
| Headline measure | `10–14 words` maximum recommendation | `10–14 words` maximum recommendation |
| Supporting copy | `16–18 px` | `18–20 px` |
| Copy max width | `32–40 characters` | `42–60 characters` |
| Action gap | `12 px` | `12–16 px` |
| Feature surface radius | `24–32 px` | `32–48 px` |

Hero values beyond the 15-role baseline type scale are deliberate responsive editorial extensions. `[R]`

### 19.4 Hero composition

- One short headline containing a single clear idea.
- One supporting sentence or two very short lines.
- One dominant filled action.
- Optional secondary text/outlined action only when it serves a materially different intent.
- One purposeful visual: a real interface preview, abstract system illustration, or restrained visual metaphor.
- Use neutral canvas with one controlled tonal accent.
- Gradients, if used, remain low-contrast and subordinate to the type and action.
- Keep decorative motion subtle, short, and optional.
- Place the main heading and primary action above the initial comfortable fold when practical, without forcing a viewport-height lock.
- Do not place multiple overlapping glass panels, floating badges, animated counters, testimonials, and decorative chips into the same hero.
- The preview should be simplified and legible, not a tiny unreadable screenshot.

### 19.5 Hero example

```html
<section class="ui-hero" aria-labelledby="hero-title">
  <div class="ui-hero__copy">
    <p class="ui-eyebrow">Optional context</p>
    <h1 id="hero-title">A clear, singular headline.</h1>
    <p class="ui-hero__support">
      One sentence that explains the value without repeating the heading.
    </p>
    <div class="ui-hero__actions">
      <button class="ui-button ui-button--filled">Primary action</button>
      <a class="ui-button ui-button--text" href="#details">
        Secondary action
      </a>
    </div>
  </div>
  <div class="ui-hero__visual" aria-hidden="true"></div>
</section>
```

```css
.ui-hero {
  display: grid;
  align-items: center;
  gap: clamp(32px, 6vw, 80px);
  max-width: 1320px;
  margin-inline: auto;
  padding-block: clamp(72px, 12vw, 144px);
  padding-inline: clamp(16px, 4vw, 40px);
}

.ui-hero h1 {
  max-width: 12ch;
  margin-block: 0 20px;
  color: var(--md-sys-color-on-surface);
  font-family: var(--md-ref-typeface-brand);
  font-size: clamp(2.5rem, 6.4vw, 5.25rem);
  font-weight: 500;
  letter-spacing: -0.035em;
  line-height: 1.06;
  text-wrap: balance;
}

.ui-hero__support {
  max-width: 52ch;
  color: var(--md-sys-color-on-surface-variant);
  font-size: clamp(1rem, 1.8vw, 1.25rem);
  line-height: 1.6;
}

.ui-hero__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  margin-block-start: 28px;
}

.ui-hero__visual {
  min-height: 320px;
  border-radius: var(--md-sys-shape-corner-extra-extra-large);
  background: var(--md-sys-color-surface-container-low);
}

@media (min-width: 840px) {
  .ui-hero {
    grid-template-columns: 5fr 7fr;
  }

  .ui-hero__visual {
    min-height: 480px;
  }
}
```

### 19.6 Feature sections

- Standard vertical padding: `64–112 px`.
- Section heading: Headline medium/large.
- Supporting text: Body large, one short paragraph maximum.
- Grid: `1` card compact, `2` medium, `3` expanded; do not create six nearly identical cards.
- Feature cards use `24 px` padding and `16–28 px` radius.
- Every card has one clear title, one short explanation, and optional icon/visual.
- Reuse the same icon style and visual framing.
- Alternate background tone only when it clarifies major section boundaries.
- When the interface preview already demonstrates a benefit, avoid a second paragraph explaining the visible obvious.

### 19.7 Footer

- One original identity/wordmark.
- Only necessary utility links.
- Links grouped by actual user need, not artificial column symmetry.
- Legal and secondary metadata use Body small with adequate contrast.
- No duplicate large call-to-action block if the preceding section already provides the final action.

## 20. Dashboard and workspace patterns

### 20.1 Dashboard scaffold

The dashboard consists of the following independent regions:

1. Adaptive primary navigation: bottom navigation, rail, or drawer.
2. Top app bar: page identity, search if globally useful, and key account/settings actions.
3. Optional contextual toolbar: filters, view selection, or batch actions.
4. Primary content: metrics, tables, charts, activity, or detail.
5. Optional supporting pane: contextual details without losing the main view.
6. Transient feedback layer: snackbar, dialog, menu, or sheet.

Do not show a region without a concrete job. A dashboard may contain only navigation, a title, and a meaningful content surface.

### 20.2 Dashboard spacing

| Element | Compact | Expanded |
| --- | ---: | ---: |
| Page horizontal padding | `16 px` | `24–40 px` |
| Top-bar height | `64 px` | `64–72 px` |
| Page title-to-toolbar gap | `16 px` | `16–24 px` |
| Dashboard grid gap | `16 px` | `20–24 px` |
| Standard tile padding | `16 px` | `20–24 px` |
| Major section gap | `24–32 px` | `32–48 px` |
| Supporting pane width | Modal/full-width | `320–420 px` |
| Dense row target | `48 px` minimum | `40–44 px` visible only with adequate target spacing |

### 20.3 Page header

- One heading, typically Headline medium.
- Optional concise supporting line only if the title does not explain context.
- One primary page-level action.
- Optional date range, search, or view selector when each changes visible content.
- Avoid separate page title, subtitle, banner, breadcrumbs, and card heading that all repeat the same noun.
- Preserve consistent header alignment across pages.
- As width collapses, place the primary action on a second line rather than shrinking labels to illegibility.

### 20.4 Overview metrics

#### Tile anatomy

- Short metric label.
- Prominent current value.
- Optional unit if not intrinsic to the displayed value.
- Optional meaningful comparison and direction.
- Optional miniature trend only if the trend explains something the value does not.
- Optional information action when the metric definition is not obvious.

#### Tile dimensions

- Minimum width: approximately `200–240 px`.
- Recommended height: `120–168 px`, content-driven.
- Internal padding: `20–24 px`.
- Radius: `16–24 px`.
- Value style: Headline large or Display small.
- Label style: Body medium or Title small.
- Comparison style: Body small; use semantic color only when the direction has real meaning.

#### Behavior

- Show `2–4` key metrics in one row where width permits.
- Never assign green to “up” and red to “down” without knowing whether that direction is beneficial.
- Use a clear empty/unknown state; do not substitute `0` for unavailable data.
- Animate numeric changes only when they improve comprehension and reduced-motion preference permits.
- If a tile opens details, make the action obvious and preserve a named target.
- Do not repeat the same metric in a tile, chart title, badge, and table heading without a distinct informational purpose.

### 20.5 Dashboard cards

- Group related information under a single descriptive heading.
- Place card actions at the trailing side of the header.
- Prefer a single “More” menu for infrequent actions.
- Cards can span `4, 6, 8, or 12` columns on a `12`-column layout.
- Chart cards: recommended minimum height `280–360 px`.
- Table cards: preferred minimum visible capacity `5–8` rows before pagination. `[R]`
- Activity cards: recommended minimum row target `48 px`.
- Preserve reading order in the DOM even if the visual grid changes.
- Avoid fixed equal heights when a card contains more important information than its neighbors.

### 20.6 Filters and sorting

- Place active filters close to the affected content.
- Common filters may appear as filter chips; advanced filters open a side/bottom sheet.
- Show selected state and a clear-all action only when filters are active.
- Reflect selected filter count where the actual criteria are not visible.
- Decide whether changes apply immediately or on explicit Apply; never mix both behaviors without clear distinction.
- Immediate filters update content without stealing focus.
- Explicit Apply keeps draft changes separate from committed filters and allows Cancel.
- Preserve filter state across paging and supported navigation.
- Search query, sort order, and filter criteria must be independently understandable.
- Show result count once, near the affected result set.

### 20.7 Detail panels

- Open adjacent to a list/table when width supports both.
- At compact width, open as a navigated detail view or modal bottom sheet depending on task complexity.
- Preserve the original selection and scroll position.
- Header contains a meaningful title and close/back action; do not repeat the same title in the first body block.
- Supporting metadata appears in labeled, scannable rows.
- Primary action remains reachable without obscuring content.
- On close, restore focus to the originating row.

### 20.8 Activity and timelines

- Use chronological order only where sequence matters.
- Row structure: actor/context, concise event, timestamp, optional affected object, optional action.
- Timestamp uses relative time with an exact accessible or hover-revealed value when useful.
- Group by meaningful time periods only when the list is long.
- Icons distinguish event classes without becoming a substitute for event text.
- A timeline connector is decorative; remove it if it harms scanability.
- Avoid exposing both a complete event sentence and a second metadata row that repeats the same actor and time.

### 20.9 Settings screens

- Use a single-column layout, recommended maximum width `720–960 px`.
- Group settings by user goal, not technical storage structure.
- Group title uses Title medium; optional group explanation only when necessary.
- Each setting row contains clear label, optional supporting sentence, and one control.
- Row height: `64–88 px` based on supporting content.
- Immediate controls show immediate confirmation only when the change is consequential.
- Changes requiring Save expose one clearly located persistent action and an unsaved-change indication.
- Theme choice appears as “System,” “Light,” and “Dark.”
- Search within settings only when the number of options makes scanning difficult.

### 20.10 Dashboard customization

- Allow changing card order, visibility, and density only when users gain meaningful control.
- Keep a useful default layout; customization must never be required to understand the first view.
- Drag-and-drop reordering must have equivalent visible Move up/Move down or position-selection actions.
- Preserve keyboard focus on the moved card and announce its new position.
- Let users restore the default arrangement.
- Avoid resizable tiles that produce unusable chart widths or clipped content.
- Persist customization without exposing implementation details in the interface.

## 21. Data visualization

### 21.1 Chart selection

| Question | Recommended form | Avoid |
| --- | --- | --- |
| How does one value change over time? | Line chart. | Unnecessary stacked bars or decorative gradients. |
| How do categories compare? | Horizontal or vertical bar chart. | Pie charts with many categories. |
| How do several trends compare? | Two to four directly labeled lines. | More than six crossing lines. |
| What is part-to-whole composition? | Stacked bar; donut only for very few meaningful categories. | Pie slices too small to label. |
| How are values distributed? | Histogram or box plot. | Treating arbitrary categories as continuous. |
| What is the relationship between two numbers? | Scatter plot. | Connecting unrelated observations. |
| Is a metric approaching a threshold? | Bullet chart, target line, or explicit status value. | Decorative speedometer gauges. |

### 21.2 Chart geometry

- Chart card height: `280–400 px` standard; `400–560 px` when analysis requires more space.
- Plot padding: approximately `16–24 px`.
- Axis label: Body small or Label medium; do not make essential labels smaller than `12 px`.
- Gridline thickness: `1 px` in a subtle outline-variant tone.
- Line stroke: `2–3 px`; highlighted line `3–4 px`.
- Point radius: `3–4 px` default; focus/hover `5–6 px`.
- Bar corner radius: `4–8 px` at exposed ends.
- Minimum meaningful bar width: `8 px`.
- Minimum interactive point hit target: `24 px`; prefer enlarged invisible hit areas approaching `40–48 px`.
- Tooltip padding: `12–16 px`.
- Maintain enough left/bottom margin for real localized labels, not placeholder labels.

### 21.3 Chart color and legend

- Use custom chart-series tokens derived from the theme; do not repurpose error/success/warning roles for arbitrary categories.
- Default to one accent series plus neutral comparison.
- Limit categorical series to approximately `5–7` before using grouping, filtering, or small multiples.
- Add stroke pattern, marker shape, or direct label when colors alone could be ambiguous.
- Directly label short series where possible; otherwise place a compact legend near the chart heading.
- Use muted gridlines and reserve strongest contrast for the data.
- Selected/highlighted series receives stronger contrast; other series may dim without becoming unreadable.
- Thresholds include a text label and line style; never rely only on red/green.

### 21.4 Chart interaction

- Pointer hover reveals a concise tooltip; keyboard focus exposes the same information.
- Time-series cursor aligns all visible series at the same domain value.
- Tooltip includes label, value, unit, comparison, and time only when each adds new information.
- Provide a visible time-range control only if changing range is meaningful.
- Brushing/dragging includes explicit zoom/filter controls as alternatives.
- Legends can toggle series only if this behavior is visually signaled and state is announced.
- Preserve selected time range and active series when the theme changes.
- Empty, loading, error, and insufficient-data states are distinct.
- Charts react immediately to theme changes, including canvas/SVG foreground, grid, focus, tooltip, and series colors.

### 21.5 Chart accessibility

- Provide a descriptive chart title and short summary of the meaningful trend.
- Offer the underlying data in an accessible table or equivalent structured description.
- Do not represent data meaning exclusively through color, hover, or animation.
- Give interactive points keyboard access or provide an equally complete accessible alternative.
- Maintain at least `3:1` contrast for graphical marks and focus indicators when necessary to understand the graphic.
- Use locale-aware number, percentage, and date formatting.
- Respect reduced motion and disable continuously animated transitions.

## 22. Theme configuration: system, light, and dark

### 22.1 Required theme modes

| Preference | Resolved appearance | Behavior |
| --- | --- | --- |
| `system` | Matches `prefers-color-scheme`. | Updates when the operating-system preference changes. |
| `light` | Always light. | Ignores operating-system changes. |
| `dark` | Always dark. | Ignores operating-system changes. |

The selected preference and the resolved theme are different values. Persist the preference; expose the resolved theme to CSS.

### 22.2 Theme interaction

- Present theme choice as a segmented control, settings radio group, or menu with three explicitly labeled options.
- Never make a two-position light/dark toggle the only control if system-following mode is required.
- Initial default: `system` unless a valid preference was previously stored.
- Apply theme before first paint to avoid a flash of the wrong appearance.
- Persist using a stable key such as `ui.theme`.
- Observe OS changes only while the preference is `system`.
- Synchronize changes across tabs if persistent browser storage is available.
- Handle unavailable storage gracefully.
- Update `color-scheme` so native form controls, scrollbars, and built-in browser surfaces match.
- Update browser theme-color metadata when appropriate.
- Re-render canvas charts, code highlighting, SVG, illustrations, and map/visual surfaces that do not automatically inherit CSS variables.
- Theme changes must not reset scroll position, close menus unnecessarily, clear forms, or restart unrelated requests.
- Avoid a long full-page color animation. If a transition exists, keep it short and respect reduced-motion preference.

### 22.3 First-paint-safe theme controller

Place this script in the document head before render-blocking styles. Under a strict Content Security Policy, attach the appropriate nonce or use a server-rendered theme attribute instead of permitting unsafe inline scripts.

```html
<meta name="color-scheme" content="light dark" />
<meta name="theme-color" content="#f9f9fc" />
<script>
  (() => {
    const storageKey = "ui.theme";
    const root = document.documentElement;
    const validPreferences = new Set(["system", "light", "dark"]);
    const media = window.matchMedia("(prefers-color-scheme: dark)");

    function readPreference() {
      try {
        const saved = window.localStorage.getItem(storageKey);
        return validPreferences.has(saved) ? saved : "system";
      } catch {
        return "system";
      }
    }

    function apply(preference) {
      const safePreference = validPreferences.has(preference)
        ? preference
        : "system";
      const resolved = safePreference === "system"
        ? (media.matches ? "dark" : "light")
        : safePreference;

      root.dataset.theme = resolved;
      root.dataset.themePreference = safePreference;
      root.style.colorScheme = resolved;

      const themeMeta = document.querySelector('meta[name="theme-color"]');
      if (themeMeta) {
        themeMeta.content = resolved === "dark" ? "#111318" : "#f9f9fc";
      }

      window.dispatchEvent(
        new CustomEvent("ui:themechange", {
          detail: { preference: safePreference, resolved }
        })
      );
    }

    function setTheme(preference) {
      if (!validPreferences.has(preference)) return;

      try {
        window.localStorage.setItem(storageKey, preference);
      } catch {
        // Theme remains usable even when persistent storage is unavailable.
      }

      apply(preference);
    }

    media.addEventListener("change", () => {
      if (root.dataset.themePreference === "system") {
        apply("system");
      }
    });

    window.addEventListener("storage", (event) => {
      if (event.key === storageKey) {
        apply(readPreference());
      }
    });

    window.uiTheme = { apply, set: setTheme, get: readPreference };
    apply(readPreference());
  })();
</script>
```

For a server-rendered page, resolve and emit the initial `data-theme`/`data-theme-preference` values before hydration when a trusted stored preference is available. Never render light server markup and dark client markup with incompatible layout assumptions.

### 22.4 Dark-theme adjustments

- Use deep neutral surfaces, not universal `#000000`.
- Prefer tonal separation to increasingly strong black shadows.
- Reduce overly saturated large-area accents while preserving contrast.
- Keep primary controls sufficiently distinguishable from the surrounding surface.
- Verify outlines, active indicators, chart marks, text-selection colors, and focus rings separately.
- Prefer lighter toned accent colors on dark neutral backgrounds.
- Preserve image/illustration transparency and avoid bright white embedded panels.
- Keep disabled controls distinguishable without relying on opacity that makes labels disappear.
- Honor forced-colors/high-contrast mode instead of fighting system colors.

## 23. Accessibility and inclusive interaction

### 23.1 Conformance baseline

Target WCAG 2.2 AA as the required baseline, while adopting stronger Material touch-target and visible-focus recommendations where practical.

| Requirement | Minimum / recommendation | Classification |
| --- | --- | --- |
| Normal text contrast | `4.5:1` | WCAG AA. |
| Large text contrast | `3:1` for text meeting WCAG’s large-text definition. | WCAG AA. |
| Essential UI boundaries and meaningful graphical marks | `3:1` against adjacent colors. | WCAG AA non-text contrast. |
| Pointer target | `24 × 24 CSS px` minimum, subject to documented exceptions. | WCAG 2.2 AA. |
| Material-recommended interaction target | `48 × 48 dp/px`. | Google/Material recommendation; stronger than WCAG minimum. |
| Visible keyboard focus | Always present and not obscured. | WCAG AA focus requirements. |
| Enhanced focus appearance | Strong `2–3 px` outline and `3:1` change where practical. | Recommended; specific WCAG 2.4.13 Focus Appearance criterion is AAA. |
| Reflow | Usable at approximately `320 CSS px` width without unnecessary horizontal scrolling. | WCAG AA reflow requirement, with exceptions for two-dimensional content. |
| Zoom | Core flows remain usable at `200%` zoom and under text enlargement. | Required practical verification. |
| Drag actions | Provide a single-pointer non-dragging alternative. | WCAG 2.2 AA dragging-movements requirement. |

### 23.2 Keyboard interaction

- All interactive controls are reachable without a mouse.
- Tab sequence follows meaningful document order, not purely visual positioning.
- Use native elements first: `button`, `a`, `input`, `select`, `dialog`.
- Do not assign positive `tabindex`.
- Composite components use roving tabindex or `aria-activedescendant` according to the W3C Authoring Practices Guide.
- Escape closes the topmost dismissible overlay.
- Enter/Space activation follows the element’s expected native behavior.
- Modal surfaces trap focus only while modal.
- On close, return focus to the invoking control or the next logical destination.
- Skip-to-content link appears on keyboard focus.
- Do not intercept browser shortcuts, text-editing shortcuts, or expected assistive-technology gestures.

### 23.3 Screen-reader semantics

- Every page has one meaningful `h1`.
- Heading levels reflect hierarchy without decorative skipping.
- Use landmarks: `header`, `nav`, `main`, `aside`, and `footer` when appropriate.
- Distinguish multiple navigation landmarks with accessible labels.
- Every icon-only button has an accessible name.
- Associate form labels, hints, error messages, and required state.
- Announce dynamic status through appropriately scoped live regions.
- Use `aria-live="polite"` for ordinary completion/status and assertive announcements only when immediate interruption is justified.
- Avoid duplicate announcements from an icon and an identical adjacent visible label.
- Do not hide focused elements with `aria-hidden`.
- Use `aria-busy` for updating regions; clear it promptly when loading ends.

### 23.4 Vision and color

- Verify both light and dark schemes with actual computed colors.
- Never encode selected, disabled, invalid, successful, or current state by color alone.
- Support browser and OS high-contrast / forced-colors modes.
- Maintain visible focus against both component surface and surrounding page.
- Allow text spacing overrides without clipped text:
  - Line height up to `1.5×` font size.
  - Paragraph spacing up to `2×` font size.
  - Letter spacing up to `0.12×` font size.
  - Word spacing up to `0.16×` font size.
- Avoid thin, low-contrast type below `14 px` for essential content.

### 23.5 Motion and vestibular sensitivity

- Honor `prefers-reduced-motion: reduce`.
- Replace travel/parallax with a short fade or immediate state change.
- Stop decorative looping automatically or provide a pause control.
- No flashing content above accepted safety thresholds.
- Do not animate charts continuously while the user is reading.
- Never animate layout in a way that moves the target away from the pointer.

### 23.6 Touch and pointer

- Prefer `48 × 48 px` target areas.
- Provide separation between adjacent small targets.
- Support pointer cancellation; do not trigger destructive actions on pointer-down.
- Avoid hover-only functionality.
- Gestures such as drag, swipe, pinch, and long-press have visible single-pointer alternatives.
- Tooltips and hover cards must not obscure the trigger or vanish while being intentionally accessed.
- Respect safe-area insets on edge-positioned controls.

### 23.7 Internationalization and RTL

- Use logical CSS properties such as `margin-inline-start` and `padding-block`.
- Allow approximately `30–40%` expansion for translated control labels. `[R]`
- Mirror navigation direction, chevrons, drawers, progress orientation when semantically appropriate, and horizontal keyboard traversal.
- Do not mirror universally directional media controls or icons tied to real-world orientation without checking meaning.
- Format numbers, dates, times, lists, and units with locale-aware APIs.
- Keep user-provided names and identifiers intact.
- Support long names, unbroken strings, non-Latin scripts, and mixed-direction text.

```css
@media (forced-colors: active) {
  .ui-interactive:focus-visible {
    outline: 3px solid Highlight;
    outline-offset: 2px;
  }

  .ui-button--outlined,
  .ui-field {
    border-color: CanvasText;
  }
}
```

## 24. Content and information hierarchy

### 24.1 Writing rules

- Use direct, conversational sentence case.
- Prefer meaningful nouns for destinations and clear verbs for actions.
- Action labels name the result: “Save changes,” “Apply filters,” “View details.”
- Avoid “Click here,” “Proceed,” “Submit,” or “OK” when a more specific verb is possible.
- Supporting text adds information that is absent from the heading.
- Keep labels short but not cryptic.
- Explain errors in plain language, including the next step.
- Status messages use facts, not dramatic or vague language.
- Use empty-state copy to guide action, not describe the obvious absence repeatedly.

### 24.2 Redundancy audit

Remove an element when it duplicates another without changing user understanding:

- Breadcrumb current label identical to the visible `h1` may remain only if the breadcrumb hierarchy itself is useful.
- Page heading and first card heading should not be identical.
- Metric title and unit should not be repeated in subtitle and tooltip unless context changes.
- Selected navigation label and page heading may match because they serve different wayfinding purposes; do not add a third redundant banner.
- A chart legend is unnecessary when every series is clearly directly labeled.
- A button icon is unnecessary when the label already expresses a simple familiar action and the icon adds no recognition.
- A tooltip must not repeat a visible adjacent label unless needed to clarify an icon-only control.
- A toast, banner, and inline message should not announce the same event simultaneously.
- A section must have enough content to justify its title, card, and surrounding padding.

### 24.3 Information priority

| Priority | Treatment |
| --- | --- |
| Primary | Strongest readable size/weight, strongest useful color, visible without interaction. |
| Secondary | Normal body text, nearby placement, muted but accessible foreground. |
| Tertiary | Small supporting text, disclosure, or tooltip only when still discoverable. |
| Rare/advanced | Overflow menu, expandable section, side sheet, or dedicated settings. |

Do not place advanced controls in the default scan path when they are rarely used; do not hide essential controls merely to create visual minimalism.

## 25. Interaction patterns

### 25.1 Form flow

1. Present a visible label and any essential format requirement.
2. Allow entry without premature interruption.
3. Validate at a sensible boundary.
4. Show a precise adjacent error if invalid.
5. Preserve all user-entered values.
6. On successful submission, confirm once in the least disruptive appropriate location.

### 25.2 Async action feedback

| Elapsed time | Recommended feedback |
| --- | --- |
| `0–150 ms` | Immediate pressed/selected acknowledgment; usually no spinner. |
| `150–1000 ms` | Local loading indicator if the action remains pending. |
| `1–5 s` | Visible named progress and preserved context. |
| `> 5 s` | Meaningful progress/description and cancellation or background continuation when possible. |

These are user-experience heuristics, not Material-mandated durations. `[R]`

### 25.3 Undo versus confirmation

- Prefer reversible actions with a visible Undo when consequences can be safely rolled back.
- Ask for confirmation before irreversible, costly, or externally consequential actions.
- A confirmation names the affected item and result.
- Default focus should not favor the destructive action when accidental activation is possible.
- Do not ask for confirmation on every routine reversible action.

### 25.4 Selection and bulk actions

- Clicking a row performs its primary action unless there is an explicit selection affordance.
- Checkbox selects without navigating.
- Once selection begins, reveal a contextual toolbar and count.
- “Select all” states the exact scope: current page, visible results, or all matching results.
- Indeterminate state accurately represents partial selection.
- Bulk actions remain disabled or explained when incompatible with selected items.
- Preserve or deliberately clear selection after an operation; communicate the result once.

### 25.5 Error recovery

- State what failed.
- Preserve visible context and entered data.
- Offer Retry where it can work.
- Offer an alternate route only when distinct.
- Distinguish a validation error from a network/server error.
- Avoid replacing an entire usable page with a full-screen failure for one nonessential section.

### 25.6 Scroll behavior

- Page scroll is the default; nested scroll containers require a concrete usability reason.
- Sticky bars and headers must not cover focused controls or anchor targets.
- Account for persistent headers using `scroll-margin-top`.
- Preserve scroll when opening/closing side panels.
- Avoid horizontal page scrolling except for data tables, charts, or genuinely two-dimensional content.
- Respect reduced-motion preference when implementing smooth scrolling.

## 26. Implementation-ready component recipes

### 26.1 Filled button

```css
.ui-button {
  display: inline-flex;
  position: relative;
  min-height: 40px;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding-inline: 24px;
  border: 0;
  border-radius: var(--md-sys-shape-corner-full);
  font-family: var(--md-ref-typeface-plain);
  font-size: 0.875rem;
  font-weight: 500;
  line-height: 1.25rem;
  text-decoration: none;
  cursor: pointer;
}

.ui-button--filled {
  color: var(--md-sys-color-on-primary);
  background: var(--md-sys-color-primary);
}

.ui-button--tonal {
  color: var(--md-sys-color-on-secondary-container);
  background: var(--md-sys-color-secondary-container);
}

.ui-button--outlined {
  color: var(--md-sys-color-primary);
  background: transparent;
  box-shadow: inset 0 0 0 1px var(--md-sys-color-outline);
}

.ui-button--text {
  color: var(--md-sys-color-primary);
  background: transparent;
}

.ui-button:focus-visible {
  outline: 3px solid var(--md-sys-color-primary);
  outline-offset: 3px;
}

.ui-button:disabled {
  color: color-mix(in srgb, var(--md-sys-color-on-surface) 38%, transparent);
  background: color-mix(
    in srgb,
    var(--md-sys-color-on-surface) 12%,
    transparent
  );
  cursor: not-allowed;
}
```

### 26.2 Card

```css
.ui-card {
  min-width: 0;
  padding: 24px;
  border-radius: var(--md-sys-shape-corner-large);
  color: var(--md-sys-color-on-surface);
  background: var(--md-sys-color-surface-container-low);
}

.ui-card__header {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
  margin-block-end: 20px;
}

.ui-card--interactive {
  transition:
    background-color var(--md-sys-motion-duration-short-4)
      var(--md-sys-motion-easing-standard),
    box-shadow var(--md-sys-motion-duration-short-4)
      var(--md-sys-motion-easing-standard);
}

@media (hover: hover) and (pointer: fine) {
  .ui-card--interactive:hover {
    background: var(--md-sys-color-surface-container);
    box-shadow: var(--ui-elevation-1);
  }
}
```

### 26.3 Responsive dashboard shell

```css
.ui-shell {
  display: grid;
  min-height: 100dvh;
  grid-template-columns: minmax(0, 1fr);
  background: var(--md-sys-color-surface);
  color: var(--md-sys-color-on-surface);
}

.ui-shell__main {
  min-width: 0;
}

.ui-shell__content {
  width: min(100%, 1600px);
  margin-inline: auto;
  padding: 24px 16px 96px;
}

@media (min-width: 600px) {
  .ui-shell {
    grid-template-columns: 80px minmax(0, 1fr);
  }

  .ui-shell__content {
    padding: 28px 24px;
  }
}

@media (min-width: 1200px) {
  .ui-shell {
    grid-template-columns: 280px minmax(0, 1fr);
  }

  .ui-shell__content {
    padding: 32px 40px;
  }
}
```

### 26.4 Accessible theme selector

```html
<fieldset class="ui-theme-selector">
  <legend>Appearance</legend>
  <label>
    <input type="radio" name="appearance" value="system" />
    System
  </label>
  <label>
    <input type="radio" name="appearance" value="light" />
    Light
  </label>
  <label>
    <input type="radio" name="appearance" value="dark" />
    Dark
  </label>
</fieldset>
```

```javascript
const themeSelector = document.querySelector(".ui-theme-selector");

if (themeSelector && window.uiTheme) {
  const preference = document.documentElement.dataset.themePreference;
  const selected = themeSelector.querySelector(
    'input[value="' + preference + '"]'
  );

  if (selected) selected.checked = true;

  themeSelector.addEventListener("change", (event) => {
    if (event.target instanceof HTMLInputElement) {
      window.uiTheme.set(event.target.value);
    }
  });

  window.addEventListener("ui:themechange", (event) => {
    const current = themeSelector.querySelector(
      'input[value="' + event.detail.preference + '"]'
    );
    if (current) current.checked = true;
  });
}
```

### 26.5 Accessible icon button

```html
<button
  class="ui-icon-button"
  type="button"
  aria-label="Open settings"
>
  <span class="material-symbols-rounded" aria-hidden="true">
    settings
  </span>
</button>
```

```css
.ui-icon-button {
  display: inline-grid;
  position: relative;
  width: 48px;
  height: 48px;
  place-items: center;
  padding: 4px;
  border: 0;
  border-radius: var(--md-sys-shape-corner-full);
  color: var(--md-sys-color-on-surface-variant);
  background: transparent;
}

.ui-icon-button::before {
  position: absolute;
  width: 40px;
  height: 40px;
  border-radius: inherit;
  content: "";
}
```

The visible icon-button container may remain `40 px` while the actual button target is `48 px`.

## 27. Quality-assurance checklist

### 27.1 Visual foundations

- [ ] Every color maps to a semantic light/dark token.
- [ ] Text and foreground/background pairs satisfy their contrast requirements.
- [ ] The 15 baseline type roles are available and used consistently.
- [ ] Spacing predominantly follows `4 px` increments and `8 px` visual rhythm.
- [ ] Component radii use the documented shape scale.
- [ ] Surface separation uses tone before excessive borders or shadows.
- [ ] Icons share a consistent style, size, and optical treatment.
- [ ] Expressive emphasis is concentrated on meaningful actions and hero moments.

### 27.2 Component behavior

- [ ] Each component has resting, hover, focus, pressed, and disabled behavior where relevant.
- [ ] Selected/current states remain understandable without color alone.
- [ ] Controls preserve geometry while loading or changing state.
- [ ] Menus, tabs, dialogs, comboboxes, and tables follow expected keyboard patterns.
- [ ] Modal focus is trapped appropriately and restored on dismissal.
- [ ] Validation messages are adjacent, useful, and programmatically associated.
- [ ] Snackbars do not obscure persistent navigation or primary actions.
- [ ] Drag-only actions provide a visible, non-dragging alternative.

### 27.3 Responsive behavior

- [ ] Compact, medium, expanded, large, and extra-large widths are explicitly tested.
- [ ] Navigation adapts without duplicating primary navigation.
- [ ] List-detail and supporting panes preserve context across layout changes.
- [ ] Content remains usable at `320 px` width.
- [ ] Tables and charts have an intentional narrow-screen strategy.
- [ ] Long translated labels do not overlap or clip.
- [ ] Compact-height windows do not waste space on oversized persistent chrome.

### 27.4 Theme behavior

- [ ] System, light, and dark are all available and correctly labeled.
- [ ] Resolved theme is applied before first paint.
- [ ] System mode follows operating-system changes.
- [ ] Manual light/dark choices ignore operating-system changes.
- [ ] Theme choice persists and synchronizes across browser tabs when possible.
- [ ] Native controls and scrollbars use the correct `color-scheme`.
- [ ] Charts, overlays, tooltips, focus indicators, and illustrations update correctly.
- [ ] Forced-colors and reduced-motion preferences remain respected.

### 27.5 Accessibility

- [ ] Normal text meets `4.5:1` and essential non-text graphics meet `3:1`.
- [ ] Material-preferred `48 × 48 px` targets are used wherever practical.
- [ ] Any smaller target still satisfies WCAG 2.2 AA minimum/exceptions.
- [ ] Keyboard focus is always visible and not obscured.
- [ ] Every icon-only action has an accessible name.
- [ ] Every page has meaningful landmarks and one `h1`.
- [ ] Screen-reader announcements are useful and nonduplicative.
- [ ] Zoom, reflow, text spacing, and RTL layouts are tested.
- [ ] Reduced-motion mode removes nonessential travel and looping.

### 27.6 Content and redundancy

- [ ] Each screen has one clear heading and one primary task.
- [ ] Each decision region has at most one dominant primary action.
- [ ] Headings, metric values, status messages, and explanations are not repeated without purpose.
- [ ] Empty, no-results, error, offline, and permission states are clearly differentiated.
- [ ] Destructive actions are explicit and routine actions are not overconfirmed.
- [ ] Every visible section and control has a distinct user-facing reason to exist.

## 28. Official sources

### 28.1 Current Material design direction

- [Material Design 3](https://m3.material.io/)
- [Material 3 Expressive launch](https://m3.material.io/blog/building-with-m3-expressive)
- [Google Design: Material 3 Expressive research](https://design.google/library/expressive-material-design-google-research)
- [Material usability foundations](https://m3.material.io/foundations/usability)
- [Applying Material 3 Expressive](https://m3.material.io/foundations/usability/applying-m-3-expressive)
- [Material design tokens](https://m3.material.io/foundations/design-tokens)
- [Material color overview](https://m3.material.io/styles/color/overview)
- [Material color roles](https://m3.material.io/styles/color/roles)
- [Material type-scale tokens](https://m3.material.io/styles/typography/type-scale-tokens)
- [Material shape corner-radius scale](https://m3.material.io/styles/shape/corner-radius-scale)
- [Material motion: how it works](https://m3.material.io/styles/motion/overview/how-it-works)
- [Material Expressive motion theming](https://m3.material.io/blog/m3-expressive-motion-theming)
- [Material layout breakpoints](https://m3.material.io/foundations/layout/breakpoints)
- [Material canonical adaptive layouts](https://m3.material.io/foundations/adaptive-design/canonical-layouts)
- [Material component catalog](https://m3.material.io/components)
- [Material icon-button guidelines and current sizes](https://m3.material.io/components/icon-buttons/guidelines)
- [Material text-field specifications](https://m3.material.io/components/text-fields/specs)
- [Material toolbar guidance](https://m3.material.io/components/toolbars/overview)
- [Material carousel guidance](https://m3.material.io/components/carousel/overview)
- [Material badge guidance](https://m3.material.io/components/badges/overview)
- [Material date-picker guidance](https://m3.material.io/components/date-pickers/overview)
- [Material time-picker guidance](https://m3.material.io/components/time-pickers/overview)
- [Material segmented-button guidance](https://m3.material.io/components/segmented-buttons/overview)

### 28.2 Google implementation references

- [Android Developers: current window-size classes](https://developer.android.com/develop/adaptive-apps/guides/use-window-size-classes)
- [Android Developers: Material minimum touch targets](https://developer.android.com/develop/ui/compose/accessibility/api-defaults)
- [Material Web repository and maintenance status](https://github.com/material-components/material-web)
- [Material Web: color roles and theming](https://material-web.dev/theming/color/)
- [Material Web: typography and type tokens](https://material-web.dev/theming/typography/)
- [Material Web: shape tokens](https://material-web.dev/theming/shape/)
- [Material Web: buttons](https://material-web.dev/components/button/)
- [Material Web filled-button spacing token source](https://github.com/material-components/material-web/blob/main/tokens/_md-comp-filled-button.scss)
- [Material Web: icon buttons](https://material-web.dev/components/icon-button/)
- [Material Web: floating action buttons](https://material-web.dev/components/fab/)
- [Material Web: text fields](https://material-web.dev/components/text-field/)
- [Material Web outlined-text-field token source](https://github.com/material-components/material-web/blob/main/tokens/_md-comp-outlined-text-field.scss)
- [Material Web: chips](https://material-web.dev/components/chip/)
- [Material Web: menus](https://material-web.dev/components/menu/)
- [Material Web: dialogs](https://material-web.dev/components/dialog/)
- [Material Web: lists](https://material-web.dev/components/list/)
- [Material Web: tabs](https://material-web.dev/components/tabs/)
- [Material Web: select](https://material-web.dev/components/select/)
- [Material Web: checkbox](https://material-web.dev/components/checkbox/)
- [Material Web: radio](https://material-web.dev/components/radio/)
- [Material Web: switch](https://material-web.dev/components/switch/)
- [Material Web: slider](https://material-web.dev/components/slider/)
- [Material Web: progress indicators](https://material-web.dev/components/progress/)
- [Material Web: ripple](https://material-web.dev/components/ripple/)
- [Google Fonts: Material Symbols guide](https://developers.google.com/fonts/docs/material_symbols)
- [Google Fonts: Google Sans Flex](https://fonts.google.com/specimen/Google%2BSans%2BFlex)
- [Google Fonts: Google Sans Code](https://fonts.google.com/specimen/Google%2BSans%2BCode)
- [Google Fonts: Roboto Flex](https://fonts.google.com/specimen/Roboto%2BFlex)

### 28.3 Accessibility and browser standards

- [W3C: WCAG 2.2 quick reference](https://www.w3.org/WAI/WCAG22/quickref/)
- [W3C: understanding minimum text contrast](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html)
- [W3C: understanding non-text contrast](https://www.w3.org/WAI/WCAG22/Understanding/non-text-contrast.html)
- [W3C: understanding minimum target size](https://www.w3.org/WAI/WCAG22/Understanding/target-size-minimum.html)
- [W3C: understanding focus appearance](https://www.w3.org/WAI/WCAG22/Understanding/focus-appearance.html)
- [W3C: understanding focus not obscured](https://www.w3.org/WAI/WCAG22/Understanding/focus-not-obscured-minimum.html)
- [W3C: understanding dragging movements](https://www.w3.org/WAI/WCAG22/Understanding/dragging-movements.html)
- [W3C: ARIA Authoring Practices Guide](https://www.w3.org/WAI/ARIA/apg/)
- [MDN: prefers-color-scheme](https://developer.mozilla.org/en-US/docs/Web/CSS/Reference/At-rules/%40media/prefers-color-scheme)
- [MDN: color-scheme](https://developer.mozilla.org/en-US/docs/Web/CSS/Reference/Properties/color-scheme)
- [MDN: ARIA live regions](https://developer.mozilla.org/en-US/docs/Web/Accessibility/ARIA/Guides/Live_regions)
- [MDN: forced-colors](https://developer.mozilla.org/en-US/docs/Web/CSS/Reference/At-rules/%40media/forced-colors)
