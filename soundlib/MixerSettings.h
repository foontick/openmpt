/*
 * MixerSettings.h
 * ---------------
 * Purpose: A struct containing settings for the mixer of soundlib.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


enum SampleFormatEnum
{
	SampleFormatUnsigned8 =  8,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt16     = 16,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt24     = 24,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt32     = 32,       // do not change value (for compatibility with old configuration settings)
	SampleFormatFloat32   = 32 + 128, // Only supported as mixer output and ISoundDevice format, NOT supported by Mod2Wave settings dialog yet. Keep in mind to update all 3 cases at once.
	SampleFormatFixed5p27 =      255, // mixbuffer format
	SampleFormatInvalid   =  0
};

template<typename Tsample> struct SampleFormatTraits;
template<> struct SampleFormatTraits<uint8>     { static const SampleFormatEnum sampleFormat = SampleFormatUnsigned8; };
template<> struct SampleFormatTraits<int16>     { static const SampleFormatEnum sampleFormat = SampleFormatInt16;     };
template<> struct SampleFormatTraits<int24>     { static const SampleFormatEnum sampleFormat = SampleFormatInt24;     };
template<> struct SampleFormatTraits<int32>     { static const SampleFormatEnum sampleFormat = SampleFormatInt32;     };
template<> struct SampleFormatTraits<float>     { static const SampleFormatEnum sampleFormat = SampleFormatFloat32;   };
template<> struct SampleFormatTraits<fixed5p27> { static const SampleFormatEnum sampleFormat = SampleFormatFixed5p27; };

template<SampleFormatEnum sampleFormat> struct SampleFormatToType;
template<> struct SampleFormatToType<SampleFormatUnsigned8> { typedef uint8     type; };
template<> struct SampleFormatToType<SampleFormatInt16>     { typedef int16     type; };
template<> struct SampleFormatToType<SampleFormatInt24>     { typedef int24     type; };
template<> struct SampleFormatToType<SampleFormatInt32>     { typedef int32     type; };
template<> struct SampleFormatToType<SampleFormatFloat32>   { typedef float     type; };
template<> struct SampleFormatToType<SampleFormatFixed5p27> { typedef fixed5p27 type; };


struct SampleFormat
{
	SampleFormatEnum value;
	SampleFormat(SampleFormatEnum v = SampleFormatInvalid) : value(v) { }
	bool operator == (SampleFormat other) const { return value == other.value; }
	bool operator != (SampleFormat other) const { return value != other.value; }
	operator SampleFormatEnum () const
	{
		return value;
	}
	bool IsValid() const
	{
		return value != SampleFormatInvalid;
	}
	bool IsUnsigned() const
	{
		if(!IsValid()) return false;
		return value == SampleFormatUnsigned8;
	}
	bool IsFloat() const
	{
		if(!IsValid()) return false;
		return value == SampleFormatFloat32;
	}
	bool IsInt() const
	{
		if(!IsValid()) return false;
		return value != SampleFormatFloat32;
	}
	bool IsMixBuffer() const
	{
		if(!IsValid()) return false;
		return value == SampleFormatFixed5p27;
	}
	uint8 GetBitsPerSample() const
	{
		if(!IsValid()) return 0;
		switch(value)
		{
		case SampleFormatUnsigned8:
			return 8;
			break;
		case SampleFormatInt16:
			return 16;
			break;
		case SampleFormatInt24:
			return 24;
			break;
		case SampleFormatInt32:
			return 32;
			break;
		case SampleFormatFloat32:
			return 32;
			break;
		case SampleFormatFixed5p27:
			return 32;
			break;
		default:
			return 0;
			break;
		}
	}

	// backward compatibility, conversion to/from integers
	operator int () const { return value; }
	SampleFormat(int v) : value(SampleFormatEnum(v)) { }
	operator long () const { return value; }
	SampleFormat(long v) : value(SampleFormatEnum(v)) { }
	operator unsigned int () const { return value; }
	SampleFormat(unsigned int v) : value(SampleFormatEnum(v)) { }
	operator unsigned long () const { return value; }
	SampleFormat(unsigned long v) : value(SampleFormatEnum(v)) { }
};

struct MixerSettings
{

	UINT m_nStereoSeparation;
	UINT m_nMaxMixChannels;
	DWORD DSPMask;
	DWORD MixerFlags;
	DWORD gdwMixingFreq;
	DWORD gnChannels;
	DWORD m_nPreAmp;

	//rewbs.resamplerConf
	long glVolumeRampUpSamples, glVolumeRampDownSamples;
	//end rewbs.resamplerConf
	
	int32 GetVolumeRampUpMicroseconds() const;
	int32 GetVolumeRampDownMicroseconds() const;
	void SetVolumeRampUpMicroseconds(int32 rampUpMicroseconds);
	void SetVolumeRampDownMicroseconds(int32 rampDownMicroseconds);

	bool IsValid() const
	{
		return true
			&& (gnChannels == 1 || gnChannels == 2 || gnChannels == 4)
			&& (gdwMixingFreq > 0)
			;
	}
	
	MixerSettings();

};
