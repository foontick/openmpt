/*
 * mptStringBuffer.h
 * -----------------
 * Purpose: Various functions for "fixing" char array strings for writing to or
 *          reading from module files, or for securing char arrays in general.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "mptString.h"

#include <algorithm>
#include <string>
#include <vector>



OPENMPT_NAMESPACE_BEGIN



namespace mpt{



template <typename Tstring, typename Tchar>
class StringBufRefImpl
{
private:
	Tchar * buf;
	std::size_t size;
public:
	explicit StringBufRefImpl(Tchar * buf, std::size_t size)
		: buf(buf)
		, size(size)
	{
		MPT_STATIC_ASSERT(sizeof(Tchar) == sizeof(typename Tstring::value_type));
		MPT_ASSERT(size > 0);
	}
	StringBufRefImpl(const StringBufRefImpl &) = delete;
	StringBufRefImpl(StringBufRefImpl &&) = default;
	StringBufRefImpl & operator = (const StringBufRefImpl &) = delete;
	StringBufRefImpl & operator = (StringBufRefImpl &&) = delete;
	operator Tstring () const
	{
		std::size_t len = std::find(buf, buf + size, Tchar('\0')) - buf; // terminate at \0
		return Tstring(buf, buf + len);
	}
	StringBufRefImpl & operator = (const Tstring & str)
	{
		std::fill(buf, buf + size, Tchar('\0'));
		std::copy(str.data(), str.data() + std::min(str.length(), size - 1), buf);
		buf[size - 1] = Tchar('\0');
		return *this;
	}
};

template <typename Tstring, typename Tchar>
class StringBufRefImpl<Tstring, const Tchar>
{
private:
	const Tchar * buf;
	std::size_t size;
public:
	explicit StringBufRefImpl(const Tchar * buf, std::size_t size)
		: buf(buf)
		, size(size)
	{
		MPT_STATIC_ASSERT(sizeof(Tchar) == sizeof(typename Tstring::value_type));
		MPT_ASSERT(size > 0);
	}
	StringBufRefImpl(const StringBufRefImpl &) = delete;
	StringBufRefImpl(StringBufRefImpl &&) = default;
	StringBufRefImpl & operator = (const StringBufRefImpl &) = delete;
	StringBufRefImpl & operator = (StringBufRefImpl &&) = delete;
	operator Tstring () const
	{
		std::size_t len = std::find(buf, buf + size, Tchar('\0')) - buf; // terminate at \0
		return Tstring(buf, buf + len);
	}
};

template <typename Tstring, typename Tchar, std::size_t size>
inline StringBufRefImpl<Tstring, Tchar> StringBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<Tstring, Tchar>(buf, size);
}

template <typename Tchar, std::size_t size>
inline StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, Tchar> AutoStringBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<typename std::basic_string<typename std::remove_const<Tchar>::type>, Tchar>(buf, size);
}


template <typename Tchar>
class CharsetStringBufRefImpl
{
private:
	StringBufRefImpl<std::string, Tchar> strbuf;
	mpt::Charset charset;
public:
	explicit CharsetStringBufRefImpl(Tchar * buf, std::size_t size, mpt::Charset charset)
		: strbuf(buf, size)
		, charset(charset)
	{
		MPT_STATIC_ASSERT(sizeof(Tchar) == 1);
		MPT_ASSERT(size > 0);
	}
	CharsetStringBufRefImpl(const CharsetStringBufRefImpl &) = delete;
	CharsetStringBufRefImpl(CharsetStringBufRefImpl &&) = default;
	CharsetStringBufRefImpl & operator = (const CharsetStringBufRefImpl &) = delete;
	CharsetStringBufRefImpl & operator = (CharsetStringBufRefImpl &&) = delete;
	operator mpt::ustring () const
	{
		return mpt::ToUnicode(charset, strbuf);
	}
	CharsetStringBufRefImpl & operator = (const mpt::ustring & ustr)
	{
		strbuf = mpt::ToCharset(charset, ustr);
		return *this;
	}
};

template <typename Tchar>
class CharsetStringBufRefImpl<const Tchar>
{
private:
	StringBufRefImpl<std::string, const Tchar> strbuf;
	mpt::Charset charset;
public:
	explicit CharsetStringBufRefImpl(const Tchar * buf, std::size_t size)
		: strbuf(buf, size)
		, charset(charset)
	{
		MPT_STATIC_ASSERT(sizeof(Tchar) == 1);
		MPT_ASSERT(size > 0);
	}
	CharsetStringBufRefImpl(const CharsetStringBufRefImpl &) = delete;
	CharsetStringBufRefImpl(CharsetStringBufRefImpl &&) = default;
	CharsetStringBufRefImpl & operator = (const CharsetStringBufRefImpl &) = delete;
	CharsetStringBufRefImpl & operator = (CharsetStringBufRefImpl &&) = delete;
	operator mpt::ustring () const
	{
		return mpt::ToUnicode(charset, strbuf);
	}
};

template <typename Tchar, std::size_t size>
inline CharsetStringBufRefImpl<Tchar> StringBuf(mpt::Charset charset, Tchar (&buf)[size])
{
	return CharsetStringBufRefImpl<Tchar>(buf, size, charset);
}


#ifdef MODPLUG_TRACKER

#if MPT_OS_WINDOWS

template <typename Tchar, std::size_t size>
inline StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, Tchar> WinStringBuf(Tchar (&buf)[size])
{
	return StringBufRefImpl<typename mpt::windows_char_traits<typename std::remove_const<Tchar>::type>::string_type, Tchar>(buf, size);
}

#if defined(_MFC_VER)

template <typename Tchar>
class CStringBufRefImpl
{
private:
	Tchar * buf;
	std::size_t size;
public:
	explicit CStringBufRefImpl(Tchar * buf, std::size_t size)
		: buf(buf)
		, size(size)
	{
		MPT_ASSERT(size > 0);
	}
	CStringBufRefImpl(const CStringBufRefImpl &) = delete;
	CStringBufRefImpl(CStringBufRefImpl &&) = default;
	CStringBufRefImpl & operator = (const CStringBufRefImpl &) = delete;
	CStringBufRefImpl & operator = (CStringBufRefImpl &&) = delete;
	operator CString () const
	{
		std::size_t len = std::find(buf, buf + size, Tchar('\0')) - buf; // terminate at \0
		return CString(buf, len);
	}
	CStringBufRefImpl & operator = (const CString & str)
	{
		std::fill(buf, buf + size, Tchar('\0'));
		std::copy(str.GetString(), str.GetString() + std::min(static_cast<std::size_t>(str.GetLength()), size - 1), buf);
		buf[size - 1] = Tchar('\0');
		return *this;
	}
};

template <typename Tchar>
class CStringBufRefImpl<const Tchar>
{
private:
	const Tchar * buf;
	std::size_t size;
public:
	explicit CStringBufRefImpl(const Tchar * buf, std::size_t size)
		: buf(buf)
		, size(size)
	{
		MPT_ASSERT(size > 0);
	}
	CStringBufRefImpl(const CStringBufRefImpl &) = delete;
	CStringBufRefImpl(CStringBufRefImpl &&) = default;
	CStringBufRefImpl & operator = (const CStringBufRefImpl &) = delete;
	CStringBufRefImpl & operator = (CStringBufRefImpl &&) = delete;
	operator CString () const
	{
		std::size_t len = std::find(buf, buf + size, Tchar('\0')) - buf; // terminate at \0
		return CString(buf, buf + len);
	}
};

template <typename Tchar, std::size_t size>
inline CStringBufRefImpl<Tchar> CStringBuf(Tchar (&buf)[size])
{
	return CStringBufRefImpl<Tchar>(buf, size);
}

#endif // _MFC_VER

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER



} // nameswpace mpt





namespace mpt { namespace String
{


#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
#endif // MPT_COMPILER_MSVC


	// Sets last character to null in given char array.
	// Size of the array must be known at compile time.
	template <size_t size>
	void SetNullTerminator(char (&buffer)[size])
	{
		STATIC_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}

	inline void SetNullTerminator(char *buffer, size_t size)
	{
		MPT_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}

	template <size_t size>
	void SetNullTerminator(wchar_t (&buffer)[size])
	{
		STATIC_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}

	inline void SetNullTerminator(wchar_t *buffer, size_t size)
	{
		MPT_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}


	// Remove any chars after the first null char
	template <size_t size>
	void FixNullString(char (&buffer)[size])
	{
		STATIC_ASSERT(size > 0);
		SetNullTerminator(buffer);
		size_t pos = 0;
		// Find the first null char.
		while(pos < size && buffer[pos] != '\0')
		{
			pos++;
		}
		// Remove everything after the null char.
		while(pos < size)
		{
			buffer[pos++] = '\0';
		}
	}

	inline void FixNullString(std::string & str)
	{
		for(std::size_t i = 0; i < str.length(); ++i)
		{
			if(str[i] == '\0')
			{
				// if we copied \0 in the middle of the buffer, terminate std::string here
				str.resize(i);
				break;
			}
		}
	}


	enum ReadWriteMode
	{
		// Reading / Writing: Standard null-terminated string handling.
		nullTerminated,
		// Reading: Source string is not guaranteed to be null-terminated (if it fills the whole char array).
		// Writing: Destination string is not guaranteed to be null-terminated (if it fills the whole char array).
		maybeNullTerminated,
		// Reading: String may contain null characters anywhere. They should be treated as spaces.
		// Writing: A space-padded string is written.
		spacePadded,
		// Reading: String may contain null characters anywhere. The last character is ignored (it is supposed to be 0).
		// Writing: A space-padded string with a trailing null is written.
		spacePaddedNull
	};


	// Copy a string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, typename Tbyte>
	void Read(std::string &dest, const Tbyte *srcBuffer, size_t srcSize)
	{

		const char *src = mpt::byte_cast<const char*>(srcBuffer);

		dest.clear();

		if(mode == nullTerminated || mode == spacePaddedNull)
		{
			// We assume that the last character of the source buffer is null.
			if(srcSize > 0)
			{
				srcSize -= 1;
			}
		}

		if(mode == nullTerminated || mode == maybeNullTerminated)
		{

			// Copy null-terminated string, stopping at null.
			try
			{
				dest.assign(src, std::find(src, src + srcSize, '\0'));
			} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
			{
				MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
			}

		} else if(mode == spacePadded || mode == spacePaddedNull)
		{

			try
			{
				// Copy string over.
				dest.assign(src, src + srcSize);

				// Convert null characters to spaces.
				std::transform(dest.begin(), dest.end(), dest.begin(), [] (char c) -> char { return (c != '\0') ? c : ' '; });

				// Trim trailing spaces.
				dest = mpt::String::RTrim(dest, std::string(" "));
				
			} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
			{
				MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
			}

		}
	}

	// Copy a charset encoded string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, typename Tbyte>
	void Read(mpt::ustring &dest, mpt::Charset charset, const Tbyte *srcBuffer, size_t srcSize)
	{
		std::string tmp;
		Read<mode>(tmp, srcBuffer, srcSize);
		dest = mpt::ToUnicode(charset, tmp);
	}

	// Used for reading strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t srcSize, typename Tbyte>
	void Read(std::string &dest, const Tbyte (&srcBuffer)[srcSize])
	{
		STATIC_ASSERT(srcSize > 0);
		Read<mode>(dest, srcBuffer, srcSize);
	}

	// Used for reading charset encoded strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t srcSize, typename Tbyte>
	void Read(mpt::ustring &dest, mpt::Charset charset, const Tbyte(&srcBuffer)[srcSize])
	{
		std::string tmp;
		Read<mode>(tmp, srcBuffer);
		dest = mpt::ToUnicode(charset, tmp);
	}

	// Copy a string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, size_t destSize, typename Tbyte>
	void Read(char (&destBuffer)[destSize], const Tbyte *srcBuffer, size_t srcSize)
	{
		STATIC_ASSERT(destSize > 0);

		char *dst = destBuffer;
		const char *src = mpt::byte_cast<const char*>(srcBuffer);

		if(mode == nullTerminated || mode == spacePaddedNull)
		{
			// We assume that the last character of the source buffer is null.
			if(srcSize > 0)
			{
				srcSize -= 1;
			}
		}

		if(mode == nullTerminated || mode == maybeNullTerminated)
		{

			// Copy string and leave one character space in the destination buffer for null.
			dst = std::copy(src, std::find(src, src + std::min(srcSize, destSize - 1), '\0'), dst);
	
		} else if(mode == spacePadded || mode == spacePaddedNull)
		{

			// Copy string and leave one character space in the destination buffer for null.
			// Convert nulls to spaces while copying.
			dst = std::replace_copy(src, src + std::min(srcSize, destSize - 1), dst, '\0', ' ');

			// Rewind dst to the first of any trailing spaces.
			while(dst - destBuffer > 0)
			{
				dst--;
				char c = *dst;
				if(c != ' ')
				{
					dst++;
					break;
				}
			}

		}

		// Fill rest of string with nulls.
		std::fill(dst, destBuffer + destSize, '\0');

	}

	// Used for reading strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t destSize, size_t srcSize, typename Tbyte>
	void Read(char (&destBuffer)[destSize], const Tbyte (&srcBuffer)[srcSize])
	{
		STATIC_ASSERT(destSize > 0);
		STATIC_ASSERT(srcSize > 0);
		Read<mode, destSize>(destBuffer, srcBuffer, srcSize);
	}


	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// You should only use this function if src and dest are dynamically sized,
	// otherwise use one of the safer overloads below.
	template <ReadWriteMode mode>
	void Write(char *destBuffer, const size_t destSize, const char *srcBuffer, const size_t srcSize)
	{
		MPT_ASSERT(destSize > 0);

		const size_t maxSize = std::min(destSize, srcSize);
		char *dst = destBuffer;
		const char *src = srcBuffer;

		// First, copy over null-terminated string.
		size_t pos = maxSize;
		while(pos > 0)
		{
			if((*dst = *src) == '\0')
			{
				break;
			}
			pos--;
			dst++;
			src++;
		}

		if(mode == nullTerminated || mode == maybeNullTerminated)
		{
			// Fill rest of string with nulls.
			std::fill(dst, dst + destSize - maxSize + pos, '\0');
		} else if(mode == spacePadded || mode == spacePaddedNull)
		{
			// Fill the rest of the destination string with spaces.
			std::fill(dst, dst + destSize - maxSize + pos, ' ');
		}

		if(mode == nullTerminated || mode == spacePaddedNull)
		{
			// Make sure that destination is really null-terminated.
			SetNullTerminator(destBuffer, destSize);
		}
	}

	// Copy a string from srcBuffer to a dynamically sized std::vector destBuffer using a given write mode.
	// Used for writing strings to files.
	// Only use this version of the function if the size of the source buffer is variable and the destination buffer also has variable size.
	template <ReadWriteMode mode>
	void Write(std::vector<char> &destBuffer, const char *srcBuffer, const size_t srcSize)
	{
		MPT_ASSERT(destBuffer.size() > 0);
		Write<mode>(destBuffer.data(), destBuffer.size(), srcBuffer, srcSize);
	}

	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// Used for writing strings to files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, size_t destSize>
	void Write(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize)
	{
		STATIC_ASSERT(destSize > 0);
		Write<mode>(destBuffer, destSize, srcBuffer, srcSize);
	}

	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// Used for writing strings to files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t destSize, size_t srcSize>
	void Write(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	{
		STATIC_ASSERT(destSize > 0);
		STATIC_ASSERT(srcSize > 0);
		Write<mode, destSize>(destBuffer, srcBuffer, srcSize);
	}

	template <ReadWriteMode mode>
	void Write(char *destBuffer, const size_t destSize, const std::string &src)
	{
		MPT_ASSERT(destSize > 0);
		Write<mode>(destBuffer, destSize, src.c_str(), src.length());
	}

	template <ReadWriteMode mode>
	void Write(std::vector<char> &destBuffer, const std::string &src)
	{
		MPT_ASSERT(destBuffer.size() > 0);
		Write<mode>(destBuffer, src.c_str(), src.length());
	}

	template <ReadWriteMode mode, size_t destSize>
	void Write(char (&destBuffer)[destSize], const std::string &src)
	{
		STATIC_ASSERT(destSize > 0);
		Write<mode, destSize>(destBuffer, src.c_str(), src.length());
	}


	// Copy from a char array to a fixed size char array.
	template <size_t destSize>
	void CopyN(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize = std::numeric_limits<size_t>::max())
	{
		const size_t copySize = std::min(destSize - 1u, srcSize);
		std::strncpy(destBuffer, srcBuffer, copySize);
		destBuffer[copySize] = '\0';
	}

	// Copy at most srcSize characters from srcBuffer to a std::string.
	static inline void CopyN(std::string &dest, const char *srcBuffer, const size_t srcSize = std::numeric_limits<size_t>::max())
	{
		dest.assign(srcBuffer, srcBuffer + mpt::strnlen(srcBuffer, srcSize));
	}


	// Copy from one fixed size char array to another one.
	template <size_t destSize, size_t srcSize>
	void Copy(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	{
		CopyN(destBuffer, srcBuffer, srcSize);
	}

	// Copy from a std::string to a fixed size char array.
	template <size_t destSize>
	void Copy(char (&destBuffer)[destSize], const std::string &src)
	{
		CopyN(destBuffer, src.c_str(), src.length());
	}

	// Copy from a fixed size char array to a std::string.
	template <size_t srcSize>
	void Copy(std::string &dest, const char (&srcBuffer)[srcSize])
	{
		CopyN(dest, srcBuffer, srcSize);
	}

	// Copy from a std::string to a std::string.
	static inline void Copy(std::string &dest, const std::string &src)
	{
		dest.assign(src);
	}


#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC


} } // namespace mpt::String



OPENMPT_NAMESPACE_END