#include <reg51.h> 
#include <intrins.h> 

#define uchar unsigned char 
#define uint  unsigned int 

sbit DQ = P1^7;  //定义DS18B20端口DQ   
sbit BEEP=P3^6 ; //蜂鸣器驱动线 

bit  presence ; 

sbit LCD_RS = P2^6;              
sbit LCD_RW = P2^5; 
sbit LCD_EN = P2^7; 

uchar code  cdis1[ ] = {"   DS18B20 OK   "}; 
uchar code  cdis2[ ] = {"                "}; 
uchar code  cdis3[ ] = {" DS18B20  ERR0R "}; 
uchar code  cdis4[ ] = {"  PLEASE CHECK  "}; 

unsigned char data  display[2] = {0x00,0x00}; 
                                     
unsigned char data  RomCode[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; 

unsigned char Temp; 
unsigned char  crc=1; 

void beep(); 

#define delayNOP(); {_nop_();_nop_();_nop_();_nop_();}; 

/*******************************************************************/ 
void delay1(int ms) 
{ 
 unsigned char y; 
  while(ms--) 
 { 
  for(y = 0; y<250; y++) 
  { 
   _nop_(); 
   _nop_(); 
   _nop_(); 
   _nop_(); 
  } 
 } 
} 

/******************************************************************/ 
/*                                                                */ 
/*检查LCD忙状态                                                   */ 
/*lcd_busy为1时，忙，等待。lcd-busy为0时,闲，可写指令与数据。     */ 
/*                                                                */ 
/******************************************************************/  

bit lcd_busy() 
 {                           
    bit result; 
    LCD_RS = 0; 
    LCD_RW = 1; 
    LCD_EN = 1; 
    delayNOP(); 
    result = (bit)(P0&0x80); 
    LCD_EN = 0; 
    return(result);  
 } 

/*******************************************************************/ 
/*                                                                 */ 
/*写指令数据到LCD                                                  */ 
/*RS=L，RW=L，E=高脉冲，D0-D7=指令码。                             */ 
/*                                                                 */ 
/*******************************************************************/ 

void lcd_wcmd(uchar cmd) 

{                           
   while(lcd_busy()); 
    LCD_RS = 0; 
    LCD_RW = 0; 
    LCD_EN = 0; 
    _nop_(); 
    _nop_();  
    P0 = cmd; 
    delayNOP(); 
    LCD_EN = 1; 
    delayNOP(); 
    LCD_EN = 0;   
} 

/*******************************************************************/ 
/*                                                                 */ 
/*写显示数据到LCD                                                  */ 
/*RS=H，RW=L，E=高脉冲，D0-D7=数据。                               */ 
/*                                                                 */ 
/*******************************************************************/ 

void lcd_wdat(uchar dat) 
{                           
   while(lcd_busy()); 
    LCD_RS = 1; 
    LCD_RW = 0; 
    LCD_EN = 0; 
    P0 = dat; 
    delayNOP(); 
    LCD_EN = 1; 
    delayNOP(); 
    LCD_EN = 0;  
} 

/*******************************************************************/ 
/*                                                                 */ 
/*  LCD初始化设定                                                  */ 
/*                                                                 */ 
/*******************************************************************/ 

void lcd_init() 
{  
    delay1(15);    
    lcd_wcmd(0x01);      //清除LCD的显示内容 
             
    lcd_wcmd(0x38);      //16*2显示，5*7点阵，8位数据 
    delay1(5); 
    lcd_wcmd(0x38);          
    delay1(5); 
    lcd_wcmd(0x38);          
    delay1(5); 

    lcd_wcmd(0x0c);      //显示开，关光标 
    delay1(5); 
    lcd_wcmd(0x06);      //移动光标 
    delay1(5); 
    lcd_wcmd(0x01);      //清除LCD的显示内容 
    delay1(5); 
} 

/*******************************************************************/ 
/*                                                                 */ 
/*  设定显示位置                                                   */ 
/*                                                                 */ 
/*******************************************************************/ 

void lcd_pos(uchar pos) 
{                           
  lcd_wcmd(pos | 0x80);  //数据指针=80+地址变量 
} 

/*******************************************************************/ 
/*                                                                 */ 
/*us级延时函数                                                     */ 
/*                                                                 */ 
/*******************************************************************/ 

void Delay(unsigned int num) 
{ 
  while( --num ); 
} 

/*******************************************************************/ 
/*                                                                 */ 
/*初始化ds1820                                                     */ 
/*                                                                 */ 
/*******************************************************************/ 
Init_DS18B20(void) 
{   
     DQ = 1;      //DQ复位 
     Delay(8);    //稍做延时 

     DQ = 0;      //将DQ拉低 
     Delay(90);   //精确延时 大于 480us 

     DQ = 1;       //拉高总线 
     Delay(8); 

     presence = DQ;    //读取存在信号 
     Delay(100); 
     DQ = 1;  
      
     return(presence); //返回信号，0=presence,1= no presence 
} 

/*******************************************************************/ 
/*                                                                 */ 
/* 读一位（bit）                                                   */ 
/*                                                                 */ 
/*******************************************************************/ 
uchar read_bit(void)  
{ 
unsigned char i; 
DQ = 0;       //将DQ 拉低开始读时间隙 
DQ = 1;  // then return high 
for (i=0; i<3; i++);  // 延时15μs 
return(DQ);  // 返回 DQ 线上的电平值 
} 

/*******************************************************************/ 
/*                                                                 */ 
/* 读一个字节                                                      */ 
/*                                                                 */ 
/*******************************************************************/ 
 ReadOneChar(void) 
{ 
unsigned char i = 0; 
unsigned char dat = 0; 

//for (i = 8; i > 0; i--) 
//  { 
//    read_bit(); 
//    DQ = 0; // 给脉冲信号 
//     dat >>= 1; 
//    DQ = 1; // 给脉冲信号 
for (i=0;i<8;i++)  
{   // 读取字节，每次读取一个字节 
if(read_bit()) dat|=0x01<<i;    // 然后将其左移 

//    if(DQ) 
//     dat |= 0x80; 
    Delay(4); 
  } 

    return (dat); 
} 

/*******************************************************************/ 
/*                                                                 */ 
/* 写一位                                                          */ 
/*                                                                 */ 
/*******************************************************************/ 
void write_bit(char bitval) { 
DQ = 0;         // 将DQ 拉低开始写时间隙 
if(bitval==1) DQ =1;   // 如果写1，DQ 返回高电平 
Delay(5);        // 在时间隙内保持电平值， 
DQ = 1;               // Delay函数每次循环延时16μs，因此delay(5) = 104μs 
} 

/*******************************************************************/ 
/*                                                                 */ 
/* 写一个字节                                                      */ 
/*                                                                 */ 
/*******************************************************************/ 
 WriteOneChar(unsigned char dat) 
{ 
  unsigned char i = 0; 
  unsigned char temp; 
//  for (i = 8; i > 0; i--) 
//  { 
   for (i=0; i<8; i++)  // 写入字节, 每次写入一位  
   { 
//    DQ = 0; 
//    DQ = dat&0x01; 
//    Delay(5); 

//    DQ = 1; 
   temp = dat>>i;  
   temp &= 0x01;  
   write_bit(temp); 
//    dat>>=1; 
    
  } 
  Delay(5); 
} 

/*******************************************************************/ 
/*                                                                 */ 
/* 读取64位序列码                                                  */ 
/*                                                                 */ 
/*******************************************************************/ 
 Read_RomCord(void) 
{ 
     unsigned char j; 
     Init_DS18B20(); 
   
     WriteOneChar(0x33);  // 读序列码的操作 
     for (j = 0; j < 8; j++) 
 { 
  RomCode[j] = ReadOneChar() ;  
 } 
} 

/*******************************************************************/ 
/*                                                                 */ 
/*DS18B20的CRC8校验程序                                            */ 
/*                                                                 */ 
/*******************************************************************/ 
/*uchar CRC8()  
{  
   uchar i,x; uchar crcbuff; 
    
   crc=0; 
   for(x = 0; x <8; x++) 
   { 
    crcbuff=RomCode[x]; 
    for(i = 0; i < 8; i++)  
     {  
      if(((crc ^ crcbuff)&0x01)==0)  
      crc >>= 1;  
       else {  
              //crc ^= 0x18;   //CRC=X8+X5+X4+1 
              crc >>= 1; crc^=0x8c; 
              //crc |= 0x80;
			    
            }          
      crcbuff >>= 1;        
 } 
   } 
     return crc; 
}
*/
uchar calcrc_1byte(uchar abyte)
{
	uchar i, crc1=0;
	for(i=0; i<8; i++) {
		if((crc1^abyte)& 1) { crc1>>=1; crc1^=0x8c;}
		else crc1>>=1;
		abyte>>=1; }
	return(crc1);
}

uchar CRC8(uchar *p,uchar len)
{
	crc=0;
	while(len--) crc=calcrc_1byte(crc^*p++);
	return(crc);
 }
/*******************************************************************/ 
/*                                                                 */ 
/* 数据转换与显示                                                  */ 
/*                                                                 */ 
/*******************************************************************/ 

 Disp_RomCode() 
{ 
   uchar j; 
   uchar H_num=0x40;       //LCD第二行初始位置 

   for(j=0;j<8;j++) 
   { 
    Temp = RomCode[j]; 

    display[0]=((Temp&0xf0)>>4); 
    if(display[0]>9) 
     { display[0]=display[0]+0x37;} 
    else{display[0]=display[0]+0x30;} 

    lcd_pos(H_num);              
    lcd_wdat(display[0]);        //高位数显示  

    H_num++; 
    display[1]=(Temp&0x0f); 
    if(display[1]>9) 
     {display[1]=display[1]+0x37;} 
    else {display[1]=display[1]+0x30;} 

    lcd_pos(H_num);              
    lcd_wdat(display[1]);        //低位数显示  
    H_num++; 
   } 
}   

/*******************************************************************/ 
/*                                                                 */ 
/* 蜂鸣器响一声                                                    */ 
/*                                                                 */ 
/*******************************************************************/ 
void beep() 
  { 
    unsigned char y; 
    for (y=0;y<100;y++) 
    { 
      Delay(60); 
      BEEP=!BEEP;                //BEEP取反 
    }  
    BEEP=1;                      //关闭蜂鸣器 
Delay(40000); 
  } 

/*******************************************************************/ 
/*                                                                 */ 
/* DS18B20 OK 显示菜单                                             */ 
/*                                                                 */ 
/*******************************************************************/ 
void  Ok_Menu () 
{  
    uchar  m; 
    lcd_init();                //初始化LCD  
             
    lcd_pos(0);                //设置显示位置为第一行的第1个字符 
     m = 0; 
    while(cdis1[m] != '\0') 
     {                         //显示字符 
       lcd_wdat(cdis1[m]); 
       m++; 
     } 

    lcd_pos(0x40);             //设置显示位置为第二行第1个字符 
     m = 0; 
    while(cdis2[m] != '\0') 
     { 
       lcd_wdat(cdis2[m]);      //显示字符 
       m++; 
     } 
} 

/*******************************************************************/ 
/*                                                                 */ 
/* DS18B20 ERROR 显示菜单                                          */ 
/*                                                                 */ 
/*******************************************************************/ 
void  Error_Menu () 
{ 
     uchar  m; 
     lcd_init();                //初始化LCD  

    lcd_pos(0);                //设置显示位置为第一行的第1个字符 
     m = 0; 
     while(cdis3[m] != '\0') 
     {                         //显示字符 
       lcd_wdat(cdis3[m]); 
       m++; 
     } 

     lcd_pos(0x40);             //设置显示位置为第二行第1个字符 
     m = 0; 
     while(cdis4[m] != '\0') 
     { 
       lcd_wdat(cdis4[m]);      //显示字符 
       m++; 
     } 
} 

/*******************************************************************/ 
/*                                                                 */ 
/* 主函数                                                          */ 
/*                                                                 */ 
/*******************************************************************/ 
void main() 
 { 
     P0 = 0xff; 
     P2 = 0xff; 

   while(1) 
  { 
     Ok_Menu (); 
     Read_RomCord();    //读取64位序列码 
     CRC8(RomCode,8);            //CRC效验 
     if(crc==0)         //CRC效验正确 
 { 
  Disp_RomCode(); //显示64位序列码 
     beep(); 
     } 
     while(!presence) 
 { 
  Init_DS18B20(); 
  delay1(1000); 
 } 

     Error_Menu (); 
do 
 {   
   Init_DS18B20(); 
   beep(); 
     } 
while(presence); 
  } 
} 