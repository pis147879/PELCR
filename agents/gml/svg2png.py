"""svg2png.py in.svg out.png [scale] [background]: render an SVG with Chromium (transparent by default)."""
import sys, pathlib
from playwright.sync_api import sync_playwright
src, dst = sys.argv[1], sys.argv[2]
scale = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
bg = sys.argv[4] if len(sys.argv) > 4 else 'transparent'
svg = pathlib.Path(src).read_text()
import re
W, H = map(int, re.search(r'viewBox="0 0 (\d+) (\d+)"', svg).groups())
with sync_playwright() as p:
    b = p.chromium.launch()
    pg = b.new_page(viewport={'width': W, 'height': H}, device_scale_factor=scale)
    pg.set_content('<html><body style="margin:0;background:%s">%s</body></html>' % (bg, svg))
    pg.screenshot(path=dst, omit_background=(bg == 'transparent'), full_page=False)
    b.close()
print(dst)
