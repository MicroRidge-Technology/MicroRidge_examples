#include "verilator_driver.hpp"
#include "Vdc_ram.h"

using namespace std::chrono_literals;
class dc_ram_test : public verilator_driver<Vdc_ram>{
    static constexpr ClockDriver::duration_t CLOCKA = 15ns;
    static constexpr ClockDriver::duration_t CLOCKB = 10ns;
    void tick_a(int ticks = 1) { update(CLOCKA * ticks); }
    void tick_b(int ticks=1){	update(CLOCKB*ticks);    }

public:
    dc_ram_test(int argc,char** argv):verilator_driver<Vdc_ram>(argc,argv){

	add_clock(ClockDriver([&](uint8_t clk){dut->clk_a=clk;},
			      CLOCKA));
	add_clock(ClockDriver([&](uint8_t clk){dut->clk_b=clk;},
			      CLOCKB));

	dut->we_a=0;
	dut->we_b=0;
        //printf("ran %ld ns\n", update().count());
	//printf("ran %ld ns\n", update().count());
	//printf("ran %ld ns\n",update().count());
	//printf("ran %ld ns\n", update().count());
	//printf("ran %ld ns\n", update().count());
	//printf("ran %ld ns\n",update(1ms).count());
	tick_a(10);
	//tick until rising edge of clock a;
	int last_a=dut->clk_a;
	while( ! (dut->clk_a==1  && last_a==0)){
	    last_a =dut->clk_a;
	    update();
	}
	debug(dut->contextp()->time());
	for(int i=0;i<(1<<6);++i){
	    dut->we_a=1;
	    dut->data_a=~i;
	    dut->addr_a = i;
	    tick_a();
	    dut->we_a=0;
	    tick_a();
	}
    }

};


int main(int argc,char** argv){

    dc_ram_test test(argc,argv);

    return 0;

}
