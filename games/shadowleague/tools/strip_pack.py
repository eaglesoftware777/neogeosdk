"""Lossless sharing of 16x32 sprite strips; preserves every indexed pixel."""
import numpy as np


class StripPack:
    def __init__(self, place):
        self.place = place
        self.lookup = {}
        self.frames = 0
        self.raw_tiles = 0

    def frame(self, image):
        image = np.asarray(image)
        if image.shape != (160,160) or image.dtype != np.uint8 or image.max() > 15:
            raise ValueError('Fighter frames must be native 160x160, uint8 indices 0..15')
        table=[]
        for y in range(0,160,32):
            for x in range(0,160,16):
                part=np.ascontiguousarray(image[y:y+32,x:x+16]);key=part.tobytes()
                if not part.any():continue
                if key not in self.lookup:self.lookup[key]=self.place(part)
                table.append(((y//32)*10+x//16,self.lookup[key]))
        self.frames+=1;self.raw_tiles+=100
        return table

    def report(self):
        return {'frames':self.frames,'unshared_tiles':self.raw_tiles,
                'unique_strips':len(self.lookup),'shared_tiles':len(self.lookup)*2,
                'pixel_loss':0}
