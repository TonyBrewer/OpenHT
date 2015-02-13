/*

 * Version 1.2.5 (20120929)
 *
 * Supported Formats: 1, 4, 8, 16, 24, 32 Bit Images
 * Alpha Bitmaps are also supported.
 * Supported compression types: RLE 8, BITFIELDS
 *
 * Created by: Benjamin Kalytta, 2006 - 2012
 * Thanks for bug fixes goes to: Chris Campbell
 *
 * Licence: Free to use, URL to my source and my name is required in your source code.
 *
 * Source can be found at http://www.kalytta.com/bitmap.h
 *
 * Warning: This code should not be used in unmodified form in a production environment.
 * It should only serve as a basis for your own development.
 * There is only a minimal error handling in this code. (Notice added 20111211)
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <iostream>
#include <fstream>
#include <string>

#include <stdlib.h>
#include <string.h>


extern int global_radius;

// Need this to allocate the buffer on the coproc
#include "hif.h"

#ifndef __LITTLE_ENDIAN__
	#ifndef __BIG_ENDIAN__
		#define __LITTLE_ENDIAN__
	#endif
#endif

#ifdef __LITTLE_ENDIAN__
	#define BITMAP_SIGNATURE 0x4d42
#else
	#define BITMAP_SIGNATURE 0x424d
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int8 uint8_t;
	typedef __int32 int32_t;
#elif defined(__GNUC__) || defined(__CYGWIN__) || defined(__MWERKS__) || defined(__WATCOMC__) || defined(__PGI) || defined(__LCC__)
	#include <stdint.h>
#else
	typedef unsigned int uint32_t;
	typedef unsigned short int uint16_t;
	typedef unsigned char uint8_t;
	typedef int int32_t;
#endif

#pragma pack(push, 1)

typedef struct _BITMAP_FILEHEADER {
	uint16_t Signature;
	uint32_t Size;
	uint32_t Reserved;
	uint32_t BitsOffset;
} BITMAP_FILEHEADER;

#define BITMAP_FILEHEADER_SIZE 14

typedef struct _BITMAP_HEADER {
	uint32_t HeaderSize;
	int32_t Width;
	int32_t Height;
	uint16_t Planes;
	uint16_t BitCount;
	uint32_t Compression;
	uint32_t SizeImage;
	int32_t PelsPerMeterX;
	int32_t PelsPerMeterY;
	uint32_t ClrUsed;
	uint32_t ClrImportant;
	uint32_t RedMask;
	uint32_t GreenMask;
	uint32_t BlueMask;
	uint32_t AlphaMask;
	uint32_t CsType;
	uint32_t Endpoints[9]; // see http://msdn2.microsoft.com/en-us/library/ms536569.aspx
	uint32_t GammaRed;
	uint32_t GammaGreen;
	uint32_t GammaBlue;
} BITMAP_HEADER;

typedef struct _RGBA {
	uint8_t Red;
	uint8_t Green;
	uint8_t Blue;
	uint8_t Alpha;
} RGBA;

typedef struct _BGRA {
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;
} BGRA;

#pragma pack(pop)

class CBitmap {
private:
	BITMAP_FILEHEADER m_BitmapFileHeader;
	BITMAP_HEADER m_BitmapHeader;
	RGBA *m_BitmapData;
	unsigned int m_BitmapSize;
	
	// Masks and bit counts shouldn't exceed 32 Bits
public:
	class CColor {
public:
		static inline unsigned int BitCountByMask(unsigned int Mask) {
			unsigned int BitCount = 0;
			while (Mask) {
				Mask &= Mask - 1;
				BitCount++;
			}
			return BitCount;
		}

		static inline unsigned int BitPositionByMask(unsigned int Mask) {
			return BitCountByMask((Mask & (~Mask + 1)) - 1);
		}

		static inline unsigned int ComponentByMask(unsigned int Color, unsigned int Mask) {
			unsigned int Component = Color & Mask;
			return Component >> BitPositionByMask(Mask);
		}

		static inline unsigned int BitCountToMask(unsigned int BitCount) {
			return (BitCount == 32) ? 0xFFFFFFFF : (1 << BitCount) - 1;
		}

		static unsigned int Convert(unsigned int Color, unsigned int FromBitCount, unsigned int ToBitCount) {
			if (ToBitCount < FromBitCount) {
				Color >>= (FromBitCount - ToBitCount);
			} else {
				Color <<= (ToBitCount - FromBitCount);
				if (Color > 0) {
					Color |= BitCountToMask(ToBitCount - FromBitCount);
				}
			}
			return Color;
		}
	};

public:
	
	CBitmap() : m_BitmapData(0), m_BitmapSize(0)  {
		Dispose();
	}
	
	CBitmap(const char* Filename) : m_BitmapData(0), m_BitmapSize(0) {
		Load(Filename);
	}
	
	~CBitmap() {
		Dispose();
	}
	
	void Dispose() {
		if (m_BitmapData) {
			delete[] m_BitmapData;
			m_BitmapData = 0;
		}
		memset(&m_BitmapFileHeader, 0, sizeof(m_BitmapFileHeader));
		memset(&m_BitmapHeader, 0, sizeof(m_BitmapHeader));
	}
	
	/* Load specified Bitmap and stores it as RGBA in an internal buffer */
	
	bool Load(const char *Filename) {
		std::ifstream file(Filename, std::ios::binary | std::ios::in);
		
		if (file.bad()) {
			return false;
		}

		if (file.is_open() == false) {
			return false;
		}
		
		Dispose();
		
		file.read((char*) &m_BitmapFileHeader, BITMAP_FILEHEADER_SIZE);
		if (m_BitmapFileHeader.Signature != BITMAP_SIGNATURE) {
			return false;
		}

		file.read((char*) &m_BitmapHeader, sizeof(BITMAP_HEADER));
		
		/* Load Color Table */
		
		file.seekg(BITMAP_FILEHEADER_SIZE + m_BitmapHeader.HeaderSize, std::ios::beg);
		
		unsigned int ColorTableSize = 0;

		if (m_BitmapHeader.BitCount == 1) {
			ColorTableSize = 2;
		} else if (m_BitmapHeader.BitCount == 4) {
			ColorTableSize = 16;
		} else if (m_BitmapHeader.BitCount == 8) {
			ColorTableSize = 256;
		}


#if 0
                fprintf(stderr,"HeaderSize is %ld\n", m_BitmapHeader.HeaderSize);
                fprintf(stderr,"Width is %ld\n", m_BitmapHeader.Width);
                fprintf(stderr,"Height is %ld\n", m_BitmapHeader.Height);
                fprintf(stderr,"Planes is %ld\n", m_BitmapHeader.Planes);
                fprintf(stderr,"BitCount is %ld\n", m_BitmapHeader.BitCount);
                fprintf(stderr,"Compression is %ld\n", m_BitmapHeader.Compression);
                fprintf(stderr,"SizeImage is %ld\n", m_BitmapHeader.SizeImage);
                fprintf(stderr,"PelsPerMeterX is %ld\n", m_BitmapHeader.PelsPerMeterX);
                fprintf(stderr,"PelsPerMeterY is %ld\n", m_BitmapHeader.PelsPerMeterY);
                fprintf(stderr,"ClrUsed is %ld\n", m_BitmapHeader.ClrUsed);
                fprintf(stderr,"ClrImportant is %ld\n", m_BitmapHeader.ClrImportant);
                fprintf(stderr,"RedMask is %ld\n", m_BitmapHeader.RedMask);
                fprintf(stderr,"GreenMask is %ld\n", m_BitmapHeader.GreenMask);
                fprintf(stderr,"BlueMask is %ld\n", m_BitmapHeader.BlueMask);
                fprintf(stderr,"AlphaMask is %ld\n", m_BitmapHeader.AlphaMask);
                fprintf(stderr,"CsType is %ld\n", m_BitmapHeader.CsType);
		

                fprintf(stderr, "ColorTableSize is %d\n", ColorTableSize);
#endif

		// Always allocate full sized color table

		BGRA* ColorTable = new BGRA[ColorTableSize]; // std::bad_alloc exception should be thrown if memory is not available
		
		file.read((char*) ColorTable, sizeof(BGRA) * m_BitmapHeader.ClrUsed);

#if 0
                for (int x = 0; x < ColorTableSize; x++) {
                    printf("%d %0x red %0x blue %0x green %0x alpha \n",
                           x,
                           ColorTable[x].Red,
                           ColorTable[x].Blue,
                           ColorTable[x].Green,
                           ColorTable[x].Alpha);
                }
#endif

		/* ... Color Table for 16 bits images are not supported yet */	
		
		m_BitmapSize = GetWidth() * GetHeight();
		m_BitmapData = new RGBA[m_BitmapSize];

                fprintf(stderr,"bit count is %d\n", GetBitCount());
		
		unsigned int LineWidth = ((GetWidth() * GetBitCount() / 8) + 3) & ~3;
		uint8_t *Line = new uint8_t[LineWidth];
		
		file.seekg(m_BitmapFileHeader.BitsOffset, std::ios::beg);

		int Index = 0;
		bool Result = true;

		if (m_BitmapHeader.Compression == 0) {
			for (unsigned int i = 0; i < GetHeight(); i++) {
				file.read((char*) Line, LineWidth);

				uint8_t *LinePtr = Line;
				
				for (unsigned int j = 0; j < GetWidth(); j++) {
					if (m_BitmapHeader.BitCount == 1) {
						uint32_t Color = *((uint8_t*) LinePtr);
						for (int k = 0; k < 8; k++) {
							m_BitmapData[Index].Red = ColorTable[Color & 0x80 ? 1 : 0].Red;
							m_BitmapData[Index].Green = ColorTable[Color & 0x80 ? 1 : 0].Green;
							m_BitmapData[Index].Blue = ColorTable[Color & 0x80 ? 1 : 0].Blue;
							m_BitmapData[Index].Alpha = ColorTable[Color & 0x80 ? 1 : 0].Alpha;
							Index++;
							Color <<= 1;
						}
						LinePtr++;
						j += 7;
					} else if (m_BitmapHeader.BitCount == 4) {
						uint32_t Color = *((uint8_t*) LinePtr);
						m_BitmapData[Index].Red = ColorTable[(Color >> 4) & 0x0f].Red;
						m_BitmapData[Index].Green = ColorTable[(Color >> 4) & 0x0f].Green;
						m_BitmapData[Index].Blue = ColorTable[(Color >> 4) & 0x0f].Blue;
						m_BitmapData[Index].Alpha = ColorTable[(Color >> 4) & 0x0f].Alpha;
						Index++;
						m_BitmapData[Index].Red = ColorTable[Color & 0x0f].Red;
						m_BitmapData[Index].Green = ColorTable[Color & 0x0f].Green;
						m_BitmapData[Index].Blue = ColorTable[Color & 0x0f].Blue;
						m_BitmapData[Index].Alpha = ColorTable[Color & 0x0f].Alpha;
						Index++;
						LinePtr++;
						j++;
					} else if (m_BitmapHeader.BitCount == 8) {
						uint32_t Color = *((uint8_t*) LinePtr);
						m_BitmapData[Index].Red = ColorTable[Color].Red;
						m_BitmapData[Index].Green = ColorTable[Color].Green;
						m_BitmapData[Index].Blue = ColorTable[Color].Blue;
						m_BitmapData[Index].Alpha = ColorTable[Color].Alpha;
						Index++;
						LinePtr++;
					} else if (m_BitmapHeader.BitCount == 16) {
						uint32_t Color = *((uint16_t*) LinePtr);
						m_BitmapData[Index].Red = ((Color >> 10) & 0x1f) << 3;
						m_BitmapData[Index].Green = ((Color >> 5) & 0x1f) << 3;
						m_BitmapData[Index].Blue = (Color & 0x1f) << 3;
						m_BitmapData[Index].Alpha = 255;
						Index++;
						LinePtr += 2;
					} else if (m_BitmapHeader.BitCount == 24) {
						uint32_t Color = *((uint32_t*) LinePtr);
						m_BitmapData[Index].Blue = Color & 0xff;
						m_BitmapData[Index].Green = (Color >> 8) & 0xff;
						m_BitmapData[Index].Red = (Color >> 16) & 0xff;
						m_BitmapData[Index].Alpha = 255;
						Index++;
						LinePtr += 3;
					} else if (m_BitmapHeader.BitCount == 32) {
						uint32_t Color = *((uint32_t*) LinePtr);
						m_BitmapData[Index].Blue = Color & 0xff;
						m_BitmapData[Index].Green = (Color >> 8) & 0xff;
						m_BitmapData[Index].Red = (Color >> 16) & 0xff;
						m_BitmapData[Index].Alpha = Color >> 24;
						Index++;
						LinePtr += 4;
					}
				}
			}
		} else if (m_BitmapHeader.Compression == 1) { // RLE 8
			uint8_t Count = 0;
			uint8_t ColorIndex = 0;
			int x = 0, y = 0;

			while (file.eof() == false) {
				file.read((char*) &Count, sizeof(uint8_t));
				file.read((char*) &ColorIndex, sizeof(uint8_t));

				if (Count > 0) {
					Index = x + y * GetWidth();
					for (int k = 0; k < Count; k++) {
						m_BitmapData[Index + k].Red = ColorTable[ColorIndex].Red;
						m_BitmapData[Index + k].Green = ColorTable[ColorIndex].Green;
						m_BitmapData[Index + k].Blue = ColorTable[ColorIndex].Blue;
						m_BitmapData[Index + k].Alpha = ColorTable[ColorIndex].Alpha;
					}
					x += Count;
				} else if (Count == 0) {
					int Flag = ColorIndex;
					if (Flag == 0) {
						x = 0;
						y++;
					} else if (Flag == 1) {
						break;
					} else if (Flag == 2) {
						char rx = 0;
						char ry = 0;
						file.read((char*) &rx, sizeof(char));
						file.read((char*) &ry, sizeof(char));
						x += rx;
						y += ry;
					} else {
						Count = Flag;
						Index = x + y * GetWidth();
						for (int k = 0; k < Count; k++) {
							file.read((char*) &ColorIndex, sizeof(uint8_t));
							m_BitmapData[Index + k].Red = ColorTable[ColorIndex].Red;
							m_BitmapData[Index + k].Green = ColorTable[ColorIndex].Green;
							m_BitmapData[Index + k].Blue = ColorTable[ColorIndex].Blue;
							m_BitmapData[Index + k].Alpha = ColorTable[ColorIndex].Alpha;
						}
						x += Count;
						// Attention: Current Microsoft STL implementation seems to be buggy, tellg() always returns 0.
						if (file.tellg() & 1) {
							file.seekg(1, std::ios::cur);
						}
					}
				}
			}
		} else if (m_BitmapHeader.Compression == 2) { // RLE 4
			/* RLE 4 is not supported */
			Result = false;
		} else if (m_BitmapHeader.Compression == 3) { // BITFIELDS
			
			/* We assumes that mask of each color component can be in any order */

			uint32_t BitCountRed = CColor::BitCountByMask(m_BitmapHeader.RedMask);
			uint32_t BitCountGreen = CColor::BitCountByMask(m_BitmapHeader.GreenMask);
			uint32_t BitCountBlue = CColor::BitCountByMask(m_BitmapHeader.BlueMask);
			uint32_t BitCountAlpha = CColor::BitCountByMask(m_BitmapHeader.AlphaMask);

			for (unsigned int i = 0; i < GetHeight(); i++) {
				file.read((char*) Line, LineWidth);
				
				uint8_t *LinePtr = Line;
				
				for (unsigned int j = 0; j < GetWidth(); j++) {
					
					uint32_t Color = 0;

					if (m_BitmapHeader.BitCount == 16) {
						Color = *((uint16_t*) LinePtr);
						LinePtr += 2;
					} else if (m_BitmapHeader.BitCount == 32) {
						Color = *((uint32_t*) LinePtr);
						LinePtr += 4;
					} else {
						// Other formats are not valid
					}
					m_BitmapData[Index].Red = CColor::Convert(CColor::ComponentByMask(Color, m_BitmapHeader.RedMask), BitCountRed, 8);
					m_BitmapData[Index].Green = CColor::Convert(CColor::ComponentByMask(Color, m_BitmapHeader.GreenMask), BitCountGreen, 8);
					m_BitmapData[Index].Blue = CColor::Convert(CColor::ComponentByMask(Color, m_BitmapHeader.BlueMask), BitCountBlue, 8);
					m_BitmapData[Index].Alpha = CColor::Convert(CColor::ComponentByMask(Color, m_BitmapHeader.AlphaMask), BitCountAlpha, 8);

					Index++;
				}
			}
		}
		
		delete [] ColorTable;
		delete [] Line;

		file.close();
		return Result;
	}
	
	bool Save(const char* Filename, unsigned int BitCount = 32) {
		bool Result = true;

		std::ofstream file(Filename, std::ios::out | std::ios::binary);

		if (file.is_open() == false) {
			return false;
		}
		
		BITMAP_FILEHEADER bfh;
		BITMAP_HEADER bh;
		memset(&bfh, 0, sizeof(bfh));
		memset(&bh, 0, sizeof(bh));

		bfh.Signature = BITMAP_SIGNATURE;
		bfh.BitsOffset = BITMAP_FILEHEADER_SIZE + sizeof(BITMAP_HEADER);
		bfh.Size = (GetWidth() * GetHeight() * BitCount) / 8 + bfh.BitsOffset;
	
		bh.HeaderSize = sizeof(BITMAP_HEADER);
		bh.BitCount = BitCount;
		
		if (BitCount == 32) {
			bh.Compression = 3; // BITFIELD
			bh.AlphaMask = 0xff000000;
			bh.BlueMask = 0x00ff0000;
			bh.GreenMask = 0x0000ff00;
			bh.RedMask = 0x000000ff;
		} else if (BitCount == 16) {
			bh.Compression = 3; // BITFIELD
			bh.AlphaMask = 0x00000000;
			bh.BlueMask = 0x0000001f;
			bh.GreenMask = 0x000007E0;
			bh.RedMask = 0x0000F800;
		} else {
			bh.Compression = 0; // RGB
		}
		
		unsigned int LineWidth = (GetWidth() + 3) & ~3;

		bh.Planes = 1;
		bh.Height = GetHeight();
		bh.Width = GetWidth();
		bh.SizeImage = (LineWidth * BitCount * GetHeight()) / 8;
		bh.PelsPerMeterX = 3780;
		bh.PelsPerMeterY = 3780;
		
		if (BitCount == 32) {
			file.write((char*) &bfh, sizeof(BITMAP_FILEHEADER));
			file.write((char*) &bh, sizeof(BITMAP_HEADER));
			file.write((char*) m_BitmapData, bh.SizeImage);
		} else if (BitCount < 16) {
			uint8_t* Bitmap = new uint8_t[bh.SizeImage];
			
			BGRA *Palette = 0;
			unsigned int PaletteSize = 0;

			if (GetBitsWithPalette(Bitmap, bh.SizeImage, BitCount, Palette, PaletteSize)) {
                            // Randy added this -- important!!!
                            bh.ClrUsed = PaletteSize;

#if 0
                            for (int x = 0; x < PaletteSize; x++) {
                                printf("%d %0x red %0x blue %0x green %0x alpha \n",
                                       x,
                                       Palette[x].Red,
                                       Palette[x].Blue,
                                       Palette[x].Green,
                                       Palette[x].Alpha);
                            }
#endif

				bfh.BitsOffset += PaletteSize * sizeof(BGRA);

				file.write((char*) &bfh, BITMAP_FILEHEADER_SIZE);
				file.write((char*) &bh, sizeof(BITMAP_HEADER));
				file.write((char*) Palette, PaletteSize * sizeof(BGRA));
				file.write((char*) Bitmap, bh.SizeImage);
			}
			delete [] Bitmap;
			delete [] Palette;
		} else {
			uint32_t RedMask = 0;
			uint32_t GreenMask = 0;
			uint32_t BlueMask = 0;
			uint32_t AlphaMask = 0;

			if (BitCount == 16) {
				RedMask = 0x0000F800;
				GreenMask = 0x000007E0;
				BlueMask = 0x0000001F;
				AlphaMask = 0x00000000;
			} else if (BitCount == 24) {
				RedMask = 0x00FF0000;
				GreenMask = 0x0000FF00;
				BlueMask = 0x000000FF;
			} else {
				// Other color formats are not valid
				Result = false;
			}

			if (Result) {
				if (GetBits(NULL, bh.SizeImage, RedMask, GreenMask, BlueMask, AlphaMask)) {
					uint8_t* Bitmap = new uint8_t[bh.SizeImage];
					if (GetBits(Bitmap, bh.SizeImage, RedMask, GreenMask, BlueMask, AlphaMask)) {
						file.write((char*) &bfh, sizeof(BITMAP_FILEHEADER));
						file.write((char*) &bh, sizeof(BITMAP_HEADER));
						file.write((char*) Bitmap, bh.SizeImage);
					}
					delete [] Bitmap;
				}
			}
		}

		file.close();
		return Result;
	}

	unsigned int GetWidth() {
		/* Add plausibility test */
		// if (abs(m_BitmapHeader.Width) > 8192) {
		//	m_BitmapHeader.Width = 8192;
		// }
		return m_BitmapHeader.Width < 0 ? -m_BitmapHeader.Width : m_BitmapHeader.Width;
	}
	
	unsigned int GetHeight() {
		/* Add plausibility test */
		// if (abs(m_BitmapHeader.Height) > 8192) {
		//	m_BitmapHeader.Height = 8192;
		// }
		return m_BitmapHeader.Height < 0 ? -m_BitmapHeader.Height : m_BitmapHeader.Height;
	}
	
	unsigned int GetBitCount() {
		/* Add plausibility test */
		// if (m_BitmapHeader.BitCount > 32) {
		//	m_BitmapHeader.BitCount = 32;
		// }
		return m_BitmapHeader.BitCount;
	}
	
	/* Copies internal RGBA buffer to user specified buffer */
	
	bool GetBits(void* Buffer, unsigned int &Size) {
		bool Result = false;
		if (Size == 0 || Buffer == 0) {
			Size = m_BitmapSize * sizeof(RGBA);
			Result = m_BitmapSize != 0;
		} else {
			memcpy(Buffer, m_BitmapData, Size);
			Result = true;
		}
		return Result;
	}

	/* Returns internal RGBA buffer */
	
	void* GetBits() {
		return m_BitmapData;
	}
	
	/* Copies internal RGBA buffer to user specified buffer and converts it into destination
	 * bit format specified by component masks.
	 *
	 * Typical Bitmap color formats (BGR/BGRA):
	 *
	 * Masks for 16 bit (5-5-5): ALPHA = 0x00000000, RED = 0x00007C00, GREEN = 0x000003E0, BLUE = 0x0000001F
	 * Masks for 16 bit (5-6-5): ALPHA = 0x00000000, RED = 0x0000F800, GREEN = 0x000007E0, BLUE = 0x0000001F
	 * Masks for 24 bit: ALPHA = 0x00000000, RED = 0x00FF0000, GREEN = 0x0000FF00, BLUE = 0x000000FF
	 * Masks for 32 bit: ALPHA = 0xFF000000, RED = 0x00FF0000, GREEN = 0x0000FF00, BLUE = 0x000000FF
	 *
	 * Other color formats (RGB/RGBA):
	 *
	 * Masks for 32 bit (RGBA): ALPHA = 0xFF000000, RED = 0x000000FF, GREEN = 0x0000FF00, BLUE = 0x00FF0000
	 *
	 * Bit count will be rounded to next 8 bit boundary. If IncludePadding is true, it will be ensured
	 * that line width is a multiple of 4. padding bytes are included if necessary.
	 *
	 * NOTE: systems with big endian byte order may require masks in inversion order.
	 */

	bool GetBits(void* Buffer, unsigned int &Size, unsigned int RedMask, unsigned int GreenMask, unsigned int BlueMask, unsigned int AlphaMask, bool IncludePadding = true) {
		bool Result = false;

		uint32_t BitCountRed = CColor::BitCountByMask(RedMask);
		uint32_t BitCountGreen = CColor::BitCountByMask(GreenMask);
		uint32_t BitCountBlue = CColor::BitCountByMask(BlueMask);
		uint32_t BitCountAlpha = CColor::BitCountByMask(AlphaMask);
		
		unsigned int BitCount = (BitCountRed + BitCountGreen + BitCountBlue + BitCountAlpha + 7) & ~7;

		if (BitCount > 32) {
			return false;
		}
		
		unsigned int w = GetWidth();
		//unsigned int LineWidth = (w + 3) & ~3;
		unsigned int dataBytesPerLine = (w * BitCount + 7) / 8;
		unsigned int LineWidth = (dataBytesPerLine + 3) & ~3;

		if (Size == 0 || Buffer == 0) {
			//Size = (LineWidth * GetHeight() * BitCount) / 8 + sizeof(unsigned int);
			Size = (GetWidth() * GetHeight() * BitCount) / 8 + sizeof(unsigned int);
			return true;
		}

		uint8_t* BufferPtr = (uint8_t*) Buffer;
		
		Result = true;

		uint32_t BitPosRed = CColor::BitPositionByMask(RedMask);
		uint32_t BitPosGreen = CColor::BitPositionByMask(GreenMask);
		uint32_t BitPosBlue = CColor::BitPositionByMask(BlueMask);
		uint32_t BitPosAlpha = CColor::BitPositionByMask(AlphaMask);
		
		unsigned int j = 0;

		for (unsigned int i = 0; i < m_BitmapSize; i++) {
			*(uint32_t*) BufferPtr =
			(CColor::Convert(m_BitmapData[i].Blue, 8, BitCountBlue) << BitPosBlue) |
			(CColor::Convert(m_BitmapData[i].Green, 8, BitCountGreen) << BitPosGreen) | 
			(CColor::Convert(m_BitmapData[i].Red, 8, BitCountRed) << BitPosRed) | 
			(CColor::Convert(m_BitmapData[i].Alpha, 8, BitCountAlpha) << BitPosAlpha);
			
			if (IncludePadding) {
				j++;
				if (j >= w) {
					for (unsigned int k = 0; k < LineWidth - dataBytesPerLine; k++) {
						BufferPtr += (BitCount >> 3);
					}
					j = 0;
				}
			}

			BufferPtr += (BitCount >> 3);
		}
		
		Size -= sizeof(unsigned int);

		return Result;
	}
	
	/* See GetBits(). 
	 * It creates a corresponding color table (palette) which have to be destroyed by the user after usage.
	 *
	 * Supported Bit depths are: 4, 8
	 *
	 * Todo: Optimize, use optimized palette, do ditehring (see my dithering class), support padding for 4 bit bitmaps
	 */

	bool GetBitsWithPalette(void* Buffer, unsigned int &Size, unsigned int BitCount, BGRA* &Palette, unsigned int &PaletteSize, bool OptimalPalette = false, bool IncludePadding = true) {
		bool Result = false;

		if (BitCount > 16) {
			return false;
		}
		
		unsigned int w = GetWidth();
		unsigned int dataBytesPerLine = (w * BitCount + 7) / 8;
		unsigned int LineWidth = (dataBytesPerLine + 3) & ~3;

		if (Size == 0 || Buffer == 0) {
			Size = (LineWidth * GetHeight() * BitCount) / 8;
			return true;
		}
		

		if (OptimalPalette) {
			PaletteSize = 0;
			// Not implemented
		} else {
			if (BitCount == 1) {
				PaletteSize = 2;
				// Not implemented: Who need that?
			} else if (BitCount == 4) { // 2:2:1
				PaletteSize = 16;
				Palette = new BGRA[PaletteSize];
				for (int r = 0; r < 4; r++) {
					for (int g = 0; g < 2; g++) {
						for (int b = 0; b < 2; b++) {
							Palette[r | g << 2 | b << 3].Red = r ? (r << 6) | 0x3f : 0;
							Palette[r | g << 2 | b << 3].Green = g ? (g << 7) | 0x7f : 0;
							Palette[r | g << 2 | b << 3].Blue = b ? (b << 7) | 0x7f : 0;
							Palette[r | g << 2 | b << 3].Alpha = 0xff;
						}
					}
				}
			} else if (BitCount == 8) { // 3:3:2
				PaletteSize = 256;
				Palette = new BGRA[PaletteSize];
#if 0
                                // This might work for color, but seems not to work
                                // for greyscale
				for (int r = 0; r < 8; r++) {
					for (int g = 0; g < 8; g++) {
						for (int b = 0; b < 4; b++) {
							Palette[r | g << 3 | b << 6].Red = r ? (r << 5) | 0x1f : 0;
							Palette[r | g << 3 | b << 6].Green = g ? (g << 5) | 0x1f : 0;
							Palette[r | g << 3 | b << 6].Blue = b ? (b << 6) | 0x3f : 0;
							Palette[r | g << 3 | b << 6].Alpha = 0xff;
						}
					}
				}
#else
                                // Randy -- This works for our greyscale example
                                for (int b = 0; b < PaletteSize; b++) {
                                    Palette[b].Red = b;
                                    Palette[b].Green = b;
                                    Palette[b].Blue = b;
                                    Palette[b].Alpha = b;
                                }
#endif
			} else if (BitCount == 16) { // 5:5:5
				// Not implemented
			}
		}

		unsigned int j = 0;
		uint8_t* BufferPtr = (uint8_t*) Buffer;
		
		for (unsigned int i = 0; i < m_BitmapSize; i++) {
			if (BitCount == 1) {
				// Not implemented: Who needs that?
			} else if (BitCount == 4) {
				*BufferPtr = ((m_BitmapData[i].Red >> 6) | (m_BitmapData[i].Green >> 7) << 2 | (m_BitmapData[i].Blue >> 7) << 3) << 4;
				i++;
				*BufferPtr |= (m_BitmapData[i].Red >> 6) | (m_BitmapData[i].Green >> 7) << 2 | (m_BitmapData[i].Blue >> 7) << 3;
			} else if (BitCount == 8) {
#if 0
                            *BufferPtr = (m_BitmapData[i].Red >> 5) | (m_BitmapData[i].Green >> 5) << 3 | (m_BitmapData[i].Blue >> 5) << 6;
#else
                            // Randy -- The above does not work for us
                            // Just grab the red bit (they are all the same) 
                            *BufferPtr = m_BitmapData[i].Red;
#endif
			} else if (BitCount == 16) {
				// Not implemented
			}

			if (IncludePadding) {
				j++;
				if (j >= w) {
					for (unsigned int k = 0; k < (LineWidth - dataBytesPerLine); k++) {
						BufferPtr += BitCount / 8;
					}
					j = 0;
				}
			}

			BufferPtr++;
		}
		
		Result = true;

		return Result;
	}

	/* Set Bitmap Bits. Will be converted to RGBA internally */

	bool SetBits(void* Buffer, unsigned int Width, unsigned int Height, unsigned int RedMask, unsigned int GreenMask, unsigned int BlueMask, unsigned int AlphaMask = 0) {
		if (Buffer == 0) {
			return false;
		}

		uint8_t *BufferPtr = (uint8_t*) Buffer;
		
		Dispose();

		m_BitmapHeader.Width = Width;
		m_BitmapHeader.Height = Height;
		m_BitmapHeader.BitCount = 32;
		m_BitmapHeader.Compression = 3; 

		m_BitmapSize = GetWidth() * GetHeight();
		m_BitmapData = new RGBA[m_BitmapSize];
		
		/* Find bit count by masks (rounded to next 8 bit boundary) */
		
		unsigned int BitCount = (CColor::BitCountByMask(RedMask | GreenMask | BlueMask | AlphaMask) + 7) & ~7;
		
		uint32_t BitCountRed = CColor::BitCountByMask(RedMask);
		uint32_t BitCountGreen = CColor::BitCountByMask(GreenMask);
		uint32_t BitCountBlue = CColor::BitCountByMask(BlueMask);
		uint32_t BitCountAlpha = CColor::BitCountByMask(AlphaMask);

		for (unsigned int i = 0; i < m_BitmapSize; i++) {
			unsigned int Color = 0;
			if (BitCount <= 8) {
				Color = *((uint8_t*) BufferPtr);
				BufferPtr += 1;
			} else if (BitCount <= 16) {
				Color = *((uint16_t*) BufferPtr);
				BufferPtr += 2;
			} else if (BitCount <= 24) {
				Color = *((uint32_t*) BufferPtr);
				BufferPtr += 3;
			} else if (BitCount <= 32) {
				Color = *((uint32_t*) BufferPtr);
				BufferPtr += 4;
			} else {
				/* unsupported */
				BufferPtr += 1;
			}
			m_BitmapData[i].Alpha = CColor::Convert(CColor::ComponentByMask(Color, AlphaMask), BitCountAlpha, 8);
			m_BitmapData[i].Red = CColor::Convert(CColor::ComponentByMask(Color, RedMask), BitCountRed, 8);
			m_BitmapData[i].Green = CColor::Convert(CColor::ComponentByMask(Color, GreenMask), BitCountGreen, 8);
			m_BitmapData[i].Blue = CColor::Convert(CColor::ComponentByMask(Color, BlueMask), BitCountBlue, 8);
		}

		return true;
	}

void FlipDiagonal() {

    const int radius = 0;

    uint32_t cols = GetWidth();
    uint32_t rows = GetHeight();

    RGBA *bits = (RGBA *)(GetBits());

    uint8_t *mybuffer = (uint8_t*) malloc( ((2*radius) + cols) * 
                                           ((2*radius) + rows) );

    for (uint32_t row = 0; row < rows; row++) {
        for (uint32_t col = 0; col < cols; col++) {
            uint32_t index = row*cols + col;
            mybuffer[index] = bits[(rows*cols)-1-index].Red;
        }
    }

    for (uint32_t index =  0; index < rows*cols; index++) {
        bits[index].Red = mybuffer[index];
    }
}


// Returns only Red bits for an 8 bit greyscale image
uint8_t *GetRedBitsWithBorder() {

    uint32_t cols = GetWidth();
    uint32_t rows = GetHeight();

    RGBA *bits = (RGBA *)(GetBits());

    uint32_t bufferSize = ( (2*global_radius) + cols) * ((2*global_radius) + rows);

    //    uint8_t *buffer = (uint8_t*) calloc( bufferSize, 1);
    uint8_t *buffer = (uint8_t*) stencil_cp_calloc( bufferSize, 1);

    for (uint32_t row = 0; row < rows; row++) {
        for (uint32_t col = 0; col < cols; col++) {
            uint32_t srcIndex = row*cols + col;
            uint32_t dstIndex = ((row+global_radius)*(cols+(2*global_radius))) +
                (col+global_radius);
            buffer[dstIndex] = bits[srcIndex].Red;
        }
    }

    return buffer;
}

// Sets only Red bits for an 8 bit greyscale image from the buffer.
// Buffer is assumed to have border bytes, and they are ignored.
void SetRedBitsFromBuffer(uint8_t *buffer) {

    uint32_t cols = GetWidth();
    uint32_t rows = GetHeight();

    RGBA *bits = (RGBA *)(GetBits());

    for (uint32_t row = 0; row < rows; row++) {
        for (uint32_t col = 0; col < cols; col++) {
            uint32_t dstIndex = row*cols + col;
            uint32_t srcIndex = ((row+global_radius)*(cols+(2*global_radius))) +
                (col+global_radius);
            bits[dstIndex].Red = buffer[srcIndex];
        }
    }
}

};   // end of class

#endif

