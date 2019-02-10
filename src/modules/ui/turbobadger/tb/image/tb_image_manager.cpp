/**
 * @file
 */

#include "tb_image_manager.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"
#include "tb_skin.h"

namespace tb {

TBImageRep::TBImageRep(TBImageManager *imageManager, TBBitmapFragment *fragment, uint32_t hashKey)
	: ref_count(0), hash_key(hashKey), image_manager(imageManager), fragment(fragment)
{
}

void TBImageRep::incRef()
{
	ref_count++;
}

void TBImageRep::decRef()
{
	ref_count--;
	if (ref_count == 0)
	{
		if (image_manager)
			image_manager->removeImageRep(this);
		delete this;
	}
}

TBImage::TBImage(TBImageRep *rep)
	: m_image_rep(rep)
{
	if (m_image_rep)
		m_image_rep->incRef();
}

TBImage::TBImage(const TBImage &image)
	: m_image_rep(image.m_image_rep)
{
	if (m_image_rep)
		m_image_rep->incRef();
}

TBImage::~TBImage()
{
	if (m_image_rep)
		m_image_rep->decRef();
}

bool TBImage::isEmpty() const
{
	return !(m_image_rep && m_image_rep->fragment);
}

int TBImage::width() const
{
	if (m_image_rep && m_image_rep->fragment)
		return m_image_rep->fragment->width();
	return 0;
}

int TBImage::height() const
{
	if (m_image_rep && m_image_rep->fragment)
		return m_image_rep->fragment->height();
	return 0;
}

TBBitmapFragment *TBImage::getBitmap() const
{
	return m_image_rep ? m_image_rep->fragment : nullptr;
}

void TBImage::setImageRep(TBImageRep *imageRep)
{
	if (m_image_rep == imageRep)
		return;

	if (m_image_rep)
		m_image_rep->decRef();

	m_image_rep = imageRep;

	if (m_image_rep)
		m_image_rep->incRef();
}

TBImageManager *g_image_manager = nullptr;

TBImageManager::TBImageManager()
{
	g_renderer->addListener(this);
}

TBImageManager::~TBImageManager()
{
	g_renderer->removeListener(this);

	// If there is TBImageRep objects live, we must unset the fragment pointer
	// since the m_frag_manager is going to be destroyed very soon.
	TBHashTableIteratorOf<TBImageRep> it(&m_image_rep_hash);
	while (TBImageRep *image_rep = it.getNextContent())
	{
		image_rep->fragment = nullptr;
		image_rep->image_manager = nullptr;
	}
}

TBImage TBImageManager::getImage(const char *filename)
{
	uint32_t hash_key = TBGetHash(filename);
	TBImageRep *image_rep = m_image_rep_hash.get(hash_key);
	if (!image_rep)
	{
		// Load a fragment. Load a destination DPI bitmap if available.
		TBBitmapFragment *fragment = nullptr;
		if (g_tb_skin->getDimensionConverter()->needConversion())
		{
			TBTempBuffer filename_dst_DPI;
			g_tb_skin->getDimensionConverter()->getDstDPIFilename(filename, &filename_dst_DPI);
			fragment = m_frag_manager.getFragmentFromFile(filename_dst_DPI.getData(), false);
		}
		if (!fragment)
			fragment = m_frag_manager.getFragmentFromFile(filename, false);

		image_rep = new TBImageRep(this, fragment, hash_key);
		if (!image_rep || !fragment || !m_image_rep_hash.add(hash_key, image_rep))
		{
			delete image_rep;
			m_frag_manager.freeFragment(fragment);
			image_rep = nullptr;
		}
		Log::debug(image_rep ? "TBImageManager - Loaded new image.\n" : "TBImageManager - Loading image failed.");
	}
	return TBImage(image_rep);
}

TBImage TBImageManager::getImage(const char *name, uint32_t *buffer, int width, int height)
{
	uint32_t hash_key = TBGetHash(name);
	TBImageRep *image_rep = m_image_rep_hash.get(hash_key);
	if (!image_rep)
	{
		TBID id(name);
		TBBitmapFragment *fragment = m_frag_manager.createNewFragment(id, false, width, height, width, buffer);

		image_rep = new TBImageRep(this, fragment, hash_key);
		if (!image_rep || !fragment || !m_image_rep_hash.add(hash_key, image_rep))
		{
			delete image_rep;
			m_frag_manager.freeFragment(fragment);
			image_rep = nullptr;
		}
		Log::debug(image_rep ? "TBImageManager - Loaded new image.\n" : "TBImageManager - Loading image failed.");
	}
	return TBImage(image_rep);
}

void TBImageManager::removeImageRep(TBImageRep *imageRep)
{
	core_assert(imageRep->ref_count == 0);
	if (imageRep->fragment)
	{
		m_frag_manager.freeFragment(imageRep->fragment);
		imageRep->fragment = nullptr;
	}
	m_image_rep_hash.remove(imageRep->hash_key);
	imageRep->image_manager = nullptr;
	Log::debug("TBImageManager - Removed image.");
}

void TBImageManager::onContextLost()
{
	m_frag_manager.deleteBitmaps();
}

void TBImageManager::onContextRestored()
{
	// No need to do anything. The bitmaps will be created when drawing.
}

} // namespace tb
