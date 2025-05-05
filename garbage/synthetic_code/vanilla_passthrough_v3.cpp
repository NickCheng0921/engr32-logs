SC_MODULE(PassThrough) {
    sc_in<bool> sys_clk;
    sc_in<bool> rst_n;

    Connections::In<Struct_A> data_i;
    Connections::Out<Struct_A> data_o;

    SC_HAS_PROCESS(PassThrough);
    PassThrough(sc_module_name name_ = "PassThrough") : sc_module(name_)
    {
        SC_THREAD(main_thread);
        sensitive << sys_clk.pos();
        async_reset_signal_is(rst_n, false);
    }

    void main_thread() {
        data_i.Reset();
        data_o.Reset();
        wait();

        while (1) {
            Struct_A input;
            if (data_i.PopNB(input)) {
                data_o.Push(input);
            }
            wait();
        }
    }
};
