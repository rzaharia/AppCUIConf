#include "../../include/ControlContext.h"

using namespace AppCUI::Controls;
using namespace AppCUI::Console;
using namespace AppCUI::Input;
using namespace AppCUI::OS;

#define MAX_LINE_SIZE_COLORS	1024

#define WRAPPER	((TextAreaControlContext*)this->Context)

void TextAreaControlContext::SelAll()
{
    SelStart = 0;
	SelEnd = Text.Len();
	SelEnd--;
	if (SelEnd<0) 
		ClearSel();
}
void TextAreaControlContext::ClearSel()
{
	SelStart=SelEnd=SelOrigin=-1;
}
void TextAreaControlContext::MoveSelTo(int poz)
{
	if (SelStart==-1) 
		return ;
	if (poz==SelOrigin) 
	{ 
		ClearSel(); 
		return; 
	}
	if (poz<SelOrigin)
	{
		SelStart=poz;
		SelEnd=SelOrigin-1;
	} else if (poz>SelOrigin)
	{
		SelEnd=poz-1;
		SelStart=SelOrigin;
	} 
	//if (poz<SelStart) SelStart=poz; else SelEnd=poz-1;
}

void TextAreaControlContext::DeleteSelected()
{
	int ss,se;
	if (SelStart==-1) 
		return;
	ss=SelStart;se=SelEnd;
	if (SelStart<SelOrigin)
	{
		SelStart++;
	}
	if (cLocation>=SelStart)
	{
		MoveTo(cLocation-(se-ss+1),false);
	}
	Text.Delete(ss, se + 1);
	UpdateLines();
	ClearSel();
}

void TextAreaControlContext::UpdateView()
{
	int *pLines = Lines.GetInt32Array();
	for (unsigned int tr=0;tr<Lines.Len();tr++)
		if (cLocation>=pLines[tr])
			cLine=tr;
	if (cLine<startLine)
	{
		startLine=cLine;
	}
	if (cLine>=startLine+viewLines)
	{
		startLine=cLine-viewLines;
		if (startLine<0) startLine=0;
	}
}
void TextAreaControlContext::UpdateLines()
{
    unsigned int txSize, lineNumber;
    Character* c = Text.GetBuffer();
    Character* s = c;
    Character* c_End = c + Text.Len();

    lineNumber = 0;
    
	textSize=0;
	cLine=0;
    Lines.Clear();
	do
	{
        txSize = (unsigned int)(c - s);
        if (cLocation >= (int)txSize)
            cLine = lineNumber;
        Lines.Push(txSize);
		lineNumber++;

        while ((c < c_End) && (c->Code != 13) && (c->Code != 10))
            c++;

        if (c < c_End)
        {
            if ((c->Code == 13) && ((c + 1) < c_End) && ((c + 1)->Code == 10))
                c += 2; //CRLF
            else if ((c->Code == 10) && ((c + 1) < c_End) && ((c + 1)->Code == 13))
                c += 2; //LFCR
            else
                c++;    //either a CR or a LF
        }
    } while (c < c_End);
	UpdateView();
}
int  TextAreaControlContext::GetLineSize(int lineIndex)
{
    int *pLines = Lines.GetInt32Array();
	if ((pLines==nullptr) || (lineIndex<0) || (lineIndex>=(int)Lines.Len()))
		return 0;
	if ((lineIndex+1)<(int)Lines.Len())
	{
		return pLines[lineIndex+1]-(pLines[lineIndex]+1);
	} else {
		return textSize-pLines[lineIndex];
	}
}
int	 TextAreaControlContext::GetLineStart(int lineIndex)
{
    int *pLines = Lines.GetInt32Array();
	if ((pLines==nullptr) || (lineIndex<0) || (lineIndex>=(int)Lines.Len()))
		return 0;
	return pLines[lineIndex];
}
void TextAreaControlContext::AnalyzeCurrentText()
{
	UpdateLines();
	cLocation=px=0;
	MoveTo(textSize,false);
}
void TextAreaControlContext::SetTabCharacter(char tabCharacter)
{
	tabChar = tabCharacter;
}
void TextAreaControlContext::SetColorFunction(Handlers::TextAreaSyntaxHighlightHandler GetLineColorFunction, void *Context)
{
	fnGetLineColor = GetLineColorFunction;
	colorPData = Context;
}

void TextAreaControlContext::DrawToolTip()
{
}
void TextAreaControlContext::DrawLine(int lineIndex,int pozY,bool activ)
{
	//int				poz,c,pozX;
	//int				lnSize,tr;
	//char			ch;
	//unsigned char	colors[MAX_LINE_SIZE_COLORS];
	//const char*		Txt;

	//if ((lineIndex>=(int)Lines.GetSize()) || (lineIndex<0))
	//	return;
	//poz = *(((int *)Lines.GetVector()) + lineIndex);
	//if ((poz<0) || (poz>=textSize))
	//	return;
	//lnSize=GetLineSize(lineIndex);
	//Txt = Text.GetText();
	//if (fnGetLineColor)
	//	fnGetLineColor(&Txt[poz],colors,lnSize,MAX_LINE_SIZE_COLORS,colorPData);
	//pozX=1;
	//if (Flags & (unsigned int)TextAreaFlags::SHOW_LINE_NUMBERS)
	//	pozX+=3;
	//for (tr=0;tr<lnSize;tr++,poz++)
	//{
	//	c=Cfg->TextCol[(int)activ];
	//	if ((fnGetLineColor) && (tr<MAX_LINE_SIZE_COLORS))
	//		c=colors[tr];
	//	if ((poz>=SelStart) && (poz<=SelEnd) && (activ)) 
	//		c=Cfg->TextSelectCol;
	//	if ((Flags & GATTR_ENABLE)==0)
	//		c=Cfg->TextInactivCol;
	//	ch=Txt[poz];
	//	if (ch=='\t') 
	//		ch=tabChar;
	//	if ((ch=='\n') || (ch=='\r')) 
	//		ch=' ';
	//	Console::WriteChar(pozX,pozY,ch,c);
	//	if ((activ) && (poz==cLocation)) 
	//		Console::SetCursorPos(pozX,pozY); 
	//	if (Txt[poz]=='\t')
	//	{
	//		if ((pozX % 4)==0)
	//			pozX+=4;
	//		else
	//			pozX = ((pozX/4)+1)*4;
	//	} else {
	//		pozX++;
	//	}
	//}
	//if ((activ) && (poz==cLocation)) 
	//	Console::SetCursorPos(pozX,pozY); 
}
void TextAreaControlContext::DrawLineNumber(int lineIndex,int pozY,bool activ)
{
	char temp[32];
	int poz = 30;
	temp[31] = 0;
	lineIndex++;
	if (lineIndex >= 0)
	{
		do
		{
			temp[poz--] = lineIndex % 10 + '0';
			lineIndex /= 10;
		} while (lineIndex > 0);
		while (poz > 27) {
			temp[poz--] = ' ';
		}
		if (poz < 27)
			temp[28] = '.';
	}
	else {
		temp[28] = '?';
		temp[29] = '?';
		temp[30] = '?';
		temp[31] = 0;
	}
	//Console::WriteString(0,pozY,&temp[28],Cfg->TextErrorCol);
	//Console::WriteChar(3,pozY,179,Cfg->TextCol[(int)activ]);
}
void TextAreaControlContext::Paint(bool activ)
{
	//int a=(int)activ;

	//if (Flags & (unsigned int)TextAreaFlags::BORDER)
	//{
	//	Console::DrawRect(0,0,Width-1,Height-1,Cfg->TextCol[a],false);
	//	if (activ)
	//	{
	//		VBar.Set(Width-1,1,Height-4,false);
	//		VBar.SetValue(cLine);
	//		VBar.SetMaxValue(Lines.GetSize());
	//		VBar.Paint();
	//	}
	//	itemsClip.Create(ScreenClip, 1, 1, Width - 2, Height - 2);
	//	Console::SetClip(&itemsClip);
	//}
	//if (Flags & GATTR_ENABLE)
	//	Console::FillRect(0,0,Width,Height,32,Cfg->TextCol[a]);
	//else 
	//	Console::FillRect(0, 0, Width, Height, 32, Cfg->TextInactivCol);
	//for (int tr=0;tr<=viewLines;tr++)
	//{
	//	DrawLine(tr+startLine,tr,activ);
	//	if (Flags & (unsigned int)TextAreaFlags::SHOW_LINE_NUMBERS)
	//		DrawLineNumber(tr+startLine,tr,activ);
	//}
}
void TextAreaControlContext::MoveTo(int newPoz,bool selected)
{
	if ((!selected) && (SelStart!=-1)) 
		ClearSel();
	
	if ((cLocation>=(int)Text.Len()) && (newPoz>cLocation)) 
		return;
	if ((selected) && (SelStart==-1)) 
	{ 
		SelStart=SelEnd=SelOrigin=cLocation; 
	}
	while (cLocation!=newPoz)
	{
		if (cLocation>newPoz) cLocation--; else if (cLocation<newPoz) cLocation++; 
		if (cLocation<0) { cLocation=newPoz=0; }
		if (cLocation >= (int)Text.Len())  newPoz=cLocation;
		if (cLocation<px) px=cLocation;
        if (cLocation > px + Layout.Width - 2) px = cLocation - Layout.Width + 2;
	} 
	if (selected) MoveSelTo(cLocation);
	UpdateView();
}
void TextAreaControlContext::MoveToLine(int times,bool selected)
{
	int xOffset,newLine,newLineSize;

	if (Lines.Len()==0)
		return;

	xOffset = cLocation - GetLineStart(cLine);
	newLine = cLine+times;
	if (newLine<0) 
		newLine=0;
	if (newLine>=(int)Lines.Len())
		newLine=Lines.Len()-1;
	newLineSize = GetLineSize(newLine);
	if (newLineSize<xOffset)
		xOffset = newLineSize;
	MoveTo(xOffset+GetLineStart(newLine),selected);
}
void TextAreaControlContext::MoveHome(bool selected)
{
	MoveTo(GetLineStart(cLine),selected);
}
void TextAreaControlContext::MoveEnd(bool selected)
{
	MoveTo(GetLineStart(cLine)+GetLineSize(cLine),selected);
}

void TextAreaControlContext::AddChar(char ch)
{
	if ((Flags & (unsigned int)TextAreaFlags::READONLY)!=0) 
		return;
	DeleteSelected();
	Text.InsertChar(ch, cLocation);
	MoveTo(cLocation+1,false);
	UpdateLines();
	SendMsg(Event::EVENT_TEXT_CHANGED);
}
void TextAreaControlContext::KeyBack()
{
	if ((Flags & (unsigned int)TextAreaFlags::READONLY) != 0)
		return;
	if (SelStart!=-1) 
	{ 
		DeleteSelected();
		return; 
	}
	if (cLocation==0) 
		return;
	Text.DeleteChar(cLocation - 1);
	MoveTo(cLocation-1,false);
	UpdateLines();
	SendMsg(Event::EVENT_TEXT_CHANGED);
}
void TextAreaControlContext::KeyDelete()
{
	if ((Flags & (unsigned int)TextAreaFlags::READONLY) != 0)
		return;
	if (SelStart!=-1) 
	{ 
		DeleteSelected();
		return; 
	}
	Text.DeleteChar(cLocation);
	UpdateLines();
	SendMsg(Event::EVENT_TEXT_CHANGED);
}
bool TextAreaControlContext::HasSelection()
{
	return ((SelStart >= 0) && (SelEnd >= 0) && (SelEnd >= SelStart));
}
void TextAreaControlContext::SetSelection(int start,int end)
{
	if ((start>=0) && (start<=end)) 
	{	
		SelStart=SelOrigin=start;SelEnd=end;
	}
}
void TextAreaControlContext::CopyToClipboard()
{
	//if (HasSelection())  
	//	Clipboard::SetText(Text.GetText(),SelStart,SelEnd);
}
void TextAreaControlContext::PasteFromClipboard()
{
	if ((Flags & (unsigned int)TextAreaFlags::READONLY) != 0)
		return;
	//int tr;
	//const char *ss;
	//ss=Clipboard::GetText();
	//if (ss!=nullptr)
	//{
	//	if (HasSelection()) 
	//		DeleteSelected();
	//	for (tr=0;ss[tr]!=0;tr++) AddChar(ss[tr]);
	//}
	//UpdateLines();
}

bool TextAreaControlContext::OnKeyEvent(int KeyCode, char AsciiCode)
{
	switch (KeyCode)
	{
		case Key::Left							: MoveTo(cLocation-1,false); return true;
		case Key::Right						    : MoveTo(cLocation + 1, false); return true;
		case Key::Up							: MoveToLine(-1,false); return true;
		case Key::PageUp						: MoveToLine(-viewLines,false); return true;
		case Key::Down							: MoveToLine(1,false); return true;
		case Key::PageDown						: MoveToLine(viewLines,false); return true;
		case Key::Home							: MoveHome(false); return true;
		case Key::End							: MoveEnd(false); return true;
		case Key::Ctrl|Key::Home				: MoveTo(0,false); return true;
		case Key::Ctrl|Key::End				    : MoveTo(textSize,false); return true;

		case Key::Shift|Key::Left				: MoveTo(cLocation - 1, true); return true;
		case Key::Shift | Key::Right			: MoveTo(cLocation + 1, true); return true;
		case Key::Shift | Key::Up				: MoveToLine(-1, true); return true;
		case Key::Shift | Key::PageUp			: MoveToLine(-viewLines, true); return true;
		case Key::Shift | Key::Down			    : MoveToLine(1, true); return true;
		case Key::Shift | Key::PageDown		    : MoveToLine(viewLines, true); return true;
		case Key::Shift | Key::Home			    : MoveHome(true); return true;
		case Key::Shift | Key::End			    : MoveEnd(true); return true;
		case Key::Shift | Key::Ctrl | Key::Home : MoveTo(0, true); return true;
		case Key::Shift | Key::Ctrl | Key::End  : MoveTo(textSize, true); return true;

		case Key::Tab							: if (Flags & (unsigned int)TextAreaFlags::PROCESS_TAB) { AddChar('\t'); return true; }
												  return false;
		case Key::Enter						    : AddChar('\n'); return true;
		case Key::Backspace					    : KeyBack(); return true;
		case Key::Delete						: KeyDelete(); return true;

		case Key::Ctrl | Key::A				    : SelAll(); return true;
		case Key::Ctrl | Key::Insert			:
		case Key::Ctrl | Key::C				    : CopyToClipboard(); return true;
		case Key::Shift| Key::Insert			:
		case Key::Ctrl | Key::V				    : PasteFromClipboard(); return true;
	}
	if (AsciiCode>0)
	{
		AddChar(AsciiCode);
		return true;
	}
	return false;
}

void TextAreaControlContext::OnAfterResize()
{
    viewLines = Layout.Height - 1;
	viewColumns = Layout.Width - 2;
	if (Flags & (unsigned int)TextAreaFlags::SHOW_LINE_NUMBERS) 
		viewColumns -= 4;
	if (Flags & (unsigned int)TextAreaFlags::BORDER)
	{
		viewLines -=2;
		viewColumns -=2;
	}
	UpdateLines();
}
void TextAreaControlContext::SetToolTip(char *ss)
{
	toolTipInfo.Set(ss);
	toolTipVisible=true;
}
bool TextAreaControlContext::IsReadOnly()
{
	return ((Flags & (unsigned int)TextAreaFlags::READONLY)!=0);
}
void TextAreaControlContext::SetReadOnly(bool value)
{
	if (value)
		this->Flags |= (unsigned int)TextAreaFlags::READONLY;
	else
		this->Flags -= ((this->Flags) & (unsigned int)TextAreaFlags::READONLY);
}
void TextAreaControlContext::SendMsg(Event::Type eventType)
{
	Host->RaiseEvent(eventType);
}

//======================================================================================================================================================================
TextArea::~TextArea()
{
	DELETE_CONTROL_CONTEXT(TextAreaControlContext);
}
bool		TextArea::Create(Control *parent, const char * text, const char * layout, TextAreaFlags flags)
{
	CHECK(text != nullptr, false, "Text should not be null !");
	CONTROL_INIT_CONTEXT(TextAreaControlContext);
    CREATE_TYPECONTROL_CONTEXT(TextAreaControlContext, Members, false);
    Members->Layout.MinWidth = 5;
    Members->Layout.MinHeight = 3;
    CHECK(Init(parent, "", layout, false), false, "Failed to create text area  !");
	Members->Flags = GATTR_ENABLE | GATTR_VISIBLE | GATTR_TABSTOP | (unsigned int)flags;
	// initializam
    CHECK(Members->Lines.Create(128), false, "Fail to create indexes for line numbers");
	Members->fnGetLineColor = nullptr;
	Members->colorPData = nullptr;
	Members->tabChar = ' ';
	Members->Host = this;

	Members->viewLines = Members->Layout.Height - 1;
	Members->viewColumns = Members->Layout.Width - 2;
	if ((unsigned int)flags & (unsigned int)TextAreaFlags::SHOW_LINE_NUMBERS)
		Members->viewColumns -= 4;
	if (Members->Flags & (unsigned int)TextAreaFlags::BORDER)
	{
		Members->viewLines -= 2;
		Members->viewColumns -= 2;
	}
	Members->startLine = 0;
	Members->ClearSel();
	Members->AnalyzeCurrentText();
	// all is good
	return true;
}
void		TextArea::Paint(Console::Renderer & renderer)
{
	CREATE_TYPECONTROL_CONTEXT(TextAreaControlContext, Members, );
	Members->Paint(Members->Focused);
}
bool		TextArea::OnKeyEvent(AppCUI::Input::Key::Type keyCode, char AsciiCode)
{
	return WRAPPER->OnKeyEvent(keyCode, AsciiCode);
}
void		TextArea::OnAfterResize(int newWidth,int newHeight)
{
	WRAPPER->OnAfterResize();
}
void		TextArea::OnFocus()
{
	WRAPPER->SelAll();
}
void		TextArea::OnAfterSetText(const char* newText)
{
	CREATE_TYPECONTROL_CONTEXT(TextAreaControlContext, Members, );
	Members->AnalyzeCurrentText();
}
void		TextArea::SetReadOnly(bool value)
{
	WRAPPER->SetReadOnly(value);
}
bool		TextArea::IsReadOnly()
{
	return WRAPPER->IsReadOnly();
}
void		TextArea::SetTabCharacter(char tabCharacter)
{
	WRAPPER->SetTabCharacter(tabCharacter);
}
void		TextArea::SetColorFunction(Handlers::TextAreaSyntaxHighlightHandler handler, void *Context)
{
	WRAPPER->SetColorFunction(handler, Context);
}

