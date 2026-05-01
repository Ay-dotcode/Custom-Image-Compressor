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
  char magic[4];
  uint32_t width;
  uint32_t height;
  uint32_t rleByteCount;

  AYHeader() : width(0), height(0), rleByteCount(0) {
    magic[0] = 'O';
    magic[1] = 'W';
    magic[2] = 'O';
    magic[3] = 'Z';
  }
  AYHeader(uint32_t w, uint32_t h, uint32_t rleBytes)
      : width(w), height(h), rleByteCount(rleBytes) {
    magic[0] = 'O';
    magic[1] = 'W';
    magic[2] = 'O';
    magic[3] = 'Z';
  }
  bool isValid() const {
    return (magic[0] == 'O' && magic[1] == 'W' && magic[2] == 'O' &&
            magic[3] == 'Z');
  }
};
#pragma pack(pop)

struct HuffNode {
  uint8_t byteVal;
  uint32_t freq;
  unique_ptr<HuffNode> left;
  unique_ptr<HuffNode> right;

  HuffNode(uint8_t val, uint32_t f)
      : byteVal(val), freq(f), left(nullptr), right(nullptr) {}

  HuffNode(unique_ptr<HuffNode> l, unique_ptr<HuffNode> r)
      : byteVal(0), freq(l->freq + r->freq), left(std::move(l)),
        right(std::move(r)) {}
};

struct CompareNode {
  bool operator()(const unique_ptr<HuffNode> &l,
                  const unique_ptr<HuffNode> &r) {
    if (l->freq != r->freq)
      return l->freq > r->freq;
    return l->byteVal > r->byteVal;
  }
};

bool saveAYFile(const string &, const ColorImage &);
ColorImage loadAYFile(const string &);
bool isSameColor(const RGBA &, const RGBA &);
vector<uint8_t> compressRLE(const ColorImage &);
ColorImage decompressRLE(const vector<uint8_t> &, uint32_t, uint32_t);
unique_ptr<HuffNode> rebuildTreeFromTable(uint32_t freq[256]);
unique_ptr<HuffNode> buildHuffmanTree(const vector<uint8_t> &);
void generateCodes(HuffNode *, string, unordered_map<uint8_t, string> &);

int main() {
  cout << "\n  .ay Custom Image Compressor\n";

  string inputPng = "input.png";
  string outputAy = "data.ay";
  string outputPng = "output.png";

  cout << "[1/2] Loading " << inputPng << " for compression...\n";
  ColorImage I;
  I.Load(inputPng);

  if (I.GetWidth() == 0) {
    cout << "Error: Could not find or load " << inputPng << endl;
    return 1;
  }

  if (saveAYFile(outputAy, I)) {
    cout << "      -> Successfully encoded to " << outputAy << "\n";
  } else {
    cout << "Error: Failed to save .ay file.\n";
    return 1;
  }

  cout << "[2/2] Loading " << outputAy << " for decompression...\n";
  ColorImage out = loadAYFile(outputAy);

  if (out.GetWidth() > 0) {
    out.Save(outputPng);
    cout << "      -> Successfully reconstructed to " << outputPng << "\n";
  } else {
    cout << "Error: Failed to decode .ay file.\n";
    return 1;
  }

  cout << "\nPipeline Complete!\n";
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

  AYHeader header(img.GetWidth(), img.GetHeight(),
                  static_cast<uint32_t>(rlePayload.size()));
  outFile.write(reinterpret_cast<const char *>(&header), sizeof(AYHeader));
  outFile.write(reinterpret_cast<const char *>(freq), sizeof(freq));

  uint8_t bitBuffer = 0;
  int bitCount = 0;

  for (uint8_t byte : rlePayload) {
    const string &code = dictionary[byte];
    
    for (char bit : code) {
      bitBuffer <<= 1;
      if (bit == '1')
        bitBuffer |= 1;
      if (++bitCount == 8) {
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
  if (!root)
    return ColorImage();

  vector<uint8_t> rleData;
  rleData.reserve(header.rleByteCount);
  HuffNode *currentNode = root.get();
  uint8_t byte;

  while (rleData.size() < header.rleByteCount &&
         inFile.read(reinterpret_cast<char *>(&byte), 1)) {
    for (int i = 7; i >= 0 && rleData.size() < header.rleByteCount; i--) {
      bool bit = (byte >> i) & 1;
      currentNode = bit ? currentNode->right.get() : currentNode->left.get();

      if (!currentNode->left && !currentNode->right) {
        rleData.push_back(currentNode->byteVal);
        currentNode = root.get();
      }
    }
  }
  return decompressRLE(rleData, header.width, header.height);
}

unique_ptr<HuffNode> rebuildTreeFromTable(uint32_t freq[256]) {
  vector<unique_ptr<HuffNode>> heap;
  for (int i = 0; i < 256; i++)
    if (freq[i] > 0)
      heap.push_back(make_unique<HuffNode>(static_cast<uint8_t>(i), freq[i]));

  if (heap.empty())
    return nullptr;

  if (heap.size() == 1)
    heap.push_back(make_unique<HuffNode>(static_cast<uint8_t>(0), 0u));

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
unique_ptr<HuffNode> buildHuffmanTree(const vector<uint8_t> &data) {
  uint32_t freq[256] = {0};
  for (uint8_t b : data)
    freq[b]++;
  return rebuildTreeFromTable(freq);
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
  int totPix = static_cast<int>(w * h);

  while (data.size() - dataIdx >= 5 && pixIdx < totPix) {
    uint8_t len = data[dataIdx++];
    uint8_t r = data[dataIdx++];
    uint8_t g = data[dataIdx++];
    uint8_t b = data[dataIdx++];
    uint8_t a = data[dataIdx++];
    RGBA color(r, g, b, a);

    for (int i = 0; i < len && pixIdx < totPix; i++) {
      img(pixIdx % w, pixIdx / w) = color;
      pixIdx++;
    }
  }
  return img;
}