function ssimval = ssim_(img1, num)

img1 = imread(img1);
%img2 = imread(img2);

%Add the following to process color image.
dimension = numel(size(img1));
if dimension==3 img_1 = rgb2gray(img1);end

x=double(imread('imShow/R.pgm'));
y=double(imread('imShow/G.pgm'));
z=double(imread('imShow/B.pgm'));

r = uint8(min(max(0, round(x                      + 1.402  * (z - 128))), 255));
g = uint8(min(max(0, round(x - 0.3441 * (y - 128) - 0.7141 * (z - 128))), 255));
b = uint8(min(max(0, round(x + 1.772  * (y - 128)                     )), 255));

combined_RGB = cat(3,r,g,b); 

OutString=num2str(num);
imwrite(combined_RGB, OutString, 'png');

img2 = rgb2gray(combined_RGB);

ssimval = ssim(img2,img_1);


%%%PSNR_RGB%%%
psnr_color = PSNR_RGB(img1,combined_RGB);

fid = fopen('../outputFile/skeleton_msssim2.txt','a');
fprintf(fid,'%f\n',ssimval);
fclose(fid);

fid = fopen('../outputFile/psnr.txt','a');
fprintf(fid,'%f\n',psnr_color);
fclose(fid);