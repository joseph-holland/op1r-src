/* Obtain SVG geometry using Chromium's SVG getBBox implementation.
 * Usage: node font_bounds.cjs input.svg output.json [chromium executable]
 * Requires the playwright package. No network access is used.
 */
const fs = require('node:fs');
const { chromium } = require('playwright');
(async () => {
  const [input, output, executablePath] = process.argv.slice(2);
  if (!input || !output) throw new Error('input.svg and output.json required');
  const browser = await chromium.launch({headless: true, ...(executablePath ? {executablePath} : {})});
  try {
    const page = await browser.newPage();
    await page.route('**/*', route => route.abort());
    await page.setContent(fs.readFileSync(input, 'utf8'));
    const boxes = await page.evaluate(() => Object.fromEntries(
      [...document.querySelector('svg').children].filter(n => n.id && n.getBBox).map(n => {
        const b = n.getBBox(); return [n.id, {x:b.x, y:b.y, width:b.width, height:b.height}];
      })));
    fs.writeFileSync(output, JSON.stringify(boxes));
  } finally { await browser.close(); }
})().catch(e => { console.error(e.message); process.exitCode = 1; });
