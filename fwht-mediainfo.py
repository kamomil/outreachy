import sys
import struct

"""
#define FWHT_FL_LUMA_IS_UNCOMPRESSED	BIT(4)
#define FWHT_FL_CB_IS_UNCOMPRESSED	BIT(5)
#define FWHT_FL_CR_IS_UNCOMPRESSED	BIT(6)
#define FWHT_FL_CHROMA_FULL_HEIGHT	BIT(7)
#define FWHT_FL_CHROMA_FULL_WIDTH	BIT(8)
#define FWHT_FL_ALPHA_IS_UNCOMPRESSED	BIT(9)

/* A 4-values flag - the number of components - 1 */
#define FWHT_FL_COMPONENTS_NUM_MSK	GENMASK(17, 16)
#define FWHT_FL_COMPONENTS_NUM_OFFSET	16

struct fwht_cframe_hdr {
	u32 magic1;
	u32 magic2;
	__be32 version;
	__be32 width, height;
	__be32 flags;
	__be32 colorspace;
	__be32 xfer_func;
	__be32 ycbcr_enc;
	__be32 quantization;
	__be32 size;
};
"""
luma_compressed = 0x00000010
cb_compressed   = 0x00000020
cr_compressed   = 0x00000040
ch_full_height  = 0x00000080
ch_full_width   = 0x00000100
components_num  = 0x00030000
pixenc          = 0x000c0000

def main():
	if len(sys.argv) != 2:
		print(f'Usage: {sys.argv[0]} <fwht file>')
		sys.exit();
	with open(sys.argv[1], 'rb') as fh:
		magic = fh.read(8)
		print(f'magic: {magic}')
		version = fh.read(4)
		print('version: %d' % struct.unpack('>L',version))
		width = fh.read(4)
		print('width: %d' % struct.unpack('>L',width))
		height = fh.read(4)
		print('height: %d' % struct.unpack('>L',height))
		flags = struct.unpack('>L', fh.read(4))
		if flags[0] & luma_compressed:
			print('luma is uncompressed')
		if flags[0] & cb_compressed:
			print('cb is uncompressed')
		if flags[0] & cr_compressed:
			print('cr is uncompressed')
		components = 1 + ((flags[0] & components_num) >> 16)
		print(f'components: {components}')
		pix = ((flags[0] & pixenc) >> 18)
		print(f'pixenc: {pix}')
		others = fh.read(16)
		size = struct.unpack('>L', fh.read(4))[0]
		print(f'size: {size}')
		others = fh.read(16)
		size = struct.unpack('>L', fh.read(4))[0]
		print(f'size: {size}')

if __name__== "__main__":
	main()
