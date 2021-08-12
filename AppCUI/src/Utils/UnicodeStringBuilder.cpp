#include <Internal.hpp>

using namespace AppCUI::Utils;
using namespace AppCUI::Graphics;

#define LOCAL_BUFFER_FLAG       0x80000000
#define MAX_ALLOCATION_SIZE     0x7FFFFFFF

bool AppCUI::Utils::ConvertUTF8CharToUnicodeChar(const char8_t* p, const char8_t* end, UnicodeChar& result)
{
    // unicode encoding (based on the code described in https://en.wikipedia.org/wiki/UTF-8)
    if (((*p) >> 5) == 6) // binary encoding 110xxxxx, followed by 10xxxxxx
    {
        CHECK(p + 1 < end, false, "Invalid unicode sequence (missing one extra character after 110xxxx)");
        CHECK((p[1] >> 6) == 2, false, "Invalid unicode sequence (110xxxx should be followed by 10xxxxxx)");
        result.Value  = (((unsigned short) ((*p) & 0x1F)) << 6) | ((unsigned short) ((*(p + 1)) & 63));
        result.Length = 2;
        return true;
    }
    if (((*p) >> 4) == 14) // binary encoding 1110xxxx, followed by 2 bytes with 10xxxxxx
    {
        CHECK(p + 2 < end, false, "Invalid unicode sequence (missing two extra characters after 1110xxxx)");
        CHECK((p[1] >> 6) == 2, false, "Invalid unicode sequence (1110xxxx should be followed by 10xxxxxx)");
        CHECK((p[2] >> 6) == 2, false, "Invalid unicode sequence (10xxxxxx should be followed by 10xxxxxx)");
        result.Value = (((unsigned short) ((*p) & 0x0F)) << 12) | (((unsigned short) ((*(p + 1)) & 63)) << 6) |
                       ((unsigned short) ((*(p + 2)) & 63));
        result.Length = 3;
        return true;
    }
    if (((*p) >> 3) == 30) // binary encoding 11110xxx, followed by 3 bytes with 10xxxxxx
    {
        CHECK(p + 3 < end, false, "Invalid unicode sequence (missing two extra characters after 11110xxx)");
        CHECK((p[1] >> 6) == 2, false, "Invalid unicode sequence (11110xxx should be followed by 10xxxxxx)");
        CHECK((p[2] >> 6) == 2, false, "Invalid unicode sequence (10xxxxxx should be followed by 10xxxxxx)");
        CHECK((p[3] >> 6) == 2, false, "Invalid unicode sequence (10xxxxxx should be followed by 10xxxxxx)");
        result.Value = (((unsigned short) ((*p) & 7)) << 18) | (((unsigned short) ((*(p + 1)) & 63)) << 12) |
                       (((unsigned short) ((*(p + 2)) & 63)) << 6) | ((unsigned short) ((*(p + 3)) & 63));
        result.Length = 4;
        return true;
    }
    // invalid 16 bytes encoding
    RETURNERROR(false, "Invalid UTF-8 encoding ");
}

void UnicodeStringBuilder::Create(char16_t* localBuffer, size_t localBufferSize)
{
    if ((localBuffer == nullptr) || (localBufferSize == 0) || (localBufferSize > MAX_ALLOCATION_SIZE))
    {
        this->Chars     = nullptr;
        this->Size      = 0;
        this->Allocated = 0;
    }
    else
    {
        this->Chars     = localBuffer;
        this->Size      = 0;
        this->Allocated = localBufferSize | LOCAL_BUFFER_FLAG;
    }
}
void UnicodeStringBuilder::Destroy()
{
    if ((!(this->Allocated & LOCAL_BUFFER_FLAG)) && (this->Chars))
        delete[] this->Chars;
    this->Chars     = nullptr;
    this->Size      = 0;
    this->Allocated = 0;
}
bool UnicodeStringBuilder::Resize(size_t size)
{
    CHECK(size < MAX_ALLOCATION_SIZE, false, "Size must be smaller than 0x7FFFFFFF");
    if ((unsigned int)size < Allocated)
        return true;
    char16_t* newBuf = new char16_t[size];
    CHECK(newBuf != nullptr, false, "Fail to allocate buffer !");
    if (this->Chars)
    {
        if (this->Size > 0)
        {
            memcpy(newBuf, this->Chars, sizeof(char16_t) * this->Size);
        }
        delete[] this->Chars;
    }
    newBuf = this->Chars;
    this->Allocated = (unsigned int) size;
    return true;
}
UnicodeStringBuilder::UnicodeStringBuilder()
{
    Create(nullptr, 0);
}
UnicodeStringBuilder::UnicodeStringBuilder(char16_t* localBuffer, size_t localBufferSize)
{
    Create(localBuffer, localBufferSize);
}
UnicodeStringBuilder::UnicodeStringBuilder(const AppCUI::Utils::ConstString& text)
{
    Create(nullptr, 0);
    if (!Set(text))
        Destroy();
}
UnicodeStringBuilder::UnicodeStringBuilder(char16_t* localBuffer, size_t localBufferSize, const AppCUI::Utils::ConstString& text)
{
    Create(localBuffer, localBufferSize);
    if (!Set(text))
        Destroy();
}
UnicodeStringBuilder::UnicodeStringBuilder(const AppCUI::Graphics::CharacterBuffer& charBuffer)
{
    Create(nullptr, 0);
    if (!Set(charBuffer))
        Destroy();
}
UnicodeStringBuilder::UnicodeStringBuilder(char16_t* localBuffer, size_t localBufferSize, const AppCUI::Graphics::CharacterBuffer& charBuffer)
{
    Create(localBuffer, localBufferSize);
    if (!Set(charBuffer))
        Destroy();
}
UnicodeStringBuilder::~UnicodeStringBuilder()
{
    Destroy();
}
bool UnicodeStringBuilder::Set(const AppCUI::Utils::ConstString& text)
{
    if (std::holds_alternative<std::string_view>(text))
    {
        size_t sz                  = std::get<std::string_view>(text).length();
        const unsigned char* start = (const unsigned char*) std::get<std::string_view>(text).data();
        const unsigned char* end   = start + sz;
        auto* p                    = this->Chars;
        CHECK(Resize(sz), false, "Fail to resize buffer !");
        while (start < end)
        {
            *p = * start;
            start++;
            p++;
        }
        this->Size = (unsigned int) sz;
        return true;
    }
    if (std::holds_alternative<std::u16string_view>(text))
    {
        size_t sz                  = std::get<std::u16string_view>(text).length();
        const char16_t* start      = (const char16_t*) std::get<std::u16string_view>(text).data();
        CHECK(Resize(sz), false, "Fail to resize buffer !");
        memcpy(this->Chars, start, sizeof(char16_t) * sz);
        this->Size = (unsigned int) sz;
        return true;
    }
    if (std::holds_alternative<std::u8string_view>(text))
    {
        size_t sz                  = std::get<std::string_view>(text).length();
        const char8_t* start       = std::get<std::u8string_view>(text).data();
        const char8_t* end         = start + sz;
        auto* p                    = this->Chars;
        this->Size                 = 0;

        CHECK(Resize(sz), false, "Fail to resize buffer !");
        UnicodeChar uc;
        while (start < end)
        {
            if ((*start) < 0x80)
            {
                *p = *start;
                start++;
            }
            else
            {
                CHECK(ConvertUTF8CharToUnicodeChar(start, end, uc), false, "Fail to convert unicode character !");
                *p = uc.Value;
                start += uc.Length;
            }
            p++;
        }
        this->Size = (unsigned int) (p - this->Chars);
        return true;
    }
    RETURNERROR(false, "Fail to Set a string (unknwon variant type)");
}
bool UnicodeStringBuilder::Set(const AppCUI::Graphics::CharacterBuffer& charBuffer)
{
    CHECK(Resize(charBuffer.Len()), false, "Fail to resize buffer !");
    Character* start = charBuffer.GetBuffer();
    Character* end   = start + charBuffer.Len();
    auto* p          = this->Chars;
    while (start < end)
    {
        *p = start->Code;
        start++;
        p++;
    }
    this->Size = (unsigned int) charBuffer.Len();
    return true;
}