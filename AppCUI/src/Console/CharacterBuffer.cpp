#include "AppCUI.h"
#include <string.h>

using namespace AppCUI::Console;

#define VALIDATE_ALLOCATED_SPACE(requiredSpace, returnValue) \
    if ((requiredSpace) > (Allocated & 0x7FFFFFFF)) { \
        CHECK(Grow(requiredSpace), returnValue, "Fail to allocate space for %d bytes", (requiredSpace)); \
    }

#define COMPUTE_TEXT_SIZE(text,textSize) if (textSize==0xFFFFFFFF) { textSize = AppCUI::Utils::String::Len(text);}


CharacterBuffer::CharacterBuffer()
{
    Allocated = 0;
    Count = 0;
    Buffer = nullptr;
}
CharacterBuffer::~CharacterBuffer(void)
{
    Destroy();
}
void CharacterBuffer::Destroy()
{
    if (Buffer)
        delete Buffer;
    Buffer = nullptr;
    Count = Allocated = 0;
}
bool CharacterBuffer::Grow(unsigned int newSize)
{
    newSize = ((newSize | 15) + 1) & 0x7FFFFFFF;
    if (newSize <= (Allocated & 0x7FFFFFFF))
        return true;
    Character * temp = new Character[newSize];
    CHECK(temp, false, "Failed to allocate: %d characters", newSize);
    if (Buffer)
    {
        memcpy(temp, Buffer, sizeof(Character)*this->Count);
        delete Buffer;
    }
    Buffer = temp;
    Allocated = newSize;
    return true;
}
bool CharacterBuffer::Add(const char * text, unsigned int color, unsigned int textSize)
{
    CHECK(text, false, "Expecting a valid (non-null) text");
    COMPUTE_TEXT_SIZE(text, textSize);
    VALIDATE_ALLOCATED_SPACE(textSize+this->Count, false);
    Character* ch = this->Buffer + this->Count;
    const unsigned char * p = (const unsigned char *)text;
    this->Count += textSize;
    while (textSize > 0)
    {
        ch->Color = color;
        ch->Code = *p;
        ch++;
        p++;
        textSize--;
    }
    return true;
}
bool CharacterBuffer::Set(const char * text, unsigned int color, unsigned int textSize)
{
    this->Count = 0;
    return Add(text, color, textSize);
}
bool CharacterBuffer::SetWithHotKey(const char * text, unsigned int & hotKeyCharacterPosition, unsigned int color, unsigned int textSize)
{
    hotKeyCharacterPosition = 0xFFFFFFFF;
    CHECK(text, false, "Expecting a valid (non-null) text");
    COMPUTE_TEXT_SIZE(text, textSize);
    VALIDATE_ALLOCATED_SPACE(textSize, false);
    Character* ch = this->Buffer;
    const unsigned char * p = (const unsigned char *)text;
    while (textSize > 0)
    {
        if ((hotKeyCharacterPosition == 0xFFFFFFFF) && ((*p) == '&') && (textSize>1 /* not the last character*/))
        {
            char tmp = p[1] | 0x20;
            if (((tmp >= 'a') && (tmp <= 'z')) || ((tmp >= '0') && (tmp <= '9')))
            {
                hotKeyCharacterPosition = (unsigned int)(p - (const unsigned char *)text);
                p++; textSize--;
                continue;
            }
        }
        ch->Color = color;
        ch->Code = *p;
        ch++;
        p++;
        textSize--;
    }
    this->Count = (unsigned int)(ch - this->Buffer);
    return true;
}
void CharacterBuffer::Clear()
{
    this->Count = 0;
}
bool CharacterBuffer::Delete(unsigned int start, unsigned int end)
{
    CHECK(end <= this->Count, false, "Invalid delete offset: %d (should be between 0 and %d)", position, this->Size);
    CHECK(start < end, false, "Start parameter (%d) should be smaller than End parameter (%d)", start, end);
    if (end == this->Count) {
        this->Count = start;
    } else {
        memmove(this->Buffer + start, this->Buffer + end, (this->Count - end) * sizeof(Character));
        this->Count -= (end - start);
    }
    return true;
}
bool CharacterBuffer::DeleteChar(unsigned int position)
{
    CHECK(position < this->Count, false, "Invalid delete offset: %d (should be between 0 and %d)", position, this->Count - 1);
    if ((position + 1) < this->Count) {
        memmove(this->Buffer + position, this->Buffer + position + 1, (this->Count - (position + 1)) * sizeof(Character));
    }
    this->Count--;
    return true;
}
bool CharacterBuffer::InsertChar(unsigned short characterCode, unsigned int position, unsigned int color)
{
    VALIDATE_ALLOCATED_SPACE(this->Count + 1, false);
    CHECK(position <= this->Count, false, "Invalid insert offset: %d (should be between 0 and %d)", position, this->Size);
    if (position < this->Count) {
        memmove(this->Buffer + position + 1, this->Buffer + position, (this->Count - position) * sizeof(Character));    
    }
    auto c = this->Buffer + position;
    c->Code = characterCode;
    c->Color = color;
    this->Count++;
    return true;
}
bool CharacterBuffer::SetColor(unsigned int start, unsigned int end, unsigned int color)
{
    if (end > this->Count)
        end = this->Count;
    CHECK(start <= end, false, "Expecting a valid parameter for start (%d) --> should be smaller than %d", start, end);
    Character* ch = this->Buffer + start;
    unsigned int sz = end - start;
    while (sz) {
        ch->Color = color;
        sz--;
        ch++;
    }
    return true;
}
void CharacterBuffer::SetColor(unsigned int color)
{
    Character* ch = this->Buffer;
    unsigned int sz = this->Count;
    while (sz) {
        ch->Color = color;
        sz--;
        ch++;
    }
}