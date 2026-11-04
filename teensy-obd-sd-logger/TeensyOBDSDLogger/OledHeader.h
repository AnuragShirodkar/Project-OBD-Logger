#pragma once
// Compact 3x5 header font: four-pixel advance leaves room for the full device ID.
inline uint16_t oledHeaderGlyph(char c){
  switch(c){
    case '0': return 0x7B6F;
    case '1': return 0x2C97;
    case '2': return 0x63E7;
    case '3': return 0x638E;
    case '4': return 0x5BC9;
    case '5': return 0x798E;
    case '6': return 0x39EF;
    case '7': return 0x7292;
    case '8': return 0x7BEF;
    case '9': return 0x7BCE;
    case 'O': return 0x7B6F;
    case 'B': return 0x6BAE;
    case 'D': return 0x6B6E;
    case '_': return 0x0007;
    case 'L': return 0x4927;
    case 'I': return 0x7497;
    case 'V': return 0x5B6A;
    case 'E': return 0x79A7;
    case 'W': return 0x5BFD;
    case 'A': return 0x2BED;
    case 'T': return 0x7492;
    case 'S': return 0x388E;
    case 'R': return 0x6BAD;
    case 'U': return 0x5B6F;
    case 'F': return 0x79A4;
    case 'N': return 0x5FFD;
    case '%': return 0x52A5;
    case '!': return 0x2482;
    case '-': return 0x01C0;
    case ' ': return 0x0000;
    default:return 0;
  }
}
inline void drawOledHeader(Adafruit_SSD1306 &display,const char *text,int x,int y,uint16_t color){
  for(;*text;++text,x+=4){
    const uint16_t bits=oledHeaderGlyph(*text);
    for(int row=0;row<5;++row)for(int col=0;col<3;++col)
      if(bits & (1u<<(14-row*3-col)))display.drawPixel(x+col,y+row,color);
  }
}
