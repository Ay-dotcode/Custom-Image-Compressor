#include "./image.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
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

struct HuffNode {
  uint8_t byteVal;
  uint32_t freq;
  unique_ptr<HuffNode> left;
  unique_ptr<HuffNode> right;

  HuffNode(uint8_t val, uint32_t freq)
      : byteVal(val), freq(freq), left(nullptr), right(nullptr) {}

  HuffNode(unique_ptr<HuffNode> l, unique_ptr<HuffNode> r)
      : byteVal(0), freq(l->freq + r->freq), left(std::move(l)),
        right(std::move(r)) {}
};

struct CompareNode {
  bool operator()(const unique_ptr<HuffNode> &l,
                  const unique_ptr<HuffNode> &r) {
    return l->freq > r->freq;
  }
};

bool saveAYFile(const string &, const ColorImage &);
ColorImage loadAYFile(const string &);
bool isSameColor(const RGBA &, const RGBA &);
vector<uint8_t> compressRLE(const ColorImage &);
ColorImage decompressRLE(const vector<uint8_t> &, uint32_t, uint32_t);
unique_ptr<HuffNode> buildHuffmanTree(const vector<uint8_t> &);
unique_ptr<HuffNode> rebuildTreeFromTable(uint32_t freq[256]);
void generateCodes(HuffNode *, string, unordered_map<uint8_t, string> &);

int main() {
  cout << "\n  .ay Custom Image Compressor\n";
  cout << "1. Encode (Compress PNG to .ay)\n";
  cout << "2. Decode (Decompress .ay to PNG)\n";
  cout << "3. Exit\n\n";
  cout << "Select an option (1-3): ";

  int choice;
  if (!(cin >> choice))
    return 1;

  switch (choice) {
  case 1: {
    string inputPng, outputAy;
    cout << "Enter input PNG (e.g., input.png): ";
    cin >> inputPng;
    cout << "Enter output .ay (e.g., data.ay): ";
    cin >> outputAy;
    ColorImage I;
    I.Load(inputPng);
    if (I.GetWidth() == 0)
      return 1;
    if (saveAYFile(outputAy, I))
      cout << "Success!\n";
    break;
  }
  case 2: {
    string inputAy, outputPng;
    cout << "Enter input .ay (e.g., data.ay): ";
    cin >> inputAy;
    cout << "Enter output PNG (e.g., output.png): ";
    cin >> outputPng;
    ColorImage out = loadAYFile(inputAy);
    if (out.GetWidth() > 0) {
      out.Save(outputPng);
      cout << "Success!\n";
    }
    break;
  }
  default:
    return 0;
  }
  return 0;
}

bool saveAYFile(const string &filename, const ColorImage &img) {
  vector<uint8_t> rlePayload = compressRLE(img);
  if (rlePayload.empty())
    return false;

  uint32_t freq[256] = {0};
  for (uint8_t b : rlePayload)
    freq[b]++;

  unique_ptr<HuffNode> root = buildHuffmanTree(rlePayload);
  unordered_map<uint8_t, string> dictionary;
  generateCodes(root.get(), "", dictionary);

  ofstream outFile(filename, ios::binary);
  if (!outFile)
    return false;

  AYHeader header(img.GetWidth(), img.GetHeight());
  outFile.write(reinterpret_cast<const char *>(&header), sizeof(AYHeader));
  outFile.write(reinterpret_cast<const char *>(freq), sizeof(freq));

  uint8_t bitBuffer = 0;
  int bitCount = 0;

  for (uint8_t byte : rlePayload) {
    string code = dictionary[byte];
    for (char bit : code) {
      bitBuffer <<= 1;
      if (bit == '1')
        bitBuffer |= 1;
      bitCount++;
      if (bitCount == 8) {
        outFile.write(reinterpret_cast<const char *>(&bitBuffer), 1);
        bitBuffer = 0;
        bitCount = 0;
      }
    }
  }

  if (bitCount > 0) {
    bitBuffer <<= (8 - bitCount);
    outFile.write(reinterpret_cast<const char *>(&bitBuffer), 1);
  }
  return true;
}

ColorImage loadAYFile(const string &filename) {
  ifstream inFile(filename, ios::binary);
  if (!inFile)
    return ColorImage();

  AYHeader header;
  inFile.read(reinterpret_cast<char *>(&header), sizeof(AYHeader));
  if (!header.isValid())
    return ColorImage();

  uint32_t freq[256];
  inFile.read(reinterpret_cast<char *>(freq), sizeof(freq));

  unique_ptr<HuffNode> root = rebuildTreeFromTable(freq);
  vector<uint8_t> rleData;
  HuffNode *currentNode = root.get();
  uint8_t byte;

  uint32_t totalPixels = header.width * header.height;
  uint32_t pixelsDecoded = 0;

  while (inFile.read(reinterpret_cast<char *>(&byte), 1)) {
    for (int i = 7; i >= 0; i--) {
      bool bit = (byte >> i) & 1;
      currentNode = bit ? currentNode->right.get() : currentNode->left.get();

      if (!currentNode->left && !currentNode->right) {
        rleData.push_back(currentNode->byteVal);
        if (rleData.size() % 5 == 0)
          pixelsDecoded += rleData[rleData.size() - 5];
        currentNode = root.get();
        if (pixelsDecoded >= totalPixels)
          break;
      }
    }
    if (pixelsDecoded >= totalPixels)
      break;
  }
  return decompressRLE(rleData, header.width, header.height);
}

unique_ptr<HuffNode> buildHuffmanTree(const vector<uint8_t> &data) {
  uint32_t freq[256] = {0};
  for (uint8_t byte : data)
    freq[byte]++;
  vector<unique_ptr<HuffNode>> heap;
  for (int i = 0; i < 256; i++)
    if (freq[i] > 0)
      heap.push_back(make_unique<HuffNode>((uint8_t)i, freq[i]));

  if (heap.empty())
    return nullptr;
  if (heap.size() == 1)
    heap.push_back(make_unique<HuffNode>((uint8_t)0, (uint32_t)0));

  make_heap(heap.begin(), heap.end(), CompareNode());
  while (heap.size() > 1) {
    pop_heap(heap.begin(), heap.end(), CompareNode());
    unique_ptr<HuffNode> left = std::move(heap.back());
    heap.pop_back();
    pop_heap(heap.begin(), heap.end(), CompareNode());
    unique_ptr<HuffNode> right = std::move(heap.back());
    heap.pop_back();
    heap.push_back(make_unique<HuffNode>(std::move(left), std::move(right)));
    push_heap(heap.begin(), heap.end(), CompareNode());
  }
  return std::move(heap.front());
}

unique_ptr<HuffNode> rebuildTreeFromTable(uint32_t freq[256]) {
  vector<unique_ptr<HuffNode>> heap;
  for (int i = 0; i < 256; i++)
    if (freq[i] > 0)
      heap.push_back(make_unique<HuffNode>((uint8_t)i, freq[i]));

  if (heap.empty())
    return nullptr;
  if (heap.size() == 1)
    heap.push_back(make_unique<HuffNode>((uint8_t)0, (uint32_t)0));

  make_heap(heap.begin(), heap.end(), CompareNode());
  while (heap.size() > 1) {
    pop_heap(heap.begin(), heap.end(), CompareNode());
    unique_ptr<HuffNode> l = std::move(heap.back());
    heap.pop_back();
    pop_heap(heap.begin(), heap.end(), CompareNode());
    unique_ptr<HuffNode> r = std::move(heap.back());
    heap.pop_back();
    heap.push_back(make_unique<HuffNode>(std::move(l), std::move(r)));
    push_heap(heap.begin(), heap.end(), CompareNode());
  }
  return std::move(heap.front());
}

void generateCodes(HuffNode *node, string path,
                   unordered_map<uint8_t, string> &dict) {
  if (!node)
    return;
  if (!node->left && !node->right) {
    dict[node->byteVal] = path;
    return;
  }
  generateCodes(node->left.get(), path + "0", dict);
  generateCodes(node->right.get(), path + "1", dict);
}

bool isSameColor(const RGBA &p1, const RGBA &p2) {
  return (p1.r == p2.r && p1.g == p2.g && p1.b == p2.b && p1.a == p2.a);
}

vector<uint8_t> compressRLE(const ColorImage &img) {
  vector<uint8_t> data;
  int w = img.GetWidth(), h = img.GetHeight();
  if (w == 0 || h == 0)
    return data;
  RGBA currPix = img(0, 0);
  uint8_t runLen = 0;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      RGBA nextPixel = img(x, y);
      if (isSameColor(currPix, nextPixel) && runLen < 255)
        runLen++;
      else {
        data.push_back(runLen);
        data.push_back(currPix.r);
        data.push_back(currPix.g);
        data.push_back(currPix.b);
        data.push_back(currPix.a);
        currPix = nextPixel;
        runLen = 1;
      }
    }
  }
  data.push_back(runLen);
  data.push_back(currPix.r);
  data.push_back(currPix.g);
  data.push_back(currPix.b);
  data.push_back(currPix.a);
  return data;
}

ColorImage decompressRLE(const vector<uint8_t> &data, uint32_t w, uint32_t h) {
  ColorImage img(w, h);
  int pixIdx = 0;
  size_t dataIdx = 0;
  int totPoss = w * h;

  while (dataIdx + 4 < data.size() && pixIdx < totPoss) {
    uint8_t len = data[dataIdx++];
    uint8_t r = data[dataIdx++], g = data[dataIdx++], b = data[dataIdx++],
            a = data[dataIdx++];
    RGBA color(r, g, b, a);

    for (int i = 0; i < len && pixIdx < totPoss; i++) {
      img(pixIdx % w, pixIdx / w) = color;
      pixIdx++;
    }
  }
  return img;
}