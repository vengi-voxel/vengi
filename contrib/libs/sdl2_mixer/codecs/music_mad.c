/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifdef MUSIC_MP3_MAD

#include "music_mad.h"
#include "mp3utils.h"

#include "mad.h"


/* NOTE: The dithering functions are GPL, which should be fine if your
         application is GPL (which would need to be true if you enabled
         libmad support in SDL_mixer). If you're using libmad under the
         commercial license, you need to disable this code.
*/
/************************ dithering functions ***************************/

#ifdef MUSIC_MP3_MAD_GPL_DITHERING

/* All dithering done here is taken from the GPL'ed xmms-mad plugin. */

/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.       */
/* Any feedback is very welcome. For any question, comments,       */
/* see http://www.math.keio.ac.jp/matumoto/emt.html or email       */
/* matumoto@math.keio.ac.jp                                        */

/* Period parameters */
#define MP3_DITH_N 624
#define MP3_DITH_M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[MP3_DITH_N]; /* the array for the state vector  */
static int mti=MP3_DITH_N+1; /* mti==MP3_DITH_N+1 means mt[MP3_DITH_N] is not initialized */

/* initializing the array with a NONZERO seed */
static void sgenrand(unsigned long seed)
{
    /* setting initial seeds to mt[MP3_DITH_N] using         */
    /* the generator Line 25 of Table 1 in          */
    /* [KNUTH 1981, The Art of Computer Programming */
    /*    Vol. 2 (2nd Ed.), pp102]                  */
    mt[0]= seed & 0xffffffff;
    for (mti=1; mti<MP3_DITH_N; mti++)
        mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}

static unsigned long genrand(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= MP3_DITH_N) { /* generate MP3_DITH_N words at one time */
        int kk;

        if (mti == MP3_DITH_N+1)   /* if sgenrand() has not been called, */
            sgenrand(4357); /* a default initial seed is used   */

        for (kk=0;kk<MP3_DITH_N-MP3_DITH_M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+MP3_DITH_M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        for (;kk<MP3_DITH_N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(MP3_DITH_M-MP3_DITH_N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        y = (mt[MP3_DITH_N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[MP3_DITH_N-1] = mt[MP3_DITH_M-1] ^ (y >> 1) ^ mag01[y & 0x1];

        mti = 0;
    }

    y = mt[mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);

    return y;
}

static long triangular_dither_noise(int nbits) {
    /* parameter nbits : the peak-to-peak amplitude desired (in bits)
     *  use with nbits set to    2 + nber of bits to be trimmed.
     * (because triangular is made from two uniformly distributed processes,
     * it starts at 2 bits peak-to-peak amplitude)
     * see The Theory of Dithered Quantization by Robert Alexander Wannamaker
     * for complete proof of why that's optimal
     */
    long v = (genrand()/2 - genrand()/2); /* in ]-2^31, 2^31[ */
    long P = 1 << (32 - nbits); /* the power of 2 */
    v /= P;
    /* now v in ]-2^(nbits-1), 2^(nbits-1) [ */

    return v;
}

#endif /* MUSIC_MP3_MAD_GPL_DITHERING */


#define MAD_INPUT_BUFFER_SIZE   (5*8192)

enum {
    MS_input_eof      = 0x0001,
    MS_input_error    = 0x0001,
    MS_decode_error   = 0x0002,
    MS_error_flags    = 0x000f
};

typedef struct {
    struct mp3file_t mp3file;
    int play_count;
    int freesrc;
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    mad_timer_t next_frame_start;
    int volume;
    int status;
    SDL_AudioStream *audiostream;
    unsigned short last_nchannels;
    unsigned int last_samplerate;

    double total_length;
    int sample_rate;
    int sample_position;

    unsigned char input_buffer[MAD_INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];
} MAD_Music;

static void read_update_buffer(struct mad_stream *stream, MAD_Music *music);

static double extract_length(struct mad_header *header, struct mad_stream *stream, Sint64 file_size)
{
    int mpeg_version = 0;
    int xing_offset = 0;
    Uint32 samples_per_frame = 0;
    Uint32 frames_count = 0;
    unsigned char const *frames_count_raw;

    /* There are two methods to compute duration:
     * - Using Xing/Info/VBRI headers
     * - Rely on filesize and first size of frame in condition of CRB
     * https://www.codeproject.com/Articles/8295/MPEG-Audio-Frame-Header#VBRHeaders
     */

    if (!stream->this_frame || !stream->next_frame ||
        stream->next_frame <= stream->this_frame ||
        (stream->next_frame - stream->this_frame) < 48) {
        return -1.0; /* Too small buffer to get any necessary headers */
    }

    mpeg_version = (stream->this_frame[1] >> 3) & 0x03;

    switch(mpeg_version) {
    case 0x03: /* MPEG1 */
        if (header->mode == MAD_MODE_SINGLE_CHANNEL) {
            xing_offset = 4 + 17;
        } else {
            xing_offset = 4 + 32;
        }
        break;
    default:  /* MPEG2 and MPEG2.5 */
        if (header->mode == MAD_MODE_SINGLE_CHANNEL) {
            xing_offset = 4 + 17;
        } else {
            xing_offset = 4 + 9;
        }
        break;
    }

    switch(header->layer)
    {
    case MAD_LAYER_I:
        samples_per_frame = 384;
        break;
    case MAD_LAYER_II:
        samples_per_frame = 1152;
        break;
    case MAD_LAYER_III:
        if (mpeg_version == 0x03) {
            samples_per_frame = 1152;
        } else {
            samples_per_frame = 576;
        }
        break;
    default:
        return -1.0;
    }

    if (SDL_memcmp(stream->this_frame + xing_offset, "Xing", 4) == 0 ||
        SDL_memcmp(stream->this_frame + xing_offset, "Info", 4) == 0) {
        /* Xing header to get the count of frames for VBR */
        frames_count_raw = stream->this_frame + xing_offset + 8;
        frames_count = ((Uint32)frames_count_raw[0] << 24) +
                       ((Uint32)frames_count_raw[1] << 16) +
                       ((Uint32)frames_count_raw[2] << 8) +
                       ((Uint32)frames_count_raw[3]);
    }
    else if (SDL_memcmp(stream->this_frame + xing_offset, "VBRI", 4) == 0) {
        /* VBRI header to get the count of frames for VBR */
        frames_count_raw = stream->this_frame + xing_offset + 14;
        frames_count = ((Uint32)frames_count_raw[0] << 24) +
                       ((Uint32)frames_count_raw[1] << 16) +
                       ((Uint32)frames_count_raw[2] << 8) +
                       ((Uint32)frames_count_raw[3]);
    } else {
        /* To get a count of frames for CBR, divide the file size with a size of one frame */
        frames_count = (Uint32)(file_size / (stream->next_frame - stream->this_frame));
    }

    return (double)(frames_count * samples_per_frame) / header->samplerate;
}

static int calculate_total_time(MAD_Music *music)
{
    mad_timer_t time = mad_timer_zero;
    struct mad_header header;
    struct mad_stream stream;
    SDL_bool is_first_frame = SDL_TRUE;
    int ret = 0;

    mad_header_init(&header);
    mad_stream_init(&stream);

    while (1)
    {
        read_update_buffer(&stream, music);

        if (mad_header_decode(&header, &stream) == -1) {
            if (MAD_RECOVERABLE(stream.error)) {
                if ((music->status & MS_input_error) == 0) {
                    continue;
                }
                if (is_first_frame) {
                    ret = -1;
                }
                break;
            } else if (stream.error == MAD_ERROR_BUFLEN) {
                if ((music->status & MS_input_error) == 0) {
                    continue;
                }
                if (is_first_frame) {
                    ret = -1;
                }
                break;
            } else {
                Mix_SetError("mad_frame_decode() failed, corrupt stream?");
                music->status |= MS_decode_error;
                if (is_first_frame) {
                    ret = -1;
                }
                break;
            }
        }

        music->sample_rate = (int)header.samplerate;
        mad_timer_add(&time, header.duration);

        if (is_first_frame) {
            music->total_length = extract_length(&header, &stream, music->mp3file.length);
            if (music->total_length > 0.0) {
                break; /* Duration has been recognized */
            }
            is_first_frame = SDL_FALSE;
            /* Otherwise, do the full scan of MP3 file to retrieve a duration */
        }
    }

    if (!is_first_frame) {
        music->total_length = (double)(mad_timer_count(time, (enum mad_units)music->sample_rate)) / (double)music->sample_rate;
    }
    mad_stream_finish(&stream);
    mad_header_finish(&header);
    SDL_memset(music->input_buffer, 0, sizeof(music->input_buffer));

    music->status = 0;

    MP3_RWseek(&music->mp3file, 0, RW_SEEK_SET);
    return ret;
}

static int MAD_Seek(void *context, double position);

static void *MAD_CreateFromRW(SDL_RWops *src, int freesrc)
{
    MAD_Music *music;

    music = (MAD_Music *)SDL_calloc(1, sizeof(MAD_Music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }
    music->volume = MIX_MAX_VOLUME;

    if (MP3_RWinit(&music->mp3file, src) < 0) {
        SDL_free(music);
        return NULL;
    }
    if (mp3_skiptags(&music->mp3file, SDL_FALSE) < 0) {
        SDL_free(music);
        Mix_SetError("music_mad: corrupt mp3 file (bad tags.)");
        return NULL;
    }

    if (calculate_total_time(music) < 0) {
        SDL_free(music);
        Mix_SetError("music_mad: corrupt mp3 file (bad stream.)");
        return NULL;
    }

    mad_stream_init(&music->stream);
    mad_frame_init(&music->frame);
    mad_synth_init(&music->synth);
    mad_timer_reset(&music->next_frame_start);

    music->freesrc = freesrc;
    return music;
}

static void MAD_SetVolume(void *context, int volume)
{
    MAD_Music *music = (MAD_Music *)context;
    music->volume = volume;
}

static int MAD_GetVolume(void *context)
{
    MAD_Music *music = (MAD_Music *)context;
    return music->volume;
}

/* Starts the playback. */
static int MAD_Play(void *context, int play_count)
{
    MAD_Music *music = (MAD_Music *)context;
    music->play_count = play_count;
    return MAD_Seek(music, 0.0);
}


/* Reads the next frame from the file.
   Returns true on success or false on failure.
 */
static void read_update_buffer(struct mad_stream *stream, MAD_Music *music)
{
    if (stream->buffer == NULL ||
        stream->error == MAD_ERROR_BUFLEN) {
        size_t read_size;
        size_t remaining;
        unsigned char *read_start;

        /* There might be some bytes in the buffer left over from last
           time.    If so, move them down and read more bytes following
           them. */
        if (stream->next_frame != NULL) {
            remaining = (size_t)(stream->bufend - stream->next_frame);
            SDL_memmove(music->input_buffer, stream->next_frame, remaining);
            read_start = music->input_buffer + remaining;
            read_size = MAD_INPUT_BUFFER_SIZE - remaining;

        } else {
            read_size = MAD_INPUT_BUFFER_SIZE;
            read_start = music->input_buffer;
            remaining = 0;
        }

        /* Now read additional bytes from the input file. */
        read_size = MP3_RWread(&music->mp3file, read_start, 1, read_size);

        if (read_size == 0) {
            if ((music->status & (MS_input_eof | MS_input_error)) == 0) {
                /* FIXME: how to detect error? */
                music->status |= MS_input_eof;

                /* At the end of the file, we must stuff MAD_BUFFER_GUARD
                   number of 0 bytes. */
                SDL_memset(read_start + read_size, 0, MAD_BUFFER_GUARD);
                read_size += MAD_BUFFER_GUARD;
            }
        }

        /* Now feed those bytes into the libmad stream. */
        mad_stream_buffer(stream, music->input_buffer,
                                            read_size + remaining);
        stream->error = MAD_ERROR_NONE;
    }
}

/* Reads the next frame from the file.
   Returns true on success or false on failure.
 */
static SDL_bool read_next_frame(MAD_Music *music)
{
    read_update_buffer(&music->stream, music);

    /* Now ask libmad to extract a frame from the data we just put in
       its buffer. */
    if (mad_frame_decode(&music->frame, &music->stream)) {
        if (MAD_RECOVERABLE(music->stream.error)) {
            mad_stream_sync(&music->stream); /* to frame seek mode */
            return SDL_FALSE;

        } else if (music->stream.error == MAD_ERROR_BUFLEN) {
            return SDL_FALSE;

        } else {
            Mix_SetError("mad_frame_decode() failed, corrupt stream?");
            music->status |= MS_decode_error;
            return SDL_FALSE;
        }
    }

    mad_timer_add(&music->next_frame_start, music->frame.header.duration);

    return SDL_TRUE;
}

/* Scale a MAD sample to 16 bits for output. */
static Sint16 scale(mad_fixed_t sample)
{
    const int n_bits_to_loose = MAD_F_FRACBITS + 1 - 16;

    /* round */
    sample += (1L << (n_bits_to_loose - 1));

#ifdef MUSIC_MP3_MAD_GPL_DITHERING
    sample += triangular_dither_noise(n_bits_to_loose + 1);
#endif

    /* clip */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    /* quantize */
    return (Sint16)(sample >> n_bits_to_loose);
}

/* Once the frame has been read, copies its samples into the output buffer. */
static SDL_bool decode_frame(MAD_Music *music)
{
    struct mad_pcm *pcm;
    unsigned int i, nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;
    Sint16 *buffer, *dst;
    int result;

    mad_synth_frame(&music->synth, &music->frame);
    pcm = &music->synth.pcm;

    if (!music->audiostream || music->last_nchannels != pcm->channels || music->last_samplerate != pcm->samplerate) {
        if (music->audiostream) {
            SDL_FreeAudioStream(music->audiostream);
        }
        music->audiostream = SDL_NewAudioStream(AUDIO_S16SYS, (Uint8)pcm->channels, (int)pcm->samplerate,
                                                music_spec.format, music_spec.channels, music_spec.freq);
        if (!music->audiostream) {
            return SDL_FALSE;
        }
        music->last_nchannels = pcm->channels;
        music->last_samplerate = pcm->samplerate;
    }

    nchannels = pcm->channels;
    nsamples = pcm->length;
    left_ch = pcm->samples[0];
    right_ch = pcm->samples[1];
    buffer = SDL_stack_alloc(Sint16, nsamples*nchannels);
    if (!buffer) {
        SDL_OutOfMemory();
        return SDL_FALSE;
    }

    dst = buffer;
    if (nchannels == 1) {
        for (i = nsamples; i--;) {
            *dst++ = scale(*left_ch++);
        }
    } else {
        for (i = nsamples; i--;) {
            *dst++ = scale(*left_ch++);
            *dst++ = scale(*right_ch++);
        }
    }

    music->sample_position += nsamples;
    result = SDL_AudioStreamPut(music->audiostream, buffer, (int)(nsamples * nchannels * sizeof(Sint16)));
    SDL_stack_free(buffer);

    if (result < 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static int MAD_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    MAD_Music *music = (MAD_Music *)context;
    int filled;

    if (music->audiostream) {
        filled = SDL_AudioStreamGet(music->audiostream, data, bytes);
        if (filled != 0) {
            return filled;
        }
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    if (read_next_frame(music)) {
        if (!decode_frame(music)) {
            return -1;
        }
    } else if (music->status & MS_input_eof) {
        int play_count = -1;
        if (music->play_count > 0) {
            play_count = (music->play_count - 1);
        }
        if (MAD_Play(music, play_count) < 0) {
            return -1;
        }
    } else if (music->status & MS_decode_error) {
        return -1;
    }
    return 0;
}

static int MAD_GetAudio(void *context, void *data, int bytes)
{
    MAD_Music *music = (MAD_Music *)context;
    return music_pcm_getaudio(context, data, bytes, music->volume, MAD_GetSome);
}

static int MAD_Seek(void *context, double position)
{
    MAD_Music *music = (MAD_Music *)context;
    mad_timer_t target;
    int int_part;

    int_part = (int)position;
    mad_timer_set(&target, (unsigned long)int_part, (unsigned long)((position - int_part) * 1000000), 1000000);

    music->sample_position = (int)(position * music->sample_rate);

    if (mad_timer_compare(music->next_frame_start, target) > 0) {
        /* In order to seek backwards in a VBR file, we have to rewind and
           start again from the beginning.    This isn't necessary if the
           file happens to be CBR, of course; in that case we could seek
           directly to the frame we want.    But I leave that little
           optimization for the future developer who discovers she really
           needs it. */
        mad_timer_reset(&music->next_frame_start);
        music->status &= ~MS_error_flags;

        MP3_RWseek(&music->mp3file, 0, RW_SEEK_SET);
        /* Avoid junk chunk be played after seek -- Vitaly Novichkov */
        SDL_memset(music->input_buffer, 0, sizeof(music->input_buffer));
    }

    /* Now we have to skip frames until we come to the right one.
       Again, only truly necessary if the file is VBR. */
    while (mad_timer_compare(music->next_frame_start, target) < 0) {
        if (!read_next_frame(music)) {
            if ((music->status & MS_error_flags) != 0) {
                /* Couldn't read a frame; either an error condition or
                     end-of-file.    Stop. */
                return Mix_SetError("Seek position out of range");
            }
        }
    }

    /* Here we are, at the beginning of the frame that contains the
       target time.    Ehh, I say that's close enough.    If we wanted to,
       we could get more precise by decoding the frame now and counting
       the appropriate number of samples out of it. */
    return 0;
}

static double MAD_Tell(void *context)
{
    MAD_Music *music = (MAD_Music *)context;
    return (double)music->sample_position / (double)music->sample_rate;
}

static double MAD_Duration(void *context)
{
    MAD_Music *music = (MAD_Music *)context;
    return music->total_length;
}

static void MAD_Delete(void *context)
{
    MAD_Music *music = (MAD_Music *)context;

    mad_stream_finish(&music->stream);
    mad_frame_finish(&music->frame);
    mad_synth_finish(&music->synth);

    if (music->audiostream) {
        SDL_FreeAudioStream(music->audiostream);
    }
    if (music->freesrc) {
        SDL_RWclose(music->mp3file.src);
    }
    SDL_free(music);
}

Mix_MusicInterface Mix_MusicInterface_MAD =
{
    "MAD",
    MIX_MUSIC_MAD,
    MUS_MP3,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    MAD_CreateFromRW,
    NULL,   /* CreateFromFile */
    MAD_SetVolume,
    MAD_GetVolume,
    MAD_Play,
    NULL,   /* IsPlaying */
    MAD_GetAudio,
    NULL,   /* Jump */
    MAD_Seek,
    MAD_Tell,
    MAD_Duration,
    NULL,   /* LoopStart */
    NULL,   /* LoopEnd */
    NULL,   /* LoopLength */
    NULL,   /* GetMetaTag */
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    MAD_Delete,
    NULL,   /* Close */
    NULL    /* Unload */
};

#endif /* MUSIC_MP3_MAD */

/* vi: set ts=4 sw=4 expandtab: */
