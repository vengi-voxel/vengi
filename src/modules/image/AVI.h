/**
 * @file
 */

#pragma once

#include "core/FourCC.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"

namespace image {

/**
 * @code
 * RIFF ('AVI '
 *     LIST ('hdrl'
 *           'avih'(<Main AVI Header>)
 *           LIST ('strl'
 *                 'strh'(<Stream header>)
 *                 'strf'(<Stream format>)
 *                 [ 'strd'(<Additional header data>) ]
 *                 [ 'strn'(<Stream name>) ]
 *                 ...
 *                )
 *            ...
 *          )
 *     LIST ('movi'
 *           {SubChunk | LIST ('rec '
 *                             SubChunk1
 *                             SubChunk2
 *                             ...
 *                            )
 *              ...
 *           }
 *           ...
 *          )
 *     ['idx1' (<AVI Index>) ]
 *    )
 * @endcode
 *
 * @link https://learn.microsoft.com/en-us/previous-versions/ms779636(v=vs.85)
 */
class AVI {
private:
	struct State {
		uint32_t moviSize = 0;
		int fps = 0;
	};

	State _state;

	struct AVIMAINHEADER {
		// Specifies a FOURCC code. The value must be 'avih'.
		uint32_t fcc = 0;
		// Specifies the size of the structure, not including the initial 8 bytes.
		uint32_t cb = 0;
		// Specifies the number of microseconds between frames. This value indicates the overall timing for the
		// file.
		uint32_t dwMicroSecPerFrame = 0;
		// Specifies the approximate maximum data rate of the file. This value indicates the number of
		// bytes per second the system must handle to present an AVI sequence as specified by the other
		// parameters contained in the main header and stream header chunks.
		uint32_t dwMaxBytesPerSec = 0;
		// Specifies the alignment for data, in bytes. Pad the data to multiples of this value.
		uint32_t dwPaddingGranularity = 0;
		// dwFlags bits AVIF_HASINDEX and AVIF_ISINTERLEAVED;
		uint32_t dwFlags = 0;
		// Specifies the total number of frames of data in the file.
		uint32_t dwTotalFrames = 0;
		// Specifies the initial frame for interleaved files. Noninterleaved files should specify zero. If you are
		// creating interleaved files, specify the number of frames in the file prior to the initial frame of the AVI
		// sequence in this member.
		uint32_t dwInitialFrames = 0;
		// Specifies the number of streams in the file. For example, a file with audio and video has two streams.
		uint32_t dwStreams = 1;
		// Specifies the suggested buffer size for reading the file. Generally, this size should be large enough to
		// contain the largest chunk in the file. If set to zero, or if it is too small, the playback software will have
		// to reallocate memory during playback, which will reduce performance. For an interleaved file, the buffer size
		// should be large enough to read an entire record, and not just a chunk.
		uint32_t dwSuggestedBufferSize = 0;
		// Specifies the width of the AVI file in pixels.
		uint32_t dwWidth = 0;
		// Specifies the height of the AVI file in pixels.
		uint32_t dwHeight = 0;
		uint32_t dwReserved[4]{0, 0, 0, 0};
	};
	AVIMAINHEADER _header;
	io::BufferedReadWriteStream _indexStream;

	/**
	 * @note Some of the members of this structure are also present in the AVIMAINHEADER structure. The data in the
	 * AVIMAINHEADER structure applies to the whole file, while the data in the AVISTREAMHEADER structure applies to one
	 * stream.
	 */
	struct AVISTREAMHEADER {
		// Specifies a FOURCC code. The value must be 'strh'.
		int32_t fcc = 0;
		// Specifies the size of the structure, not including the initial 8 bytes.
		uint32_t cb = 0;
		// Contains a FOURCC that specifies the type of the data contained in the stream. - 'vids' in our case
		uint32_t fccType = 0;
		// Optionally, contains a FOURCC that identifies a specific data handler. The data handler is the preferred
		// handler for the stream. For audio and video streams, this specifies the codec for decoding the stream.
		uint32_t fccHandler = 0;
		// Contains any flags for the data stream. The bits in the high-order word of these flags are specific to the
		// type of data contained in the stream. The following standard flags are defined.
		uint32_t dwFlags = 0;
		// Specifies priority of a stream type. For example, in a file with multiple audio streams, the one with the
		// highest priority might be the default stream.
		uint16_t wPriority = 0;
		// Language tag.
		uint16_t wLanguage = 0;
		// Specifies how far audio data is skewed ahead of the video frames in interleaved files. Typically, this is
		// about 0.75 seconds. If you are creating interleaved files, specify the number of frames in the file prior to
		// the initial frame of the AVI sequence in this member. For more information, see the remarks for the
		// dwInitialFrames member of the AVIMAINHEADER structure.
		uint32_t dwInitialFrames = 0;
		// Used with dwRate to specify the time scale that this stream will use. Dividing dwRate by dwScale gives the
		// number of samples per second. For video streams, this is the frame rate. For audio streams, this rate
		// corresponds to the time needed to play nBlockAlign bytes of audio, which for PCM audio is the just the sample
		// rate.
		uint32_t dwScale = 1;
		// See dwScale.
		uint32_t dwRate = 0;
		// Specifies the starting time for this stream. The units are defined by the dwRate and dwScale members in the
		// main file header. Usually, this is zero, but it can specify a delay time for a stream that does not start
		// concurrently with the file.
		uint32_t dwStart = 0;
		// Specifies the length of this stream. The units are defined by the dwRate and dwScale members of the stream's
		// header.
		uint32_t dwLength = 0;
		// Specifies how large a buffer should be used to read this stream. Typically, this contains a value
		// corresponding to the largest chunk present in the stream. Using the correct buffer size makes playback more
		// efficient. Use zero if you do not know the correct buffer size.
		uint32_t dwSuggestedBufferSize = 0;
		// Specifies an indicator of the quality of the data in the stream. Quality is represented as a number between 0
		// and 10,000. For compressed data, this typically represents the value of the quality parameter passed to the
		// compression software. If set to –1, drivers use the default quality value.
		uint32_t dwQuality = 0;
		// Specifies the size of a single sample of data. This is set to zero if the samples can vary in size. If this
		// number is nonzero, then multiple samples of data can be grouped into a single chunk within the file. If it is
		// zero, each sample of data (such as a video frame) must be in a separate chunk. For video streams, this number
		// is typically zero, although it can be nonzero if all video frames are the same size. For audio streams, this
		// number should be the same as the nBlockAlign member of the WAVEFORMATEX structure describing the audio.
		uint32_t dwSampleSize = 0;
		// Specifies the destination rectangle for a text or video stream within the movie rectangle specified by the
		// dwWidth and dwHeight members of the AVI main header structure. The rcFrame member is typically used in
		// support of multiple video streams. Set this rectangle to the coordinates corresponding to the movie rectangle
		// to update the whole movie rectangle. Units for this member are pixels. The upper-left corner of the
		// destination rectangle is relative to the upper-left corner of the movie rectangle.
		uint16_t dwRectX1 = 0;
		uint16_t dwRectY1 = 0;
		uint16_t dwRectX2 = 0;
		uint16_t dwRectY2 = 0;
	};

	struct BITMAPINFOHEADER {
		// Specifies the number of bytes required by the structure. This value does not include the size of the color
		// table or the size of the color masks, if they are appended to the end of structure. See Remarks.
		uint32_t biSize = 40;
		// Specifies the width of the bitmap, in pixels. For information about calculating the stride of the bitmap, see
		// Remarks.
		int32_t biWidth = 0;
		// Specifies the height of the bitmap, in pixels.
		//
		// For uncompressed RGB bitmaps, if biHeight is positive, the bitmap is a bottom-up DIB with the origin at
		// the lower left corner. If biHeight is negative, the bitmap is a top-down DIB with the origin at the upper
		// left corner. For YUV bitmaps, the bitmap is always top-down, regardless of the sign of biHeight. Decoders
		// should offer YUV formats with positive biHeight, but for backward compatibility they should accept YUV
		// formats with either positive or negative biHeight. For compressed formats, biHeight must be positive,
		// regardless of image orientation.
		int32_t biHeight = 0;
		// Specifies the number of planes for the target device. This value must be set to 1.
		uint16_t biPlanes = 1;
		// Specifies the number of bits per pixel (bpp). For uncompressed formats, this value is the average number of
		// bits per pixel. For compressed formats, this value is the implied bit depth of the uncompressed image, after
		// the image has been decoded.
		uint16_t biBitCount = 24;
		// For compressed video and YUV formats, this member is a FOURCC code, specified as a DWORD in little-endian
		// order. For example, YUYV video has the FOURCC 'VYUY' or 0x56595559
		// See Remarks for more information. Note that BI_JPG and BI_PNG are not valid video formats.
		// For 16-bpp bitmaps, if biCompression equals BI_RGB, the format is always RGB 555. If biCompression equals
		// BI_BITFIELDS, the format is either RGB 555 or RGB 565. Use the subtype GUID in the AM_MEDIA_TYPE structure to
		// determine the specific RGB type.
		uint32_t biCompression = FourCC('M', 'J', 'P', 'G');
		// Specifies the size, in bytes, of the image. This can be set to 0 for uncompressed RGB bitmaps.
		uint32_t biSizeImage = 0;
		// Specifies the horizontal resolution, in pixels per meter, of the target device for the bitmap.
		int32_t biXPelsPerMeter = 0;
		// Specifies the vertical resolution, in pixels per meter, of the target device for the bitmap.
		int32_t biYPelsPerMeter = 0;
		// Specifies the number of color indices in the color table that are actually used by the bitmap.
		uint32_t biClrUsed = 0;
		// Specifies the number of color indices that are considered important for displaying the bitmap. If this value
		// is zero, all colors are important.
		uint32_t biClrImportant = 0;
	};

	bool writeHeader(io::SeekableWriteStream &stream, uint32_t riffSize = 0, uint32_t moviSize = 0);

public:
	bool open(io::SeekableWriteStream &stream, int width, int height, int fps = 27);
	bool writeFrame(io::SeekableWriteStream &stream, const uint8_t *rgba, int width, int height);
	bool writeJPEGFrame(io::SeekableWriteStream &stream, const uint8_t *jpeg, size_t size);
	bool close(io::SeekableWriteStream &stream);
};

} // namespace image
