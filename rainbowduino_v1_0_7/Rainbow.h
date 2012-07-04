#ifndef Rainbow_h
#define Rainbow_h


//=============================================
//MBI5168
#define SH_DIR_OE    DDRC
#define SH_DIR_SDI   DDRC
#define SH_DIR_CLK   DDRC
#define SH_DIR_LE    DDRC

#define SH_BIT_OE    0x08
#define SH_BIT_SDI   0x01
#define SH_BIT_CLK   0x02
#define SH_BIT_LE    0x04

#define SH_PORT_OE   PORTC
#define SH_PORT_SDI  PORTC
#define SH_PORT_CLK  PORTC
#define SH_PORT_LE   PORTC
//============================================
#define clk_rising  {SH_PORT_CLK&=~SH_BIT_CLK;SH_PORT_CLK|=SH_BIT_CLK;}
#define le_high     {SH_PORT_LE|=SH_BIT_LE;}
#define le_low      {SH_PORT_LE&=~SH_BIT_LE;}
#define enable_oe   {SH_PORT_OE&=~SH_BIT_OE;}
#define disable_oe  {SH_PORT_OE|=SH_BIT_OE;}

#define shift_data_1     {SH_PORT_SDI|=SH_BIT_SDI;}
#define shift_data_0     {SH_PORT_SDI&=~SH_BIT_SDI;}
//============================================
#define open_line0	{PORTB=0x04;}
#define open_line1	{PORTB=0x02;}
#define open_line2	{PORTB=0x01;}
#define open_line3	{PORTD=0x80;}
#define open_line4	{PORTD=0x40;}
#define open_line5	{PORTD=0x20;}
#define open_line6	{PORTD=0x10;}
#define open_line7	{PORTD=0x08;}
#define close_all_line	{PORTD&=~0xf8;PORTB&=~0x07;}
//============================================


//shift 1 bit to the rgb data driver MBI5168
void shift_1_bit(unsigned char LS);

//sweep the specific line,used in the Timer1 ISR
void flash_line(unsigned char line,unsigned char level);

//one line with 8 rgb data,so totally 8x3=24 bits
void shift_24_bit(unsigned char line,unsigned char level);

//open the specific line
void open_line(unsigned char line);
//===============================================
//0x0bgr
#define RED 0x000f
#define GREEN 0x00f0
#define BLUE 0x0f00
#define BLACK 0x0000
#define WHITE 0x0fff
#define VIOLET 0x0f0f
#define YELLOW 0x00ff
#define AQUA  0x0ff0
#define RANDOM 0x5e3

//diagonal type
#define  LEFT_BOTTOM_TO_RIGHT_TOP 0
#define  LEFT_TOP_TO_RIGHT_BOTTOM 1

//when change leds,other led state 
#define OTHERS_ON 1
#define OTHERS_OFF 0

//shift direction
#define LEFT   0
#define RIGHT  1
#define UP     2
#define DOWN   3


class Rainbow
{
  public:
  //used for receiving color data from serial port,they can be 
  unsigned short receiveBuffer[8][8];
  
  public:
  Rainbow(void);
 
  void init(void);//invoke initIO and initTimer1
  void initIO(void);//initialize IO for controlling the leds            
  void initTimer1(void);//initialize Timer1 to 100us overflow 
  
  void closeAll();//close all leds
  void closeOneLine(unsigned char line);//close one line
  void closeOneColumn(unsigned char column);//close one column
  void closeOneDot(unsigned char line, unsigned char column);//close specific dot
  void closeOneDot(unsigned char x, unsigned char y, unsigned char z); // close one dot in 4x4x4 cube
  void closeOneDiagonal(unsigned char line, unsigned char type);//close one diagonal line
  
  void lightAll(unsigned short colorData);//light all with one color
  void lightAll(unsigned short colorData[8][8]);//light all with matrix data
  void lightAll(void);//light all with receiveBuffer
  
  void lightOneLine(unsigned char line, unsigned short color,unsigned char othersState);//only light one line with one color
  void lightOneLine(unsigned char line, unsigned short color[8],unsigned char othersState);//only light one line with 8 colors
  
  void lightOneColumn(unsigned char column, unsigned short color,unsigned char othersState);//only light one column with one color
  void lightOneColumn(unsigned char column, unsigned short color[8],unsigned char othersState);//only light one column with 8 colors
  void lightOneColumn(unsigned char column, unsigned short color[8][8],unsigned char othersState);//only light one column with receiveBuffer 8 colors
  
  void lightOneDot(unsigned char line, unsigned char column, unsigned short color,unsigned char othersState);//only light one dot at specific position
  void lightOneDot(unsigned char x, unsigned char y, unsigned char z, unsigned short color,unsigned char othersState);//only light one dot at specific position
  
  // light dots, lines, plane based on coords, 0xff is wild card meaing light all on that axis
  void light3D(unsigned char x,unsigned char y, unsigned char z, unsigned short color,unsigned char othersState);
  
  void lightOneDiagonal(unsigned char line, unsigned char type, unsigned short color,unsigned char othersState);//only light one diagonal line  with one color
  void lightOneDiagonal(unsigned char line, unsigned char type, unsigned short *color,unsigned char othersState);//only light one diagonal line with a number of colors
  void lightOneDiagonal(unsigned char line, unsigned char type, unsigned short color[8][8],unsigned char othersState);//only light one diagonal line with receiveBuffer colors
    
  void shiftPic(unsigned char shift,unsigned short colorData[8][8]);//shift pic  
  void dispPresetPic(unsigned char shift,unsigned char index);//disp picture preset in the flash with specific index and shift position
  void dispChar(unsigned char ASCII,unsigned short color,unsigned char shift);//disp character with specific shift position
  void dispColor(unsigned short color);//disp specific color
  
  void fillColorBuffer(unsigned char color);//fullfill one color(16bits) with conitues color data(8bits)
  void fillColorBuffer(unsigned short colorMatrix[8][8]);//fullfill the whole color buffer 
  
};

#endif

