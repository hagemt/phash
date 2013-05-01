#include <cstdlib>
#include <cmath>
#include <utility>
#include <vector>

#if __cplusplus == 199711L
#include <tr1/unordered_set>
#else // C++11?
#include <unordered_set>
#endif // C++

#include "image.h"

namespace std {
#if __cplusplus == 199711L
namespace tr1 {
#endif // C++
template<> struct hash<Color> {
	typedef Color argument_type;
	typedef std::size_t result_type;
	result_type operator()(const argument_type &arg) const {
		result_type val = (result_type) (arg.red ^ arg.green ^ arg.blue);
		return val;
	}
};
#if __cplusplus == 199711L
} // namespace tr1
#endif // C++
} // namespace std

// ============================================================================
// ============================================================================

/* Some useful definitions and helpers */
typedef std::vector<int> ROW;
#if __cplusplus == 199711L
typedef std::tr1::unordered_set<Color> BUCKET;
#else // C++11?
typedef std::unordered_set<Color> BUCKET;
#endif // C++
static const Color WHITE(255, 255, 255);
static const Offset ZERO(0, 0);
#define SQ(X) (X * X)

static int
Try(
		const Image<Color> &input, const Image<Offset> &offset,
		BUCKET *hash, const int s_hash, std::pair<int, int> &loc)
{
	int iw, ih, ow, oh, mode, collisions = 0;
	assert(hash);
	iw = input.Width();
	ih = input.Height();
	ow = offset.Width();
	oh = offset.Height();
	/* Track hits into the offset table */
	std::vector<ROW> hits(ow, ROW(oh, 0));
	/* Run the hashing as currently offset */
	std::pair<int, int> xy;
	for (int x = 0; x < iw; ++x) {
		for (int y = 0; y < ih; ++y) {
			Color c = input.GetPixel(x, y);
			if (!(c == WHITE)) {
				/* Lookup where this falls in offset */
				xy = std::make_pair(x % ow, y % oh);
				if (++hits[xy.first][xy.second] > mode) {
					mode = hits[xy.first][xy.second];
					loc = xy;
				}
				/* Use this offset to hash */
				Offset o = offset.GetPixel(xy.first, xy.second);
				xy = std::make_pair((x + o.dx) % s_hash, (y + o.dy) % s_hash);
				BUCKET *bucket = &hash[xy.first * s_hash + xy.second];
				/* Mark any collisions */
				if (!bucket->empty()) ++collisions;
				bucket->insert(c);
			}
		}
	}
	return collisions;
}

static void
Reset(BUCKET *&c, BUCKET *d = NULL)
{
	if (c) delete[] c;
	c = d;
}

static void
Fill(
		const Image<Color> &input,
		const Image<Offset> &offset,
		const BUCKET *temp_data,
		Image<Color> &hash_data)
{
	int ih, iw, ow, oh, hw, hh;
	iw = input.Width();
	ih = input.Height();
	ow = offset.Width();
	oh = offset.Height();
	hw = hash_data.Width();
	hh = hash_data.Height();
	/* Set the pixels to their final state */
	std::pair<int, int> xy;
	for (int x = 0; x < iw; ++x) {
		for (int y = 0; y < ih; ++y) {
			Color c = input.GetPixel(x, y);
			if (!(c == WHITE)) {
				Offset o = offset.GetPixel(x % ow, y % oh);
				xy = std::make_pair((x + o.dx) % hw, (y + o.dy) % hh);
				const BUCKET *bucket = &temp_data[xy.first * hw + xy.second];
				assert(bucket->size() == 1);
				//Color c = bucket->empty() ? WHITE : *bucket->begin();
				hash_data.SetPixel(xy.first, xy.second, *bucket->begin());
			}
		}
	}
}

static void
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
	/* Set all occupancy pixels */
	occupancy.Allocate(w, h);
	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			bool not_white = !(input.GetPixel(x, y) == WHITE);
			occupancy.SetPixel(x, y, not_white);
			p += not_white; // FIXME bad?
		}
	}
	/* These are some simple constraints */
	int s_hash, s_offset, size = w * h;
	s_hash = (int) ceil(sqrt((double) p * 1.01));
	s_offset = (int) ceil(sqrt((double) p) / 2.0);
	/* These will contain intermediate data */
	std::pair<int, int> max;
	BUCKET *colors = NULL;
	/* Repeatedly attempt to find a perfect hash-function */
	while (true) {
		/* Create a blank offset of the given size */
		offset.Allocate(s_offset, s_offset);
		offset.SetAllPixels(ZERO);
		/* If compression grows larger than the source, fail */
		if (s_hash < s_offset || size < SQ(s_hash) || size < SQ(s_offset)) {
			std::cerr << "No perfect hash-function exists!" << std::endl;
			hash_data.Allocate(s_hash, s_hash);
			hash_data.SetAllPixels(WHITE);
			Reset(colors);
			return;
		}
		/* Try all offset values to locate a collision-free hash */
		for (int i = 0; i < s_offset; ++i) {
			for (int j = 0; j < s_offset; ++j) {
				Reset(colors, new BUCKET[SQ(s_hash)]);
				/* Attempt to perform a hash using the current offsets */
				if (Try(input, offset, colors, s_hash, max)) {
					if (i == 0 && j == 0) continue;
					offset.SetAllPixels(Offset(i, j));
					// TODO use max to just increment the offset positions
					offset.SetPixel(max.first, max.second, ZERO);
				} else {
					// TODO are we really done?
					hash_data.Allocate(s_hash, s_hash);
					hash_data.SetAllPixels(WHITE);
					Fill(input, offset, colors, hash_data);
					return;
				}
			}
		}
		/* Rehash with larger offset */
		++s_offset;
		//++s_hash; // FIXME and hash?
	}
}

static void
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
	output.SetAllPixels(WHITE);
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

// ============================================================================
// ============================================================================

static void
Compare(
		const Image<Color> &input1,
		const Image<Color> &input2,
		Image<bool> &output)
{
	int i1w, i1h, i2w, i2h, count = 0;
	i1w = input1.Width();
	i1h = input1.Height();
	i2w = input2.Width();
	i2h = input2.Height();
	// confirm that the files are the same size
	if (i1w != i2w || i1h != i2h) {
		std::cerr << "Error: can't compare images with different dimensions: "
			<< i1w << "x" << i1h << " vs " << i2w << "x" << i2h << std::endl;
	} else {
		// confirm that the files have the same contents
		output.Allocate(i1w, i1h);
		for (int i = 0; i < i1w; i++) {
			for (int j = 0; j < i1h; j++) {
				Color c1 = input1.GetPixel(i, j);
				Color c2 = input2.GetPixel(i, j);
				bool mismatch = !(c1 == c2);
				count += mismatch; // FIXME bad?
				output.SetPixel(i, j, mismatch);
			}
		}
		// inform the user of the results
		std::cout << "The images ";
		if (count) {
			std::cout << "differ at " << count << " pixel(s).";
		} else {
			std::cout << "are identical.";
		}
		std::cout << std::endl;
	}
}

// ============================================================================
// ============================================================================

// to allow visualization of the custom offset image format
static void
ConvertOffsetToColor(const Image<Offset> &input, Image<Color> &output)
{
	int iw, ih, r, g, b;
	iw = input.Width();
	ih = input.Height();
	// prepare the output image to be the same size as the input image
	output.Allocate(iw, ih);
	for (int i = 0; i < iw; i++) {
		for (int j = 0; j < ih; j++) {
			// grab the offset value for this pixel in the image
			Offset off = input.GetPixel(i, j);
			// set the pixel in the output image
			r = off.dx * 16; g = off.dy * 16; b = 255;
			assert(r >= 0x00 && r <= 0xFF);
			assert(g >= 0x00 && g <= 0xFF);
			// to make a pretty image with purple, cyan, blue, & white pixels:
			output.SetPixel(i, j, Color(r, g, b));
		}
	}
}

// ============================================================================
// ============================================================================

static void
usage(char *argv)
{
	using std::cerr;
	cerr << "Four usage options:" << std::endl;
	cerr << " 1) " << argv << " compress input.ppm occupancy.pbm data.ppm offset.offset\n";
	cerr << " 2) " << argv << " uncompress occupancy.pbm data.ppm offset.offset output.ppm\n";
	cerr << " 3) " << argv << " compare input1.ppm input2.ppm output.pbm\n";
	cerr << " 4) " << argv << " visualize_offset input.offset output.ppm\n";
}

// ============================================================================
// ============================================================================

int
main(int argc, char *argv[])
{
	// The first argument should specify a command
	if (argc < 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
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
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
