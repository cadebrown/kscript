#!/usr/bin/env ks
""" vid.ks - read video


@author: Cade Brown <cade@kscript.org>

"""

import av


imgs = av.open("./assets/vid/rabbitman.mp4")
#imgs = av.open("./assets/img/t0.png")

svid = imgs.best_video()
#print (imgs)


for (s, img) in imgs {
    if s == svid {
        print (img.shape)
    }
}
