/**
 * @file
 */

#pragma once

#include "tb_bitmap_fragment.h"
#include "tb_core.h"
#include "tb_hashtable.h"
#include "tb_linklist.h"
#include "tb_renderer.h"

namespace tb {

class TBImageManager;

/** TBImageRep is the internal contents of a TBImage. Owned by reference counting from TBImage. */
class TBImageRep {
	friend class TBImageManager;
	friend class TBImage;

	TBImageRep(TBImageManager *image_manager, TBBitmapFragment *fragment, uint32_t hash_key);

	void incRef();
	void decRef();

	int ref_count;
	uint32_t hash_key;
	TBImageManager *image_manager;
	TBBitmapFragment *fragment;
};

/** TBImage is a reference counting object representing a image loaded by TBImageManager.
	As long as there are TBImage objects for a certain image, it will be kept loaded in memory.
	It may be empty if the image has not yet been set, or if the TBImageManager is destroyed
	when the image is still alive.
*/
class TBImage {
public:
	TBImage() : m_image_rep(nullptr) {
	}
	TBImage(TBImageRep *rep);
	TBImage(const TBImage &image);
	~TBImage();

	/** Return true if this image is empty. */
	bool isEmpty() const;

	/** Return the width of this image, or 0 if empty. */
	int width() const;

	/** Return the height of this image, or 0 if empty. */
	int height() const;

	/** Return the bitmap fragment for this image, or nullptr if empty. */
	TBBitmapFragment *getBitmap() const;

	const TBImage &operator=(const TBImage &image) {
		setImageRep(image.m_image_rep);
		return *this;
	}
	bool operator==(const TBImage &image) const {
		return m_image_rep == image.m_image_rep;
	}
	bool operator!=(const TBImage &image) const {
		return m_image_rep != image.m_image_rep;
	}

private:
	void setImageRep(TBImageRep *image_rep);
	TBImageRep *m_image_rep;
};

/** TBImageManager loads images returned as TBImage objects.

	It internally use a TBBitmapFragmentManager that create fragment maps for loaded images,
	and keeping track of which images are loaded so they are not loaded several times.

	Images are forgotten when there are no longer any TBImage objects for a given file.
*/

class TBImageManager : private TBRendererListener {
public:
	TBImageManager();
	~TBImageManager();

	/** Return a image object for the given filename.
		If it fails, the returned TBImage object will be empty. */
	TBImage getImage(const char *filename);
	TBImage getImage(const char *name, uint32_t *buffer, int width, int height);

#ifdef TB_RUNTIME_DEBUG_INFO
	/** Render the skin bitmaps on screen, to analyze fragment positioning. */
	void debug() {
		m_frag_manager.debug();
	}
#endif

	// Implementing TBRendererListener
	virtual void onContextLost();
	virtual void onContextRestored();

private:
	TBBitmapFragmentManager m_frag_manager;
	TBHashTableOf<TBImageRep> m_image_rep_hash;

	friend class TBImageRep;
	void removeImageRep(TBImageRep *image_rep);
};

/** The global TBImageManager. */
extern TBImageManager *g_image_manager;

} // namespace tb
