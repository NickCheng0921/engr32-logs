SC_MODULE(CmdRouter) {
    sc_in<bool> sys_clk;
    sc_in<bool> rst_n;

    Connections::In<CmdReq> cmd_i;

    Connections::Combinational<CmdReq> custom_adder_mod_o;
    Connections::Combinational<CmdReq> custom_bitwise_mod_o;
    Connections::Combinational<CmdReq> custom_shiftxor_mod_o;
    Connections::Combinational<CmdReq> custom_multsat_mod_o;

    static const int ROUTER_FIFO_DEPTH = 16;

    sc_fifo<CmdReq, ROUTER_FIFO_DEPTH> adder_fifo;
    sc_fifo<CmdReq, ROUTER_FIFO_DEPTH> bitwise_fifo;
    sc_fifo<CmdReq, ROUTER_FIFO_DEPTH> shiftxor_fifo;
    sc_fifo<CmdReq, ROUTER_FIFO_DEPTH> multsat_fifo;

    // SC_THREADS process each command type separately to allow parallel processing

    SC_HAS_PROCESS(CmdRouter);
    CmdRouter(sc_module_name name_ = "CmdRouter") : sc_module(name_) {
        SC_THREAD(dispatch_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);

        SC_THREAD(adder_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);

        SC_THREAD(bitwise_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);

        SC_THREAD(shiftxor_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);

        SC_THREAD(multiplysat_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);
    }

    void dispatch_thread() {
        cmd_i.Reset();
        // SC_FIFOs reset in SC_THREAD that writes to them
        adder_fifo.reset_write();
        bitwise_fifo.reset_write();
        shiftxor_fifo.reset_write();
        multsat_fifo.reset_write();
        wait();

        while (1) {
            CmdReq req;
            if (cmd_i.PopNB(req)) {
                if (req.cmd_type[0] && !adder_fifo.full()) adder_fifo.push(req);
                else if (req.cmd_type[1] && !bitwise_fifo.full()) bitwise_fifo.push(req);
                else if (req.cmd_type[2] && !shiftxor_fifo.full()) shiftxor_fifo.push(req);
                else if (req.cmd_type[3] && !multsat_fifo.full()) multsat_fifo.push(req);
            }
            wait();
        }
    }

    void adder_thread() {
        custom_adder_mod_o.Reset();
        wait();

        while (1) {
            if (!adder_fifo.empty()) {
                CmdReq r = adder_fifo.pop();
                adder_mod_o.Push(r);
            }
            wait();
        }
    }

    void bitwise_thread() {
        custom_bitwise_mod_o.Reset();
        wait();

        while (1) {
            if (!bitwise_fifo.empty()) {
                CmdReq r = bitwise_fifo.pop();
                custom_bitwise_mod_o.Push(r);
            }
            wait();
        }
    }

    void shiftxor_thread() {
        custom_shiftxor_mod_o.Reset();
        wait();

        while (1) {
            if (!shiftxor_fifo.empty()) {
                CmdReq r = shiftxor_fifo.pop();
                custom_shiftxor_mod_o.Push(r);
            }
            wait();
        }
    }

    void multiplysat_thread() {
        custom_multsat_mod_o.Reset();
        wait();

        while (1) {
            if (!multsat_fifo.empty()) {
                CmdReq r = multsat_fifo.pop();
                custom_multsat_mod_o.Push(r);
            }
            wait();
        }
    }
};
