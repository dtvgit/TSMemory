#include "stdafx.h"
#include "StdUtil.h"




int StdUtil::snprintf(char *s,size_t n,const char *format, ...)
{
	va_list args;
	int Length;

	va_start(args,format);
#if defined(__STDC_VERSION__) && __STDC_VERSION__>=199901L
	// �����I��VC�ł����R���p�C���ł��Ȃ��̂Ŗ��Ӗ�����...
	Length=::vsnprintf(s,n,format,args);
#else
	if (n>0) {
		Length=::_vsnprintf(s,n-1,format,args);
		s[n-1]='\0';
	} else {
		Length=0;
	}
#endif
	va_end(args);
	return Length;
}


int StdUtil::snprintf(wchar_t *s,size_t n,const wchar_t *format, ...)
{
	va_list args;
	int Length;

	va_start(args,format);
	if (n>0) {
		Length=::_vsnwprintf(s,n-1,format,args);
		s[n-1]='\0';
	} else {
		Length=0;
	}
	va_end(args);
	return Length;
}


int StdUtil::vsnprintf(char *s,size_t n,const char *format,va_list args)
{
	int Length;

#if defined(__STDC_VERSION__) && __STDC_VERSION__>=199901L
	Length=::vsnprintf(s,n,format,args);
#else
	if (n>0) {
		Length=::_vsnprintf(s,n-1,format,args);
		s[n-1]='\0';
	} else {
		Length=0;
	}
#endif
	return Length;
}


int StdUtil::vsnprintf(wchar_t *s,size_t n,const wchar_t *format,va_list args)
{
	int Length;

	if (n>0) {
		Length=::_vsnwprintf(s,n-1,format,args);
		s[n-1]='\0';
	} else {
		Length=0;
	}
	return Length;
}


char *StdUtil::strncpy(char *dest,size_t n,const char *src)
{
	size_t length=::strlen(src);

	if (n-1<length) {
		::memcpy(dest,src,n-1);
		dest[n-1]='\0';
	} else {
		::strcpy(dest,src);
	}
	return dest;
}


wchar_t *StdUtil::strncpy(wchar_t *dest,size_t n,const wchar_t *src)
{
	size_t length=::wcslen(src);

	if (n-1<length) {
		::memcpy(dest,src,(n-1)*sizeof(wchar_t));
		dest[n-1]='\0';
	} else {
		::wcscpy(dest,src);
	}
	return dest;
}


char *StdUtil::strdup(const char *s)
{
	if (!s)
		return NULL;
	char *dup=new char[::strlen(s)+1];
	::strcpy(dup,s);
	return dup;
}


wchar_t *StdUtil::strdup(const wchar_t *s)
{
	if (!s)
		return NULL;
	wchar_t *dup=new wchar_t[::wcslen(s)+1];
	::wcscpy(dup,s);
	return dup;
}
