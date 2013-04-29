#include <cstdlib>
#include <cmath>
#include "image.h"

// ===================================================================================================
// ===================================================================================================


void
Compress(
		const Image<Color> &input,
		Image<bool> &occupancy,
		Image<Color> &hash_data,
		Image<Offset> &offset)
{
	/* Calculate p + occupancy */
	int h, w, p = 0;
	w = input.Width();
	h = input.Height();
	occupancy.Allocate(w, h);
	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			bool not_white = !input.GetPixel(x, y).isWhite();
			occupancy.SetPixel(x, y, not_white);
			p += not_white; // FIXME bad?
		}
	}

	/* Construct hash_data + offset */
	int s_hash, s_offset;
	s_hash = (int) ceil(sqrt((double) p * 1.01));
	s_offset = (int) ceil(sqrt((double) p) / 2.0);
	// TODO use these s's to hash_data and offset
	hash_data.Allocate(s_hash, s_hash);
	offset.Allocate(s_offset, s_offset);
	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			Color c = input.GetPixel(x, y);
			hash_data.SetPixel(x % s_hash, y % s_hash, c);
			Offset o(0, 0);
			offset.SetPixel(x % s_offset, y % s_offset, o);
		}
	}
}


void
UnCompress(
		const Image<bool> &occupancy,
		const Image<Color> &hash_data,
		const Image<Offset> &offset,
		Image<Color> &output)
{
	/* Fetch useful values */
	int h, w, hh, hw, oh, ow;
	w = occupancy.Width();
	h = occupancy.Height();
	hw = hash_data.Width();
	hh = hash_data.Height();
	ow = offset.Width();
	oh = offset.Height();

	/* Set output pixels */
	output.Allocate(w, h);
	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			if (occupancy.GetPixel(x, y)) {
				Offset o = offset.GetPixel(x % ow, y % oh);
				Color c = hash_data.GetPixel((x + o.dx) % hw, (y + o.dy) % hh);
				output.SetPixel(x, y, c);
			}
		}
	}
}


// ===================================================================================================
// ===================================================================================================

void Compare(const Image<Color> &input1, const Image<Color> &input2, Image<bool> &output) {

  // confirm that the files are the same size
  if (input1.Width() != input2.Width() ||
      input1.Height() != input2.Height()) {
    std::cerr << "Error: can't compare images of different dimensions: " 
         << input1.Width() << "x" << input1.Height() << " vs " 
         << input2.Width() << "x" << input2.Height() << std::endl;
  } else {
    
    // confirm that the files are the same size
    output.Allocate(input1.Width(),input1.Height());
    int count = 0;
    for (int i = 0; i < input1.Width(); i++) {
      for (int j = 0; j < input1.Height(); j++) {
        Color c1 = input1.GetPixel(i,j);
        Color c2 = input2.GetPixel(i,j);
        if (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b)
          output.SetPixel(i,j,true);
        else {
          count++;
          output.SetPixel(i,j,false);
        }
      }      
    }     

    // confirm that the files are the same size
    if (count == 0) {
      std::cout << "The images are identical." << std::endl;
    } else {
      std::cout << "The images differ at " << count << " pixel(s)." << std::endl;
    }
  }
}

// ===================================================================================================
// ===================================================================================================


// to allow visualization of the custom offset image format
void ConvertOffsetToColor(const Image<Offset> &input, Image<Color> &output) {
  // prepare the output image to be the same size as the input image
  output.Allocate(input.Width(),input.Height());
  for (int i = 0; i < output.Width(); i++) {
    for (int j = 0; j < output.Height(); j++) {
      // grab the offset value for this pixel in the image
      Offset off = input.GetPixel(i,j);
      // set the pixel in the output image
      int dx = off.dx;
      int dy = off.dy;
      assert (dx >= 0 && dx <= 15);
      assert (dy >= 0 && dy <= 15);
      // to make a pretty image with purple, cyan, blue, & white pixels:
      int red = dx * 16;
      int green = dy *= 16;
      int blue = 255;
      assert (red >= 0 && red <= 255);
      assert (green >= 0 && green <= 255);
      output.SetPixel(i,j,Color(red,green,blue));
    }
  }
}


// ===================================================================================================
// ===================================================================================================

void usage(char* executable) {
  std::cerr << "Usage:  4 options" << std::endl;
  std::cerr << "  1)  " << executable << " compress input.ppm occupancy.pbm data.ppm offset.offset" << std::endl;
  std::cerr << "  2)  " << executable << " uncompress occupancy.pbm data.ppm offset.offset output.ppm" << std::endl;
  std::cerr << "  3)  " << executable << " compare input1.ppm input2.ppm output.pbm" << std::endl;
  std::cerr << "  4)  " << executable << " visualize_offset input.offset output.ppm" << std::endl;
}


// ===================================================================================================
// ===================================================================================================

int main(int argc, char* argv[]) {
  if (argc < 2) { usage(argv[0]); exit(1); }

  if (argv[1] == std::string("compress")) {
    if (argc != 6) { usage(argv[0]); exit(1); }
    // the original image:
    Image<Color> input;
    // 3 files form the compressed representation:
    Image<bool> occupancy;
    Image<Color> hash_data;
    Image<Offset> offset;
    input.Load(argv[2]);
    Compress(input,occupancy,hash_data,offset);
    // save the compressed representation
    occupancy.Save(argv[3]);
    hash_data.Save(argv[4]);
    offset.Save(argv[5]);

  } else if (argv[1] == std::string("uncompress")) {
    if (argc != 6) { usage(argv[0]); exit(1); }
    // the compressed representation:
    Image<bool> occupancy;
    Image<Color> hash_data;
    Image<Offset> offset;
    occupancy.Load(argv[2]);
    hash_data.Load(argv[3]);
    offset.Load(argv[4]);
    // the reconstructed image
    Image<Color> output;
    UnCompress(occupancy,hash_data,offset,output);
    // save the reconstruction
    output.Save(argv[5]);
  
  } else if (argv[1] == std::string("compare")) {
    if (argc != 5) { usage(argv[0]); exit(1); }
    // the original images
    Image<Color> input1;
    Image<Color> input2;    
    input1.Load(argv[2]);
    input2.Load(argv[3]);
    // the difference image
    Image<bool> output;
    Compare(input1,input2,output);
    // save the difference
    output.Save(argv[4]);

  } else if (argv[1] == std::string("visualize_offset")) {
    if (argc != 4) { usage(argv[0]); exit(1); }
    // the 8-bit offset image (custom format)
    Image<Offset> input;
    input.Load(argv[2]);
    // a 24-bit color version of the image
    Image<Color> output;
    ConvertOffsetToColor(input,output);
    output.Save(argv[3]);

  } else {
    usage(argv[0]);
    exit(1);
  }
}
