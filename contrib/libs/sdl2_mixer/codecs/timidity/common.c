/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.

    common.c
*/

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#include "options.h"
#include "common.h"

#if defined(__WIN32__) || defined(__OS2__)
#define CHAR_DIRSEP '\\'
#define is_dirsep(c) ((c) == '/' || (c) == '\\')
#define is_abspath(p) ((p)[0] == '/' || (p)[0] == '\\' || ((p)[0] && (p)[1] == ':'))
#else /* unix: */
#define CHAR_DIRSEP '/'
#define is_dirsep(c) ((c) == '/')
#define is_abspath(p) ((p)[0] == '/')
#endif

/* The paths in this list will be tried whenever we're reading a file */
static PathList *pathlist = NULL; /* This is a linked list */

/* This is meant to find and open files for reading */
SDL_RWops *open_file(const char *name)
{
  SDL_RWops *rw;

  if (!name || !(*name))
    {
      SNDDBG(("Attempted to open nameless file.\n"));
      return 0;
    }

  /* First try the given name */

  SNDDBG(("Trying to open %s\n", name));
  if ((rw = SDL_RWFromFile(name, "rb")))
    return rw;

  if (!is_abspath(name))
  {
    char current_filename[1024];
    PathList *plp = pathlist;
    size_t l;

    while (plp)  /* Try along the path then */
      {
	*current_filename = 0;
	l = strlen(plp->path);
	if(l)
	  {
	    strcpy(current_filename, plp->path);
	    if(!is_dirsep(current_filename[l - 1]))
	    {
	      current_filename[l] = CHAR_DIRSEP;
	      current_filename[l + 1] = '\0';
	    }
	  }
	strcat(current_filename, name);
	SNDDBG(("Trying to open %s\n", current_filename));
	if ((rw = SDL_RWFromFile(current_filename, "rb")))
	  return rw;
	plp = plp->next;
      }
  }
  
  /* Nothing could be opened. */
  SNDDBG(("Could not open %s\n", name));
  return 0;
}

/* This'll allocate memory or die. */
void *safe_malloc(size_t count)
{
  void *p;

  p = malloc(count);
  if (p == NULL) {
    SNDDBG(("Sorry. Couldn't malloc %d bytes.\n", count));
  }

  return p;
}

/* This adds a directory to the path list */
void add_to_pathlist(const char *s, size_t l)
{
  PathList *plp = safe_malloc(sizeof(PathList));

  if (plp == NULL)
      return;

  plp->path = safe_malloc(l + 1);
  if (plp->path == NULL)
  {
      free (plp);
      return;
  }
  strncpy(plp->path, s, l);
  plp->path[l] = 0;
  plp->next = pathlist;
  pathlist = plp;
}

void free_pathlist(void)
{
    PathList *plp = pathlist;
    PathList *next;

    while (plp)
    {
	next = plp->next;
	free(plp->path);
	free(plp);
	plp = next;
    }
    pathlist = NULL;
}
