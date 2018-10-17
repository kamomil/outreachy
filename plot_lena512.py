import matplotlib.pyplot as plt
import numpy as np


#X = np.random.random((100, 100)) # sample 2D array
#plt.imshow(X, cmap="gray")
#plt.show()

fname = 'lena_gray.raw'
a = np.fromfile(fname, dtype=np.uint8)
print(type(a))

B = np.reshape(a, (-1, 512))
plt.imshow(B, cmap="gray")
plt.show()


