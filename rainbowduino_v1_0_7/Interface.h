#ifndef INTERFACE_H
#define INTERFACE_H

//interface state
#define COMMAND_MODE 0
#define DATA_MODE 1

//commands
#define DISP_PRESET_PIC  0
#define DISP_CHAR        1
#define DISP_COLOR       2
#define SET_DOT          3
#define DISP_RANDOM      4
#define DISP_3D          5

#define CHANGE_TO_DATA   'D'

//used for serial interface 
// 115200 is default
//BluetoothBee runs at 38400
#define BAUDRATE  38400 // 115200

//used for I2C interface
#define I2C_ADDR  4


class Interface
{
  public:
  unsigned char serialState;
  
  public:
  Interface(void);
  
  void init(void);
  void initBluetoothBee(void);

  void process(void);
  void processSerial(void);
  void processI2C(void);
  
  private:
  void processCmd(void);
  void processData(void);
  unsigned char checkCmd(unsigned char cmd[7]);
  void resolveCmd(unsigned char cmd[7]);
  
};

#endif

