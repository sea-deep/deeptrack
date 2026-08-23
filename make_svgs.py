import json

vl53_svg = """<svg width="60" height="60" xmlns="http://www.w3.org/2000/svg">
  <rect width="60" height="60" fill="#333" />
  <text x="30" y="30" fill="white" font-size="10" text-anchor="middle" alignment-baseline="middle">VL53L0X</text>
</svg>"""

tb66_svg = """<svg width="80" height="80" xmlns="http://www.w3.org/2000/svg">
  <rect width="80" height="80" fill="#333" />
  <text x="40" y="40" fill="white" font-size="10" text-anchor="middle" alignment-baseline="middle">TB6612FNG</text>
</svg>"""

with open("vl53l0x.svg", "w") as f: f.write(vl53_svg)
with open("tb6612fng.svg", "w") as f: f.write(tb66_svg)

for chip in ['vl53l0x', 'tb6612fng']:
    with open(f'{chip}.chip.json', 'r') as f:
        data = json.load(f)
    
    data['author'] = 'wokwi' # maybe wokwi is required?
    
    # Or maybe it just needs the icon?
    # Wokwi custom chip docs: no, there is no icon field, wait there is!
    # No, wait, if you don't provide an SVG, Wokwi draws a generic DIP chip.
    pass

