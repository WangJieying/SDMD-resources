function ssimval = ssim_1(img1, img2)

img1 = imread(img1);
img2 = imread(img2);

%Add the following to process color image.
dimension = numel(size(img1));
if dimension==3 img_1 = rgb2gray(img1);end

dimension = numel(size(img2));
if dimension==3 img_2 = rgb2gray(img2); else img_2 = img2; end;


ssimval = ssim(img_2,img_1);


fid = fopen('../outputFile/JPGmsssim.txt','a');
fprintf(fid,'%f\n',ssimval);
fclose(fid);
