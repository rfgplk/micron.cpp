#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../src/images/ubmp.hpp"
int
main()
{
  const int width = 400;
  const int height = 300;

  std::vector<micron::pixel> pixels(width * height);

  for ( int y = 0; y < height; ++y ) {
    for ( int x = 0; x < width; ++x ) {
      const double u = static_cast<double>(x) / width;
      const double v = static_cast<double>(y) / height;

      const double wave_x = std::sin(u * 3.14159 * 4) * 0.5 + 0.5;
      const double wave_y = std::cos(v * 3.14159 * 4) * 0.5 + 0.5;
      const double checkerboard = ((x / 20) + (y / 20)) % 2 == 0 ? 0.2 : 0.0;

      micron::pixel &px = pixels[y * width + x];
      px.r = wave_x * u + checkerboard;
      px.g = wave_y * v + checkerboard;
      px.b = (1.0 - u) * (1.0 - v) + checkerboard;
    }
  }

  std::string bmp_data;

  try {
    micron::writer::write(bmp_data, width, height, pixels);

    std::cout << "BMP encoded successfully!" << std::endl;
    std::cout << "Data size: " << bmp_data.size() << " bytes" << std::endl;

    std::ofstream file("output.bmp", std::ios::binary);
    if ( file ) {
      file.write(bmp_data.data(), bmp_data.size());
      file.close();
      std::cout << "Saved to output.bmp" << std::endl;
    }

    micron::header_t header = micron::reader::extract_header(bmp_data);
    std::cout << "\nBMP Header:" << std::endl;
    std::cout << "  Type: 0x" << std::hex << header.type << std::dec << std::endl;
    std::cout << "  Size: " << header.size << " bytes" << std::endl;
    std::cout << "  Offset: " << header.offset << " bytes" << std::endl;

    micron::dib_header_t dib_header = micron::reader::extract_dib_header(bmp_data);
    std::cout << "\nDIB Header:" << std::endl;
    std::cout << "  Width: " << dib_header.width << std::endl;
    std::cout << "  Height: " << dib_header.height << std::endl;
    std::cout << "  Bits per pixel: " << dib_header.bits_per_pixel << std::endl;
    std::cout << "  Compression: " << dib_header.compression << std::endl;
    std::cout << "  Image size: " << dib_header.image_size << " bytes" << std::endl;

    std::vector<micron::pixel> decoded_pixels;
    int decoded_width = 0;
    int decoded_height = 0;

    micron::reader::read(bmp_data, decoded_width, decoded_height, decoded_pixels);

    std::cout << "\nDecoded BMP: " << decoded_width << "x" << decoded_height << std::endl;
    std::cout << "Number of pixels decoded: " << decoded_pixels.size() << std::endl;

    std::cout << "\nSample pixel at (100, 100):" << std::endl;
    const micron::pixel &sample_px = decoded_pixels[100 * width + 100];
    std::cout << "  R: " << sample_px.r << std::endl;
    std::cout << "  G: " << sample_px.g << std::endl;
    std::cout << "  B: " << sample_px.b << std::endl;

    std::cout << "\nVerifying round-trip encoding/decoding..." << std::endl;
    bool match = true;
    int mismatches = 0;
    const double tolerance = 0.01;

    for ( int i = 0; i < width * height; ++i ) {
      const double dr = std::abs(pixels[i].r - decoded_pixels[i].r);
      const double dg = std::abs(pixels[i].g - decoded_pixels[i].g);
      const double db = std::abs(pixels[i].b - decoded_pixels[i].b);

      if ( dr > tolerance || dg > tolerance || db > tolerance ) {
        match = false;
        ++mismatches;
      }
    }

    if ( match ) {
      std::cout << "✓ All pixels match perfectly!" << std::endl;
    } else {
      std::cout << "✗ Found " << mismatches << " mismatched pixels (tolerance: " << tolerance << ")" << std::endl;
      std::cout << "  This is normal due to gamma correction and quantization" << std::endl;
    }

  } catch ( const std::exception &e ) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
