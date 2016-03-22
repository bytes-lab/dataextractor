#ifndef _unzip_H
#define _unzip_H

// UNZIPPING functions -- for unzipping.
// This file is a repackaged form of extracts from the zlib code available
// at www.gzip.org/zlib, by Jean-Loup Gailly and Mark Adler. The original
// copyright notice may be found in unzip.cpp. The repackaging was done
// by Lucian Wischik to simplify and extend its use in Windows/C++. Also
// encryption and unicode filenames have been added.


#ifndef _zip_H
DECLARE_HANDLE(HZIP);
#endif
// An HZIP identifies a zip file that has been opened

typedef DWORD ZRESULT;
// return codes from any of the zip functions. Listed later.

typedef struct
{ int index;                 // index of this file within the zip
  TCHAR name[MAX_PATH];      // filename within the zip
  DWORD attr;                // attributes, as in GetFileAttributes.
  FILETIME atime,ctime,mtime;// access, create, modify filetimes
  long comp_size;            // sizes of item, compressed and uncompressed. These
  long unc_size;             // may be -1 if not yet known (e.g. being streamed in)
} ZIPENTRY;


HZIP OpenZip(const TCHAR *fn, const char *password);
HZIP OpenZip(void *z,unsigned int len, const char *password);
HZIP OpenZipHandle(HANDLE h, const char *password);
// OpenZip - opens a zip file and returns a handle with which you can
// subsequently examine its contents. You can open a zip file from:
// from a pipe:             OpenZipHandle(hpipe_read,0);
// from a file (by handle): OpenZipHandle(hfile,0);
// from a file (by name):   OpenZip("c:\\test.zip","password");
// from a memory block:     OpenZip(bufstart, buflen,0);
// If the file is opened through a pipe, then items may only be
// accessed in increasing order, and an item may only be unzipped once,
// although GetZipItem can be called immediately before and after unzipping
// it. If it's opened in any other way, then full random access is possible.
// Note: pipe input is not yet implemented.
// Note: zip passwords are ascii, not unicode.
// Note: for windows-ce, you cannot close the handle until after CloseZip.
// but for real windows, the zip makes its own copy of your handle, so you
// can close yours anytime.

ZRESULT GetZipItem(HZIP hz, int index, ZIPENTRY *ze);
// GetZipItem - call this to get information about an item in the zip.
// If index is -1 and the file wasn't opened through a pipe,
// then it returns information about the whole zipfile
// (and in particular ze.index returns the number of index items).
// Note: the item might be a directory (ze.attr & FILE_ATTRIBUTE_DIRECTORY)
// See below for notes on what happens when you unzip such an item.
// Note: if you are opening the zip through a pipe, then random access
// is not possible and GetZipItem(-1) fails and you can't discover the number
// of items except by calling GetZipItem on each one of them in turn,
// starting at 0, until eventually the call fails. Also, in the event that
// you are opening through a pipe and the zip was itself created into a pipe,
// then then comp_size and sometimes unc_size as well may not be known until
// after the item has been unzipped.

ZRESULT FindZipItem(HZIP hz, const TCHAR *name, bool ic, int *index, ZIPENTRY *ze);
// FindZipItem - finds an item by name. ic means 'insensitive to case'.
// It returns the index of the item, and returns information about it.
// If nothing was found, then index is set to -1 and the function returns
// an error code.

ZRESULT UnzipItem(HZIP hz, int index, const TCHAR *fn);
ZRESULT UnzipItem(HZIP hz, int index, void *z,unsigned int len);
ZRESULT UnzipItemHandle(HZIP hz, int index, HANDLE h);
// UnzipItem - given an index to an item, unzips it. You can unzip to:
// to a pipe:             UnzipItemHandle(hz,i, hpipe_write);
// to a file (by handle): UnzipItemHandle(hz,i, hfile);
// to a file (by name):   UnzipItem(hz,i, ze.name);
// to a memory block:     UnzipItem(hz,i, buf,buflen);
// In the final case, if the buffer isn't large enough to hold it all,
// then the return code indicates that more is yet to come. If it was
// large enough, and you want to know precisely how big, GetZipItem.
// Note: zip files are normally stored with relative pathnames. If you
// unzip with ZIP_FILENAME a relative pathname then the item gets created
// relative to the current directory - it first ensures that all necessary
// subdirectories have been created. Also, the item may itself be a directory.
// If you unzip a directory with ZIP_FILENAME, then the directory gets created.
// If you unzip it to a handle or a memory block, then nothing gets created
// and it emits 0 bytes.
ZRESULT SetUnzipBaseDir(HZIP hz, const TCHAR *dir);
// if unzipping to a filename, and it's a relative filename, then it will be relative to here.
// (defaults to current-directory).


ZRESULT CloseZip(HZIP hz);
// CloseZip - the zip handle must be closed with this function.

#endif // _unzip_H
