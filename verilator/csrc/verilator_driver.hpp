#ifndef VERILATOR_DRIVER_HPP
#define VERILATOR_DRIVER_HPP

#include <chrono>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <vector>
#include "verilated.h"
#include <verilated_fst_c.h>

#define debug(a) printf("%s:%d %s = %lld\n",__FILE__,__LINE__,#a,(long long)(a))

#define except_assert(test) do{\
	if( !(test)) {	    \
	    char str[1000];\
	    snprintf(str,900,"%s:%d Exception: assertion %s failed!", __FILE__, __LINE__, #test); \
	    throw std::runtime_error(str); \
	}				   \
    }while(0)


class ClockDriver{
public:
    using duration_t = std::chrono::nanoseconds;
    enum class edge_e { RISE_EDGE,FALL_EDGE,BOTH_EDGE};
private:
    duration_t  down_time,up_time;
    duration_t m_last_update;
     ///< Function to set the value of the clock net
    struct callback_t {
	std::function<void(edge_e)> fun;
	edge_e edge;
    };
    std::function<void(uint8_t)> clock_fun;
    std::vector<callback_t> callback_functions;
    int clk_val;
public:
    ClockDriver(std::function<void(uint8_t)> fun,duration_t period)
	:m_last_update(0),clk_val(0)
        {
	    up_time = period/2;
	    down_time = period -up_time;
	    clock_fun = fun;
	    clk_val=0;
	}
    void add_callback(std::function < void(edge_e)> fun, edge_e e=edge_e::RISE_EDGE) {

	callback_functions.push_back({fun,e});
    }
    duration_t next_update(){
	if(clk_val){
	    return m_last_update+up_time;
	}else{
	    return m_last_update+down_time;
	}
    }
    duration_t get_period(){
        return down_time+up_time;
    }
    duration_t last_update(){
	return m_last_update;
    }
    void update(duration_t now){
	clk_val=!clk_val;
	clock_fun(clk_val);
        m_last_update = now;
    }
    void exec_callbacks(){
	edge_e ignore = clk_val ? edge_e::FALL_EDGE : edge_e::RISE_EDGE;
	edge_e edge = clk_val ? edge_e::RISE_EDGE : edge_e::FALL_EDGE;
	for(auto &cb : callback_functions){
	    if (cb.edge != ignore){
		cb.fun(edge);
	    }
	}
    }
};
template <typename dut_t> class verilator_driver {
    using duration_t = ClockDriver::duration_t;
    std::vector<std::reference_wrapper<ClockDriver>> m_clocks;

    VerilatedContext* m_context;
    VerilatedFstC*  m_trace;
    duration_t sim_timeout;

protected:
    dut_t*  dut;


    verilator_driver(int argc,char** argv)
	{
	    char* waveform_file=NULL;
	    for(int a=1;a<argc;a++){
		char* idx=strstr(argv[a],"+verilator");
		if (idx==NULL){
		    //if +verilator not in arg, grab last arg and use it as waveform file
		    waveform_file = argv[a];
		}
	    }
	    m_context = new VerilatedContext;
	    m_context->commandArgs(argc, argv);
	    if(waveform_file){
		m_context->traceEverOn(true);
	    }

	    dut = new dut_t(m_context);
	    if(waveform_file){
		m_trace = new VerilatedFstC;
		dut->trace(m_trace,99);
		m_trace->open("test.fst");
	    }else{
		m_trace =NULL;
	    }
	    sim_timeout=std::chrono::milliseconds(10);
    }
    ~verilator_driver(){
	delete m_trace;
	delete dut;
	delete m_context;
    }
    void add_clock(ClockDriver& cd){
	m_clocks.push_back(cd);
    }
    duration_t update(){
        duration_t min_update = m_clocks.begin()->get().next_update();
	duration_t now=duration_t(m_context->time());
        for(auto &cd: m_clocks){
	    auto &c  = cd.get();
	    auto x = c.next_update();
            if(x < min_update)
                min_update= x;
        }

        dut->eval();
	for(auto &cd: m_clocks){
	    auto &c  = cd.get();
	    if(c.next_update() == min_update){
		c.update(min_update);
	    }
        }
	m_context->time(min_update.count());
	if(m_trace){
	    m_trace->dump(m_context->time());
	}
	dut->eval();
        for (auto &cd : m_clocks) {
	    auto &c  = cd.get();
	    if(c.last_update() == min_update){
		c.exec_callbacks();
	    }
	}
	if(sim_timeout != std::chrono::milliseconds(0) && now >= sim_timeout){
	    throw std::runtime_error("Simulation timed out\n");
	}
	return min_update-now;


    }
    duration_t run(duration_t run_time){
	duration_t ran_time=update();
	while(ran_time < run_time){
	    ran_time += update();
	}
	return ran_time;
    }
    template<typename pin_t>
    duration_t run_until_rising_edge(pin_t& pin, int bit=0){
	bool new_val = !!(pin &(1<<bit));
	bool last_val=new_val;
	duration_t ran_time(0);
	while( !( new_val==1  && last_val==0)){
	    last_val = new_val;
	    ran_time+=update();
	    new_val =!!(pin &(1<<bit));
	}
	return ran_time;
    }
};


#endif //VERILATOR_DRIVER_HPP
