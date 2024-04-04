#include "verilator_driver.hpp"
#include "Vdc_ram.h"

using namespace std::chrono_literals;
class dc_ram_test : public verilator_driver<Vdc_ram>{
    static constexpr ClockDriver::duration_t CLOCKA = 15ns;
    static constexpr ClockDriver::duration_t CLOCKB = 10ns;
    void tick_a(int ticks = 1) {
	while(ticks--){
	    run_until_rising_edge(dut->clk_a);
	}
    }
    void tick_b(int ticks=1){
	while(ticks--){
	    run_until_rising_edge(dut->clk_b);
	}
    }
public:
    dc_ram_test(int argc,char** argv):verilator_driver<Vdc_ram>(argc,argv){

	add_clock(ClockDriver([&](uint8_t clk){dut->clk_a=clk;},
			      CLOCKA));
	add_clock(ClockDriver([&](uint8_t clk){dut->clk_b=clk;},
			      CLOCKB));

	dut->we_a=0;
	dut->we_b=0;
	tick_a(10);

	run_until_rising_edge(dut->clk_a);

	int ram_depth = (1<<6);

        /* write the ram on port a
         * read back on port a, verify contents
         * read back on port b, verify contents
	 *
	 * write the ram on port b
	 * read back on port b, verify contents
	 * read back on port a, verify contents
	 */
	for(int i=0;i<ram_depth;++i){
	    dut->we_a=1;
	    dut->data_a=~i;
	    dut->addr_a = i;
	    tick_a();
	    dut->we_a=0;
            tick_a();
	}

	for(uint8_t i=0;i<ram_depth;++i){
	    dut->addr_a = i;
	    uint8_t expected_val = ~i;
	    except_assert(dut->q_a != expected_val);
            tick_a();
	    except_assert(dut->q_a == expected_val);
	}
	dut->addr_b = 17;
	tick_b(10);

	for(uint8_t i=0;i<ram_depth;++i){
	    dut->addr_b = i;
	    uint8_t expected_val = ~i;
	    except_assert(dut->q_b != expected_val);
            tick_b();
	    except_assert(dut->q_b == expected_val);
	}

	/*write on port b*/
	for(int i=0;i<ram_depth;++i){
	    dut->we_b=1;
	    dut->data_b=~i +7;
	    dut->addr_b = i;
	    tick_b();
	    dut->we_b=0;
            tick_b();
	}

	for(uint8_t i=0;i<ram_depth;++i){
	    dut->addr_b = i;
	    uint8_t expected_val = ~i +7 ;
	    except_assert(dut->q_b != expected_val);
            tick_b();
	    except_assert(dut->q_b == expected_val);
	}

	tick_a(10);
	for(uint8_t i=0;i<ram_depth;++i){
	    dut->addr_a = i;
	    uint8_t expected_val = ~i +7 ;
	    except_assert(dut->q_a != expected_val);
            tick_a();
	    except_assert(dut->q_a == expected_val);
	}


    }

};


int main(int argc,char** argv){

    try{
	dc_ram_test test(argc,argv);
    }catch(std::exception &e){
	printf("Test Failed:\n\t%s\n",e.what());
    }
    return 0;

}
