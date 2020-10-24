/*
	PicoText
	mperron (2019)

	A class which draws text onto the screen using a bitmap font.
*/
class PicoText :
	public Drawable
{
	SDL_Texture *font, *font_shadow;
	SDL_Rect region;
	string message;

	// Size of the individual letters in pixels. This includes an extra pixel
	// on the right and bottom which isn't normally used, that represents the
	// space between characters and lines of text. Some characters have
	// descenders which occupy the extra bottom line.
	int c_width = 6;
	int c_height = 7;

	// Extra space between lines of text.
	int leading = 0;

	int ticks_perchar = 0;
	int ticks_passed = 0;

	int shadow_offset_x = 0;
	int shadow_offset_y = 0;

	unsigned int blink_on = 0;
	unsigned int blink_off = 0;

public:
	PicoText(SDL_Renderer *rend, SDL_Rect region, string message) :
		Drawable(rend)
	{
		this->region = region;
		this->message = message;

		// Load the default font image.
		font = textureFromBmp(rend, "fonts/6x7.bmp", true);
		font_shadow = textureFromBmp(rend, "fonts/6x7.bmp", true);
	}

	~PicoText(){
		SDL_DestroyTexture(font);
		SDL_DestroyTexture(font_shadow);
	}

	void set_shadow(int x, int y){
		shadow_offset_x = x;
		shadow_offset_y = y;
	}

	void set_font(string bitmap, int c_width, int c_height){
		SDL_DestroyTexture(font);
		SDL_DestroyTexture(font_shadow);

		font = textureFromBmp(rend, bitmap.c_str(), true);
		font_shadow = textureFromBmp(rend, bitmap.c_str(), true);

		this->c_width = c_width;
		this->c_height = c_height;
	}

	void draw(int ticks){
		static unsigned int blink_counter = 0;

		// Blink text.
		if(blink_on || blink_off){
			blink_counter += ticks;

			// Skip draw if we are beyond the on period.
			if((blink_counter % (blink_on + blink_off)) > blink_on)
				return;

			// Keep the blink_counter from growing indefinitely.
			while(blink_counter > (blink_on + blink_off))
				blink_counter -= (blink_on + blink_off);
		}


		SDL_Rect src = (SDL_Rect){
			0,0,
			c_width, c_height
		};
		SDL_Rect dst = (SDL_Rect){
			region.x, region.y,
			c_width, c_height
		};
		bool wrapped = false;
		bool shadow = (shadow_offset_x || shadow_offset_y);

		int chars_max = -1;
		if(ticks_perchar){
			unsigned int chars = ((ticks_passed + ticks) / ticks_perchar);

			// We don't want chars_max or ticks_passed to count up
			// indefinitely. Cap these values as soon as we're beyond the
			// length of the string.
			if(chars <= message.size()){
				ticks_passed += ticks;
				chars_max = chars;
			}
		}

		// FIXME This renders the text character by character on every single
		// frame regardless of whether the text has changed. If we're not doing
		// a delayed typewriter effect, and the text content or color hasn't
		// changed, we should instead read from a texture which was
		// pre-rendered. The method calls set_message and set_color could
		// trigger a redraw.
		int chars_printed = 0;
		int chars_perline = (region.w / c_width), chars_thisline = 0;
		for(char c : message){
			// Break early for the typewriter effect.
			if((chars_max >= 0) && (chars_printed++ > chars_max))
				break;

			if(wrapped){
				chars_thisline = 0;
				wrapped = false;

				// If the text just wrapped at the edge of the text region, skip a
				// following space which would be at the start of the line and look
				// bad, and a following newline which would otherwise result in a
				// double newline.
				if((c == '\n') || (c == ' '))
					continue;
			}

			// New line and carriage return.
			if(c == '\n'){
				dst.x = region.x;
				dst.y += (c_height + leading);
				chars_thisline = 0;

				continue;
			}

			if((c >= 'a') && (c <= 'z'))
				c -= 0x20;

			// The font starts at space.
			c -= ' ';

			// Unknown characters print a space.
			if(c >= 65)
				c = 0;

			src.x = (c * c_width);

			if(shadow){
				SDL_Rect dst_shadow = (SDL_Rect){
					dst.x + shadow_offset_x, dst.y + shadow_offset_y,
					dst.w + shadow_offset_x, dst.h + shadow_offset_y
				};

				SDL_RenderCopy(rend, font_shadow, &src, &dst_shadow);
			}

			SDL_RenderCopy(rend, font, &src, &dst);

			dst.x += c_width;
			chars_thisline++;

			// Automatically wrap at the edge of the text region.
			// FIXME - this could be smarter, so it doesn't break up words.
			if(chars_thisline > chars_perline){
				dst.x = region.x;
				dst.y += (c_height + leading);
				wrapped = true;
			}
		}
	}

	void set_message(string message){
		this->message = message;
	}

	// Set the color of the text at any time.
	void set_color(char r, char g, char b, bool shadow = false){
		SDL_SetTextureColorMod((shadow ? font_shadow : font), r, g, b);
	}
	void set_color(SDL_Color col, bool shadow = false){
		set_color(col.r, col.g, col.b, shadow);
	}

	// Set the alpha/transparency for the text at any time.
	void set_alpha(char a, bool shadow = false){
		SDL_SetTextureAlphaMod((shadow ? font_shadow : font), a);
	}

	void set_blink(unsigned int on, unsigned int off){
		blink_on = on;
		blink_off = off;
	}

	// Set the number of millisecond ticks to hang on each character when
	// simulating a typewriter. Set to 0 (default) to disable this effect.
	void set_ticks_perchar(int ticks){
		ticks_perchar = ticks;
	}

	void set_pos(int x, int y){
		region.x = x;
		region.y = y;
	}
};
