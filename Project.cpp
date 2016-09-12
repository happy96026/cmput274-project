/* CMPUT 274 Project
 * 
 * Names / Sections: 
 * Adam Narten / A2
 * Daniel Choi / A2
 * 
 * Acknowledgements:
 * bmpDraw function was copied from the arduino-ua/examples/Parrot directory
 * The use of the piezo buzzer was thanks to D. Cuartielles from
 * https://www.arduino.cc/en/Tutorial/PlayMelody
 * 
 * Description:
 * The classic game of battleship on two arduinos
 * 
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>

// standard U of A library settings, assuming Atmel Mega SPI pins
#define SD_CS    5  // Chip select line for SD card
#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)

// define colours for screen squares
#define BLACK		0x0000
#define BLUE		0x001F
#define RED			0xF800
#define GREEN		0x07E0
#define YELLOW	0xFFE0
#define WHITE		0xFFFF
#define GREY		0xC618

/* Joystick Definitions */
#define JOYSTICK_VERT	0			// Analog input A0 - vertical
#define JOYSTICK_HORIZ	1		// Analog input A1 - horizontal
#define JOYSTICK_BUTTON	9		// Digital input pin 9 for the button
int init_joystick_vert, init_joystick_horiz;

/* Pushbutton */
#define PUSHBUTTON 10 	// Digital input pin 10 for the button

#define SPEAKER 12			// Digital output for piezo buzzer
#define BUFFPIXEL 20		// For printing bmp images

// Tones for Piezo buzzer
//       note,  period, frequency.
#define  g2			5102		// 196 Hz
#define  gs2		4808		// 208 Hz
#define  a3			4545		// 220 Hz
#define  as3		4291		// 233 Hz
#define  b3			4049		// 247 Hz
#define  c3			3822		// 262 Hz
#define  cs3		3608		// 277 Hz
#define  d3			3405		// 294 Hz
#define  ds3		3214		// 311 Hz
#define  e3			3033		// 330 Hz
#define  f3			2863		// 349 Hz
#define  fs3		2702		// 370 Hz
#define  g3			2551		// 392 Hz
#define  gs3		2408		// 415 Hz
#define  a4			2273		// 440 Hz
#define  as4		2145		// 466 Hz
#define  b4			2025		// 494 Hz
#define  c4			1911		// 523 Hz
#define  cs4		1805		// 554 Hz
#define  d4			1703		// 587 Hz
#define  ds4		1608		// 622 Hz
#define  e4			1517		// 659 Hz
#define  f4			1433		// 698 Hz
#define  fs4		1351		// 740 Hz
#define  g4			1276		// 784 Hz
#define  gs4		1205		// 830 Hz
#define  a5			1136		// 880 Hz
#define  as5		1073		// 932 Hz
#define  b5			1012		// 988 Hz
#define  c5			955			// 1047 Hz
#define  R			0				// Rest

// Melodies
int hit_melody[] = {  c5,  b5, as5,  a5, gs4,  g4, fs4,  f4,  e4, ds4,  d4, cs4,  c4,  b4, as4,  a4, gs3,  g3, fs3,  f3,  e3, ds3,  d3, cs3,  c3,  b3, as3,  a3, gs2,  g2,  c4,  g3,  c4};
int hit_beats[]  = {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,  48,  16,  64};
int miss_melody[] = {  c5,  b5, as5,  a5, gs4,  g4, fs4,  f4,  e4, ds4,  d4, cs4,  c4,  b4, as4,  a4, gs3,  g3, fs3,  f3,  e3, ds3,  d3, cs3,  c3,  b3, as3,  a3, gs2,  g2,  c4,  b4, as4};
int miss_beats[]  = {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,  48,  48,  48};
int win_melody[] = {  g3,  R,  g3,  R,  g3,  c4};
int win_beats[]  = {  32,  4,  24,  4,   8,  64};

// Melody length, for looping.
int MAX_COUNT_HIT = sizeof(hit_melody) / 2; 
int MAX_COUNT_MISS = sizeof(miss_melody) / 2;
int MAX_COUNT_WIN = sizeof(win_melody) / 2;

long tempo = 10000; // Set overall tempo for melodies
int pause = 1000; // Set length of pause between notes
int tone_ = 0; // initialize tone
int beat = 0; // initialize beat
long duration = 0; // initalize duration


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

Sd2Card card;

typedef struct {
	int size;
	int location[10];
	int max_x_h;
	int max_y_v;
	char* file_x;
	char* file_y;
	int count;
	bool orientation;
} ship;

ship carrier;
ship battleship;
ship submarine;
ship destroyer;
ship patrol;

int my_hits[34];
int their_hits[34];
int my_misses[166];
int their_misses[166];
int th = 0, tm = 0, mh = 0, mm = 0; // counters for above arrays
int cursor_x, cursor_y;
int old_cursor_x, old_cursor_y;
int vertical, horizontal, select, pushbutton;
int max_y_h = 117, max_x_v = 116;

// Pulse the speaker to play a tone for a particular duration
void playTone() {
	long elapsed_time = 0;
	if (tone_ > 0) { // if this isn't a Rest beat, 
		while (elapsed_time < duration) { // while the tone has played less than duration
			digitalWrite(SPEAKER,HIGH);
			delayMicroseconds(tone_ / 2);

			digitalWrite(SPEAKER, LOW);
			delayMicroseconds(tone_ / 2);

			// Keep track of how long we pulsed
			elapsed_time += (tone_);
		}
	}
	else { // Rest beat
			delayMicroseconds(duration);                               
	}                                
}

void playHit() {
	for (int i=0; i<MAX_COUNT_HIT; i++) {
		tone_ = hit_melody[i];
		beat = hit_beats[i];
		duration = beat * tempo; // Set up timing

		playTone();
		delayMicroseconds(pause); // A pause between notes...
	}
}

void playMiss() {
	for (int i=0; i<MAX_COUNT_MISS; i++) {
		tone_ = miss_melody[i];
		beat = miss_beats[i];
		duration = beat * tempo; // Set up timing

		playTone();
		delayMicroseconds(pause); // A pause between notes...
	}
}

void playWin() {
	for (int i=0; i<MAX_COUNT_WIN; i++) {
		tone_ = win_melody[i];
		beat = win_beats[i];
		duration = beat * tempo; // Set up timing

		playTone();
		delayMicroseconds(pause); // A pause between notes...
	}
}

// read files to open bmp files
uint16_t read16(File f) {
	uint16_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read(); // MSB
	return result;
}

uint32_t read32(File f) {
	uint32_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read();
	((uint8_t *)&result)[2] = f.read();
	((uint8_t *)&result)[3] = f.read(); // MSB
	return result;
}


// draw a bmp image
void bmpDraw(char *filename, uint8_t x, uint8_t y) {

	File     bmpFile;
	int      bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t  bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
	uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean  goodBmp = false;       // Set to true on valid header parse
	boolean  flip    = true;        // BMP is stored bottom-to-top
	int      w, h, row, col;
	uint8_t  r, g, b;
	uint32_t pos = 0, startTime = millis();

	if((x >= tft.width()) || (y >= tft.height())) return;

	Serial.println();
	Serial.print("Loading image '");
	Serial.print(filename);
	Serial.println('\'');

	// Open requested file on SD card
	if ((bmpFile = SD.open(filename)) == NULL) {
		Serial.print("File not found");
		return;
	}

	// Parse BMP header
	if(read16(bmpFile) == 0x4D42) { // BMP signature
		Serial.print("File size: "); Serial.println(read32(bmpFile));
		(void)read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
		// Read DIB header
		Serial.print("Header size: "); Serial.println(read32(bmpFile));
		bmpWidth  = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if(read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
			Serial.print("Bit Depth: "); Serial.println(bmpDepth);
			if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
				Serial.print("Image size: ");
				Serial.print(bmpWidth);
				Serial.print('x');
				Serial.println(bmpHeight);

				// BMP rows are padded (if needed) to 4-byte boundary
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if(bmpHeight < 0) {
					bmpHeight = -bmpHeight;
					flip      = false;
				}

				// Crop area to be loaded
				w = bmpWidth;
				h = bmpHeight;
				if((x+w-1) >= tft.width())  w = tft.width()  - x;
				if((y+h-1) >= tft.height()) h = tft.height() - y;

				// Set TFT address window to clipped image bounds
				tft.setAddrWindow(x, y, x+w-1, y+h-1);

				for (row=0; row<h; row++) { // For each scanline...
					if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
						pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
					else     // Bitmap is stored top-to-bottom
						pos = bmpImageoffset + row * rowSize;
					if(bmpFile.position() != pos) { // Need seek?
						bmpFile.seek(pos);
						buffidx = sizeof(sdbuffer); // Force buffer reload
					}

					for (col=0; col<w; col++) { // For each pixel...
						// Time to read more pixel data?
						if (buffidx >= sizeof(sdbuffer)) { // Indeed
							bmpFile.read(sdbuffer, sizeof(sdbuffer));
							buffidx = 0; // Set index to beginning
						}

						// Convert pixel from BMP to TFT format, push to display
						b = sdbuffer[buffidx++];
						g = sdbuffer[buffidx++];
						r = sdbuffer[buffidx++];
						tft.pushColor(tft.Color565(r,g,b));
					} // end pixel
				} // end scanline
				Serial.print("Loaded in ");
				Serial.print(millis() - startTime);
				Serial.println(" ms");
			} // end goodBmp
		}
	}

	bmpFile.close();
	if(!goodBmp) Serial.println("BMP format not recognized.");
}

// check which ship corresponds to the specified coordinate
ship correspondance(int x, int y) {
	for (int i = 0; i < 9; ++(++i)) {
		if ((x == carrier.location[i]) && (y == carrier.location[i+1])) {
			return carrier;
		}
		if ((x == battleship.location[i]) && (y == battleship.location[i+1])) {
			return battleship;
		}
		if ((x == submarine.location[i]) && (y == submarine.location[i+1]))	{
			return submarine;
		}
		if ((x == destroyer.location[i]) && (y == destroyer.location[i+1])) {
			return destroyer;
		}
		if ((x == patrol.location[i]) && (y == patrol.location[i+1])) {
			return patrol;
		}
	}
}

// print text on screen
void uitext(char* text) {
	tft.fillRect(0, 129, 128, 31, BLACK);
	tft.setCursor(0, 129);
	tft.print(text);
}

// determine which buffer to send depending on type of ship
void type(int x, int y) {
	ship valid = correspondance(x, y);
	if ((valid.location[0] == carrier.location[0]) && (valid.location[1] == carrier.location[1])) {
		carrier.count = carrier.count + 1;
		if (carrier.count == 5) {
			uitext("Your carrier has\nbeen destroyed");
			Serial3.write('5');
		}
		else {
			uitext("Your ship has\nbeen hit!");
			Serial3.write('H');
		}	
	}
	else if ((valid.location[0] == battleship.location[0]) && (battleship.location[1] == battleship.location[1])) {
		battleship.count = battleship.count + 1;
		if (battleship.count == 4) {
			uitext("Your battleship has\nbeen destroyed");
			Serial3.write('4');
		}
		else {
			uitext("Your ship has\nbeen hit!");
			Serial3.write('H');
		}	
	}
	else if ((valid.location[0] == submarine.location[0]) && (valid.location[1] == submarine.location[1])) {
		submarine.count = submarine.count + 1;
		if (submarine.count == 3) {
			uitext("Your submarine has\nbeen destroyed");
			Serial3.write('3');
		}
		else {
			uitext("Your ship has\nbeen hit!");
			Serial3.write('H');
		}
	}
	else if ((valid.location[0] == destroyer.location[0]) && (valid.location[1] == destroyer.location[1])) {
		destroyer.count = destroyer.count + 1;
		if (destroyer.count == 3) {
			uitext("Your destroyer has\nbeen destroyed");
			Serial3.write('2');
		}
		else {
			uitext("Your ship has\nbeen hit!");
			Serial3.write('H');
		}
	}
	else if ((valid.location[0] == patrol.location[0]) && (valid.location[1] == patrol.location[1])) {
		patrol.count = patrol.count + 1;
		if (patrol.count == 2) {
			uitext("Your patrol boat has\nbeen destroyed");
			Serial3.write('1');
		}
		else {
			uitext("Your ship has\nbeen hit!");
			Serial3.write('H');
		}	
	}
	else {
		uitext("Your ship has\nbeen hit!");
		Serial3.write('H');
	}	
}

// evaluate if W buffer has been recieved
bool eval() {
	int amount = Serial3.available();
	while (amount > 0) {
		if ((char)Serial3.read() == 'W') {
			return true;
		}
		amount = amount - 1;
	}
	return false;
}

// Read values from joystick and calculate change in cursor
void change_cursor(int max_X, int max_Y) {
	/* read values from the joystick */
	vertical = analogRead(JOYSTICK_VERT);		// will be 0-1023
	horizontal = analogRead(JOYSTICK_HORIZ);	// will be 0-1023
	select = digitalRead(JOYSTICK_BUTTON);		// HIGH if not pressed, LOW otherwise
		
	// change in cursor
	old_cursor_x = cursor_x;
	old_cursor_y = cursor_y;
	if (horizontal > init_joystick_horiz + 200) {
		cursor_x = cursor_x + 12;
		Serial3.write('R');
	}
	if (horizontal < init_joystick_horiz - 200) {
		cursor_x = cursor_x - 12;
		Serial3.write('L');
	}		
	if (vertical > init_joystick_vert + 200) {
		cursor_y = cursor_y + 12;
		Serial3.write('D');
	}
	if (vertical < init_joystick_vert - 200) {
		cursor_y = cursor_y - 12;
		Serial3.write('U');
	}
	cursor_x = constrain(cursor_x, 8, max_X);
	cursor_y = constrain(cursor_y, 9, max_Y);
	
	// print out the values for reference
	Serial.print("VER= ");
	Serial.print(vertical,DEC);
	Serial.print(" HOR= "); 
	Serial.print(horizontal,DEC);
	Serial.print(" BUT= ");
	Serial.print(select,DEC);
	Serial.print(" cursor_x: ");
	Serial.print(cursor_x);
	Serial.print(" cursor_y: ");
	Serial.println(cursor_y);
	delay(150);
	return;
}

// Returns true (1) if square is occupied by a ship
bool shipINsquare(int x, int y) {
	for (int i = 0; i < 9; ++(++i)) {
		if (((x == carrier.location[i]) && (y == carrier.location[i+1])) || 
				((x == battleship.location[i]) && (y == battleship.location[i+1])) ||
				((x == submarine.location[i]) && (y == submarine.location[i+1])) || 
				((x == destroyer.location[i]) && (y == destroyer.location[i+1])) ||
				((x == patrol.location[i]) && (y == patrol.location[i+1]))) {
			return 1;
		}
	}
	return 0;
}

// Check square for whether it was a hit or a miss. Returns 1 if it is.
bool check_square(int* array, int max_size, int x, int y) {
	for (int i = 0; i < max_size; ++(++i)) {
		if ((x == array[i]) && (y == array[i+1])) {
			return 1;
		}
	}
	return 0;
}

// reprint a square when cursor is moved (on offense)
void print_square_offense(int x, int y) {
	if (check_square(my_hits, 33, x, y)) {
		tft.fillRect(x, y, 11, 11, RED);
	}
	else if (check_square(my_misses, 165, x, y)) {
		tft.fillRect(x, y, 11, 11, WHITE);
	} 
	else {
		tft.fillRect(x, y, 11, 11, BLUE);
	}
}

// reprint a square when cursor is moved (on defense)
void print_square_defense(int x, int y) {
	ship valid;
	if (check_square(their_hits, 33, x, y)) {
		tft.fillRect(x, y, 11, 11, RED);
	}
	else if (check_square(their_misses, 165, x, y)) {
		tft.fillRect(x, y, 11, 11, WHITE);
	}
	else if (shipINsquare(x, y)) {
		valid = correspondance(x,y);
		if (!valid.orientation) {
			bmpDraw(valid.file_x, valid.location[0], valid.location[1]);
		}
		else {
			bmpDraw(valid.file_y, valid.location[0], valid.location[1]);
		}
		for (int i = 0; i < th; ++(++i)) {
			tft.fillRect(their_hits[i], their_hits[i+1], 11, 11, RED);
		}
	}
	else {
		tft.fillRect(x, y, 11, 11, BLUE);
	}
}

// reprint a square when cursor is moved (for placement)
void print_square(int x, int y) {
	if (shipINsquare(x, y)) {
		tft.fillRect(x, y, 11, 11, GREY);
	} else {tft.fillRect(x, y, 11, 11, BLUE);}
}

// Place a ship and store it's location
void placement(ship* p_ship) {
	cursor_x = 8, cursor_y = 9; // A1
	old_cursor_x = 8, old_cursor_y = 9;
	
	// draw initial position
	for (int i = 0; i < p_ship->size; ++i) {
		tft.fillRect(cursor_x + (i * 12), cursor_y, 11, 11, GREY);
	}
	
	while(true) { // Horizontal placement
		pushbutton = digitalRead(PUSHBUTTON);
		p_ship->orientation = false;
		// when the cursor is moved redraw a patch of the map and new cursor
		if ((old_cursor_x != cursor_x) || (old_cursor_y != cursor_y)) {
			for (int i = 0; i < p_ship->size; ++i) {
				print_square(old_cursor_x + (i * 12), old_cursor_y);
			}
			for (int i = 0; i < p_ship->size; ++i) {
				tft.fillRect(cursor_x + (i * 12), cursor_y, 11, 11, GREY);
			}
		}
		
		change_cursor(p_ship->max_x_h, max_y_h);
		
		if (pushbutton == 0) { // If user chose to place ship
			bool invalid = false;
			for (int i = 0; i < p_ship->size; ++i) { // check overlap
				if (shipINsquare(cursor_x + (i * 12), cursor_y)) {
					uitext("Error placing ship!\nOverlapping another\nship!");
					invalid = true;
					break;
				}
			}
			if (invalid) {continue;}
			for (int i = 0; i < (2*p_ship->size); ++(++i)) { // store location
				p_ship->location[i] = cursor_x + (i * 6);
				p_ship->location[i+1] = cursor_y;
			}
			return;
		}
		
		if (select == 0) { // If user chose to rotate ship
			for (int i = 0; i < p_ship->size; ++i) { // cover old location
				print_square(cursor_x + (i * 12), cursor_y);
			}
			if (cursor_y > p_ship->max_y_v) {cursor_y = p_ship->max_y_v;}
			old_cursor_y = cursor_y;
			for (int i = 0; i < p_ship->size; ++i) { // display new location
				tft.fillRect(cursor_x, cursor_y + (i * 12), 11, 11, GREY);
			}
			delay(150);
			while(true) { // Vertical placement
				pushbutton = digitalRead(PUSHBUTTON);
				p_ship->orientation = true;
				// when the cursor is moved redraw a patch of the map and new cursor
				if ((old_cursor_x != cursor_x) || (old_cursor_y != cursor_y)) {
					for (int i = 0; i < p_ship->size; ++i) {
						print_square(old_cursor_x, old_cursor_y  + (i * 12));
					}
					for (int i = 0; i < p_ship->size; ++i) {
						tft.fillRect(cursor_x, cursor_y + (i * 12), 11, 11, GREY);
					}
				}
				
				change_cursor(max_x_v, p_ship->max_y_v);
				
				if (pushbutton == 0) { // If user chose to place ship
					bool invalid = false;
					for (int i = 0; i < p_ship->size; ++i) { // check overlap
						if (shipINsquare(cursor_x, cursor_y + (i * 12))) {
							uitext("Error placing ship!\nOverlapping another\nship!");
							invalid = true;
							break;
						}
					}
					if (invalid) {continue;}
					for (int i = 0; i < (2*p_ship->size); ++(++i)) { // store location
						p_ship->location[i] = cursor_x;
						p_ship->location[i+1] = cursor_y + (i * 6);
					}
					return;
				}
				if (select == 0) { // If user chose to rotate ship
					for (int i = 0; i < p_ship->size; ++i) { // cover old location
						print_square(cursor_x, cursor_y + (i * 12));
					}
					if (cursor_x > p_ship->max_x_h) {cursor_x = p_ship->max_x_h;}
					old_cursor_x = cursor_x;
					for (int i = 0; i < p_ship->size; ++i) { // display new location
						tft.fillRect(cursor_x + (i * 12), cursor_y, 11, 11, GREY);
					}
					delay(150);
					break;
				}
			}
		}
	}
}

void attack() {	
	// print battlefield with your past hits and misses
	tft.fillRect(8, 9, 120, 120, BLACK);
	// draw the battlefield
	for (int j = 9; j <= 117;) {
		for (int i = 8; i <= 116;) {
			tft.fillRect(i, j, 11, 11, BLUE);
			i = i + 12;
		}
		j = j + 12;
	}
	for (int i = 0; i < mh; ++(++i)) {
		tft.fillRect(my_hits[i], my_hits[i+1], 11, 11, RED);
	}
	for (int i = 0; i < mm; ++(++i)) {
		tft.fillRect(my_misses[i], my_misses[i+1], 11, 11, WHITE);
	}
	
	cursor_x = 8, cursor_y = 9; // A1
	old_cursor_x = 8, old_cursor_y = 9;
	
	// draw initial position
	tft.fillRect(cursor_x, cursor_y, 11, 11, YELLOW);
	
	while(true) {		
		// when the cursor is moved redraw a patch of the map and new cursor
		if ((old_cursor_x != cursor_x) || (old_cursor_y != cursor_y)) {
			print_square_offense(old_cursor_x, old_cursor_y);
			tft.fillRect(cursor_x, cursor_y, 11, 11, YELLOW);
		}
		
		change_cursor(116, 117);
		
		if (select == 0) { // If user chose to fire!
			bool invalid = false;
			// check overlap
			if ((check_square(my_hits, 33, cursor_x, cursor_y)) ||
					(check_square(my_misses, 165, cursor_x, cursor_y))) {
				uitext("Error firing shot!\nSame location as\npast attack!");
				invalid = true;
			}
			if (invalid) {continue;}
			Serial3.write('F');
			int incomingByte = 0;
			int incomingByte1 = 0;
			while (true) {
				incomingByte = Serial3.read();
				if (incomingByte != -1) {
					if (incomingByte != 'M') {
						if (incomingByte == 'H') {
							tft.fillRect(cursor_x, cursor_y, 11, 11, RED);
							uitext("Enemy ship has\nbeen hit!");
						}
						else if (incomingByte == '5') {
							tft.fillRect(cursor_x, cursor_y, 11, 11, RED);
							uitext("Carrier has\nbeen destroyed!");
						}
						else if (incomingByte == '4') {
							tft.fillRect(cursor_x, cursor_y, 11, 11, RED);
							uitext("Battleship has\nbeen destroyed!");
						}
						else if (incomingByte == '3') {
							tft.fillRect(cursor_x, cursor_y, 11, 11, RED);
							uitext("Submarine has\nbeen destroyed!");
						}
						else if (incomingByte == '2') {
							tft.fillRect(cursor_x, cursor_y, 11, 11, RED);
							uitext("Destroyer has\nbeen destroyed");
						}
						else if (incomingByte == '1') {
							tft.fillRect(cursor_x, cursor_y, 11, 11, RED);
							uitext("Patrol boat has\nbeen destroyed!");
						}
						my_hits[mh] = cursor_x;
						my_hits[mh+1] = cursor_y;
						++(++mh);
					}
					if (incomingByte == 'M') {
						tft.fillRect(cursor_x, cursor_y, 11, 11, WHITE);
						uitext("Missed all ships!");
						my_misses[mm] = cursor_x;
						my_misses[mm+1] = cursor_y;
						++(++mm);
					}
					
					delay(1500);
					return;
				}
			}
		}
	}
}

void defend() {
	// print battlefield with your ships, hits, and misses
	for (int i = 0; i < mh; ++(++i)) {
		tft.fillRect(my_hits[i], my_hits[i+1], 11, 11, BLUE);
	}
	for (int i = 0; i < mm; ++(++i)) {
		tft.fillRect(my_misses[i], my_misses[i+1], 11, 11, BLUE);
	}

	if (!carrier.orientation) {
		bmpDraw(carrier.file_x, carrier.location[0], carrier.location[1]);
	}
	else {
		bmpDraw(carrier.file_y, carrier.location[0], carrier.location[1]);
	}
	if (!battleship.orientation) {
		bmpDraw(battleship.file_x, battleship.location[0], battleship.location[1]);
	}
	else {
		bmpDraw(battleship.file_y, battleship.location[0], battleship.location[1]);
	}
	if (!submarine.orientation) {
		bmpDraw(submarine.file_x, submarine.location[0], submarine.location[1]);
	}
	else {
		bmpDraw(submarine.file_y, submarine.location[0], submarine.location[1]);
	}
	if (!destroyer.orientation) {
		bmpDraw(destroyer.file_x, destroyer.location[0], destroyer.location[1]);
	}
	else {
		bmpDraw(destroyer.file_y, destroyer.location[0], destroyer.location[1]);
	}
	if (!patrol.orientation) {
		bmpDraw(patrol.file_x, patrol.location[0], patrol.location[1]);
	}
	else {
		bmpDraw(patrol.file_y, patrol.location[0], patrol.location[1]);
	}
	
	for (int i = 0; i < th; ++(++i)) {
		tft.fillRect(their_hits[i], their_hits[i+1], 11, 11, RED);
	}
	for (int i = 0; i < tm; ++(++i)) {
		tft.fillRect(their_misses[i], their_misses[i+1], 11, 11, WHITE);
	}
	
	
	// inital position
	cursor_x = 8, cursor_y = 9; // A1
	old_cursor_x = 8, old_cursor_y = 9;
	
	// draw initial position
	tft.fillRect(cursor_x, cursor_y, 11, 11, YELLOW);
	int incomingByte = 0;
	while(true) {		
		// when the cursor is moved redraw a patch of the map and new cursor
		incomingByte = Serial3.read();
		if (incomingByte != -1) {
			if (incomingByte == 'F') {
				if (shipINsquare(cursor_x, cursor_y)) {
					type(cursor_x, cursor_y);
					playHit();
					tft.fillRect(cursor_x, cursor_y, 11, 11, RED);
					their_hits[th] = cursor_x;
					their_hits[th+1] = cursor_y;
					++(++th);
				} else {
					Serial3.write('M');
					playMiss();
					tft.fillRect(cursor_x, cursor_y, 11, 11, WHITE);
					uitext("Enemy missed all\nof your ships!");
					their_misses[tm] = cursor_x;
					their_misses[tm+1] = cursor_y;
					++(++tm);
				}
				delay(1500);
				return;
			}
			
			old_cursor_x = cursor_x;
			old_cursor_y = cursor_y;
			
			if ((char)incomingByte == 'R') {
				cursor_x = cursor_x + 12;
			}
			if ((char)incomingByte == 'L') {
				cursor_x = cursor_x - 12;
			}
			if ((char)incomingByte == 'D') {
				cursor_y = cursor_y + 12;
			}
			if ((char)incomingByte == 'U') {
				cursor_y = cursor_y - 12;
			}
			cursor_x = constrain(cursor_x, 8, 116);
			cursor_y = constrain(cursor_y, 9, 117);
			
			if ((old_cursor_x != cursor_x) || (old_cursor_y != cursor_y)) {
				print_square_defense(old_cursor_x, old_cursor_y);
				tft.fillRect(cursor_x, cursor_y, 11, 11, YELLOW);
			}
		}
	}
}

bool wait_on_serial3( uint8_t nbytes, long timeout ) {
	unsigned long deadline = millis() + timeout; // wraparound not a problem
	while (Serial3.available()<nbytes && (timeout<0 || millis()<deadline)) 
	{
		delay(1); // be nice, no busy loop
	}
	return Serial3.available()>=nbytes;
}

// state machine
void data_exchange_client() {
	typedef enum {start, ack, init, wait, defense, offense, end} State;
	char* stateNames[] = {"start", "ack", "init", "wait", "defense", "offense", "end"};
	State state = start;
	int buffer, incomingByte;
	uitext("Waiting to connect...");
	while ( state != end ) {
		if( state == start ) {
			Serial3.write('C');
			state = ack;
		}	
		else if ( state == ack ) {
			if (( wait_on_serial3(1,1000) ) && ( (char)Serial3.read() == 'S' )) {
				Serial3.write('C');
				state = init;
			}
			else {
				state = start;
			}
		}
		else if ( state == init ) {
			// Place Carrier
			uitext("Please place your\nCarrier");
			placement(&carrier);
			delay(500);
			
			// Place Battleship
			uitext("Please place your\nBattleship");
			placement(&battleship);
			delay(500);	
			
			// Place Submarine
			uitext("Please place your\nSubmarine ");
			placement(&submarine);
			delay(500);
				
			// Place Destroyer
			uitext("Please place your\nDestroyer");
			placement(&destroyer);
			delay(500);
			
			// Place Patrol Boat
			uitext("Please place your\nPatrol Boat");
			placement(&patrol);
			delay(500);
			
			uitext("Placement Complete!\n");			
						
			if ( eval() ) {
				Serial3.write('W');
				state = defense;
			}
			else {
				state = wait;
			}		
		}
		else if ( state == wait ) {
			uitext("Waiting...");
			while ( (char)Serial3.read() != 'W' ) {
				Serial3.write('W');
			}
			state = offense;
		}
		else if ( state == defense ) {
			uitext("Defense\n");
			defend();
			if (their_hits[33] != 0) {
				state = end;
				uitext("You Lose!");
			} else {
				state = offense;
			}
		}
		else if ( state == offense) {
			uitext("Offense\n");
			attack();
			if (my_hits[33] != 0) {
				playWin();
				state = end;
				uitext("You Win!");
			} else {
				state = defense;
			}
		}
		Serial.print("Current State: "); Serial.println(stateNames[state]); 
	}
}

void data_exchange_server() {
	typedef enum {start, ack, init, wait, defense, offense, end} State;
	char* stateNames[] = {"start", "ack", "init", "wait", "defense", "offense", "end"};
	State state = start;
	int incomingByte;
	uitext("Waiting to connect...");
	while ( state != end ) {
		if (( state == start )&&( (char)Serial3.read() == 'C' )) {
			Serial3.write('S');
			state = ack;
		}
		else if ( state == ack ) {
			if (( wait_on_serial3(1,1000) ) && ( (char)Serial3.read() == 'C' )) {
				state = init;
			}
			else {
				state = start;
			} 
		}
		else if ( state == init ) {
			// Place Carrier
			uitext("Please place your\nCarrier");
			placement(&carrier);
			delay(500);
			
			// Place Battleship
			uitext("Please place your\nBattleship");
			placement(&battleship);
			delay(500);	
			
			// Place Submarine
			uitext("Please place your\nSubmarine ");
			placement(&submarine);
			delay(500);
				
			// Place Destroyer
			uitext("Please place your\nDestroyer");
			placement(&destroyer);
			delay(500);
			
			// Place Patrol Boat
			uitext("Please place your\nPatrol Boat");
			placement(&patrol);
			delay(500);
			
			uitext("Placement Complete!\n");
			
			if ( eval() ) {
				Serial3.write('W');
				state = defense;
			}
			else {
				state = wait;
			}
			
		} 
		else if ( state == wait ) {
			uitext("Waiting...");
			while ( (char)Serial3.read() != 'W' ) {
				Serial3.write('W');
			}
			state = offense;
		}
		else if ( state == defense ) {
			uitext("Defense");
			defend();
			if (their_hits[33] != 0) {
				state = end;
				uitext("You Lose!");
			} else {
				state = offense;
			}
		}
		else if ( state == offense) {
			uitext("Offense");
			attack();
			if (my_hits[33] != 0) {
				state = end;
				playWin();
				uitext("You Win!");
			} else {
				state = defense;
			}
		}
		Serial.print("Current State: "); Serial.println(stateNames[state]); 
	}
}

int main() {
	init();
	
	// Attach USB for applicable processors
	#ifdef USBCON
		USBDevice.attach();
	#endif
	
	Serial.begin(9600);
	Serial3.begin(9600);
	
	// Setup
	tft.initR(INITR_BLACKTAB);   // initialize a ST7735R chip, black tab
	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
	Serial.println("Failed!");
	while(true) {} // something is wrong
	}
	Serial.println("OK!");
	
	// Some more initialization 
	Serial.print("Doing raw initialization...");
	if (!card.init(SPI_HALF_SPEED, SD_CS)) {
		Serial.println("Failed!");
		while(true) {} // something is wrong
	} else {
		Serial.println("OK!");
	}
	
	/* Initialize Joystick */
	pinMode(JOYSTICK_BUTTON, INPUT);
	digitalWrite(JOYSTICK_BUTTON,HIGH);
	init_joystick_vert = analogRead(JOYSTICK_VERT);
	init_joystick_horiz = analogRead(JOYSTICK_HORIZ);
	
	/* Initialize Pushbutton */
	pinMode(PUSHBUTTON, INPUT);
	digitalWrite(PUSHBUTTON, HIGH);
	pinMode(SPEAKER, OUTPUT);
	
	tft.fillScreen(0); // fill screen with black
	tft.setTextWrap(false);
	tft.setTextColor(WHITE, BLACK); // white characters on black background
	
	// draw the numbers
	for (int i = 1; i <= 9; ++i) {
		tft.setCursor(i*12-1, 1); // where the characters will be displayed
		tft.print(i);
	}
	tft.setCursor(116, 1); tft.print(1);
	tft.setCursor(121, 1); tft.print(0);
	
	// draw the letters
	int i = 11;
	for (char c = 'A'; c < 'K'; ++c) {
			tft.setCursor(1, i); tft.print(c);
			i = i + 12;
	}
	
	// draw the battlefield
	for (int j = 9; j <= 117;) {
		for (int i = 8; i <= 116;) {
			tft.fillRect(i, j, 11, 11, BLUE);
			i = i + 12;
		}
		j = j + 12;
	}
	
	// define ship standards
	carrier.size = 5;
	carrier.max_x_h = 68;
	carrier.max_y_v = 69;
	carrier.file_x = "ship5_x.bmp";
	carrier.file_y = "ship5_y.bmp";
	carrier.count = 0;
	carrier.orientation = false;
	battleship.size = 4;
	battleship.max_x_h = 80;
	battleship.max_y_v = 81;
	battleship.file_x = "ship4_x.bmp";
	battleship.file_y = "ship4_y.bmp";
	battleship.count = 0;
	battleship.orientation = false;
	submarine.size = 3;
	submarine.max_x_h = 92;
	submarine.max_y_v = 93;
	submarine.file_x = "ship3_x.bmp";
	submarine.file_y = "ship3_y.bmp";
	submarine.count = 0;
	submarine.orientation = false;
	destroyer.size = 3;
	destroyer.max_x_h = 92;
	destroyer.max_y_v = 93;
	destroyer.file_x = "ship2_x.bmp";
	destroyer.file_y = "ship2_y.bmp";
	destroyer.orientation = false;
	patrol.size = 2;
	patrol.max_x_h = 104;
	patrol.max_y_v = 105;
	patrol.file_x = "ship1_x.bmp";
	patrol.file_y = "ship1_y.bmp";
	patrol.count = 0;
	patrol.orientation = false;	
	
	/* Initialize client/server */
	int Pin = 13;
	
	if (digitalRead(Pin) == 1) {
		data_exchange_server();
	}
	if (digitalRead(Pin) == 0) {
		data_exchange_client();
	}
	
	Serial.end();
	Serial3.end();
	return 0;
}
