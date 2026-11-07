#!/usr/bin/env python3
"""Rebuild real SteamSynth knob sprites from the original botanical painting.

Only the knobs are rotatable: the printed scales, labels and botanical landscape
remain in the stationary plate.  Each rotary is a SINGLE transparent RGBA
sprite (engraved brass body + ONE pointer) and rotates as a rigid assembly.

Requirements for asset authoring only: Pillow, numpy, opencv-python.
Run from the repository root: python tools/rebuild_knobs.py
"""
from pathlib import Path
import argparse
import cv2
import numpy as np
from PIL import Image, ImageDraw

LARGE = [(345,306),(600,306),(850,306),(1100,306),(1360,306)]
SMALL = [(279,666),(383,666),(580,666),(683,666),(779,666),
         (196,838),(1308,832),(1393,832),(1476,832)]
POLLINATE = (1035,666)

def round_sprite(original, x, y, size, edge, feather):
    half=size//2
    crop=original.crop((x-half,y-half,x+half,y+half))
    yy,xx=np.ogrid[:size,:size]
    dist=np.hypot(xx-(size-1)/2,yy-(size-1)/2)
    alpha=np.uint8(np.round(np.clip((edge-dist)/feather,0,1)*255))
    crop.putalpha(Image.fromarray(alpha))
    return crop

def rebuild(reference, plate_file, hardware_file):
    original=Image.open(reference).convert('RGBA')
    plate=np.asarray(Image.open(plate_file).convert('RGB')).copy()
    assert original.size==(1600,900) and plate.shape[:2]==(900,1600)
    mask=np.zeros((900,1600),np.uint8)
    # Eliminate ALL baked-in ring/shaft/pointer pixels, not merely the center.
    for xy in LARGE:cv2.circle(mask,xy,95,255,-1)
    for xy in SMALL:cv2.circle(mask,xy,31,255,-1)
    cv2.circle(mask,POLLINATE,46,255,-1)
    plate=cv2.cvtColor(cv2.inpaint(cv2.cvtColor(plate,cv2.COLOR_RGB2BGR),mask,5,cv2.INPAINT_TELEA),cv2.COLOR_BGR2RGB)
    Image.fromarray(plate).save(plate_file,quality=94,subsampling=0)
    atlas=Image.open(hardware_file).convert('RGBA')
    assert atlas.size==(1200,1024)
    painter=ImageDraw.Draw(atlas)
    # Five COMPLETE large knobs. The former ring/medallion double-rendering is gone.
    for i,(x,y) in enumerate(LARGE):
        painter.rectangle((i*190,0,i*190+183,183),fill=(0,0,0,0))
        painter.rectangle((i*190,190,i*190+183,373),fill=(0,0,0,0))
        atlas.alpha_composite(round_sprite(original,x,y,184,92,4),(i*190,0))
    # Nine smaller brass controls, each with its own integrated marker.
    for i,(x,y) in enumerate(SMALL):
        painter.rectangle((i*65,389,i*65+59,448),fill=(0,0,0,0))
        atlas.alpha_composite(round_sprite(original,x,y,60,30.2,3.4),(i*65,389))
    # Pollinate gets a real carved flower dial rather than two translucent overlays.
    painter.rectangle((700,389,791,480),fill=(0,0,0,0))
    atlas.alpha_composite(round_sprite(original,*POLLINATE,92,46,3.5),(700,389))
    # Former independent ring is no longer drawn, but clear it to prevent regression.
    painter.rectangle((800,389,891,480),fill=(0,0,0,0))
    atlas.save(hardware_file,optimize=True)

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
    a=p.parse_args(); ui=a.root/'assets/ui'
    rebuild(ui/'reference_original.jpg',ui/'plate.jpg',ui/'hardware.png')
    print('Created 5 large + 9 small + 1 Pollinate independent rotatable sprites')