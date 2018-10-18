import matplotlib.pyplot as plt
import numpy as np
import sys

if len(sys.argv) != 4:
    print(f'usage: {sys.argv[0]} raw-image width height')
    sys.exit()


#X = np.random.random((100, 100)) # sample 2D array
#plt.imshow(X, cmap="gray")
#plt.show()

fname = sys.argv[1]
a = np.fromfile(fname, dtype=np.uint8)
print(type(a))

B = np.reshape(a, (-1, int(sys.argv[2])))
plt.imshow(B, cmap="gray")
plt.show()


