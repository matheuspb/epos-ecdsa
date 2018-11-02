#include <tstp.h>
#include <gpio.h>
#include <machine.h>
#include <smart_data.h>
#include <transducer.h>
#include <utility/ostream.h>

using namespace EPOS;

const RTC::Microsecond INTEREST_EXPIRY = 1000000;
const RTC::Microsecond INTEREST_PERIOD = INTEREST_EXPIRY / 2;

int main()
{
    OStream cout;

    cout << "main()" << endl;

    TSTP::Time t = TSTP::now();
    TSTP::epoch(t);

    // //TSTP config
    // TSTP::Coordinates center_hu1_node1(100,100,100);
    // TSTP::Coordinates center_hu1_node2(-4900,-200,-6100);
    // TSTP::Coordinates center_hu2_node1(5500,-100,7000);
    // TSTP::Coordinates center_hu2_node2(4100,5600,-3000);

    TSTP::Coordinates center_test(10,0,0);

    // TSTP::Region region_hu1_node1(center_hu1_node1, 0, 0, -1);
    // TSTP::Region region_hu1_node2(center_hu1_node2, 0, 0, -1);
    // TSTP::Region region_hu2_node1(center_hu2_node1, 0, 0, -1);
    // TSTP::Region region_hu2_node2(center_hu2_node2, 0, 0, -1);

    TSTP::Region region_test(center_test, 0, 0, -1);

    //Smartdata
    // Water_Flow_M170 flow_hu1_gateway(0, INTEREST_EXPIRY, static_cast<Water_Flow_M170::Mode>(Water_Flow_M170::PRIVATE | Water_Flow_M170::CUMULATIVE));
    // Water_Flow_WSTAR flow_hu1_node1(region_hu1_node1, INTEREST_EXPIRY, INTEREST_PERIOD, Water_Flow_WSTAR::CUMULATIVE);
    // Water_Flow_M170 flow_hu1_node2(region_hu1_node2, INTEREST_EXPIRY, INTEREST_PERIOD, Water_Flow_M170::CUMULATIVE);

    // Water_Flow flow_hu2_gateway(0, INTEREST_EXPIRY, static_cast<Water_Flow::Mode>(Water_Flow::PRIVATE | Water_Flow::CUMULATIVE));
    // Water_Flow flow_hu2_node1(region_hu2_node1, INTEREST_EXPIRY, INTEREST_PERIOD, Water_Flow::CUMULATIVE);
    // Water_Flow flow_hu2_node2(region_hu2_node2, INTEREST_EXPIRY, INTEREST_PERIOD, Water_Flow::CUMULATIVE);

    //Test
    Water_Flow_WSTAR flow(region_test, INTEREST_EXPIRY, INTEREST_PERIOD, Water_Flow_M170::CUMULATIVE);

    while(1)
    {
        Machine::delay(INTEREST_EXPIRY);
        cout << "----- NEW ITERATION -----" << endl;

        //HU1
        // Water_Flow_M170::DB_Record r0 = flow_hu1_gateway.db_record();
        // cout << "r0 = " << r0 << endl;
        // Water_Flow_WSTAR::DB_Record r1 = flow_hu1_node1.db_record();
        // cout << "r1 = " << r1 << endl;
        // Water_Flow_M170::DB_Record r2 = flow_hu1_node2.db_record();
        // cout << "r2 = " << r2 << endl;

        //HU2
        // Water_Flow::DB_Record r0 = flow_hu2_gateway.db_record();
        // cout << "r0 = " << r0 << endl;
        // Water_Flow::DB_Record r1 = flow_hu2_node1.db_record();
        // cout << "r1 = " << r1 << endl;
        // Water_Flow::DB_Record r2 = flow_hu2_node2.db_record();
        // cout << "r2 = " << r2 << endl;

        Water_Flow::DB_Record r0 = flow.db_record();
        cout << "r0 = " << r0 << endl;

        cout << endl;
    }

    return 0;
}
