#!/usr/bin/python3
import sys, pygame, time, math

face1 = pygame.image.load(sys.argv[1])
face2 = pygame.image.load(sys.argv[2])
face3 = pygame.image.load(sys.argv[3])

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
        for y in range(rh):
            dest.set_at((x, y + ey + yoff), src.get_at((int((x / rw) * src_w), int((y / rh) * src_h))))
    
    return dest

out_surf = pygame.Surface((u * 2, u * 2), pygame.SRCALPHA)
out_surf.fill((255, 255, 255, 0))

w = int(u * (math.sqrt(3) / 2))
h = int(u * 1.5)

out_xoff = u - w

f2 = skew(face2, pygame.Surface((w, h), pygame.SRCALPHA), int(u / 2), u)
f2.fill((200, 200, 200, 255), None, pygame.BLEND_RGBA_MULT)
out_surf.blit(f2, (out_xoff, output_res[1] - h))

f3 = skew(face3, pygame.Surface((w, h), pygame.SRCALPHA), -int(u / 2), u)
f3.fill((160, 160, 160, 255), None, pygame.BLEND_RGBA_MULT)
out_surf.blit(f3, (out_xoff + w, output_res[1] - h))

f1 = pygame.Surface((300, 300), pygame.SRCALPHA)
f1.blit(pygame.transform.scale(face1, (300, 300)), (0, 0))
f1 = pygame.transform.rotate(f1, 45)
f1 = pygame.transform.scale(f1, (int(u * math.sqrt(3)), u))
out_surf.blit(f1, (out_xoff, 0))

screen.fill((0, 0, 0))
screen.blit(out_surf, (0, 0))

if len(sys.argv) >= 4:
    out_small = pygame.transform.smoothscale(out_surf, (100, 100))
    print("Saving to '%s'..." % sys.argv[4])
    pygame.image.save(out_small, sys.argv[4])

pygame.display.flip()

time.sleep(0.5)
