```SystemC
SC_MODULE(L1Cache) {
    sc_in<bool> sys_clk;
    sc_in<bool> rst_n;

    Connections::In<CacheReq> cpu_req_i;
    Connections::Out<CacheResp> cpu_resp_o;

    Connections::Out<MemReq> mem_req_o;
    Connections::In<MemResp> mem_resp_i;

    static const int NUM_LINES = 256;

    struct CacheLine {
        ac_int<243, false> tag;
        ac_int<256, false> data;
        bool valid;
    };

    CacheLine cache[NUM_LINES];

    sc_signal<bool> miss_pending;
    sc_signal<ac_int<256, false>> miss_addr;

    SC_HAS_PROCESS(L1Cache);
    L1Cache(sc_module_name name_ = "L1Cache") : sc_module(name_) {
        SC_THREAD(cpu_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);

        SC_THREAD(mem_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);
    }

    void cpu_thread() {
        cpu_req_i.Reset();
        cpu_resp_o.Reset();
        wait();

        while (1) {
            CacheReq req;
            if (cpu_req_i.PopNB(req)) {
                ac_int<8, false> index = req.addr.slc<8>(5);
                ac_int<243, false> tag = req.addr.slc<243>(13);

                CacheLine line = cache[index];

                if (line.valid && line.tag == tag) {
                    CacheResp resp;
                    resp.valid = true;
                    resp.data = req.write ? req.data : line.data;

                    if (req.write) {
                        line.data = req.data;
                        MemReq mem_req;
                        mem_req.addr = req.addr;
                        mem_req_o.Push(mem_req);
                    }

                    cpu_resp_o.Push(resp);
                }
                else {
                    miss_addr.write(req.addr);
                    miss_pending.write(true);
                    CacheResp stall = {0, false};
                    cpu_resp_o.Push(stall);
                }
            }
            wait();
        }
    }

    void mem_thread() {
        mem_resp_i.Reset();
        mem_req_o.Reset();
        wait();

        while (1) {
            if (miss_pending.read()) {
                MemReq req;
                req.addr = miss_addr.read();
                mem_req_o.Push(req);

                MemResp mem_data;
                mem_resp_i.Pop(mem_data);

                ac_int<8, false> index = req.addr.slc<8>(5);
                ac_int<243, false> tag = req.addr.slc<243>(13);

                cache[index].valid = true;
                cache[index].tag = tag;
                cache[index].data = mem_data.data;

                miss_pending.write(false);
            }
            wait();
        }
    }
}
```