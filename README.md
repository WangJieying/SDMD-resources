# Spline-Based Dense Medial Descriptors for Image Compression: Supplementary Material

This supplementary material shows the comparison of SDMD with JPEG for five image types and the boundary Y-structure elimination results. We also provide the full source code of SDMD below for replication purposes.

## 1. Comparison with SDMD and JPEG.

 For each image, we show the quality score (SSIM) and compression ratio (CR) of the JPEG (green asterisks) and SDMD (red filled dots) under several different quality settings. We also show the reconstruction of two methods under one specific quality setting.


  - [Scientific visualization data](./scivis.md)
  - [Computer graphics](./cg.md)
  - [Medical images](./medical.md)
  - [Graphics art images](./abstract.md)
  - [Cartoon images](./cartoon.md)
  
  
## 2. Boundary Y-structure elimination results

For each image, we show the result without using the Y-removal method ('Original result') and with this scheme used ('Y-removal result') and also their SSIM vs. CR under several different quality settings in Boundary Y-structure elimination.ppt. 

## 3. Source code

We implemented the entire SDMD method in C++. We compute MAT and reconstruct the threshold-sets from a rasterized spline using the public CUDA code. The full source code is provided [here](./codeReadME.md).
