/*
 * Texture Filtering
 * Version:  1.0
 *
 * Copyright (C) 2007  Hiroshi Morii   All Rights Reserved.
 * Email koolsmoky(at)users.sourceforge.net
 * Web   http://www.3dfxzone.it/koolsmoky
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef __MSC__
#pragma warning(disable: 4786)
#endif

#include <fstream>
#include <unordered_map>
#include <zlib.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>

#include <osal_files.h>

#include "TxCache.h"
#include "TxDbg.h"

class TxCacheImpl
{
public:
	virtual ~TxCacheImpl() = default;

	virtual bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) = 0;
	virtual bool get(Checksum checksum, GHQTexInfo *info) = 0;
	virtual bool save(const wchar_t *path, const wchar_t *filename, const int config) = 0;
	virtual bool load(const wchar_t *path, const wchar_t *filename, const int config, bool force) = 0;
	virtual bool del(Checksum checksum) = 0;
	virtual bool isCached(Checksum checksum) = 0;
	virtual void clear() = 0;
	virtual bool empty() const = 0;
	virtual uint32 getOptions() const = 0;
	virtual void setOptions(uint32 options) = 0;

	virtual uint64 size() const = 0;
	virtual uint64 totalSize() const = 0;
	virtual uint64 cacheLimit() const = 0;
};


/************************** TxMemoryCache *************************************/

class TxMemoryCache : public TxCacheImpl
{
public:
	TxMemoryCache(uint32 _options, uint64 cacheLimit, dispInfoFuncExt callback);
	~TxMemoryCache();

	bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) override;
	bool get(Checksum checksum, GHQTexInfo *info) override;

	bool save(const wchar_t *path, const wchar_t *filename, const int config) override;
	bool load(const wchar_t *path, const wchar_t *filename, const int config, bool force) override;
	bool del(Checksum checksum) override;
	bool isCached(Checksum checksum) override;
	void clear() override;
	bool empty() const  override { return _cache.empty(); }

	uint64 size() const  override { return _cache.size(); }
	uint64 totalSize() const  override { return _totalSize; }
	uint64 cacheLimit() const  override { return _cacheLimit; }
	uint32 getOptions() const override { return _options; }
	void setOptions(uint32 options) override { _options = options; }

private:
	struct TXCACHE {
		int size;
		GHQTexInfo info;
		std::list<uint64>::iterator it;
	};

	uint32 _options;
	dispInfoFuncExt _callback;
	uint64 _cacheLimit;
	uint64 _totalSize;

	std::map<uint64, TXCACHE*> _cache;
	std::list<uint64> _cachelist;

	uint8 *_gzdest0 = nullptr;
	uint8 *_gzdest1 = nullptr;
	uint32 _gzdestLen = 0;
};

TxMemoryCache::TxMemoryCache(uint32 options,
	uint64 cacheLimit,
	dispInfoFuncExt callback)
	: _options(options)
	, _cacheLimit(cacheLimit)
	, _callback(callback)
	, _totalSize(0U)
{
	/* zlib memory buffers to (de)compress hires textures */
	if (_options & (GZ_TEXCACHE | GZ_HIRESTEXCACHE)) {
		_gzdest0 = TxMemBuf::getInstance()->get(0);
		_gzdest1 = TxMemBuf::getInstance()->get(1);
		_gzdestLen = (TxMemBuf::getInstance()->size_of(0) < TxMemBuf::getInstance()->size_of(1)) ?
			TxMemBuf::getInstance()->size_of(0) : TxMemBuf::getInstance()->size_of(1);

		if (!_gzdest0 || !_gzdest1 || !_gzdestLen) {
			_options &= ~(GZ_TEXCACHE | GZ_HIRESTEXCACHE);
			_gzdest0 = nullptr;
			_gzdest1 = nullptr;
			_gzdestLen = 0;
		}
	}
}

TxMemoryCache::~TxMemoryCache()
{
	/* free memory, clean up, etc */
	clear();
}

bool TxMemoryCache::add(Checksum checksum, GHQTexInfo *info, int dataSize)
{
	/* NOTE: dataSize must be provided if info->data is zlib compressed. */

	if (!checksum || !info->data || _cache.find(checksum) != _cache.end())
		return false;

	uint8 *dest = info->data;
	uint32 format = info->format;

	if (dataSize == 0) {
		dataSize = TxUtil::sizeofTx(info->width, info->height, info->format);

		if (!dataSize)
			return false;

		if (_options & (GZ_TEXCACHE | GZ_HIRESTEXCACHE)) {
			/* zlib compress it. compression level:1 (best speed) */
			uLongf destLen = _gzdestLen;
			dest = (dest == _gzdest0) ? _gzdest1 : _gzdest0;
			if (compress2(dest, &destLen, info->data, dataSize, 1) != Z_OK) {
				dest = info->data;
				DBG_INFO(80, wst("Error: zlib compression failed!\n"));
			} else {
				DBG_INFO(80, wst("zlib compressed: %.02fkb->%.02fkb\n"), (float)dataSize / 1000, (float)destLen / 1000);
				dataSize = destLen;
				format |= GL_TEXFMT_GZ;
			}
		}
	}

	/* if cache size exceeds limit, remove old cache */
	if (_cacheLimit != 0) {
		_totalSize += dataSize;
		if ((_totalSize > _cacheLimit) && !_cachelist.empty()) {
			/* _cachelist is arranged so that frequently used textures are in the back */
			std::list<uint64>::iterator itList = _cachelist.begin();
			while (itList != _cachelist.end()) {
				/* find it in _cache */
				auto itMap = _cache.find(*itList);
				if (itMap != _cache.end()) {
					/* yep we have it. remove it. */
					_totalSize -= (*itMap).second->size;
					free((*itMap).second->info.data);
					delete (*itMap).second;
					_cache.erase(itMap);
				}
				itList++;

				/* check if memory cache has enough space */
				if (_totalSize <= _cacheLimit)
					break;
			}
			/* remove from _cachelist */
			_cachelist.erase(_cachelist.begin(), itList);

			DBG_INFO(80, wst("+++++++++\n"));
		}
		_totalSize -= dataSize;
	}

	/* cache it */
	uint8 *tmpdata = (uint8*)malloc(dataSize);
	if (tmpdata == nullptr)
		return false;

	TXCACHE *txCache = new TXCACHE;
	/* we can directly write as we filter, but for now we get away
	* with doing memcpy after all the filtering is done.
	*/
	memcpy(tmpdata, dest, dataSize);

	/* copy it */
	memcpy(&txCache->info, info, sizeof(GHQTexInfo));
	txCache->info.data = tmpdata;
	txCache->info.format = format;
	txCache->size = dataSize;

	/* add to cache */
	if (_cacheLimit != 0) {
		_cachelist.push_back(checksum);
		txCache->it = --(_cachelist.end());
	}
	/* _cache[checksum] = txCache; */
	_cache.insert(std::map<uint64, TXCACHE*>::value_type(checksum, txCache));

#ifdef DEBUG
	DBG_INFO(80, wst("[%5d] added!! crc:%08X %08X %d x %d gfmt:%x total:%.02fmb\n"),
		_cache.size(), checksum._hi, checksum._low,
		info->width, info->height, info->format & 0xffff, (double)_totalSize / 1000000);

	if (_cacheLimit != 0) {
		DBG_INFO(80, wst("cache max config:%.02fmb\n"), (double)_cacheLimit / 1000000);

		if (_cache.size() != _cachelist.size()) {
			DBG_INFO(80, wst("Error: cache/cachelist mismatch! (%d/%d)\n"), _cache.size(), _cachelist.size());
		}
	}
#endif

	/* total cache size */
	_totalSize += dataSize;

	return true;
}

bool TxMemoryCache::get(Checksum checksum, GHQTexInfo *info)
{
	if (!checksum || _cache.empty())
		return false;

	/* find a match in cache */
	auto itMap = _cache.find(checksum);
	if (itMap != _cache.end()) {
		/* yep, we've got it. */
		memcpy(info, &(((*itMap).second)->info), sizeof(GHQTexInfo));

		/* push it to the back of the list */
		if (_cacheLimit != 0) {
			_cachelist.erase(((*itMap).second)->it);
			_cachelist.push_back(checksum);
			((*itMap).second)->it = --(_cachelist.end());
		}

		/* zlib decompress it */
		if (info->format & GL_TEXFMT_GZ) {
			uLongf destLen = _gzdestLen;
			uint8 *dest = (_gzdest0 == info->data) ? _gzdest1 : _gzdest0;
			if (uncompress(dest, &destLen, info->data, ((*itMap).second)->size) != Z_OK) {
				DBG_INFO(80, wst("Error: zlib decompression failed!\n"));
				return false;
			}
			info->data = dest;
			info->format &= ~GL_TEXFMT_GZ;
			DBG_INFO(80, wst("zlib decompressed: %.02fkb->%.02fkb\n"), (float)(((*itMap).second)->size) / 1000, (float)destLen / 1000);
		}

		return true;
	}

	return false;
}

bool TxMemoryCache::save(const wchar_t *path, const wchar_t *filename, int config)
{
	if (_cache.empty())
		return false;

	/* dump cache to disk */
	char cbuf[MAX_PATH];

	osal_mkdirp(path);

#ifdef OS_WINDOWS
	wchar_t curpath[MAX_PATH];
	GETCWD(MAX_PATH, curpath);
	CHDIR(path);
#else
	char curpath[MAX_PATH];
	GETCWD(MAX_PATH, curpath);
	wcstombs(cbuf, path, MAX_PATH);
	CHDIR(cbuf);
#endif

	wcstombs(cbuf, filename, MAX_PATH);

	gzFile gzfp = gzopen(cbuf, "wb1");
	DBG_INFO(80, wst("gzfp:%x file:%ls\n"), gzfp, filename);
	if (gzfp) {
		/* write header to determine config match */
		gzwrite(gzfp, &config, 4);

		auto itMap = _cache.begin();
		int total = 0;
		while (itMap != _cache.end()) {
			uint8 *dest = (*itMap).second->info.data;
			uint32 destLen = (*itMap).second->size;
			uint32 format = (*itMap).second->info.format;

			/* to keep things simple, we save the texture data in a zlib uncompressed state. */
			/* sigh... for those who cannot wait the extra few seconds. changed to keep
			* texture data in a zlib compressed state. if the GZ_TEXCACHE or GZ_HIRESTEXCACHE
			* option is toggled, the cache will need to be rebuilt.
			*/
			/*if (format & GL_TEXFMT_GZ) {
			dest = _gzdest0;
			destLen = _gzdestLen;
			if (dest && destLen) {
			if (uncompress(dest, &destLen, (*itMap).second->info.data, (*itMap).second->size) != Z_OK) {
			dest = nullptr;
			destLen = 0;
			}
			format &= ~GL_TEXFMT_GZ;
			}
			}*/

			if (dest && destLen) {
				/* texture checksum */
				gzwrite(gzfp, &((*itMap).first), 8);

				/* other texture info */
				gzwrite(gzfp, &((*itMap).second->info.width), 4);
				gzwrite(gzfp, &((*itMap).second->info.height), 4);
				gzwrite(gzfp, &format, 4);
				gzwrite(gzfp, &((*itMap).second->info.texture_format), 2);
				gzwrite(gzfp, &((*itMap).second->info.pixel_type), 2);
				gzwrite(gzfp, &((*itMap).second->info.is_hires_tex), 1);

				gzwrite(gzfp, &destLen, 4);
				gzwrite(gzfp, dest, destLen);
			}

			itMap++;

			if (_callback)
				(*_callback)(wst("Total textures saved to HDD: %d\n"), ++total);
		}
		gzclose(gzfp);
	}

	CHDIR(curpath);

	return !_cache.empty();
}

bool TxMemoryCache::load(const wchar_t *path, const wchar_t *filename, int config, bool force)
{
	/* find it on disk */
	char cbuf[MAX_PATH];

#ifdef OS_WINDOWS
	wchar_t curpath[MAX_PATH];
	GETCWD(MAX_PATH, curpath);
	CHDIR(path);
#else
	char curpath[MAX_PATH];
	GETCWD(MAX_PATH, curpath);
	wcstombs(cbuf, path, MAX_PATH);
	CHDIR(cbuf);
#endif

	wcstombs(cbuf, filename, MAX_PATH);

	gzFile gzfp = gzopen(cbuf, "rb");
	DBG_INFO(80, wst("gzfp:%x file:%ls\n"), gzfp, filename);
	if (gzfp) {
		/* yep, we have it. load it into memory cache. */
		int dataSize;
		uint64 checksum;
		int tmpconfig;
		/* read header to determine config match */
		gzread(gzfp, &tmpconfig, 4);

		if (tmpconfig == config || force) {
			do {
				GHQTexInfo tmpInfo;

				gzread(gzfp, &checksum, 8);

				gzread(gzfp, &tmpInfo.width, 4);
				gzread(gzfp, &tmpInfo.height, 4);
				gzread(gzfp, &tmpInfo.format, 4);
				gzread(gzfp, &tmpInfo.texture_format, 2);
				gzread(gzfp, &tmpInfo.pixel_type, 2);
				gzread(gzfp, &tmpInfo.is_hires_tex, 1);

				gzread(gzfp, &dataSize, 4);

				tmpInfo.data = (uint8*)malloc(dataSize);
				if (tmpInfo.data) {
					gzread(gzfp, tmpInfo.data, dataSize);

					/* add to memory cache */
					add(checksum, &tmpInfo, (tmpInfo.format & GL_TEXFMT_GZ) ? dataSize : 0);

					free(tmpInfo.data);
				} else {
					gzseek(gzfp, dataSize, SEEK_CUR);
				}

				/* skip in between to prevent the loop from being tied down to vsync */
				if (_callback && (!(_cache.size() % 100) || gzeof(gzfp)))
					(*_callback)(wst("[%d] total mem:%.02fmb - %ls\n"), _cache.size(), (float)_totalSize / 1000000, filename);

			} while (!gzeof(gzfp));
			gzclose(gzfp);
		}
	}

	CHDIR(curpath);

	return !_cache.empty();
}

bool TxMemoryCache::del(Checksum checksum)
{
	if (!checksum || _cache.empty())
		return false;

	auto itMap = _cache.find(checksum);
	if (itMap != _cache.end()) {

		/* for texture cache (not hi-res cache) */
		if (!_cachelist.empty())
			_cachelist.erase(((*itMap).second)->it);

		/* remove from cache */
		free((*itMap).second->info.data);
		_totalSize -= (*itMap).second->size;
		delete (*itMap).second;
		_cache.erase(itMap);

		DBG_INFO(80, wst("removed from cache: checksum = %08X %08X\n"), checksum._low, checksum._hi);

		return true;
	}

	return false;
}

bool TxMemoryCache::isCached(Checksum checksum)
{
	return _cache.find(checksum) != _cache.end();
}

void TxMemoryCache::clear()
{
	if (!_cache.empty()) {
		auto itMap = _cache.begin();
		while (itMap != _cache.end()) {
			free((*itMap).second->info.data);
			delete (*itMap).second;
			itMap++;
		}
		_cache.clear();
	}

	if (!_cachelist.empty())
		_cachelist.clear();

	_totalSize = 0;
}

/************************** TxFileCache *************************************/

class TxFileStorage : public TxCacheImpl
{
public:
	TxFileStorage(uint32 _options, const wchar_t *cachePath, dispInfoFuncExt callback);
	~TxFileStorage() = default;

	bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) override;
	bool get(Checksum checksum, GHQTexInfo *info) override;

	bool save(const wchar_t *path, const wchar_t *filename, const int config) override;
	bool load(const wchar_t *path, const wchar_t *filename, const int config, bool force) override;
	bool del(Checksum checksum) override { return false; }
	bool isCached(Checksum checksum) override;
	void clear() override;
	bool empty() const override { return _storage.empty(); }

	uint64 size() const override { return _storage.size(); }
	uint64 totalSize() const override { return _totalSize; }
	uint64 cacheLimit() const override { return 0UL; }
	uint32 getOptions() const override { return _options; }
	void setOptions(uint32 options) override { _options = options; }

private:
	bool open(bool forRead);
	bool writeData(uint32 destLen, const GHQTexInfo & info);
	bool readData(GHQTexInfo & info);
	void buildFullPath();

	uint32 _options;
	tx_wstring _cachePath;
	tx_wstring _filename;
	std::string _fullPath;
	dispInfoFuncExt _callback;
	uint64 _totalSize = 0;

	using StorageMap = std::unordered_map<uint64, int64>;
	StorageMap _storage;

	uint8 *_gzdest0 = nullptr;
	uint8 *_gzdest1 = nullptr;
	uint32 _gzdestLen = 0;

	std::ifstream _infile;
	std::ofstream _outfile;
	int64 _storagePos = 0;
	bool _dirty = false;
	static const int _fakeConfig;
	static const int64 _initialPos;
};

const int TxFileStorage::_fakeConfig = -1;
const int64 TxFileStorage::_initialPos = sizeof(int64) + sizeof(int);

TxFileStorage::TxFileStorage(uint32 options,
	const wchar_t *cachePath,
	dispInfoFuncExt callback)
	: _options(options)
	, _callback(callback)
{
	/* save path name */
	if (cachePath)
		_cachePath.assign(cachePath);

	_gzdest0 = TxMemBuf::getInstance()->get(0);
	_gzdest1 = TxMemBuf::getInstance()->get(1);
	_gzdestLen = (TxMemBuf::getInstance()->size_of(0) < TxMemBuf::getInstance()->size_of(1)) ?
		TxMemBuf::getInstance()->size_of(0) : TxMemBuf::getInstance()->size_of(1);

	if (!_gzdest0 || !_gzdest1 || !_gzdestLen) {
		_options &= ~(GZ_TEXCACHE | GZ_HIRESTEXCACHE);
		_gzdest0 = nullptr;
		_gzdest1 = nullptr;
		_gzdestLen = 0;
	}
}

#define FWRITE(a) _outfile.write((char*)(&a), sizeof(a))
#define FREAD(a) _infile.read((char*)(&a), sizeof(a))

void TxFileStorage::buildFullPath()
{
	char cbuf[MAX_PATH * 2];
	tx_wstring filename = _cachePath + OSAL_DIR_SEPARATOR_STR + _filename;
	wcstombs(cbuf, filename.c_str(), MAX_PATH * 2);
	_fullPath = cbuf;
}

bool TxFileStorage::open(bool forRead)
{
	if (_infile.is_open())
		_infile.close();
	if (_outfile.is_open())
		_outfile.close();

	if (forRead) {
		/* find it on disk */
		_infile.open(_fullPath, std::ifstream::in | std::ifstream::binary);
		DBG_INFO(80, wst("file:%s %s\n"), _fullPath.c_str(), _infile.good() ? "opened for read" : "failed to open");
		return _infile.good();
	}

	if (osal_path_existsA(_fullPath.c_str()) != 0) {
		assert(_storagePos != 0L);
		_outfile.open(_fullPath, std::ofstream::out | std::ofstream::binary);
		DBG_INFO(80, wst("file:%s %s\n"), _fullPath.c_str(), _outfile.good() ? "opened for write" : "failed to open");
		return _outfile.good();
	}

	if (osal_mkdirp(_cachePath.c_str()) != 0)
		return false;

	_outfile.open(_fullPath, std::ofstream::out | std::ofstream::binary);
	DBG_INFO(80, wst("file:%s %s\n"), _fullPath.c_str(), _outfile.good() ? L"created for write" : L"failed to create");
	if (!_outfile.good())
		return false;

	FWRITE(_fakeConfig);
	_storagePos = _initialPos;
	FWRITE(_storagePos);

	return _outfile.good();
}

void TxFileStorage::clear()
{
	if (empty() && osal_path_existsA(_fullPath.c_str()) == 0)
		return;

	_storage.clear();
	_storagePos = 0UL;
	_dirty = false;

	if (_infile.is_open())
		_infile.close();
	if (_outfile.is_open())
		_outfile.close();

	_outfile.open(_fullPath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
	FWRITE(_fakeConfig);
	_storagePos = _initialPos;
	FWRITE(_storagePos);
	_outfile.close();
}

bool TxFileStorage::writeData(uint32 dataSize, const GHQTexInfo & info)
{
	if (info.data == nullptr || dataSize == 0)
		return false;
	FWRITE(info.width);
	FWRITE(info.height);
	FWRITE(info.format);
	FWRITE(info.texture_format);
	FWRITE(info.pixel_type);
	FWRITE(info.is_hires_tex);
	FWRITE(dataSize);
	_outfile.write((char*)info.data, dataSize);
	return _outfile.good();
}

bool TxFileStorage::readData(GHQTexInfo & info)
{
	FREAD(info.width);
	FREAD(info.height);
	FREAD(info.format);
	FREAD(info.texture_format);
	FREAD(info.pixel_type);
	FREAD(info.is_hires_tex);

	uint32 dataSize = 0U;
	FREAD(dataSize);
	if (dataSize == 0)
		return false;

	if (_gzdest0 == nullptr)
		return false;

	_infile.read((char*)_gzdest0, dataSize);
	if (!_infile.good())
		return false;

	/* zlib decompress it */
	if (info.format & GL_TEXFMT_GZ) {
		uLongf destLen = _gzdestLen;
		if (uncompress(_gzdest1, &destLen, _gzdest0, dataSize) != Z_OK) {
			DBG_INFO(80, wst("Error: zlib decompression failed!\n"));
			return false;
		}
		info.data = _gzdest1;
		info.format &= ~GL_TEXFMT_GZ;
		DBG_INFO(80, wst("zlib decompressed: %.02gkb->%.02gkb\n"), dataSize / 1024.0, destLen / 1024.0);
	} else {
		info.data = _gzdest0;
	}

	return true;
}

bool TxFileStorage::add(Checksum checksum, GHQTexInfo *info, int dataSize)
{
	/* NOTE: dataSize must be provided if info->data is zlib compressed. */

	if (!checksum || !info->data || _storage.find(checksum) != _storage.end())
		return false;

	if (_infile.is_open() || !_outfile.is_open())
		if (!open(false))
			return false;

	if (!_dirty) {
		// Make position of storage data invalid.
		// It will prevent attempts to load unsaved storage file.
		_outfile.seekp(sizeof(_options), std::ofstream::beg);
		int64 pos = -1;
		FWRITE(pos);
	}

	uint8 *dest = info->data;
	uint32 format = info->format;

	if (dataSize == 0) {
		dataSize = TxUtil::sizeofTx(info->width, info->height, info->format);

		if (!dataSize)
			return false;

		if (_options & (GZ_TEXCACHE | GZ_HIRESTEXCACHE)) {
			/* zlib compress it. compression level:1 (best speed) */
			uLongf destLen = _gzdestLen;
			dest = (dest == _gzdest0) ? _gzdest1 : _gzdest0;
			if (compress2(dest, &destLen, info->data, dataSize, 1) != Z_OK) {
				dest = info->data;
				DBG_INFO(80, wst("Error: zlib compression failed!\n"));
			} else {
				DBG_INFO(80, wst("zlib compressed: %.02fkb->%.02fkb\n"), dataSize / 1024.0, destLen / 1024.0);
				dataSize = destLen;
				format |= GL_TEXFMT_GZ;
			}
		}
	}

	GHQTexInfo infoToWrite = *info;
	infoToWrite.data = dest;
	infoToWrite.format = format;

	_outfile.seekp(_storagePos, std::ofstream::beg);
	assert(_storagePos == _outfile.tellp());

	_storage.insert(StorageMap::value_type(checksum._checksum, _storagePos));
	if (!writeData(dataSize, infoToWrite))
		return false;
	_storagePos = _outfile.tellp();
	_dirty = true;

#ifdef DEBUG
	DBG_INFO(80, wst("[%5d] added!! crc:%08X %08X %d x %d gfmt:%x total:%.02fmb\n"),
		_storage.size(), checksum._hi, checksum._low,
		info->width, info->height, info->format & 0xffff, (double)(_totalSize / 1024) / 1024.0);
#endif

	/* total storage size */
	_totalSize += dataSize;

	return true;
}

std::unordered_map<uint64_t, int64_t> ChecksumPatch = {
	//side 1
	{949418576, 0x5FF7ECEC},//5 !
	{10882665135961641694, 0x2F4EDB9D},//IX !
	{5414539152399578714, 0xB80E023F},//8 !!
	{10882665134782356329, 0x4AE635CE},//3 !!
	{2515104943, 0x0CCCAA5B},//!! X
	{11186014692243632140, 0x35B9F368},//!! 6

	//side 2
	{ 10882665136203779059, 0x9015ED9E },//P22 5 !
	{ 5414539149093704324, 0xD6AB076A },//P21 8 !
	{ 11186014690300810000, 0x1625825E },//P32 6 !
	{ 5414539151466847353, 0x05D90B0B },//P31 2 !
	{ 3404850891, 0x080B6D4B },//P41 3 !
	{ 1833555532, 0x1A332B7C },//P42 1 !

	//side 3
	{ 943979577, 0x5E4DF38B },//P21 2 !
	{ 3533519242, 0xD6ED2362 },//P22 6 !
	{ 2234966330, 0xB07C6A7E },//P31 5 !
	{ 1503921592, 0xE05C90EE },//P32 7 !
	{ 2591730432, 0xF9776A92 },//P41 8 !
	{ 2964873944, 0x2E24C44F },//P42 1 !

	//side 4
	{ 5414539151491076179, 0x390CED3F },//P21 5 !
	{ 5414539151285576078, 0x7A7C2364 },//P31 3 !
	{ 11186014689284962803, 0x1D829500 },//P32 1 !
	{3919890579, 0xBF8DACB3},//P41 7 !
	{10882665136854288861, 0xE48CE0F3},//P42 8 !
	{ 1423883039, 0x3B14582D },//P22 2 !

	//top
	{5502028069262581124, 0xC933E141},//P23 12 !
	{5502028070078972781, 0xE1E006F3},//P24 14 !
	{14664577533604172876, 0xC996EF57},//P33 13 !
	{5502028069585931648, 0x26EBC37E},//P43 3 !
	{5502028067428979633, 0xF2B7A163},//PP34 16 !
	{5414539148579585546, 0x021684BB},//P44 7 !
	{ 5502028070045778849, 0x8E0981EC },//P32 !
	{ 5502028066273592584, 0xEA2D877D },//P22 !
	{ 5502028069937715905, 0xA75435AE },//P21 !
	{ 5502028070085518301, 0x01562A8F },//P31 !
	{ 5502028066402158781, 0xA5659FDF },//P41 !
	{ 5502028067270540643, 0x352F61EF },//P42 !
	

	{3414363026, 0},//P33 (maps to multiple?)
	{3622161792, 0},//P43 (maps to multiple?)


	{ 10882665136854288861, 0 },//None
	{5414539151285576078, 0},//None
	{11186014689284962803, 0},//None
	
	{ 5414539150103370152, 0 },
	{10882665138023449373, 0},
	{5414539149301971536, 0},
	{5414539150867657903, 0},

	{1183159006, 0},
	{4047025754, 0},
	{3873641, 0},
	{3078320140, 0},
	{5414539149879856177, 0},
	{10882665136793763036, 0},

	{5502028067605713056, 0},
	{5414539152272443539, 0},	
	{5502028069311277329, 0},

	
	{5502028067464568065, 0},
	{11186014692698831242, 0},
	{10882665136282404280, 0},
	{10882665137743356632, 0},


	{10882665135986160333, 0},
	{ 5414539149997265656, 0 },
	{ 5414539149296532537, 0 },
	{ 5414539150587519290, 0 },
	{ 5414539150944283392, 0 },
	{ 10882665137380901185, 0 },
	{ 10882665136202365727, 0 },

	{ 5414539151193485679, 0},//N
	{ 10882665136612038220, 0 },//
	{ 5414539151757403851, 0 },//	
	{ 5502028067833184045, 0 },//
	

	{ 1260670635, 0 },//N
	{ 1281040736, 0 },//N

	{1425296371, 0 },
	{227032586, 0 },
	{2533817928, 0 },//Maps to multiple

	{262575180, 0 },
	{1465209777, 0 },
	{ 4115202925, 0 },
	{ 2075806173, 0 },
	{ 3298811268, 0 },
	{ 2933023118, 0 },
	{ 119650803, 0 },
	{ 2604447000, 0 },	
};

int active = 1;

std::vector<uint64_t> unaccounted;

#include <inttypes.h>


bool TxFileStorage::get(Checksum checksum, GHQTexInfo* info)
{
	if (!checksum || _storage.empty())
		return false;

	//if (checksum != Checksum(214739547))
		//return get(Checksum(214739547), info);

	/* find a match in storage */
	auto itMap = _storage.find(checksum);
	if (itMap == _storage.end())
	{
		if (active)
		{
			auto patch = ChecksumPatch.find(checksum);

			if (patch != ChecksumPatch.end())
			{
				if (patch->second == 0)
					return false;

				bool ret = get(patch->second, info);
				if (!ret)
					int t = 54;
				return ret;
			}


			bool found = false;
			for (auto& key : unaccounted)
			{
				if (key == checksum._checksum)
					found = true;
			}
			if (!found)
			{
				unaccounted.push_back(checksum._checksum);
				FILE* file;
				file = fopen("unaccounted.txt", "w");

				if (file)
				{
					for (auto& key : unaccounted)
						fprintf(file, "%" PRIu64 "\n", key);

					fclose(file);
				}
			}
		}
		
		return false;
	}

	if (_outfile.is_open() || !_infile.is_open())
		if (!open(true))
			return false;

	_infile.seekg(itMap->second, std::ifstream::beg);
	return readData(*info);
}

bool TxFileStorage::save(const wchar_t *path, const wchar_t *filename, int config)
{
	assert(_cachePath == path);
	if (_filename.empty()) {
		_filename = filename;
		buildFullPath();
	} else
		assert(_filename == filename);

	if (!_dirty)
		return true;

	if (_storage.empty() || _storagePos == 0UL)
		return false;

	if (_infile.is_open() || !_outfile.is_open())
		if (!open(false))
			return false;

	_outfile.seekp(0L, std::ofstream::beg);

	FWRITE(config);
	FWRITE(_storagePos);
	_outfile.seekp(_storagePos, std::ofstream::beg);
	int storageSize = static_cast<int>(_storage.size());
	FWRITE(storageSize);
	if (_callback)
		(*_callback)(wst("Saving texture storage...\n"));
	for (auto& item : _storage) {
		FWRITE(item.first);
		FWRITE(item.second);
	}
	_outfile.close();
	if (_callback)
		(*_callback)(wst("Done\n"));

	return true;
}

bool TxFileStorage::load(const wchar_t *path, const wchar_t *filename, int config, bool force)
{
	assert(_cachePath == path);
	if (_filename.empty()) {
		_filename = filename;
		buildFullPath();
	} else
		assert(_filename == filename);

	if (_outfile.is_open() || !_infile.is_open())
		if (!open(true))
			return false;

	int tmpconfig = 0;
	/* read header to determine config match */
	_infile.seekg(0L, std::ifstream::beg);
	FREAD(tmpconfig);
	FREAD(_storagePos);
	if (tmpconfig == _fakeConfig) {
		if (_storagePos != _initialPos)
			return false;
	} else if (tmpconfig != config && !force)
		return false;

	if (_storagePos <= sizeof(config) + sizeof(_storagePos))
		return false;
	_infile.seekg(_storagePos, std::ifstream::beg);

	int storageSize = 0;
	FREAD(storageSize);
	if (storageSize <= 0)
		return false;

	if (_callback)
		(*_callback)(wst("Loading texture storage...\n"));
	uint64 key;
	int64 value;
	for (int i = 0; i < storageSize; ++i) {
		FREAD(key);
		FREAD(value);
		_storage.insert(StorageMap::value_type(key, value));
	}
	if (_callback)
		(*_callback)(wst("Done\n"));

	_dirty = false;
	return !_storage.empty();
}

bool TxFileStorage::isCached(Checksum checksum)
{
	return _storage.find(checksum) != _storage.end();
}

/************************** TxCache *************************************/

TxCache::~TxCache()
{
}

TxCache::TxCache(uint32 options,
	uint64 cachesize,
	const wchar_t *cachePath,
	const wchar_t *ident,
	dispInfoFuncExt callback)
	: _callback(callback)
{
	/* save path name */
	if (cachePath)
		_cachePath.assign(cachePath);

	/* save ROM name */
	if (ident)
		_ident.assign(ident);

	if ((options & FILE_CACHE_MASK) == 0)
		_pImpl.reset(new TxMemoryCache(options, cachesize, _callback));
	else
		_pImpl.reset(new TxFileStorage(options, cachePath, _callback));
}

bool TxCache::add(Checksum checksum, GHQTexInfo *info, int dataSize)
{
	return _pImpl->add(checksum, info, dataSize);
}

bool TxCache::get(Checksum checksum, GHQTexInfo *info)
{
	return _pImpl->get(checksum, info);
}

uint64 TxCache::size() const
{
	return _pImpl->size();
}

uint64 TxCache::totalSize() const
{
	return _pImpl->totalSize();
}

uint64 TxCache::cacheLimit() const
{
	return _pImpl->cacheLimit();
}

bool TxCache::save()
{
	return _pImpl->save(_cachePath.c_str(), _getFileName().c_str(), _getConfig());
}

bool TxCache::load(bool force)
{
	return _pImpl->load(_cachePath.c_str(), _getFileName().c_str(), _getConfig(), force);
}

bool TxCache::del(Checksum checksum)
{
	return _pImpl->del(checksum);
}

bool TxCache::isCached(Checksum checksum)
{
	return _pImpl->isCached(checksum);
}

void TxCache::clear()
{
	_pImpl->clear();
}

bool TxCache::empty() const
{
	return _pImpl->empty();
}

uint32 TxCache::getOptions() const
{
	return _pImpl->getOptions();
}

void TxCache::setOptions(uint32 options)
{
	_pImpl->setOptions(options);
}
