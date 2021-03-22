#!/usr/bin/python3

"""
    mc4, a web voxel building game
    Copyright (C) 2019-2021 kholland4

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""

import sys, pygame, time, math

slab = False
stair = False

face1 = None
face2 = None
face3 = None

if len(sys.argv) == 2 or len(sys.argv) == 3:
    face1 = pygame.image.load(sys.argv[1])
    face2 = pygame.image.load(sys.argv[1])
    face3 = pygame.image.load(sys.argv[1])
elif len(sys.argv) == 4 or len(sys.argv) == 5:
    face1 = pygame.image.load(sys.argv[1]) # top face
    face2 = pygame.image.load(sys.argv[2]) # left face
    face3 = pygame.image.load(sys.argv[3]) # right face
else:
    print("Usage:")
    print("  %s all-sides-texture.png" % sys.argv[0])
    print("  %s all-sides-texture.png out.png" % sys.argv[0])
    print("  %s top-tex.png left-tex.png right-tex.png" % sys.argv[0])
    print("  %s top-tex.png left-tex.png right-tex.png out.png" % sys.argv[0])
    sys.exit(1)

u = 160

output_res = (u * 2, u * 2)

screen = pygame.display.set_mode(output_res)

def skew(src, dest, amt, rh):
    dest_w = dest.get_size()[0]
    dest_h = dest.get_size()[1]
    
    src_w = src.get_size()[0]
    src_h = src.get_size()[1]
    src_aspect = src_h / src_w
    
    dest.fill((255, 255, 255, 0))
    
    rw = dest_w
    ey = -min(amt, 0)
    for x in range(rw):
        yoff = int((x / rw) * amt)
        m = 0
        if slab:
            m = int(rh / 2)
        for y in range(m, rh):
            dest.set_at((x, y + ey + yoff), src.get_at((int((x / rw) * src_w), int((y / rh) * src_h))))
    
    return dest

out_surf = pygame.Surface((u * 2, u * 2), pygame.SRCALPHA)
out_surf.fill((255, 255, 255, 0))

w = int(u * (math.sqrt(3) / 2))
h = int(u * 1.5)

out_xoff = u - w

if stair:
    newFace2 = pygame.Surface((16, 16), pygame.SRCALPHA)
    newFace2.blit(face2, (0, 0))
    face2 = newFace2
    pygame.draw.rect(face2, (0, 0, 0, 0), pygame.Rect(8, 0, 8, 8))
f2 = skew(face2, pygame.Surface((w, h), pygame.SRCALPHA), int(u / 2), u)
f2.fill((200, 200, 200, 255), None, pygame.BLEND_RGBA_MULT)
out_surf.blit(f2, (out_xoff, output_res[1] - h))

if stair:
    newFace3_top = pygame.Surface((16, 16), pygame.SRCALPHA)
    newFace3_top.blit(face3, (0, 0))
    pygame.draw.rect(newFace3_top, (0, 0, 0, 0), pygame.Rect(0, 8, 16, 8))
    
    f3_top = skew(newFace3_top, pygame.Surface((w, h), pygame.SRCALPHA), -int(u / 2), u)
    f3_top.fill((160, 160, 160, 255), None, pygame.BLEND_RGBA_MULT)
    out_surf.blit(f3_top, (out_xoff + int(w / 2), output_res[1] - h - int(u / 4)))
    
    newFace3_bottom = pygame.Surface((16, 16), pygame.SRCALPHA)
    newFace3_bottom.blit(face3, (0, 0))
    pygame.draw.rect(newFace3_bottom, (0, 0, 0, 0), pygame.Rect(0, 0, 16, 8))
    
    f3_bottom = skew(newFace3_bottom, pygame.Surface((w, h), pygame.SRCALPHA), -int(u / 2), u)
    f3_bottom.fill((160, 160, 160, 255), None, pygame.BLEND_RGBA_MULT)
    out_surf.blit(f3_bottom, (out_xoff + w, output_res[1] - h))
else:
    f3 = skew(face3, pygame.Surface((w, h), pygame.SRCALPHA), -int(u / 2), u)
    f3.fill((160, 160, 160, 255), None, pygame.BLEND_RGBA_MULT)
    out_surf.blit(f3, (out_xoff + w, output_res[1] - h))

if stair:
    f1a = pygame.Surface((300, 300), pygame.SRCALPHA)
    f1a.blit(pygame.transform.scale(face1, (300, 300)), (0, 0))
    pygame.draw.rect(f1a, (0, 0, 0, 0), pygame.Rect(0, 150, 300, 150))
    f1a = pygame.transform.rotate(f1a, 45)
    f1a = pygame.transform.scale(f1a, (int(u * math.sqrt(3)), u))
    f1a_yoff = 0
    out_surf.blit(f1a, (out_xoff, f1a_yoff))
    
    f1b = pygame.Surface((300, 300), pygame.SRCALPHA)
    f1b.blit(pygame.transform.scale(face1, (300, 300)), (0, 0))
    pygame.draw.rect(f1b, (0, 0, 0, 0), pygame.Rect(0, 0, 300, 150))
    f1b = pygame.transform.rotate(f1b, 45)
    f1b = pygame.transform.scale(f1b, (int(u * math.sqrt(3)), u))
    f1b_yoff = u / 2
    out_surf.blit(f1b, (out_xoff, f1b_yoff))
else:
    f1 = pygame.Surface((300, 300), pygame.SRCALPHA)
    f1.blit(pygame.transform.scale(face1, (300, 300)), (0, 0))
    f1 = pygame.transform.rotate(f1, 45)
    f1 = pygame.transform.scale(f1, (int(u * math.sqrt(3)), u))
    f1_yoff = 0
    if slab:
        f1_yoff = u / 2
    out_surf.blit(f1, (out_xoff, f1_yoff))

screen.fill((0, 0, 0))
screen.blit(out_surf, (0, 0))

if len(sys.argv) == 3 or len(sys.argv) == 5:
    out_small = pygame.transform.smoothscale(out_surf, (100, 100))
    print("Saving to '%s'..." % sys.argv[len(sys.argv) - 1])
    pygame.image.save(out_small, sys.argv[len(sys.argv) - 1])

pygame.display.flip()

time.sleep(0.5)
