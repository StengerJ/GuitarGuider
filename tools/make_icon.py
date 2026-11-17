"""Generates assets/GuitarGuider.ico: a brass guitar pick with 'GG' on amp-tolex blue-grey."""
from PIL import Image, ImageDraw, ImageFont
import os, sys

S = 1024
img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
d.rounded_rectangle([40, 40, S - 40, S - 40], radius=220, fill=(29, 33, 39, 255))

# pick: two cubic curves approximated with a polygon
import math
def bez(p0, p1, p2, p3, n=60):
    pts = []
    for i in range(n + 1):
        t = i / n
        a = (1 - t) ** 3; b = 3 * (1 - t) ** 2 * t; c = 3 * (1 - t) * t * t; e = t ** 3
        pts.append((a * p0[0] + b * p1[0] + c * p2[0] + e * p3[0], a * p0[1] + b * p1[1] + c * p2[1] + e * p3[1]))
    return pts
cx, top, bot, w = S / 2, 190, 880, 330
left = bez((cx, bot), (cx - w - 20, top + (bot - top) * 0.55), (cx - w, top), (cx, top))
right = bez((cx, top), (cx + w, top), (cx + w + 20, top + (bot - top) * 0.55), (cx, bot))
d.polygon(left + right, fill=(217, 164, 65, 255))

font_path = os.path.join(os.path.dirname(__file__), "..", "assets", "fonts", "BarlowCondensed-Bold.ttf")
font = ImageFont.truetype(font_path, 330)
text = "GG"
bbox = d.textbbox((0, 0), text, font=font)
tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
d.text((cx - tw / 2 - bbox[0], 430 - th / 2 - bbox[1]), text, font=font, fill=(29, 33, 39, 255))

out = os.path.join(os.path.dirname(__file__), "..", "assets", "GuitarGuider.ico")
img.save(out, sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
img.resize((512, 512), Image.LANCZOS).save(os.path.join(os.path.dirname(__file__), "..", "assets", "GuitarGuider.png"))
print("wrote", out)
