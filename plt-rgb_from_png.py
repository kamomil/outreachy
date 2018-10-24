from scipy import misc
M = misc.imread("globe-scene-fish-bowl-pngcrush.png")
import matplotlib.pyplot as plt
# print(M)
print(type(M))
print(M.shape)
print(M[50][40][3])
plt.imshow(M)
plt.show()
