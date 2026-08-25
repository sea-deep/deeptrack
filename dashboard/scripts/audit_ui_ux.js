import fs from 'fs';
import path from 'path';
import { JSDOM } from 'jsdom';

console.log('🔍 [UI/UX & A11Y AUDIT SUITE] Initializing automated inspection...\n');

const componentDir = path.resolve('src/lib/components');
const routeDir = path.resolve('src/routes');

function getAllFiles(dir, ext = '.svelte') {
  let files = [];
  fs.readdirSync(dir, { withFileTypes: true }).forEach(entry => {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      files = files.concat(getAllFiles(fullPath, ext));
    } else if (entry.name.endsWith(ext)) {
      files.push(fullPath);
    }
  });
  return files;
}

const svelteFiles = [...getAllFiles(componentDir), ...getAllFiles(routeDir)];

let totalIssues = 0;
let issueList = [];

console.log(`📁 Scanning ${svelteFiles.length} Svelte UI templates...`);

for (const file of svelteFiles) {
  const content = fs.readFileSync(file, 'utf-8');
  const relPath = path.relative(process.cwd(), file);

  const htmlOnly = content
    .replace(/<script[\s\S]*?<\/script>/gi, '')
    .replace(/<style[\s\S]*?<\/style>/gi, '')
    .replace(/\{#if[\s\S]*?\}/g, '')
    .replace(/\{:else[\s\S]*?\}/g, '')
    .replace(/\{\/if\}/g, '')
    .replace(/\{#each[\s\S]*?\}/g, '')
    .replace(/\{\/each\}/g, '')
    .replace(/\{@[\s\S]*?\}/g, '')
    .replace(/\{[^{}]*\}/g, '""');

  const dom = new JSDOM(`<!DOCTYPE html><html><body>${htmlOnly}</body></html>`);
  const doc = dom.window.document;

  // Rule 1: Buttons must have accessible text or aria-label/title
  const buttons = doc.querySelectorAll('button');
  buttons.forEach((btn, idx) => {
    const text = btn.textContent?.trim();
    const ariaLabel = btn.getAttribute('aria-label') || btn.getAttribute('title');
    if (!text && !ariaLabel) {
      issueList.push({
        file: relPath,
        type: 'A11Y_MISSING_BUTTON_LABEL',
        severity: 'HIGH',
        detail: `Button #${idx + 1} has no text or aria-label/title.`
      });
      totalIssues++;
    }
  });

  // Rule 2: Non-interactive elements with click handlers
  if (content.includes('onclick=') && (content.includes('<div') || content.includes('<span') || content.includes('<section'))) {
    const rawLines = content.split('\n');
    rawLines.forEach((line, lineIdx) => {
      if (line.includes('onclick=') && line.includes('<div') && !line.includes('role=') && !line.includes('svelte-ignore')) {
        issueList.push({
          file: `${relPath}:${lineIdx + 1}`,
          type: 'A11Y_CLICKABLE_DIV_WITHOUT_ROLE',
          severity: 'MEDIUM',
          detail: `Clickable <div> missing explicit role="button" and tabindex.`
        });
        totalIssues++;
      }
    });
  }

  // Rule 3: Form Inputs must have associated labels or aria-label
  const inputs = doc.querySelectorAll('input, select, textarea');
  inputs.forEach((input, idx) => {
    const id = input.getAttribute('id');
    const ariaLabel = input.getAttribute('aria-label') || input.getAttribute('placeholder') || input.getAttribute('title');
    const hasLabel = id ? doc.querySelector(`label[for="${id}"]`) : false;
    if (!hasLabel && !ariaLabel) {
      issueList.push({
        file: relPath,
        type: 'A11Y_MISSING_INPUT_LABEL',
        severity: 'HIGH',
        detail: `Input #${idx + 1} (${input.tagName}) missing associated label or aria-label.`
      });
      totalIssues++;
    }
  });

  // Rule 4: Touch Target Dimensions (<32px clickable element)
  if (/\b(h-5|h-6|w-5|w-6|!h-6)\b/.test(content) && content.includes('<button')) {
    issueList.push({
      file: relPath,
      type: 'UX_TOUCH_TARGET_TOO_SMALL',
      severity: 'LOW',
      detail: `Detected sub-32px clickable button class. Recommended minimum target is 40-48px.`
    });
    totalIssues++;
  }
}

// 2. CSS & Design Token Audit on app.css
const cssContent = fs.readFileSync('src/app.css', 'utf-8');
console.log('🎨 Auditing CSS Design Tokens in src/app.css...');

const requiredTokens = [
  '--md-sys-color-primary',
  '--md-sys-color-on-primary',
  '--md-sys-color-surface',
  '--md-sys-color-surface-container',
  '--md-sys-color-outline-variant',
  '--md-sys-color-error',
  '--ui-brand-cyan',
  '--ui-color-success',
  '--ui-color-warning'
];

for (const token of requiredTokens) {
  if (!cssContent.includes(token)) {
    issueList.push({
      file: 'src/app.css',
      type: 'DESIGN_TOKEN_MISSING',
      severity: 'HIGH',
      detail: `Missing required Material 3 design token: ${token}`
    });
    totalIssues++;
  }
}

console.log('\n========================================');
console.log(`📊 AUDIT COMPLETE: Found ${totalIssues} UI/UX / A11y findings.`);
console.log('========================================\n');

issueList.forEach((issue, idx) => {
  console.log(`[${idx + 1}] [${issue.severity}] [${issue.type}] in ${issue.file}`);
  console.log(`    ↳ ${issue.detail}`);
});

if (totalIssues === 0) {
  console.log('🎉 ALL UI/UX AND ACCESSIBILITY CHECKS PASSED WITH 0 FINDINGS!');
}

process.exit(totalIssues > 0 ? 1 : 0);
