// EPOS Cortex IMU/LSM330 Mediator Declarations

#ifndef __cortex_lsm330_h__
#define __cortex_lsm330_h__

#include <utility/math.h>
#include <i2c.h>
#include <alarm.h>
#include <machine.h>
#include <cpu.h>
#include <imu.h>

__BEGIN_SYS

class LSM330
{
private:

    enum {
        //Slave adresses
        I2C_ACC_ADDR        = 0x1D, // The 7-bit base slave address for the linear aceleration sensor
        I2C_GYRO_ADDR       = 0x6B, // The 7-bit base slave address for the angular rate sensor (0x6B)

        //Accelerometer registers
        CTRL_REG5_A         = 0x20, // Register to select power mode for the acc
        CTRL_REG7_A         = 0x25, // Register to select general configuration for the acc
        FIFO_CTRL_REG_A     = 0x2E, // Control register for FIFO

        //Configurations for the accelerometer (To do: include all possible commands)
        POWER_MODE_100      = 0x67, // Enable X, Y and Z axis, power on mode and output data rate 100 Hz
        POWER_MODE_1600     = 0x97, // Enable X, Y and Z axis, power on mode and output data rate 1600 Hz
        FIFO_ENABLE         = 0x40, // Enable FIFO setting the FIFO_EN bit on register CTRL_REG7_A
        STREAM_MODE         = 0x40, // Enable Strean Mode for FIFO on register FIFO_CTRL_REG_A

        //Gyroscope registers
        CTRL_REG1_G         = 0x20, // Register to select power mode for the gyro
        CTRL_REG5_G         = 0x24, // Register to enable FIFO
        FIFO_CTRL_REG_G     = 0x2E, // Control register for FIFO

        //Configurations for the gyroscope
        POWER_MODE_95       = 0x0F, // Output data rate of 95 Hz and Bandwidth of  12.5Hz
        POWER_MODE_760      = 0xFF, // Output data rate of 760 Hz and Bandwidth of 100 Hz

        //Data registers
        REG_OUT_X_L         = 0x28, // OUT_X_L_A
        REG_OUT_X_H         = 0x29, // OUT_X_H_A
        REG_OUT_Y_L         = 0x2A, // OUT_Y_L_A
        REG_OUT_Y_H         = 0x2B, // OUT_Y_H_A
        REG_OUT_Z_L         = 0x2C, // OUT_Z_L_A
        REG_OUT_Z_H         = 0x2D, // OUT_Z_H_A
    };

public:
    LSM330() : _i2c(I2C_Common::Role::MASTER, 'B', 1, 'B', 0) {} //master, SDA = PB1, SCL = PB0

    ~LSM330(){}

    void beginAcc(){
        char config[2];

        /*************** Configurations for the accelerometer ***************/
        //Configure power mode
        config[0] = CTRL_REG5_A;
        config[1] = POWER_MODE_100;
        _i2c.put(I2C_ACC_ADDR, config, 2, true);
        Alarm::delay(50);
        //Read CTRL_REG5_A register
        _i2c.put(I2C_ACC_ADDR, CTRL_REG5_A);
        Alarm::delay(50);

        //Enable FIFO
        config[0] = CTRL_REG7_A;
        config[1] = FIFO_ENABLE;
        _i2c.put(I2C_ACC_ADDR, config, 2, true);
        Alarm::delay(50);
        //Read CTRL_REG7_A register
        _i2c.put(I2C_ACC_ADDR, CTRL_REG7_A);
        Alarm::delay(50);

        //Enable FIFO Stream Mode
        config[0] = FIFO_CTRL_REG_A;
        config[1] = STREAM_MODE;
        _i2c.put(I2C_ACC_ADDR, config, 2, true);
        Alarm::delay(50);
        //Read FIFO_CTRL_REG_A register
        _i2c.put(I2C_ACC_ADDR, FIFO_CTRL_REG_A);
        Alarm::delay(50);
    }

    void beginGyro(){
        char config[2];

        /********************* Configurations gyroscope *********************/
        //Configure power mode
        config[0] = CTRL_REG1_G;
        config[1] = POWER_MODE_95;
        _i2c.put(I2C_GYRO_ADDR, config, 2, true);
        Alarm::delay(50);
        //Read CTRL_REG5_A register
        _i2c.put(I2C_GYRO_ADDR,CTRL_REG1_G);
        Alarm::delay(50);

        //Enable FIFO
        config[0] = CTRL_REG5_G;
        config[1] = FIFO_ENABLE;
        _i2c.put(I2C_GYRO_ADDR, config, 2, true);
        Alarm::delay(50);
        //Read CTRL_REG7_A register
        _i2c.put(I2C_GYRO_ADDR, CTRL_REG5_G);
        Alarm::delay(50);

        //Enable FIFO Stream Mode
        config[0] = FIFO_CTRL_REG_G;
        config[1] = STREAM_MODE;
        _i2c.put(I2C_GYRO_ADDR, config, 2, true);
        Alarm::delay(50);
        //Read FIFO_CTRL_REG_A register
        _i2c.put(I2C_GYRO_ADDR,  FIFO_CTRL_REG_G);
        Alarm::delay(50);

    }

    bool measureAcc() {
        char dataXLSB[2]={};
        char dataYLSB[2]={};
        char dataZLSB[2]={};
        char dataXMSB[2]={};
        char dataYMSB[2]={};
        char dataZMSB[2]={};

        int ax, ay, az;

        /* Reads 16 bits of data, 2 for each axel (the value is expressed as two's complement) */
        //lsb X axel - reg (0x28)
        _i2c.put(I2C_ACC_ADDR, REG_OUT_X_L);
        Alarm::delay(5);
        _i2c.get(I2C_ACC_ADDR, dataXLSB);               //i2c->get(I2C_ACC_ADDR, dataXLSB, 2);
        Alarm::delay(5);
        if(dataXLSB == 0)
            return false;

        //msb X axel - reg(0x29)
        _i2c.put(I2C_ACC_ADDR, REG_OUT_X_H);
        Alarm::delay(5);
        _i2c.get(I2C_ACC_ADDR, dataXMSB);              //_i2c.get(I2C_ACC_ADDR, dataXMSB, 2);
        Alarm::delay(5);
        if(dataXMSB == 0)
            return false;

        //lsb Y axel - reg (0x2A)
        _i2c.put(I2C_ACC_ADDR, REG_OUT_Y_L);
        Alarm::delay(5);
        _i2c.get(I2C_ACC_ADDR, dataYLSB);             //_i2c.get(I2C_ACC_ADDR, dataYLSB, 2);
        Alarm::delay(5);
        if(dataYLSB == 0)
            return false;

        //msb Y axel - reg (0x2B)
        _i2c.put(I2C_ACC_ADDR, REG_OUT_Y_H);
        Alarm::delay(5);
        _i2c.get(I2C_ACC_ADDR, dataYMSB);             //_i2c.get(I2C_ACC_ADDR, dataYMSB, 2);
        Alarm::delay(5);
        if(dataYMSB == 0)
            return false;

        //lsb Z axel - reg (0x2C)
        _i2c.put(I2C_ACC_ADDR, REG_OUT_Z_L);
        Alarm::delay(5);
        _i2c.get(I2C_ACC_ADDR, dataZLSB);            //_i2c.get(I2C_ACC_ADDR, dataZLSB, 2);
        Alarm::delay(5);
        if(dataZLSB == 0)
            return false;

        //msb Z axel - reg (0x2D)
        _i2c.put(I2C_ACC_ADDR, REG_OUT_Z_H);
        Alarm::delay(5);
        _i2c.get(I2C_ACC_ADDR, dataZMSB);            //_i2c.get(I2C_ACC_ADDR, dataZMSB, 2);
        Alarm::delay(5);
        if(dataZMSB == 0)
            return false;


        ax = ((int)dataXMSB[0] << 8) | (int)dataXLSB[0];
        ay = ((int)dataYMSB[0] << 8) | (int)dataYLSB[0];
        az = ((int)dataZMSB[0] << 8) | (int)dataZLSB[0];

        /* Convert the two's complement value */
        if (ax > 32767) {
            ax-= 65536;
        }
        if (ay > 32767) {
            ay -= 65536;
        }
        if (az > 32767) {
            az -= 65536;
        }

        /* Data conversion:
            -Aceleration range +-2g
            -Linear aceleration sensitivity 0.061 mg/digit
        */
        acc_x = ((float)ax * 0.000061);
        acc_y = ((float)ay * 0.000061);
        acc_z = ((float)az * 0.000061);

        return true;
    }

    bool measureAngles() {

        char dataXLSB[2] = {};
        char dataYLSB[2] = {};
        char dataZLSB[2] = {};
        char dataXMSB[2] = {};
        char dataYMSB[2] = {};
        char dataZMSB[2] = {};

        int gx, gy, gz;
        float deltaGx, deltaGy, deltaGz;

        /*Reads 6 bits of data, 2 for each axel (the value is expressed as two's complement)*/
        //lsb X axel - reg (0x28)
        _i2c.put(I2C_GYRO_ADDR, REG_OUT_X_L);
        Alarm::delay(5);
        _i2c.get(I2C_GYRO_ADDR, dataXLSB);
        Alarm::delay(5);
        if(dataXLSB == 0)
            return false;

        //msb X axel - reg(0x29)
        _i2c.put(I2C_GYRO_ADDR, REG_OUT_X_H);
        Alarm::delay(5);
        _i2c.get(I2C_GYRO_ADDR, dataXMSB);
        Alarm::delay(5);
        if(dataXMSB == 0)
            return false;

        //lsb Y axel - reg (0x2A)
        _i2c.put(I2C_GYRO_ADDR, REG_OUT_Y_L);
        Alarm::delay(5);
        _i2c.get(I2C_GYRO_ADDR, dataYLSB);
        Alarm::delay(5);
        if(dataYLSB == 0)
            return false;

        //msb Y axel - reg (0x2B)
        _i2c.put(I2C_GYRO_ADDR, REG_OUT_Y_H);
        Alarm::delay(5);
        _i2c.get(I2C_GYRO_ADDR, dataYMSB);
        Alarm::delay(5);
        if(dataYMSB == 0)
            return false;

        //lsb Z axel - reg (0x2C)
        _i2c.put(I2C_GYRO_ADDR, REG_OUT_Z_L);
        Alarm::delay(5);
        _i2c.get(I2C_GYRO_ADDR, dataZLSB);
        Alarm::delay(5);
        if(dataZLSB == 0)
            return false;

        //msb Z axel - reg (0x2D)
        _i2c.put(I2C_GYRO_ADDR, REG_OUT_Z_H);
        Alarm::delay(5);
        _i2c.get(I2C_GYRO_ADDR, dataZMSB);
        Alarm::delay(5);
        if(dataZMSB == 0)
            return false;


        gx = ((int)dataXMSB[0] << 8) | (int)dataXLSB[0];
        gy = ((int)dataYMSB[0] << 8) | (int)dataYLSB[0];
        gz = ((int)dataZMSB[0] << 8) | (int)dataZLSB[0];

        /*Convert the two's complement value*/
        if (gx > 32767) {
            gx -= 65536;
        }
        if (gy > 32767) {
            gy -= 65536;
        }
        if (gz > 32767) {
            gz -= 65536;
        }

        /* Data conversion:
            -Angular rate range +-250dps
            -Sensitivity 0.00875 dps/digit
        */
        gyro_x = deg2rad((float)gx * 0.00875);
        gyro_y = deg2rad((float)gy * 0.00875);
        gyro_z = deg2rad((float)gz * 0.00875);

        return true;
    }

public:
    float acc_x;
    float acc_y;
    float acc_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;

private:
    I2C _i2c;
};

__END_SYS

#endif