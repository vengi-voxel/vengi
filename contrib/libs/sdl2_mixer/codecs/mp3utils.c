/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL_stdinc.h"
#include "SDL_rwops.h"

#include "mp3utils.h"

#if defined(MUSIC_MP3_MAD) || defined(MUSIC_MP3_MPG123)

/*********************** SDL_RW WITH BOOKKEEPING ************************/

size_t MP3_RWread(struct mp3file_t *fil, void *ptr, size_t size, size_t maxnum) {
    size_t remaining = (size_t)(fil->length - fil->pos);
    size_t ret;
    maxnum *= size;
    if (maxnum > remaining) maxnum = remaining;
    ret = SDL_RWread(fil->src, ptr, 1, maxnum);
    fil->pos += (Sint64)ret;
    return ret;
}

Sint64 MP3_RWseek(struct mp3file_t *fil, Sint64 offset, int whence) {
    Sint64 ret;
    switch (whence) {
    case RW_SEEK_CUR:
        offset += fil->pos;
        break;
    case RW_SEEK_END:
        offset += fil->length;
        break;
    }
    if (offset < 0) return -1;
    if (offset > fil->length)
        offset = fil->length;
    ret = SDL_RWseek(fil->src, fil->start + offset, RW_SEEK_SET);
    if (ret < 0) return ret;
    fil->pos = offset;
    return (fil->pos - fil->start);
}


/*************************** TAG HANDLING: ******************************/

static SDL_INLINE SDL_bool is_id3v1(const unsigned char *data, long length) {
    /* http://id3.org/ID3v1 :  3 bytes "TAG" identifier and 125 bytes tag data */
    if (length < 128 || SDL_memcmp(data,"TAG",3) != 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}
static SDL_bool is_id3v2(const unsigned char *data, size_t length) {
    /* ID3v2 header is 10 bytes:  http://id3.org/id3v2.4.0-structure */
    /* bytes 0-2: "ID3" identifier */
    if (length < 10 || SDL_memcmp(data,"ID3",3) != 0) {
        return SDL_FALSE;
    }
    /* bytes 3-4: version num (major,revision), each byte always less than 0xff. */
    if (data[3] == 0xff || data[4] == 0xff) {
        return SDL_FALSE;
    }
    /* bytes 6-9 are the ID3v2 tag size: a 32 bit 'synchsafe' integer, i.e. the
     * highest bit 7 in each byte zeroed.  i.e.: 7 bit information in each byte ->
     * effectively a 28 bit value.  */
    if (data[6] >= 0x80 || data[7] >= 0x80 || data[8] >= 0x80 || data[9] >= 0x80) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}
static long get_id3v2_len(const unsigned char *data, long length) {
    /* size is a 'synchsafe' integer (see above) */
    long size = (long)((data[6]<<21) + (data[7]<<14) + (data[8]<<7) + data[9]);
    size += 10; /* header size */
    /* ID3v2 header[5] is flags (bits 4-7 only, 0-3 are zero).
     * bit 4 set: footer is present (a copy of the header but
     * with "3DI" as ident.)  */
    if (data[5] & 0x10) {
        size += 10; /* footer size */
    }
    /* optional padding (always zeroes) */
    while (size < length && data[size] == 0) {
        ++size;
    }
    return size;
}
static SDL_bool is_apetag(const unsigned char *data, size_t length) {
   /* http://wiki.hydrogenaud.io/index.php?title=APEv2_specification
    * Header/footer is 32 bytes: bytes 0-7 ident, bytes 8-11 version,
    * bytes 12-17 size. bytes 24-31 are reserved: must be all zeroes. */
    Uint32 v;

    if (length < 32 || SDL_memcmp(data,"APETAGEX",8) != 0) {
        return SDL_FALSE;
    }
    v = (Uint32)((data[11]<<24) | (data[10]<<16) | (data[9]<<8) | data[8]); /* version */
    if (v != 2000U && v != 1000U) {
        return SDL_FALSE;
    }
    v = 0; /* reserved bits : */
    if (SDL_memcmp(&data[24],&v,4) != 0 || SDL_memcmp(&data[28],&v,4) != 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}
static long get_ape_len(const unsigned char *data) {
    Uint32 flags, version;
    long size = (long)((data[15]<<24) | (data[14]<<16) | (data[13]<<8) | data[12]);
    version = (Uint32)((data[11]<<24) | (data[10]<<16) | (data[9]<<8) | data[8]);
    flags = (Uint32)((data[23]<<24) | (data[22]<<16) | (data[21]<<8) | data[20]);
    if (version == 2000U && (flags & (1U<<31))) size += 32; /* header present. */
    return size;
}
static SDL_INLINE int is_lyrics3tag(const unsigned char *data, long length) {
    /* http://id3.org/Lyrics3
     * http://id3.org/Lyrics3v2 */
    if (length < 15) return 0;
    if (SDL_memcmp(data+6,"LYRICS200",9) == 0) return 2; /* v2 */
    if (SDL_memcmp(data+6,"LYRICSEND",9) == 0) return 1; /* v1 */
    return 0;
}
static long get_lyrics3v1_len(struct mp3file_t *m) {
    const char *p; long i, len;
    char buf[5104];
    /* needs manual search:  http://id3.org/Lyrics3 */
    if (m->length < 20) return -1;
    len = (m->length > 5109)? 5109 : (long)m->length;
    MP3_RWseek(m, -len, RW_SEEK_END);
    MP3_RWread(m, buf, 1, (len -= 9)); /* exclude footer */
    /* strstr() won't work here. */
    for (i = len - 11, p = buf; i >= 0; --i, ++p) {
        if (SDL_memcmp(p, "LYRICSBEGIN", 11) == 0)
            break;
    }
    if (i < 0) return -1;
    return len - (long)(p - buf) + 9 /* footer */;
}
static SDL_INLINE long get_lyrics3v2_len(const unsigned char *data, long length) {
    /* 6 bytes before the end marker is size in decimal format -
     * does not include the 9 bytes end marker and size field. */
    if (length != 6) return 0;
    return SDL_strtol((const char *)data, NULL, 10) + 15;
}
static SDL_INLINE SDL_bool verify_lyrics3v2(const unsigned char *data, long length) {
    if (length < 11) return SDL_FALSE;
    if (SDL_memcmp(data,"LYRICSBEGIN",11) == 0) return SDL_TRUE;
    return SDL_FALSE;
}
#define MMTAG_PARANOID
static SDL_bool is_musicmatch(const unsigned char *data, long length) {
  /* From docs/musicmatch.txt in id3lib: https://sourceforge.net/projects/id3lib/
     Overall tag structure:

      +-----------------------------+
      |           Header            |
      |    (256 bytes, OPTIONAL)    |
      +-----------------------------+
      |  Image extension (4 bytes)  |
      +-----------------------------+
      |        Image binary         |
      |  (var. length >= 4 bytes)   |
      +-----------------------------+
      |      Unused (4 bytes)       |
      +-----------------------------+
      |  Version info (256 bytes)   |
      +-----------------------------+
      |       Audio meta-data       |
      | (var. length >= 7868 bytes) |
      +-----------------------------+
      |   Data offsets (20 bytes)   |
      +-----------------------------+
      |      Footer (48 bytes)      |
      +-----------------------------+
     */
    if (length < 48) return SDL_FALSE;
    /* sig: 19 bytes company name + 13 bytes space */
    if (SDL_memcmp(data,"Brava Software Inc.             ",32) != 0) {
        return SDL_FALSE;
    }
    /* 4 bytes version: x.xx */
    if (!SDL_isdigit(data[32]) || data[33] != '.' ||
        !SDL_isdigit(data[34]) ||!SDL_isdigit(data[35])) {
        return SDL_FALSE;
    }
    #ifdef MMTAG_PARANOID
    /* [36..47]: 12 bytes trailing space */
    for (length = 36; length < 48; ++length) {
        if (data[length] != ' ') return SDL_FALSE;
    }
    #endif
    return SDL_TRUE;
}
static long get_musicmatch_len(struct mp3file_t *m) {
    const Sint32 metasizes[4] = { 7868, 7936, 8004, 8132 };
    const unsigned char syncstr[10] = {'1','8','2','7','3','6','4','5',0,0};
    unsigned char buf[256];
    Sint32 i, j, imgext_ofs, version_ofs;
    long len;

    MP3_RWseek(m, -68, RW_SEEK_END);
    MP3_RWread(m, buf, 1, 20);
    imgext_ofs  = (Sint32)((buf[3] <<24) | (buf[2] <<16) | (buf[1] <<8) | buf[0] );
    version_ofs = (Sint32)((buf[15]<<24) | (buf[14]<<16) | (buf[13]<<8) | buf[12]);
    if (version_ofs <= imgext_ofs) return -1;
    if (version_ofs <= 0 || imgext_ofs <= 0) return -1;
    /* Try finding the version info section:
     * Because metadata section comes after it, and because metadata section
     * has different sizes across versions (format ver. <= 3.00: always 7868
     * bytes), we can _not_ directly calculate using deltas from the offsets
     * section. */
    for (i = 0; i < 4; ++i) {
    /* 48: footer, 20: offsets, 256: version info */
        len = metasizes[i] + 48 + 20 + 256;
        if (m->length < len) return -1;
        MP3_RWseek(m, -len, RW_SEEK_END);
        MP3_RWread(m, buf, 1, 256);
        /* [0..9]: sync string, [30..255]: 0x20 */
        #ifdef MMTAG_PARANOID
        for (j = 30; j < 256; ++j) {
            if (buf[j] != ' ') break;
        }
        if (j < 256) continue;
        #endif
        if (SDL_memcmp(buf, syncstr, 10) == 0) {
            break;
        }
    }
    if (i == 4) return -1; /* no luck. */
    #ifdef MMTAG_PARANOID
    /* unused section: (4 bytes of 0x00) */
    MP3_RWseek(m, -(len + 4), RW_SEEK_END);
    MP3_RWread(m, buf, 1, 4); j = 0;
    if (SDL_memcmp(buf, &j, 4) != 0) return -1;
    #endif
    len += (version_ofs - imgext_ofs);
    if (m->length < len) return -1;
    MP3_RWseek(m, -len, RW_SEEK_END);
    MP3_RWread(m, buf, 1, 8);
    j = (Sint32)((buf[7] <<24) | (buf[6] <<16) | (buf[5] <<8) | buf[4]);
    if (j < 0) return -1;
    /* verify image size: */
    /* without this, we may land at a wrong place. */
    if (j + 12 != version_ofs - imgext_ofs) return -1;
    /* try finding the optional header */
    if (m->length < len + 256) return len;
    MP3_RWseek(m, -(len + 256), RW_SEEK_END);
    MP3_RWread(m, buf, 1, 256);
    /* [0..9]: sync string, [30..255]: 0x20 */
    if (SDL_memcmp(buf, syncstr, 10) != 0) {
        return len;
    }
    #ifdef MMTAG_PARANOID
    for (j = 30; j < 256; ++j) {
        if (buf[j] != ' ') return len;
    }
    #endif
    return len + 256; /* header is present. */
}

static int probe_id3v1(struct mp3file_t *fil, unsigned char *buf, int atend) {
    if (fil->length >= 128) {
        MP3_RWseek(fil, -128, RW_SEEK_END);
        if (MP3_RWread(fil, buf, 1, 128) != 128)
            return -1;
        if (is_id3v1(buf, 128)) {
            if (!atend) { /* possible false positive? */
                if (is_musicmatch(buf + 128 - 48, 48) ||
                    is_apetag    (buf + 128 - 32, 32) ||
                    is_lyrics3tag(buf + 128 - 15, 15)) {
                    return 0;
                }
            }
            fil->length -= 128;
            return 1;
            /* FIXME: handle possible double-ID3v1 tags?? */
        }
    }
    return 0;
}
static int probe_mmtag(struct mp3file_t *fil, unsigned char *buf) {
    long len;
    if (fil->length >= 68) {
        MP3_RWseek(fil, -48, RW_SEEK_END);
        if (MP3_RWread(fil, buf, 1, 48) != 48)
            return -1;
        if (is_musicmatch(buf, 48)) {
            len = get_musicmatch_len(fil);
            if (len < 0) return -1;
            if (len >= fil->length) return -1;
            fil->length -= len;
            return 1;
        }
    }
    return 0;
}
static int probe_apetag(struct mp3file_t *fil, unsigned char *buf) {
    long len;
    if (fil->length >= 32) {
        MP3_RWseek(fil, -32, RW_SEEK_END);
        if (MP3_RWread(fil, buf, 1, 32) != 32)
            return -1;
        if (is_apetag(buf, 32)) {
            len = get_ape_len(buf);
            if (len >= fil->length) return -1;
            fil->length -= len;
            return 1;
        }
    }
    return 0;
}
static int probe_lyrics3(struct mp3file_t *fil, unsigned char *buf) {
    long len;
    if (fil->length >= 15) {
        MP3_RWseek(fil, -15, RW_SEEK_END);
        if (MP3_RWread(fil, buf, 1, 15) != 15)
            return -1;
        len = is_lyrics3tag(buf, 15);
        if (len == 2) {
            len = get_lyrics3v2_len(buf, 6);
            if (len >= fil->length) return -1;
            if (len < 15) return -1;
            MP3_RWseek(fil, -len, RW_SEEK_END);
            if (MP3_RWread(fil, buf, 1, 11) != 11)
                return -1;
            if (!verify_lyrics3v2(buf, 11)) return -1;
            fil->length -= len;
            return 1;
        }
        else if (len == 1) {
            len = get_lyrics3v1_len(fil);
            if (len < 0) return -1;
            fil->length -= len;
            return 1;
        }
    }
    return 0;
}

int mp3_skiptags(struct mp3file_t *fil, SDL_bool keep_id3v2)
{
    unsigned char buf[128];
    long len; size_t readsize;
    int c_id3, c_ape, c_lyr, c_mm;
    int rc = -1;

    /* MP3 standard has no metadata format, so everyone invented
     * their own thing, even with extensions, until ID3v2 became
     * dominant: Hence the impossible mess here.
     *
     * Note: I don't yet care about freaky broken mp3 files with
     * double tags. -- O.S.
     */

    readsize = MP3_RWread(fil, buf, 1, 128);
    if (!readsize) goto fail;

    /* ID3v2 tag is at the start */
    if (is_id3v2(buf, readsize)) {
        len = get_id3v2_len(buf, (long)readsize);
        if (len >= fil->length) goto fail;
        if (!keep_id3v2) {
            fil->start  += len;
            fil->length -= len;
        }
    }
    /* APE tag _might_ be at the start (discouraged
     * but not forbidden, either.)  read the header. */
    else if (is_apetag(buf, readsize)) {
        len = get_ape_len(buf);
        if (len >= fil->length) goto fail;
        fil->start += len;
        fil->length -= len;
    }

    /* it's not impossible that _old_ MusicMatch tag
     * placing itself after ID3v1. */
    if ((c_mm = probe_mmtag(fil, buf)) < 0) {
        goto fail;
    }
    /* ID3v1 tag is at the end */
    if ((c_id3 = probe_id3v1(fil, buf, !c_mm)) < 0) {
        goto fail;
    }
    /* we do not know the order of ape or lyrics3
     * or musicmatch tags, hence the loop here.. */
    c_ape = 0;
    c_lyr = 0;
    for (;;) {
        if (!c_lyr) {
        /* care about mp3s with double Lyrics3 tags? */
            if ((c_lyr = probe_lyrics3(fil, buf)) < 0)
                goto fail;
            if (c_lyr) continue;
        }
        if (!c_mm) {
            if ((c_mm = probe_mmtag(fil, buf)) < 0)
                goto fail;
            if (c_mm) continue;
        }
        if (!c_ape) {
            if ((c_ape = probe_apetag(fil, buf)) < 0)
                goto fail;
            if (c_ape) continue;
        }
        break;
    } /* for (;;) */

    rc = (fil->length > 0)? 0 : -1;
    fail:
    MP3_RWseek(fil, 0, RW_SEEK_SET);
    return rc;
}
#endif /* MUSIC_MP3_??? */

/* vi: set ts=4 sw=4 expandtab: */
