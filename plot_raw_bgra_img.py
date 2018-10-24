import matplotlib.pyplot as plt
import numpy as np
import sys

# ffmpeg -vcodec png -i globe-scene-fish-bowl-pngcrush.png -vcodec rawvideo -f rawvideo -pix_fmt argb texture.raw

if len(sys.argv) != 4:
    print(f'usage: {sys.argv[0]} raw-image width height')
    sys.exit()


#X = np.random.random((100, 100)) # sample 2D array
#plt.imshow(X, cmap="gray")
#plt.show()

fname = sys.argv[1]
a = np.fromfile(fname, dtype=np.uint8)
print(type(a))


BB = np.reshape(a, (int(sys.argv[3]), int(sys.argv[2]),4 ))

B = BB[:,:,0]
G = BB[:,:,1]
R = BB[:,:,2]
A = BB[:,:,3]



#print(B)
print(type(BB))
print(BB.shape)

arr = np.dstack((R,G,B,A))

print(arr.shape)
print(A.shape)

plt.imshow(arr)
plt.show()


