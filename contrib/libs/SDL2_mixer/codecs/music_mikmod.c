/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>

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

#ifdef MUSIC_MOD_MIKMOD

/* This file supports MOD tracker music streams */

#include "SDL_loadso.h"

#include "music_mikmod.h"

#include "mikmod.h"


/* libmikmod >= 3.3.2 constified several funcs */
#if (LIBMIKMOD_VERSION < 0x030302)
#define MIKMOD3_CONST
#else
#define MIKMOD3_CONST const
#endif

typedef struct {
    int loaded;
    void *handle;

    void (*MikMod_Exit)(void);
    CHAR* (*MikMod_InfoDriver)(void);
    CHAR* (*MikMod_InfoLoader)(void);
    int (*MikMod_Init)(MIKMOD3_CONST CHAR*);
    void (*MikMod_RegisterAllLoaders)(void);
    void (*MikMod_RegisterDriver)(struct MDRIVER*);
    int* MikMod_errno;
    MIKMOD3_CONST char* (*MikMod_strerror)(int);
    void (*MikMod_free)(void*);
    BOOL (*Player_Active)(void);
    void (*Player_Free)(MODULE*);
    MODULE* (*Player_LoadGeneric)(MREADER*,int,BOOL);
    void (*Player_SetPosition)(UWORD);
    void (*Player_SetVolume)(SWORD);
    void (*Player_Start)(MODULE*);
    void (*Player_Stop)(void);
    ULONG (*VC_WriteBytes)(SBYTE*,ULONG);
    struct MDRIVER* drv_nos;
    UWORD* md_device;
    UWORD* md_mixfreq;
    UWORD* md_mode;
    UBYTE* md_musicvolume;
    UBYTE* md_pansep;
    UBYTE* md_reverb;
    UBYTE* md_sndfxvolume;
    UBYTE* md_volume;
} mikmod_loader;

static mikmod_loader mikmod;

#ifdef MIKMOD_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    mikmod.FUNC = (SIG) SDL_LoadFunction(mikmod.handle, #FUNC); \
    if (mikmod.FUNC == NULL) { SDL_UnloadObject(mikmod.handle); return -1; }
#define VARIABLE_LOADER(NAME, SIG) \
    mikmod.NAME = (SIG) SDL_LoadFunction(mikmod.handle, #NAME); \
    if (mikmod.NAME == NULL) { SDL_UnloadObject(mikmod.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    mikmod.FUNC = FUNC;
#define VARIABLE_LOADER(NAME, SIG) \
    mikmod.NAME = &NAME;
#endif

static int MIKMOD_Load(void)
{
    if (mikmod.loaded == 0) {
#ifdef MIKMOD_DYNAMIC
        mikmod.handle = SDL_LoadObject(MIKMOD_DYNAMIC);
        if (mikmod.handle == NULL) {
            return -1;
        }
#elif defined(__MACOSX__)
        extern void Player_Start(MODULE*) __attribute__((weak_import));
        if (Player_Start == NULL)
        {
            /* Missing weakly linked framework */
            Mix_SetError("Missing mikmod.framework");
            return -1;
        }
#endif
        FUNCTION_LOADER(MikMod_Exit, void (*)(void))
        FUNCTION_LOADER(MikMod_InfoDriver, CHAR* (*)(void))
        FUNCTION_LOADER(MikMod_InfoLoader, CHAR* (*)(void))
        FUNCTION_LOADER(MikMod_Init, int (*)(MIKMOD3_CONST CHAR*))
        FUNCTION_LOADER(MikMod_RegisterAllLoaders, void (*)(void))
        FUNCTION_LOADER(MikMod_RegisterDriver, void (*)(struct MDRIVER*))
        VARIABLE_LOADER(MikMod_errno, int*)
        FUNCTION_LOADER(MikMod_strerror, MIKMOD3_CONST char* (*)(int))
#ifdef MIKMOD_DYNAMIC
        mikmod.MikMod_free = (void (*)(void*)) SDL_LoadFunction(mikmod.handle, "MikMod_free");
        if (!mikmod.MikMod_free) {
            /* libmikmod-3.2.0-beta2 and earlier doesn't have it. */
            mikmod.MikMod_free = free;
        }
#else
#if (LIBMIKMOD_VERSION < 0x030200) || (LIBMIKMOD_VERSION == 0x030200 && !defined(DMODE_NOISEREDUCTION))
        /* libmikmod 3.2.0-beta2 or earlier */
        mikmod.MikMod_free = free;
#else
        mikmod.MikMod_free = MikMod_free;
#endif
#endif /* MIKMOD_DYNAMIC */
        FUNCTION_LOADER(Player_Active, BOOL (*)(void))
        FUNCTION_LOADER(Player_Free, void (*)(MODULE*))
        FUNCTION_LOADER(Player_LoadGeneric, MODULE* (*)(MREADER*,int,BOOL))
        FUNCTION_LOADER(Player_SetPosition, void (*)(UWORD))
        FUNCTION_LOADER(Player_SetVolume, void (*)(SWORD))
        FUNCTION_LOADER(Player_Start, void (*)(MODULE*))
        FUNCTION_LOADER(Player_Stop, void (*)(void))
        FUNCTION_LOADER(VC_WriteBytes, ULONG (*)(SBYTE*,ULONG))
        VARIABLE_LOADER(drv_nos, MDRIVER*)
        VARIABLE_LOADER(md_device, UWORD*)
        VARIABLE_LOADER(md_mixfreq, UWORD*)
        VARIABLE_LOADER(md_mode, UWORD*)
        VARIABLE_LOADER(md_musicvolume, UBYTE*)
        VARIABLE_LOADER(md_pansep, UBYTE*)
        VARIABLE_LOADER(md_reverb, UBYTE*)
        VARIABLE_LOADER(md_sndfxvolume, UBYTE*)
        VARIABLE_LOADER(md_volume, UBYTE*)
    }
    ++mikmod.loaded;

    return 0;
}

static void MIKMOD_Unload(void)
{
    if (mikmod.loaded == 0) {
        return;
    }
    if (mikmod.loaded == 1) {
#ifdef MIKMOD_DYNAMIC
        SDL_UnloadObject(mikmod.handle);
#endif
    }
    --mikmod.loaded;
}


typedef struct
{
    int play_count;
    int volume;
    MODULE *module;
    SDL_AudioStream *stream;
    SBYTE *buffer;
    ULONG buffer_size;
    Mix_MusicMetaTags tags;
} MIKMOD_Music;


static int MIKMOD_Seek(void *context, double position);
static void MIKMOD_Delete(void *context);

/* Initialize the MOD player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
static int MIKMOD_Open(const SDL_AudioSpec *spec)
{
    CHAR *list;

    /* Set the MikMod music format */
    if (spec->format == AUDIO_S8 || spec->format == AUDIO_U8) {
        /* MIKMOD audio format is AUDIO_U8 */
        *mikmod.md_mode = 0;
    } else {
        /* MIKMOD audio format is AUDIO_S16SYS */
        *mikmod.md_mode = DMODE_16BITS;
    }
    if (spec->channels > 1) {
        *mikmod.md_mode |= DMODE_STEREO;
    }
    *mikmod.md_mixfreq = (spec->freq <= 65535) ? (UWORD)spec->freq : 44100;
    *mikmod.md_device  = 0;
    *mikmod.md_volume  = 96;
    *mikmod.md_musicvolume = 128;
    *mikmod.md_sndfxvolume = 128;
    *mikmod.md_pansep  = 128;
    *mikmod.md_reverb  = 0;
    *mikmod.md_mode    |= DMODE_HQMIXER|DMODE_SOFT_MUSIC|DMODE_SURROUND;

    list = mikmod.MikMod_InfoDriver();
    if (list) {
      mikmod.MikMod_free(list);
    } else {
      mikmod.MikMod_RegisterDriver(mikmod.drv_nos);
    }

    list = mikmod.MikMod_InfoLoader();
    if (list) {
      mikmod.MikMod_free(list);
    } else {
      mikmod.MikMod_RegisterAllLoaders();
    }

    if (mikmod.MikMod_Init(NULL)) {
        Mix_SetError("%s", mikmod.MikMod_strerror(*mikmod.MikMod_errno));
        return -1;
    }
    return 0;
}

/* Uninitialize the music players */
static void MIKMOD_Close(void)
{
    if (mikmod.MikMod_Exit) {
        mikmod.MikMod_Exit();
    }
}

typedef struct
{
    /* MREADER basic members in libmikmod2/3: */
    int  (*Seek)(struct MREADER*, long, int);
    long (*Tell)(struct MREADER*);
    BOOL (*Read)(struct MREADER*, void*, size_t);
    int  (*Get)(struct MREADER*);
    BOOL (*Eof)(struct MREADER*);
    /* no iobase members in libmikmod <= 3.2.0-beta2 */
    long iobase, prev_iobase;

    Sint64 eof;
    SDL_RWops *src;
} LMM_MREADER;

int LMM_Seek(struct MREADER *mr,long to,int dir)
{
    Sint64 offset = to;
    LMM_MREADER* lmmmr = (LMM_MREADER*)mr;
    if (dir == SEEK_SET) {
        offset += lmmmr->iobase;
        if (offset < lmmmr->iobase)
            return -1;
    }
    return (SDL_RWseek(lmmmr->src, offset, dir) < lmmmr->iobase)? -1 : 0;
}
long LMM_Tell(struct MREADER *mr)
{
    LMM_MREADER* lmmmr = (LMM_MREADER*)mr;
    return (long)(SDL_RWtell(lmmmr->src) - lmmmr->iobase);
}
BOOL LMM_Read(struct MREADER *mr,void *buf,size_t sz)
{
    LMM_MREADER* lmmmr = (LMM_MREADER*)mr;
    return (BOOL)SDL_RWread(lmmmr->src, buf, sz, 1);
}
int LMM_Get(struct MREADER *mr)
{
    unsigned char c;
    LMM_MREADER* lmmmr = (LMM_MREADER*)mr;
    if (SDL_RWread(lmmmr->src, &c, 1, 1)) {
        return c;
    }
    return EOF;
}
BOOL LMM_Eof(struct MREADER *mr)
{
    Sint64 offset;
    LMM_MREADER* lmmmr = (LMM_MREADER*)mr;
    offset = LMM_Tell(mr);
    return offset >= lmmmr->eof;
}
MODULE *MikMod_LoadSongRW(SDL_RWops *src, int maxchan)
{
    LMM_MREADER lmmmr;
    SDL_memset(&lmmmr, 0, sizeof(LMM_MREADER));
    lmmmr.Seek = LMM_Seek;
    lmmmr.Tell = LMM_Tell;
    lmmmr.Read = LMM_Read;
    lmmmr.Get = LMM_Get;
    lmmmr.Eof = LMM_Eof;

    lmmmr.prev_iobase = lmmmr.iobase = SDL_RWtell(src);
    SDL_RWseek(src, 0, RW_SEEK_END);
    lmmmr.eof = SDL_RWtell(src);
    SDL_RWseek(src, lmmmr.iobase, RW_SEEK_SET);
    lmmmr.src = src;
    return mikmod.Player_LoadGeneric((MREADER*)&lmmmr, maxchan, 0);
}

/* Load a MOD stream from an SDL_RWops object */
void *MIKMOD_CreateFromRW(SDL_RWops *src, int freesrc)
{
    MIKMOD_Music *music;
    SDL_AudioFormat format;
    Uint8 channels;

    music = (MIKMOD_Music *)SDL_calloc(1, sizeof(*music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }
    music->volume = MIX_MAX_VOLUME;

    music->module = MikMod_LoadSongRW(src, 64);
    if (!music->module) {
        Mix_SetError("%s", mikmod.MikMod_strerror(*mikmod.MikMod_errno));
        MIKMOD_Delete(music);
        return NULL;
    }

    /* Allow implicit looping, disable fade out and other flags. */
    music->module->extspd  = 1;
    music->module->panflag = 1;
    music->module->wrap    = 0;
    music->module->loop    = 1;
    music->module->fadeout = 0;

    meta_tags_init(&music->tags);
    meta_tags_set(&music->tags, MIX_META_TITLE, music->module->songname);

    if ((*mikmod.md_mode & DMODE_16BITS) == DMODE_16BITS) {
        format = AUDIO_S16SYS;
    } else {
        format = AUDIO_U8;
    }
    if ((*mikmod.md_mode & DMODE_STEREO) == DMODE_STEREO) {
        channels = 2;
    } else {
        channels = 1;
    }
    music->stream = SDL_NewAudioStream(format, channels, *mikmod.md_mixfreq,
                                       music_spec.format, music_spec.channels, music_spec.freq);
    if (!music->stream) {
        MIKMOD_Delete(music);
        return NULL;
    }

    music->buffer_size = music_spec.samples * (SDL_AUDIO_BITSIZE(format) / 8) * channels;
    music->buffer = (SBYTE *)SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        MIKMOD_Delete(music);
        return NULL;
    }

    if (freesrc) {
        SDL_RWclose(src);
    }

    return music;
}

/* Set the volume for a MOD stream */
static void MIKMOD_SetVolume(void *context, int volume)
{
    MIKMOD_Music *music = (MIKMOD_Music *)context;
    music->volume = volume;
    mikmod.Player_SetVolume((SWORD)volume);
}

/* Set the volume for a MOD stream */
static int MIKMOD_GetVolume(void *context)
{
    MIKMOD_Music *music = (MIKMOD_Music *)context;
    return music->volume;
}

/* Start playback of a given MOD stream */
static int MIKMOD_Play(void *context, int play_count)
{
    MIKMOD_Music *music = (MIKMOD_Music *)context;
    music->play_count = play_count;
    music->module->initvolume = (UBYTE)music->volume;
    mikmod.Player_Start(music->module);
    return MIKMOD_Seek(music, 0.0);
}

/* Return non-zero if a stream is currently playing */
static SDL_bool MIKMOD_IsPlaying(void *context)
{
    (void)context;
    return mikmod.Player_Active() ? SDL_TRUE : SDL_FALSE;
}

/* Play some of a stream previously started with MOD_play() */
static int MIKMOD_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    MIKMOD_Music *music = (MIKMOD_Music *)context;
    int filled;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    /* This never fails, and always writes a full buffer */
    mikmod.VC_WriteBytes(music->buffer, music->buffer_size);

    if (SDL_AudioStreamPut(music->stream, music->buffer, (int)music->buffer_size) < 0) {
        return -1;
    }

    /* Check to see if we're done now */
    if (!mikmod.Player_Active()) {
        if (music->play_count == 1) {
            music->play_count = 0;
            SDL_AudioStreamFlush(music->stream);
        } else {
            int play_count = -1;
            if (music->play_count > 0) {
                play_count = (music->play_count - 1);
            }
            if (MIKMOD_Play(music, play_count) < 0) {
                return -1;
            }
        }
    }
    return 0;
}
static int MIKMOD_GetAudio(void *context, void *data, int bytes)
{
    return music_pcm_getaudio(context, data, bytes, MIX_MAX_VOLUME, MIKMOD_GetSome);
}

/* Jump to a given order */
static int MIKMOD_Jump(void *context, int order)
{
    (void)context;
    mikmod.Player_SetPosition((UWORD)order);
    return 0;
}

/* Jump (seek) to a given position (time is in seconds) */
static int MIKMOD_Seek(void *context, double position)
{
    (void)context;
    /* FIXME: WRONG: THIS IS NOT A TIME SEEK */
    mikmod.Player_SetPosition((UWORD)position);
    return 0;
}

/* Stop playback of a stream previously started with MOD_play() */
static void MIKMOD_Stop(void *context)
{
    (void)context;
    mikmod.Player_Stop();
}

static const char* MIKMOD_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    MIKMOD_Music *music = (MIKMOD_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Close the given MOD stream */
static void MIKMOD_Delete(void *context)
{
    MIKMOD_Music *music = (MIKMOD_Music *)context;

    meta_tags_clear(&music->tags);
    if (music->module) {
        mikmod.Player_Free(music->module);
    }
    if (music->stream) {
        SDL_FreeAudioStream(music->stream);
    }
    if (music->buffer) {
        SDL_free(music->buffer);
    }
    SDL_free(music);
}

Mix_MusicInterface Mix_MusicInterface_MIKMOD =
{
    "MIKMOD",
    MIX_MUSIC_MIKMOD,
    MUS_MOD,
    SDL_FALSE,
    SDL_FALSE,

    MIKMOD_Load,
    MIKMOD_Open,
    MIKMOD_CreateFromRW,
    NULL,   /* CreateFromFile */
    MIKMOD_SetVolume,
    MIKMOD_GetVolume,
    MIKMOD_Play,
    MIKMOD_IsPlaying,
    MIKMOD_GetAudio,
    MIKMOD_Jump,
    MIKMOD_Seek,
    NULL,   /* Tell */
    NULL,   /* Duration */
    NULL,   /* LoopStart */
    NULL,   /* LoopEnd */
    NULL,   /* LoopLength */
    MIKMOD_GetMetaTag,
    NULL,   /* Pause */
    NULL,   /* Resume */
    MIKMOD_Stop,
    MIKMOD_Delete,
    MIKMOD_Close,
    MIKMOD_Unload
};

#endif /* MUSIC_MOD_MIKMOD */

/* vi: set ts=4 sw=4 expandtab: */
