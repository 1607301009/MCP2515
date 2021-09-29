/** I2C.c */
#include <intrins.h>
#include <reg52.h>

#define I2CDelay() {_nop_();_nop_();_nop_();_nop_();}
sbit I2C_SCL = P2 ^ 1;
sbit I2C_SDA = P2 ^ 0;

/* 产生总线起始信号 */
void I2CStart() {
  I2C_SDA = 1; // 确保 SDA、SCL 都是高电平
  I2C_SCL = 1;
  I2CDelay();
  I2C_SDA = 0; // 拉低 SDA
  I2CDelay();
  I2C_SCL = 0; // 拉低 SCL
}

/* 产生总线停止信号 */
void I2CStop() {
  I2C_SCL = 0; // 确保 SDA、SCL 都是低电平
  I2C_SDA = 0;
  I2CDelay();
  I2C_SCL = 1; // 拉高 SCL
  I2CDelay();
  I2C_SDA = 1; // 拉高 SDA
  I2CDelay();
}

/* I2C总线写操作，dat-待写入字节，返回值-从机应答位的值 */
bit I2CWrite(unsigned char dat) {
  bit ack;            // 用于暂存响应位的值
  unsigned char mask; // 用于检测字节内某一位值的掩码

  /* 从高位至低位依次执行 */
  for (mask = 0x80; mask != 0; mask >>= 1) {
    /* 将该位的值输出至 SDA */
    if ((mask & dat) == 0)
      I2C_SDA = 0;
    else
      I2C_SDA = 1;

    I2CDelay();
    I2C_SCL = 1;      // 拉高 SCL
    I2CDelay();
    I2C_SCL = 0;      // 拉低 SCL，完成一个位周期
  }

  I2C_SDA = 1;        // 8 位数据发送完毕后，主设备释放 SDA 以检测从设备响应
  I2CDelay();
  I2C_SCL = 1;        // 拉高 SCL
  ack = I2C_SDA;      // 读取此时 SDA 的值，即从设备响应值
  I2CDelay();
  I2C_SCL = 0;        // 再次拉低 SCL 完成响应

  return (~ack);      // 应答值取反以符合日常逻辑，0 表示不存在/忙/写入失败，1 表示存在/空闲/写入成功
}

/* I²C 总线读操作，并且发送 NAK 非应答信号，返回读取到的字节 */
unsigned char I2CReadNAK() {
  unsigned char mask;
  unsigned char dat;
  I2C_SDA = 1;        // 确保主设备释放 SDA

  /* 从高位到低位依次进行 */
  for (mask = 0x80; mask != 0; mask >>= 1) {
    I2CDelay();
    I2C_SCL = 1;      // 拉高 SCL

    /* SDA 的值为 0 时，dat 对应位置清零，为 1 时对应位置 1 */
    if (I2C_SDA == 0)
      dat &= ~mask;
    else
      dat |= mask;

    I2CDelay();
    I2C_SCL = 0;      // 拉低 SCL 让从设备发送下一位
  }

  I2C_SDA = 1;        // 8 位数据发送完毕后拉高 SDA，发送非应答信号
  I2CDelay();
  I2C_SCL = 1;        // 拉高 SCL
  I2CDelay();
  I2C_SCL = 0;        // 拉低 SCL 完成非应答位

  return dat;
}

/* I²C 总线读操作，并且发送 ACK 应答信号，返回读取到的字节 */
unsigned char I2CReadACK() {
  unsigned char mask;
  unsigned char dat;
  I2C_SDA = 1;        // 确保主设备释放 SDA

  /* 从高位到低位依次进行 */
  for (mask = 0x80; mask != 0; mask >>= 1) {
    I2CDelay();
    I2C_SCL = 1;      // 拉高 SCL

    /* SDA 的值为 0 时，dat 对应位置清零，为 1 时对应位置 1 */
    if (I2C_SDA == 0)
      dat &= ~mask;
    else
      dat |= mask;

    I2CDelay();
    I2C_SCL = 0;      // 拉低 SCL 让从设备发送下一位
  }

  I2C_SDA = 0;        // 8 位数据发送完毕后拉高 SDA，发送应答信号
  I2CDelay();
  I2C_SCL = 1;        // 拉高 SCL
  I2CDelay();
  I2C_SCL = 0;        // 拉低 SCL 完成应答位

  return dat;
}


/* EEPROM 读取函数，参数 buf 是数据接收指针，参数 addr 是 EEPROM 中起始地址，参数 len 是待读取的长度 */
void E2Read(unsigned char *buf, unsigned char addr, unsigned char len) {
    /* 采用寻址操作查询当前是否可以进行读写操作 */
    do {
        I2CStart();
        /* 应答就跳出循环，非应答就进行下次查询 */
        if (I2CWrite(0x50 << 1)) {
            break;
        }
        I2CStop();
    } while (1);

    I2CWrite(addr);               // 写入起始地址
    I2CStart();                   // 发送重复启动信号
    I2CWrite((0x50 << 1) | 0x01); // 设备寻址，并设置后续为读操作

    /* 连续读取 len-1 个字节的数据 */
    while (len > 1) {
        *buf++ = I2CReadACK(); // 最后 1 个字节之前为读取操作 + 应答
        len--;
    }

    *buf = I2CReadNAK(); // 最后 1 个字节为读取操作 + 非应答
    I2CStop();
}

/* EEPROM 写入函数，参数 buf 是源数据指针，参数 addr 是 EEPROM 的起始地址，参数 len 是待写入的数据长度 */
void E2Write(unsigned char *buf, unsigned char addr, unsigned char len) {
    /* 等待上次写入操作完成 */
    while (len > 0) {
        /* 采用寻址操作查询当前是否可以进行读写操作 */
        do {
            I2CStart();

            /* 应答就跳出循环，非应答就进行下次查询 */
            if (I2CWrite(0x50 << 1)) {
                break;
            }

            I2CStop();
        } while (1);

        /* 采用页模式连续的写入多个字节 */
        I2CWrite(addr); // 写入起始地址

        while (len > 0) {
            I2CWrite(*buf++); // 写入一个字节数据
            len--;            // 待写入长度计数递减
            addr++;           // EEPROM 地址递增

            /* 通过检测低 3 位是否为零来判断检查地址是否到达了页边界 */
            if ((addr & 0x07) == 0) {
                break; // 如果到达了页边界，就跳出循环结束本次写操作
            }
        }
        I2CStop();
    }
}

