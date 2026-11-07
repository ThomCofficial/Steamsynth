"""Fail CI if SteamSynth falls back to fake/duplicated knob overlays.

Checks that the visual instrument uses independently fabricated RGBA hardware,
not a flattened UI image with a second, misaligned rim painted on top.
"""
from pathlib import Path
from PIL import Image, ImageChops
import numpy as np
p=Path(__file__).resolve().parents[1]/'assets'/'ui'
plate=Image.open(p/'plate.jpg').convert('RGB')
a=Image.open(p/'hardware.png').convert('RGBA')
assert plate.size==(1600,900)
assert a.size==(1200,1024)
# Five coherent one-piece brass knobs, each with its engraved face and pointer.
for i in range(5):
 x=i*190
 sprite=a.crop((x,0,x+184,184))
 assert sprite.getpixel((92,92))[3]>245, f'Knob {i}: missing face'
 assert sprite.getpixel((92,8))[3]>150, f'Knob {i}: missing rim'
 assert sprite.getpixel((2,2))[3]==0, f'Knob {i}: no transparent corner'
 assert a.getpixel((x+92,282))[3]==0, f'Knob {i}: old second medallion still present'
 assert ImageChops.difference(sprite,sprite.rotate(80)).getbbox() is not None
 # The source plate must not retain the old engraved brass face underneath.
 cx=[345,600,850,1100,1360][i]
 r,g,b=plate.getpixel((cx,306))
 assert min(r,g,b)>70, f'Knob {i}: dirty static faceplate under dial'
# Small knob sprites have their own alpha and full metal bodies.
for i in range(9):
 x=i*65
 assert a.getpixel((x+30,419))[3]>245, f'Small knob {i}: missing'
 assert a.getpixel((x,389))[3]==0, f'Small knob {i}: unmasked rectangle'
# Pollinate is a real, opaque engraved flower control, not a pale overlay.
assert a.getpixel((746,435))[3]>245
assert a.getpixel((800+46,435))[3]==0, 'Old floating flower ring remains'
# Four independently rendered tabs, in both lit and unlit states.
for i in range(4):
 assert a.getpixel((i*154+75,526))[3]>200
 assert a.getpixel((i*154+75,599))[3]>200
# Keyboard is rendered from separate transparent white and black key assets.
for x,y in [(20,760),(68,760),(112,760),(140,760)]:
 assert a.getpixel((x,y))[3]>0
assert plate.getpixel((600,800))!=(255,255,255)
print('PASS: 5 coherent brass dials, 9 small dials, pollinate, lit tabs, keyboard; no duplicated rims')