"""Lossless sharing of 16x32 sprite strips; preserves every indexed pixel."""
import numpy as np


class StripPack:
    def __init__(self, place):
        self.place = place
        self.lookup = {}
        self.frames = 0
        self.raw_tiles = 0

    def frame(self, image, banks=None):
        image = np.asarray(image)
        if image.shape not in ((160,160),(192,192)) or image.dtype != np.uint8 or image.max() > 15:
            raise ValueError('Fighter frames must be native 160x160 or 192x192, uint8 indices 0..15')
        height,width=image.shape;columns=width//16
        if banks is not None and (banks.shape!=(height//16,columns) or banks.max()>3):raise ValueError("Fighter tile palette map must match the canvas with banks 0..3")
        table=[]
        for y in range(0,height,32):
            for x in range(0,width,16):
                part=np.ascontiguousarray(image[y:y+32,x:x+16]);key=part.tobytes()
                if not part.any():continue
                if key not in self.lookup:self.lookup[key]=self.place(part)
                bits=0 if banks is None else int(banks[y//16,x//16]) | (int(banks[y//16+1,x//16])<<2)
                address=self.lookup[key]
                if not 0<=address<=0xffffe:raise ValueError('C-ROM strip exceeds 20-bit tile space')
                if address&0xffff==0xffff:raise ValueError('A shared two-tile strip crosses a C bank; align its reservation')
                table.append(((y//32)*columns+x//16,address&0xffff,bits|((address>>16)<<4)))
        self.frames+=1;self.raw_tiles+=height*width//256
        return table

    def report(self):
        return {'frames':self.frames,'unshared_tiles':self.raw_tiles,
                'unique_strips':len(self.lookup),'shared_tiles':len(self.lookup)*2,
                'pixel_loss':0}
