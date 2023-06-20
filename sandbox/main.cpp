#define DEBUG

#include <iostream>
#include "../devices/SimplePC.h"
#include "../SimpleDebug.h"
#include "../SimpleConnection.h"

using namespace Simple;

struct TestConnection : public MultiConnection {
    IOBuffer store_buffer;
    vector<TestConnection *> receivers;

    TestConnection(int id) : MultiConnection(id, 3, 50){}

    void SendRxPacket(PacketInfo& p) override {
        if(GetClockNow() % 2 == 0)  //Introduce Noise
            MultiConnection::SendRxPacket(p);
    }

    void onPacketReceived(PacketInfo& _, IOBuffer &io) final {
        auto& info = reinterpret_cast<MultiPacketInfo&>(_);
        print("[%i]: C[%i]->C[%i]: Packet[%i]", ID, info.From, info.To, info.Type);
        switch (info.Type) {
            case 1:
                assert(2.5f == io.ReadStd<float>(), "Packet Type 1 Failed!");
                break;
            case 2:
                assert(8L == io.ReadStd<long>(), "Packet Type 2 Failed!");
                break;
            case 3:
                print("%s", io.Interpret<char>());
                break;
        }
        println(" Processed");
    }

    void onPacketCorrupted(PacketInfo& info) final { println("Connection [%i]: Corrupted", ID); }

    void WriteToSocket(IOBuffer& io, int nbytes) final {
        auto s = io.Position();
        for (auto a: receivers){
            io.Seek(s);
            //a->store_buffer.WriteByte(nbytes);   //Introduce Noise
            a->store_buffer.ReadFrom(io, nbytes);
          //  a->store_buffer.WriteByte(nbytes);   //Introduce Noise
        }
    }

    void ReadFromSocket() final {
        uint8_t buffer[50];
        store_buffer.SeekStart();
        while(store_buffer.BytesAvailable() > 0)
            ReceiveBytes(buffer, store_buffer.ReadBytesUnlocked(buffer, 50));
        store_buffer.ClearToPosition();
    }
};

TestConnection *c0 = new TestConnection(0);
TestConnection *c1 = new TestConnection(1);
TestConnection *c2 = new TestConnection(2);

void test_connection() {
    c0->receivers = {c1, c2};
    c1->receivers = {c0, c2};
    c2->receivers = {c0, c1};
    c0->Start();
    c1->Start();
    c2->Start();

    c0->Send(1, 1, 2.5f);
    c1->Send(0, 2, 8L);
    define_local_lambda(print_c0, [], void, (IOBuffer & io), io.PrintfEnd("\tFrom C[0]\t", 123));
    c0->SendData(2, 3, print_c0);

    c0->Send(1, 1, 2.5f);
    define_local_lambda(internal_timer_lam, [], void, (IOBuffer& io), io.PrintfEnd("\tHello From Timer!\t"));

    define_local_lambda(timer_lam, [&], void, (Timer & t), {
        c0->Send(2, 1, 2.5f);
        c1->Send(0, 2, 8L);
        c0->SendData(2, 3, internal_timer_lam);
        for(auto c : {c0, c1, c2}){
            println("C[%i]: Rx:%i Tx:%i", c->ID, c->ReadBufferSize(), c->WriteBufferSize());
        }
    });

    auto t = new Timer(true, 3000, timer_lam);
    t->Start();

    while (Yield());
}

void create_timer(int &var) {
    define_global_lambda(lam, [&], void, (Timer& t), {
        println("Timer Fire Value: %i", var);
        var += 1;
        if (var == 10) {
            println("Stopping Timer");
            t.Stop();
            delete &t;
            println("End Timer!");
        }
    });
    auto t = new Timer(true, 1000, lam);
    t->Start();
}

void test_async() {
    async([], println("My Async Task!"));
    println("Post Async Init, Pre Async Print!");
}

void test_io() {
    int iiV = 123;
    long iV = 1234578910;
    double dV = .345345;
    float fV = .342344;
    auto tup = make_tuple(iiV, iiV, fV);
    auto v = vector<int>{2, 3, 4};
    auto arr = array<int, 3>{5, 6, 7};

    println("Enter A Number From 1-5");

    char c;
    Out.Read(&c);
    println("Read Value: %c", c);

    IOBuffer io, rio;
    io.WriteStd(iV);
    io.WriteStd(dV);
    io.WriteStd(fV);
    io.WriteStd(tup);
    io.WriteStd(v);
    io.WriteStd(arr);
    io.WriteStd(2.5f, 3.5, 5L);
    io.WriteStd(fV, dV, iV);
    io.Printf("Test %i\n\r", 2);

    io.SeekStart();
    assert(io.ReadStd<long>() == iV, "Long Serialization Fail!");
    assert(io.ReadStd<double>() == dV, "Double Serialization Fail!");
    assert(io.ReadStd<float>() == fV, "Float Serialization Fail!");
    assert((io.ReadStd<tuple<int, int, float>>() == tup), "Tuple Serialization Fail!");
    assert((io.ReadStd<vector<int>>() == v), "Vector Serialization Fail!");
    assert((io.ReadStd<array<int, 3>>() == arr), "Array Serialization Fail!");

    define_local_lambda(lam_1, [], void, (float f, double d, long l), {
        println("IO Lambda");
        assert(f == 2.5f, "Float Arg Fail!");
        assert(d == 3.5, "Float Arg Fail!");
        assert(l == 5L, "Long Arg Fail!");
    });
    io.ReadStd(lam_1);

    define_local_lambda(lam_2, [=], void, (float cF, double cD, long cL), {
        println("IO Lambda");
        assert(cF == fV, "Args 0 Serialization Fail!");
        assert(cD == dV, "Args 1 Serialization Fail!");
        assert(cL == iV, "Args 2 Serialization Fail!");
    });
    io.ReadStd(lam_2);

    assert(strcmp(rio.Interpret<char>(io.ReadLine(rio)), "Test 2") == 0, "Print Serialization Fail!");

    println("Finished IO Testing!");
}

int main() {
    int local_var = 7;

    if (!InitializeIO())
        printf("Wrong Endian Type!");

    println("%s", "Initializing Test Suite!");
    test_io();
    create_timer(local_var);
    test_async();
    test_connection();
}
