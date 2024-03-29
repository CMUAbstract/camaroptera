from PIL import Image
import numpy as np
import os

cwd = str(os.getcwd())
directory = os.fsencode(cwd)
w, h = 160, 120

def main():

    for file in os.listdir(directory):
        filename = os.fsdecode(file)
        if filename.endswith(".txt"):
            print(str(filename))
            f = open(str(cwd + "/" + filename), 'r')

            string = f.read()

            str_split = string.split(' ')
            str_split = list(map(int, str_split))

            data = np.zeros((h, w), dtype=np.uint8)

            for y in range(0, h):
                    for x in range(0, w):
                            data[y][x] = str_split[((w) * y) + (x)]

            output_file = filename.split('.')
            img = Image.fromarray(data, 'L') #L = 8bit pixel B/W
            img = img.rotate(180)
            print("Output file: " + str(cwd + "/" + str(output_file[0]) + ".bmp") )
            img.save(str(cwd + "/" + str(output_file[0]) + ".bmp"))

if __name__ == '__main__':
  main()
