#!/usr/local/anaconda3/bin/python3

import cv2

cam = cv2.VideoCapture(0)
ret, img = cam.read()
cv2.imwrite('test.jpg', img)
cam.release()
