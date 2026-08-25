/** @type {import('tailwindcss').Config} */
export default {
  content: ['./src/**/*.{html,js,svelte,ts}'],
  darkMode: "class",
  theme: {
    extend: {
      colors: {
        primary: "#9fcaff",
        "on-primary": "#003258",
        "primary-container": "#00497d",
        "on-primary-container": "#d1e4ff",
        surface: "#111318",
        "on-surface": "#e2e2e6",
        "surface-variant": "#43474e",
        "on-surface-variant": "#c3c7cf",
        "surface-dim": "#111318",
        "surface-container-lowest": "#0c0e13",
        "surface-container-low": "#1a1c21",
        "surface-container": "#1e2025",
        "surface-container-highest": "#282a2f",
        outline: "#8d9199",
        "outline-variant": "#43474e",
        cyan: {
          DEFAULT: "#59dbc7",
          container: "#005047",
          "on-container": "#a5f2e3"
        }
      },
      fontSize: {
        'xs': ['0.8125rem', { lineHeight: '1.25rem' }],    /* 13px, 20px */
        'sm': ['0.875rem', { lineHeight: '1.25rem' }],     /* 14px, 20px */
        'base': ['0.9375rem', { lineHeight: '1.375rem' }], /* 15px, 22px */
        'lg': ['1.0625rem', { lineHeight: '1.5rem' }],     /* 17px, 24px */
        'xl': ['1.25rem', { lineHeight: '1.75rem' }],      /* 20px, 28px */
        '2xl': ['1.5rem', { lineHeight: '2rem' }],         /* 24px, 32px */
        '3xl': ['1.75rem', { lineHeight: '2.25rem' }],     /* 28px, 36px */
        '4xl': ['2.25rem', { lineHeight: '2.5rem' }],      /* 36px, 40px */
      },
      fontFamily: {
        ui: ['var(--font-ui)'],
        mono: ['var(--font-mono)']
      }
    }
  },
  plugins: [],
}
