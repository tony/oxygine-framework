#include <string.h>
#include <map>
#include "../minizip/ioapi_mem.h"
#include "core/ZipFileSystem.h"
#include "core/Object.h"
#include "utils/stringUtils.h"
//#include "utils/utils.h"

#ifdef __S3E__
#include "s3eFile.h"
#endif

using namespace oxygine;
using namespace std;
namespace oxygine
{
namespace file
{
	bool sortFiles (const file_entry &ob1, const file_entry &ob2)
	{
		return strcmp_cns(ob1.name, ob2.name) < 0;
	}

	bool readEntry(const file_entry *entry, file::buffer &bf)
	{
		bf.data.resize(0);
		int r = unzGoToFilePos(entry->zp, const_cast<unz_file_pos*>(&entry->pos));
		OX_ASSERT(r == UNZ_OK);

		unz_file_info file_info;
		r = unzGetCurrentFileInfo(entry->zp, &file_info, 0, 0, 0, 0, 0, 0);
		OX_ASSERT(r == UNZ_OK);

		unzOpenCurrentFile(entry->zp);		

		bf.data.resize(file_info.uncompressed_size);
		r = unzReadCurrentFile(entry->zp, &bf.data.front(), bf.data.size());
		OX_ASSERT(r == file_info.uncompressed_size);

		unzCloseCurrentFile(entry->zp);
		return true;
	}

	Zips::Zips():_sort(false)
	{

	}

	Zips::~Zips()
	{
		reset();
	}

	void Zips::add(unzFile zp)
	{
		_zps.push_back(zp);

		do 
		{
			unz_file_pos pos;
			unzGetFilePos(zp, &pos);

			file_entry entry;				
			unzGetCurrentFileInfo(zp, 0, entry.name, sizeof(entry.name) - 1, 0, 0, 0, 0);
			entry.pos = pos;
			entry.zp = zp;

			strcpy(entry.name, entry.name);

			_files.push_back(entry);

		} while (unzGoToNextFile(zp) != UNZ_END_OF_LIST_OF_FILE);

		_sort = true;
	}

	void Zips::add(const unsigned char* data, unsigned int size)
	{
		zlib_filefunc_def ff;
		fill_memory_filefunc(&ff);
		char str[255];
		safe_sprintf(str, "%x+%x", data, size);
		unzFile zp = unzOpen2(str, &ff);
		OX_ASSERT(zp);
		if (!zp)
			return;

		add(zp);
	}

	void Zips::add(const char *name)
	{
		unzFile zp = unzOpen(name);
		OX_ASSERT(zp);
		if (!zp)
			return;

		add(zp);
	}

	void Zips::update()
	{
		sort(_files.begin(), _files.end(), sortFiles);
		log::messageln("ZipFS, total files: %d", _files.size());
	}

	bool Zips::read(const char *name, file::buffer &bf)
	{
		const file_entry *entry = getEntry(name);
		if (!entry)
			return false;
		return readEntry(entry, bf);
	}

	void Zips::reset()
	{
		for (zips::iterator i = _zps.begin(); i != _zps.end(); ++i)
		{
			unzFile f = *i;
			unzClose(f);
		}

		_zps.resize(0);
		_files.resize(0);
	}


	bool findEntry (const file_entry &g, const char *name)
	{
		return strcmp_cns(g.name, name) < 0;
	}

	const file_entry *Zips::getEntry(const char *name)
	{
		if (_sort)
		{
			update();
			_sort = false;
		}

		files::const_iterator it = lower_bound(_files.begin(), _files.end(), name, findEntry);
		if (it != _files.end())
		{
			const file_entry &g = *it;
			if (!strcmp_cns(g.name, name))
				return &g;
		}

		return 0;
	}
	
	class fileHandleZip: public fileHandle
	{
	public:

		fileHandleZip(const file_entry *entry):_entry(entry)
		{
			int r = 0;
			r = unzGoToFilePos(entry->zp, const_cast<unz_file_pos*>(&entry->pos));
			OX_ASSERT(r == UNZ_OK);
			r = unzOpenCurrentFile(entry->zp);
			OX_ASSERT(r == UNZ_OK);
		}

		void release()
		{
			int r = unzCloseCurrentFile(_entry->zp);
			OX_ASSERT(r == UNZ_OK);
			delete this;
		}

		unsigned int read(void *dest, unsigned int size)
		{
			return unzReadCurrentFile(_entry->zp, dest, size);
		}

		unsigned int write(const void *src, unsigned int size)
		{
			return 0;
		}

		unsigned int getSize() const
		{
			unz_file_info file_info;
			int res = unzGetCurrentFileInfo(_entry->zp, &file_info, 0, 0, 0, 0, 0, 0);
			return file_info.uncompressed_size;
		}

		const file_entry *_entry;
		
	};


	void ZipFileSystem::add(const char *zip)
	{
		_zips.add(zip);
	}

	void ZipFileSystem::add(const unsigned char* data, unsigned int size)
	{
		_zips.add(data, size);
	}

	void ZipFileSystem::reset()
	{
		_zips.reset();
	}

	FileSystem::status ZipFileSystem::_open(const char *file, const char *mode, error_policy ep, file::fileHandle *&fh)
	{
		const file_entry *entry = _zips.getEntry(file);
		if (entry)
		{
			fh = new fileHandleZip(entry);
			return status_ok;
		}

		handleErrorPolicy(ep, "can't find zip file entry: %s", file);
		return status_error;
	}

	FileSystem::status ZipFileSystem::_deleteFile(const char* file)
	{
		return status_error;
	}

	FileSystem::status ZipFileSystem::_renameFile(const char* src, const char *dest)
	{
		return status_error;
	}
}
}