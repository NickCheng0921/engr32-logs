SC_MODULE(SegmentOverflow) {
    sc_in<bool> sys_clk;
    sc_in<bool> rst_n;

    Connections::In<Struct_A> data_i;
    Connections::Out<Struct_B> data_o;

    SC_HAS_PROCESS(SegmentOverflow);
    SegmentOverflow(sc_module_name name_ = "SegmentOverflow") : sc_module(name_)
    {
        SC_THREAD(segment_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);
    }

    void segment_thread() {
        data_i.Reset();
        data_o.Reset();
        wait();

        while (1) {
            Struct_A input;
            if (data_i.PopNB(input)) {
                Struct_B output;
                ac_int<64, false> lo = input.data.slc<64>(0);
                ac_int<64, false> hi = input.data.slc<64>(64);
                ac_int<65, false> result = (ac_int<65, false>)lo + hi; // to check for overflow, we extend lo

                output.status[0] = result[64];          // overflow
                output.status[1] = (lo == hi) ? 1 : 0;    // equality
                output.status[2] = 1;                    // valid result
                data_o.Push(output);
            }
            wait();
        }
    }
};
