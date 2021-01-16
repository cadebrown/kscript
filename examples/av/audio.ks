#!/usr/bin/env ks
""" vid.ks - read video


@author: Cade Brown <cade@kscript.org>

"""

import av

# Open a video
fp = av.open("./assets/video/rabbitman.mp4")

# Get the best streams
aud = fp.best_audio()

for chunk in aud {
    print (chunk.shape)
}

