import sys
import os
import struct

class YM2610_ADPCM_A:
    """YM2610 ADPCM-A Encoder (6-channel, 12-bit internal)"""
    STEP_TABLE = [
        16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 
        80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 
        307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 
        1060, 1166, 1282, 1411, 1552
    ]
    STEP_ADJUST = [-1, -1, -1, -1, 2, 5, 7, 9]
    
    def __init__(self):
        self.history = 0  # 12-bit signed
        self.step_idx = 0

    def encode_sample(self, sample_16bit):
        # Scale 16-bit to 12-bit
        target = sample_16bit >> 4
        diff = target - self.history
        step_size = self.STEP_TABLE[self.step_idx]
        
        code = 0x08 if diff < 0 else 0x00
        magnitude = min(7, abs(diff) * 4 // step_size)
        code |= magnitude
        
        # Update state (mimic decoder)
        delta = (2 * magnitude + 1) * step_size // 8
        if code & 0x08:
            self.history -= delta
        else:
            self.history += delta
            
        # 12-bit wrapping
        self.history &= 0xFFF
        if self.history & 0x800:
            self.history -= 0x1000
            
        self.step_idx = max(0, min(48, self.step_idx + self.STEP_ADJUST[magnitude]))
        return code

class YM2610_ADPCM_B:
    """YM2610 ADPCM-B Encoder (1-channel, 16-bit internal)"""
    STEP_ADJUST = [57, 57, 57, 57, 77, 102, 128, 153]
    
    def __init__(self):
        self.history = 0
        self.step_size = 127

    def encode_sample(self, sample_16bit):
        diff = sample_16bit - self.history
        
        code = 0x08 if diff < 0 else 0x00
        magnitude = min(7, (abs(diff) << 2) // self.step_size)
        code |= magnitude
        
        # Update state (mimic decoder)
        delta = (2 * magnitude + 1) * self.step_size // 8
        if code & 0x08:
            self.history -= delta
        else:
            self.history += delta
            
        self.history = max(-32768, min(32767, self.history))
        
        self.step_size = (self.step_size * self.STEP_ADJUST[magnitude]) // 64
        self.step_size = max(127, min(24576, self.step_size))
        
        return code

def main():
    if len(sys.argv) < 4:
        print("Usage: python adpcm_enc.py <a|b> <input_raw> <output_adpcm>")
        sys.exit(1)
        
    mode = sys.argv[1].lower()
    input_path = sys.argv[2]
    output_path = sys.argv[3]
    
    if mode == 'a':
        encoder = YM2610_ADPCM_A()
    elif mode == 'b':
        encoder = YM2610_ADPCM_B()
    else:
        print(f"Unknown mode: {mode}")
        sys.exit(1)
        
    if not os.path.exists(input_path):
        print(f"Input file not found: {input_path}")
        sys.exit(1)
        
    with open(input_path, 'rb') as f:
        raw_data = f.read()
        
    # Input is 16-bit signed little-endian PCM (raw)
    num_samples = len(raw_data) // 2
    samples = struct.unpack(f'<{num_samples}h', raw_data)
    
    output_data = bytearray()
    
    # Process samples in pairs for 4-bit packing
    for i in range(0, len(samples), 2):
        s1 = samples[i]
        s2 = samples[i+1] if i+1 < len(samples) else 0
        
        c1 = encoder.encode_sample(s1)
        c2 = encoder.encode_sample(s2)
        
        # High nibble is first sample
        output_data.append(((c1 & 0x0F) << 4) | (c2 & 0x0F))
        
    with open(output_path, 'wb') as f:
        f.write(output_data)
        
    print(f"Encoded {len(samples)} samples to {output_path}")

if __name__ == "__main__":
    main()
