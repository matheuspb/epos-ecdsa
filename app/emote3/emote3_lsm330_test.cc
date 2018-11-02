#include <utility/ostream.h>
#include <periodic_thread.h>
#include <imu.h>

using namespace EPOS;

int main()
{
    OStream cout;
    cout << "EPOSMote III LSM330 Test" << endl;
    const unsigned int FREQUENCY = 120; // Hz
    const unsigned int PERIOD = ((1/FREQUENCY)*1000000);

    LSM330 * imu = new LSM330();
    imu->beginAcc();
    imu->beginGyro();
    while(true) {
        imu->measureAcc();
        cout << "Acceleration = (X="<<imu->acc_x<<",Y,="<<imu->acc_y<<",Z="<<imu->acc_z<<") g" << endl;
        imu->measureAngles();
        cout << "Angle rate = (X="<<imu->gyro_x<<",Y,="<<imu->gyro_y<<",Z="<<imu->gyro_z<<") rad/s" << endl;
        Alarm::delay(PERIOD);
    }

    return 0;
}
