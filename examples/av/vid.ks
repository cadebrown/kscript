#!/usr/bin/env ks
""" vid.ks - read video


@author: Cade Brown <cade@kscript.org>

"""

import av


imgs = av.open("./assets/vid/rabbitman.mp4")
#imgs = av.open("./assets/img/t0.png")

#svid = imgs.best_video()


s0 = imgs.stream(0)
s1 = imgs.stream(1)

ct = 0

for img in s0 {
    print (img.shape ?? img)
    ct += 1
    if ct > 5, break
}


for img in s1 {
    print (img.shape ?? img)
    ct += 1
    if ct > 10, break
}




#for (s, img) in imgs {
    #if s == svid {
#    print (img.shape ?? img)
    #}
#}
