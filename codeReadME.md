This software package is the technical support for our paper "Spline-Based Dense Medial Descriptors for Lossy Image
Compression". 

The whole pipeline of the code is: Given a grayscale image or a color image, we first thresholding it into n (256 for 8-bit images) binary images. For each binary image, or layer that we call, we first remove the small 'island' or noise of it, then we extract its skeletons, then we fit skeletons with B-splines. Then all the control points, degree of each spline, etc will be encoded into 'output.sir' in the imShow file. The SIR file is all we need to store for the original image, which is also used to calculate the compression ratio. All of these steps implemented in the imConvert file, and imShow file is responsible for decode the SIR file and then reconstruct it back to the original image.

# 1. Building


The software needs a C++ compiler, OpenGL, the CUDA, and GLUT to build. 
Under a Debian based linux they can be installed using the following command.

* install

sudo apt-get install cuda zpaq libzstd-dev libsnappy-dev gcc git make cmake libboost-all-dev freeglut3-dev libglew-dev ragel libvala-0.40-dev glmark2 valac liblz4-dev liblzma-dev libbz2-1.0

* to build:

cd Code/imShow && make

cd ../../

cd Code/imConvert && make


# 2. Running


cd Code/imConvert

./skeletonify config.txt

cd ../imShow

./show_skeleton output.sir 

where config.txt is a configuration file. The detailed explanation is shown in the last section.

**I also write a bash script to execute the whole pipeline, named 'benchmark' in the Code file.**

What you need to do are: 

(a) Put images that you want to test into Image/type1/someimage.ppm 

(b) Change parameters as you want in this script. These parameters include how many layers do you want to select("layers"); what size of small-scale details that you want to remove ("islandThreshold"); how much to smooth isophote or isochrome contours in an image ("Saliency"); and how accurately B-splines fit the skeletons ("hausdorff") etc, see the detailed explanation in the 4th section.

(c) Run ./benchmark. Then the generated result will be shown in the Code file, and the compression ratio together with the SSIM score will be written into the compressRate2.txt and skeleton_msssim2.txt separately which located in 'outputFile'.


# 3. Function


imConvert
---------

This program read PGM (grayscale images) or PPM (color images) files, and convert them to the SIR file, which stands for "Skeletonal Image Representation". The main steps/functions are listed below:

* removeIslands
Remove small 'islands'/noise according to the islandThreshold variable. Notice that both "on" and "off" regions will be filtered.

* calculateImportance
This function will select layers according to the layer_selection threshold. It mainly contains two methods to select the given number layers: the cumulative histogram and local-maxima method, see details at [this paper](https://jieying-wang.netlify.app/publication/example/example.pdf). For those layers/intensities who are selected, their importance would be 1, otherwise that would be 0.

* removeLayers
This function is complementary to the previous one. If one layer is unimportant (done by the previous function), it would be removed. Specifically, we adjust the value of each pixel to the nearest 'important' level.

* computeSkeletons
This function calculates the skeleton for each layer. The core function to calculate the skeleton is in computeSkeleton(), which uses CUDA to implement the real-time 2D skeletonization. See details [here](http://www.cs.rug.nl/svcg/Shapes/CUDASkel).

* SplineFit
This function fits skeletons with B-spline curves using a least-squares algorithm. See details [here](https://www.geometrictools.com/Documentation/BSplineCurveLeastSquaresFit.pdf).

* writeColorCP
This function encodes control points of B-splines into the ofstream. Later the generated ofstream will be compressed further with an external compressor. You can set what external compression method you prefer by parameter 'compression_method' in the configuration file.

imShow
---------

This program is responsible for decode the SIR file and then do the reconstruction.

* ImageDecoder::load
Load SIR file, decompress data and decode data by the function decode_plane().

* get_interp_layer
This function achieves a smooth distance-based interpolation between two neighboring layers in the Hausdorff sense. See details in the paper "A Dense Medial Descriptor for Image Analysis".

* getAlphaMapOfLayer
This function draws all disks. The alpha will be 1 at the location that should be drawn, otherwise 0. 

# 4. Other remarks


As said before, config.txt is a configuration file. The following is a detailed explanation of all parameters:

* outputLevel
This parameter controls which type do you want to output in the terminal. Available options are 'q', 'e', 'n' and 'v'. See the main function in detail.

* colorspace 
This parameter determines which color space do you want to use for color images. Options contain 'hsv', 'ycbcr'/'yuv', 'rgb' and leave it empty.

* layer_selection
This one set which layer selection method do you want to use. 'cumulative' is for cumulative histogram method, other setting or empty lead to local-maxima method. 

* compression_method
This parameter controls which external compression algorithm do you want to use. 'zpaq' has been confirmed to be a very effective one.
 
* encoding
This one set which encoding method to use. 'traditional' means the delta-encoding method, which has been proved to be an effective algorithm when used with the 'zpaq' external compression. See details in the CDMD paper.

*overlap_pruning = false

*overlap_pruning_epsilon = .05

*bundle = false

*alpha = 1

*epsilon = 5

Because the overlap pruning and bundling techniques are not mature enough, we prefer not to use these two functions.

* outputFile 
This parameter indicates where to put the output file, i.e., the .sir file.

* filename
Input image, must be PGM or PPM file.

* num_layers
One of the most important parameters. It indicates how many layers do you want to choose. '15', '20', and '25' are good range in general.

* islandThreshold
What size of noise do you want to remove. In general, 0.001-0.02 should be a good range.

* ssThreshold
Another important parameter. It represents the saliency skeleton threshold. 0.2-2 should be a good range.

* SkeletonThreshold 
This parameter is a support for the saliency skeleton. After the saliency skeleton thresholding, there will be a collection of disconnected, skeletal components. Only the longest 'core' component/fragment should be preserved. So this parameter is used to filter those small disconnected skeleton branches. See details in the paper "Feature Preserving Smoothing of Shapes using Saliency Skeletons".

* distinguishable_interval
This one is a support for the layer selection procedure. It represents the smallest gray level difference that the human eye can recognize. It is set to 5 in general.

* hausdorff
This parameter determines how accurately B-splines fit the skeletons. 0.001-0.003 should be a good range.
-------------
