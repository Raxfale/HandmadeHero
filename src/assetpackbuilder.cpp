//
// Handmade Hero - asset builder
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include <QGuiApplication>
#include <QImage>
#include <QFile>
#include <iostream>
#include <fstream>
#include "asset.h"
#include "assetpack.h"

using namespace std;

const float PI = 3.14159265359;


void write_header(ostream &fout)
{
  fout << '\xCA';
  fout << 'H' << 'H' << 'A';
  fout << '\x0D' << '\x0A';
  fout << '\x1A';
  fout << '\x0A';
}


void write_chunk(ostream &fout, const char type[4], uint32_t length, void const *data)
{
  uint32_t checksum = 0;

  for(size_t i = 0; i < length; ++i)
    checksum ^= static_cast<uint8_t const*>(data)[i] << (i % 4);

  fout.write((char*)&length, sizeof(length));
  fout.write((char*)type, 4);
  fout.write((char*)data, length);
  fout.write((char*)&checksum, sizeof(checksum));
}


void write_compressed_chunk(ostream &fout, const char type[4], uint32_t length, void const *data)
{
  write_chunk(fout, type, length, data);
}


void write_image_asset(ostream &fout, AssetType type, const char *path, std::vector<AssetTag> const &tags = {})
{
  QImage image(path);

  image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

  PackAssetHeader aset = { static_cast<uint32_t>(type) };

  write_chunk(fout, "ASET", sizeof(aset), &aset);

  for(auto &tag : tags)
  {
    PackAssetTag atag = { static_cast<uint32_t>(tag.id), tag.value };

    write_chunk(fout, "ATAG", sizeof(atag), &atag);
  }

  PackImageHeader ihdr = { (uint32_t)image.width(), (uint32_t)image.height(), (size_t)fout.tellp() + sizeof(ihdr) + sizeof(PackChunk) + sizeof(uint32_t) };

  write_chunk(fout, "IHDR", sizeof(ihdr), &ihdr);

  write_chunk(fout, "IDAT", image.byteCount(), image.bits());

  write_chunk(fout, "AEND", 0, nullptr);

  cout << "  " << path << endl;
}


void compress(const char *path)
{
  ifstream fin(path, ios::binary);

  fstream fout("tmp.hha", ios::in | ios ::out | ios::binary | ios::trunc);

  write_header(fout);

  // First Pass, add asset metadata

  fin.seekg(sizeof(PackHeader), ios::beg);

  while (fin)
  {
    PackChunk chunk;

    fin.read((char*)&chunk, sizeof(chunk));

    if (chunk.type == 0x444e4548) // HEND
      break;

    switch (chunk.type)
    {
      case 0x54455341: // ASET
      case 0x47415441: // ATAG
      case 0x52444849: // IHDR
      case 0x444e4541: // AEND
        {
          std::vector<char> buffer(chunk.length);

          fin.read(buffer.data(), chunk.length);

          write_chunk(fout, (char*)&chunk.type, buffer.size(), buffer.data());

          break;
        }

      default:
        fin.seekg(chunk.length, ios::cur);
    }

    fin.seekg(sizeof(uint32_t), ios::cur);
  }

  write_chunk(fout, "HEND", 0, nullptr);

  // Second Pass, add compressed data

  fout.seekg(sizeof(PackHeader), ios::beg);

  while (fout)
  {
    PackChunk chunk;

    fout.read((char*)&chunk, sizeof(chunk));

    if (chunk.type == 0x444e4548) // HEND
      break;

    auto position = fout.tellg();

    switch (chunk.type)
    {
       case 0x52444849: // IHDR
        {
          PackImageHeader ihdr;

          fout.read((char*)&ihdr, sizeof(ihdr));

          // write compressed idat

          PackChunk idat;

          fin.seekg(ihdr.dataoffset);

          fin.read((char*)&idat, sizeof(idat));

          std::vector<char> buffer(idat.length);

          fin.read(buffer.data(), idat.length);

          fout.seekp(0, ios::end);

          ihdr.dataoffset = fout.tellp();

          write_compressed_chunk(fout, "IDAT", buffer.size(), buffer.data());

          // rewrite ihdr

          fout.seekg((size_t)position - sizeof(PackChunk), ios::beg);

          write_chunk(fout, "IHDR", sizeof(ihdr), &ihdr);

          break;
        }
    }

    fout.seekg(position, ios::beg);
    fout.seekg(chunk.length + sizeof(uint32_t), ios::cur);
  }

  fin.close();
  fout.close();

  QFile::remove(path);
  QFile::rename("tmp.hha", path);
}


void write_test1()
{
  ofstream fout("test1.hha", ios::binary | ios::trunc);

  write_header(fout);

  write_image_asset(fout, AssetType::HeroHead, "../../data/test/test_hero_front_head.bmp", { { AssetTagId::Orientation, 0*PI/2 }, { AssetTagId::Orientation, 2*PI } });
  write_image_asset(fout, AssetType::HeroHead, "../../data/test/test_hero_right_head.bmp", { { AssetTagId::Orientation, 1*PI/2 } });
  write_image_asset(fout, AssetType::HeroHead, "../../data/test/test_hero_back_head.bmp", { { AssetTagId::Orientation, 2*PI/2 } });
  write_image_asset(fout, AssetType::HeroHead, "../../data/test/test_hero_left_head.bmp", { { AssetTagId::Orientation, 3*PI/2 } });

  write_image_asset(fout, AssetType::HeroTorso, "../../data/test/test_hero_front_torso.bmp", { { AssetTagId::Orientation, 0*PI/2 }, { AssetTagId::Orientation, 2*PI } });
  write_image_asset(fout, AssetType::HeroTorso, "../../data/test/test_hero_right_torso.bmp", { { AssetTagId::Orientation, 1*PI/2 } });
  write_image_asset(fout, AssetType::HeroTorso, "../../data/test/test_hero_back_torso.bmp", { { AssetTagId::Orientation, 2*PI/2 } });
  write_image_asset(fout, AssetType::HeroTorso, "../../data/test/test_hero_left_torso.bmp", { { AssetTagId::Orientation, 3*PI/2 } });

  write_image_asset(fout, AssetType::HeroCape, "../../data/test/test_hero_front_cape.bmp", { { AssetTagId::Orientation, 0*PI/2 }, { AssetTagId::Orientation, 2*PI } });
  write_image_asset(fout, AssetType::HeroCape, "../../data/test/test_hero_right_cape.bmp", { { AssetTagId::Orientation, 1*PI/2 } });
  write_image_asset(fout, AssetType::HeroCape, "../../data/test/test_hero_back_cape.bmp", { { AssetTagId::Orientation, 2*PI/2 } });
  write_image_asset(fout, AssetType::HeroCape, "../../data/test/test_hero_left_cape.bmp", { { AssetTagId::Orientation, 3*PI/2 } });

  write_chunk(fout, "HEND", 0, nullptr);

  fout.close();

  compress("test1.hha");
}


void write_test2()
{
  ofstream fout("test2.hha", ios::binary | ios::trunc);

  write_header(fout);

  write_image_asset(fout, AssetType::Tree, "../../data/test2/tree00.bmp");
  write_image_asset(fout, AssetType::Tree, "../../data/test2/tree01.bmp");
  write_image_asset(fout, AssetType::Tree, "../../data/test2/tree02.bmp");

  write_chunk(fout, "HEND", 0, nullptr);

  fout.close();

//  compress("test2.hha");
}


int main(int argc, char **argv)
{
  QGuiApplication app(argc, argv);

  cout << "Asset Builder" << endl;

  try
  {
    write_test1();
    write_test2();
  }
  catch(std::exception &e)
  {
    cerr << "Critical Error: " << e.what() << endl;
  }
}
