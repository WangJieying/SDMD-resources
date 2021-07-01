function ssimval = ssim_small(img1, num)

img1 = imread(img1);
%img2 = imread(img2);

%Add the following to process color image.
dimension = numel(size(img1));
if dimension==3 img1 = rgb2gray(img1);end

x=double(imread('imShow/R.pgm'));
y=double(imread('imShow/G.pgm'));
z=double(imread('imShow/B.pgm'));

r = uint8(min(max(0, round(x                      + 1.402  * (z - 128))), 255));
g = uint8(min(max(0, round(x - 0.3441 * (y - 128) - 0.7141 * (z - 128))), 255));
b = uint8(min(max(0, round(x + 1.772  * (y - 128)                     )), 255));

combined_RGB = cat(3,r,g,b); 
RGB_small = imresize(combined_RGB, [200 NaN]);

OutString=num2str(num);
imwrite(RGB_small, OutString, 'png');

img2 = rgb2gray(RGB_small);

ssimval = ssim(img2,img1);

fid = fopen('../outputFile/skeleton_msssim2.txt','a');
fprintf(fid,'%f\n',ssimval);
fclose(fid);