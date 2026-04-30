#include "./image.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

#pragma pack(push, 1)
class AYHeader {
public:
  char magic[2];
  uint32_t width;
  uint32_t height;
  uint8_t channels;

  AYHeader() : width(0), height(0), channels(4) {
    magic[0] = 'A';
    magic[1] = 'Y';
  }

  AYHeader(uint32_t w, uint32_t h, uint8_t c = 4)
      : width(w), height(h), channels(c) {
    magic[0] = 'A';
    magic[1] = 'Y';
  }

  bool isValid() const { return (magic[0] == 'A' && magic[1] == 'Y'); }
};
#pragma pack(pop)

bool isSameColor(const RGBA &, const RGBA &);
vector<uint8_t> compressRLE(const ColorImage &);
ColorImage decompressRLE(const vector<uint8_t> &, uint32_t, uint32_t);
bool saveAYFile(const string &, const ColorImage &);
ColorImage loadAYFile(const string &);

int main() {
  cout << "\n  .ay Custom Image Compressor\n";
  cout << "1. Encode (Compress PNG to .ay)\n";
  cout << "2. Decode (Decompress .ay to PNG)\n";
  cout << "3. Exit\n\n";
  cout << "Select an option (1-3): ";

  int choice;
  if (!(cin >> choice)) {
    cout << "Error: Invalid input format." << endl;
    return 1;
  }

  switch (choice) {
  case 1: {
    string inputPng, outputAy;
    cout << "Enter the input PNG filename (e.g., input.png): ";
    cin >> inputPng;
    cout << "Enter the output .ay filename (e.g., data.ay): ";
    cin >> outputAy;

    ColorImage I;
    I.Load(inputPng);

    if (I.GetWidth() == 0 || I.GetHeight() == 0) {
      cout << "Error: Failed to load " << inputPng << endl;
      return 1;
    }

    cout << "Compressing and saving to " << outputAy << endl;
    if (!saveAYFile(outputAy, I)) {
      cout << "Error: Failed to save .ay file" << endl;
      return 1;
    }
    cout << "Successfully encoded image to " << outputAy << endl;
    break;
  }
  case 2: {
    string inputAy, outputPng;

    cout << "Enter the input .ay filename (e.g., data.ay): ";
    cin >> inputAy;
    cout << "Enter the output PNG filename (e.g., output.png): ";
    cin >> outputPng;
    cout << "Loading and decompressing from " << inputAy << endl;
    ColorImage outputImg = loadAYFile(inputAy);

    if (outputImg.GetWidth() > 0) {
      outputImg.Save(outputPng);
      cout << "Successfully reconstructed image to " << outputPng << endl;
    } else {
      cout << "Error: Failed to decompress or load .ay file" << endl;
      return 1;
    }
    break;
  }
  case 3: {
    cout << "Exiting program..." << endl;
    return 0;
  }
  default: {
    cout << "Error: Invalid option selected." << endl;
    return 1;
  }
  }

  return 0;
}

bool isSameColor(const RGBA &p1, const RGBA &p2) {
  return (p1.r == p2.r && p1.g == p2.g && p1.b == p2.b && p1.a == p2.a);
}
bool saveAYFile(const string &filename, const ColorImage &img) {
  AYHeader header(img.GetWidth(), img.GetHeight());
  vector<uint8_t> payload = compressRLE(img);
  ofstream outFile(filename, ios::binary);
  if (!outFile)
    return false;
  outFile.write(reinterpret_cast<const char *>(&header), sizeof(AYHeader));

  if (!payload.empty())
    outFile.write(reinterpret_cast<const char *>(payload.data()),
                  payload.size());

  outFile.close();
  return true;
}

ColorImage loadAYFile(const string &filename) {
  ifstream inFile(filename, ios::binary | ios::ate);
  if (!inFile)
    return ColorImage();
  streamsize fileSize = inFile.tellg();
  inFile.seekg(0, ios::beg);
  AYHeader header;
  inFile.read(reinterpret_cast<char *>(&header), sizeof(AYHeader));

  if (!header.isValid()) {
    cout << "Error: File is not a valid .ay format!" << endl;
    return ColorImage();
  }

  streamsize payloadSize = fileSize - sizeof(AYHeader);
  vector<uint8_t> payload(payloadSize);

  if (payloadSize > 0) {
    inFile.read(reinterpret_cast<char *>(payload.data()), payloadSize);
  }
  inFile.close();

  return decompressRLE(payload, header.width, header.height);
}

vector<uint8_t> compressRLE(const ColorImage &img) {
  vector<uint8_t> data;
  return data;
}

ColorImage decompressRLE(const vector<uint8_t> &data, uint32_t w, uint32_t h) {
  ColorImage img(w, h);
  return img;
}