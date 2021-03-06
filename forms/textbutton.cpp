
#include "forms/textbutton.h"
#include "framework/framework.h"

namespace OpenApoc {

TextButton::TextButton( Framework &fw, Control* Owner, UString Text, std::shared_ptr<BitmapFont> font ) : Control( fw, Owner ), text( Text ), font( font ), buttonbackground(fw.data->load_image( "UI/TEXTBUTTONBACK.PNG" )), TextHAlign( HorizontalAlignment::Centre ), TextVAlign( VerticalAlignment::Centre )
{
	this->buttonclick = fw.data->load_sample("xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW" );
	cached = nullptr;
}

TextButton::~TextButton()
{
}

void TextButton::EventOccured( Event* e )
{
	Control::EventOccured( e );

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseDown )
	{
		fw.soundBackend->playSample(buttonclick);
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseClick )
	{
		Event* ce = new Event();
		ce->Type = e->Type;
		memcpy( (void*)&(ce->Data.Forms), (void*)&(e->Data.Forms), sizeof( FRAMEWORK_FORMS_EVENT ) );
		ce->Data.Forms.EventFlag = FormEventType::ButtonClick;
		fw.PushEvent( ce );
	}
}

void TextButton::OnRender()
{
	if( cached == nullptr || cached->size != Vec2<unsigned int>{Size.x, Size.y})
	{
		cached.reset(new Surface{Vec2<unsigned int>{Size.x, Size.y}});

		RendererSurfaceBinding b(*fw.renderer, cached);

		fw.renderer->drawScaled(buttonbackground, Vec2<float>{0,0}, Vec2<float>{Size.x, Size.y}); 
		fw.renderer->drawFilledRect(Vec2<float>{3,3}, Vec2<float>{Size.x-6, Size.y-6}, Colour{160,160,160});
		fw.renderer->drawLine(Vec2<float>{2,4}, Vec2<float>{Size.x-2, 4}, Colour{220,220,220});
		fw.renderer->drawLine(Vec2<float>{2, Size.y - 4}, Vec2<float>{Size.x - 2, Size.y - 4}, Colour{80,80,80});
		fw.renderer->drawLine(Vec2<float>{2, Size.y - 3}, Vec2<float>{Size.x - 2, Size.y - 3}, Colour{64,64,64});
		fw.renderer->drawRect(Vec2<float>{3,3}, Vec2<float>{Size.x-3, Size.y-3}, Colour{48,48,48});

		int xpos;
		int ypos;

		switch( TextHAlign )
		{
			case HorizontalAlignment::Left:
				xpos = 0;
				break;
			case HorizontalAlignment::Centre:
				xpos = (Size.x / 2) - (font->GetFontWidth( text ) / 2);
				break;
			case HorizontalAlignment::Right:
				xpos = Size.x - font->GetFontWidth( text );
				break;
			default:
				LogError("Unknown TextHAlign");
				return;
		}

		switch( TextVAlign )
		{
			case VerticalAlignment::Top:
				ypos = 0;
				break;
			case VerticalAlignment::Centre:
				ypos = (Size.y / 2) - (font->GetFontHeight() / 2);
				break;
			case VerticalAlignment::Bottom:
				ypos = Size.y - font->GetFontHeight();
				break;
			default:
				LogError("Unknown TextHAlign");
				return;
		}

		auto textImage = font->getString(text);
		fw.renderer->draw(textImage, Vec2<float>{xpos,ypos});
	}
	fw.renderer->draw(cached, Vec2<float>{0,0});

	if( mouseDepressed && mouseInside )
	{
		fw.renderer->drawRect(Vec2<float>{1,1}, Vec2<float>{Size.x-2, Size.y-2}, Colour{255,255,255}, 2);
	}
}

void TextButton::Update()
{
	// No "updates"
}

void TextButton::UnloadResources()
{
}

UString TextButton::GetText()
{
	return text;
}

void TextButton::SetText( UString Text )
{
	text = Text;
}

}; //namespace OpenApoc
