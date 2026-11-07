"""Checks that SteamSynth's interactive layers exist as independent sprites.
This intentionally rejects a single flat screenshot as a substitute for controls.
"""
from pathlib import Path
from PIL import Image
p=Path(__file__).resolve().parents[1]/'assets'/'ui'
plate=Image.open(p/'plate.jpg')
a=Image.open(p/'hardware.png').convert('RGBA')
assert plate.size==(1600,900)
assert a.size==(1200,1024)
# Five stationary engraved medallions and five annular rotating rims.
for i in range(5):
 x=i*190
 assert a.getpixel((x+92,92))[3]==0, 'Knob rim must be hollow: motif stays stationary'
 assert a.getpixel((x+92,282))[3]>200, 'Botanical medallion missing'
 assert a.getpixel((x+92,17))[3]>100, 'Rotating brass indicator missing'
# Separate interactive tabs in both states.
for i in range(4):
 assert a.getpixel((i*154+75,526))[3]>200
 assert a.getpixel((i*154+75,599))[3]>200
# Ivory and ebony keys must be independent alpha sprites, not a painted keyboard screenshot.
for x,y in [(20,760),(68,760),(112,760),(140,760)]:assert a.getpixel((x,y))[3]>0
# The keybed has been cleared in the base art and can be repainted key-by-key.
assert plate.getpixel((600,800))!=(255,255,255)
print('PASS: botanical plate, five separate rotating rims, fixed medallions, 4 interactive tab pairs, individual key sprites')