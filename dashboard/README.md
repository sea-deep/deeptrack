# DEEPTRACK dashboard

SvelteKit operator interface for DEEPTRACK. Demo mode is an explicitly marked
judge sandbox with no connection controls. Real mode uses Web Serial and accepts
only the production gateway's protocol-1 NDJSON; it starts unknown and never
falls back to fixtures.

## Local development

Install exactly the committed dependency graph and start Vite:

```sh
npm ci
npm run dev
```

Run the complete local verification set:

```sh
npm run check
npm run build
npm run test:console
npm run audit:ui
```

## Data rules

- `SIMULATED` is never presented as live hardware evidence.
- MQ-4 values remain raw or qualitative; no ppm, %LEL, or safe-air claim is allowed.
- The water probe supplies contact evidence, not water depth.
- Route/map views remain estimated and must not be called SLAM.
- Battery voltage or percentage stays unknown until measurement hardware exists.
- Cloud authentication/history must not enter the rover safety or command path.

The implemented laptop contract is newline-delimited JSON from the USB gateway.
Binary ESP-NOW structs are rover/gateway-only and live in
`../firmware/shared/DeeptrackProtocol.h`. The removed legacy HTTP control and
in-memory telemetry endpoints are not part of the real command path.
