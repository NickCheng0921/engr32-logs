SC_MODULE(WritebackBuffer) {
    sc_in<bool> sys_clk;
    sc_in<bool> rst_n;

    Connections::In<WBReq> wb_req1_i;
    Connections::In<WBReq> wb_req2_i;
    Connections::Out<MemReq> mem_o;

    static const int BUF_IN_DEPTH = 8;

    sc_fifo<WBReq, BUF_IN_DEPTH> fifo1;
    sc_fifo<WBReq, BUF_IN_DEPTH> fifo2;

    SC_HAS_PROCESS(WritebackBuffer);
    WritebackBuffer(sc_module_name name_ = "WritebackBuffer") : sc_module(name_) {
        SC_THREAD(input1_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);

        SC_THREAD(input2_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);

        SC_THREAD(process_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);
    }

    void input1_thread() {
        wb_req1_i.Reset();
        fifo1.reset_write();
        wait();

        while (1) {
            WBReq in1;
            if (wb_req1_i.PopNB(in1) && !fifo1.full()) {
                fifo1.push(in1);
            }
            wait();
        }
    }

    void input2_thread() {
        wb_req2_i.Reset();
        fifo2.reset_write();
        wait();

        while (1) {
            WBReq in2;
            if (wb_req2_i.PopNB(in2) && !fifo2.full()) {
                fifo2.push(in2);
            }
            wait();
        }
    }

    void process_thread() {
        mem_o.Reset();
        fifo1.reset_read();
        fifo2.reset_read();
        wait();

        while (1) {
            if (!fifo1.empty() && !fifo2.empty()) {
                WBReq d1 = fifo1.pop();
                WBReq d2 = fifo2.pop();

                ac_int<256, false> lower = d1.data.slc<256>(0);
                ac_int<256, false> upper = d2.data.slc<256>(256);

                ac_int<512, false> combined;
                combined.set_slc(0, lower);
                combined.set_slc(256, upper);

                ac_int<512, false> final_data = combined ^ 0xDEADBEEF;

                MemReq out;
                out.data = final_data;
                mem_o.Push(out);
            }
            wait();
        }
    }
};
