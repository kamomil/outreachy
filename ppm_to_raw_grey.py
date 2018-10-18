import matplotlib.pyplot as plt
import numpy as np
import sys

if len(sys.argv) != 2:
    sys.exit(f'usage: {sys.argv[0]} ppm image\nimage should be 640x480')

fname = sys.argv[1]

#X = np.random.random((100, 100)) # sample 2D array
#plt.imshow(X, cmap="gray")
#plt.show()

a = np.fromfile(fname, dtype=np.uint8)
print(type(a))
a = a[15:]
print(a.shape[0])

z = np.zeros((640*480, ), dtype=np.uint8)
print(z.shape)

j = 0
for i in range(0,a.shape[0],3):
    z[j] = (a[i]/3 + a[i+1]/3 +a[i+2]/3)
    j += 1
print(j)
# z_to_show = np.reshape(z, (480, -1))
# plt.imshow(z_to_show, cmap="gray")
# plt.show()

z.astype('uint8').tofile(f'{fname}.raw')
