#include <iostream>
#include "../devices/SimplePC.h"
#include "../SimpleDebug.hpp"
#include "../SimpleConnection.hpp"

using namespace Simple;

struct TestConnection : public TDMAMultiConnection {
    IOBuffer store_buffer;
    vector<TestConnection *> receivers;

    TestConnection(int id) : TDMAMultiConnection(id, 3, 10, 3){
        SetRetryTimeout(30);
    }

    void onPacketReceived(PacketInfo& _, IOBuffer &io) final {
        auto& info = reinterpret_cast<MultiPacketInfo&>(_);
        println("[%i]: C[%i]->C[%i]: Packet[%i]", ID, info.From, info.To, info.Type);
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
    }

    void onPacketCorrupted(PacketInfo& info) final { println("Connection [%i]: Corrupted", ID); }

    SocketReturn WriteToSocket(PacketInfo& pi, IOBuffer& io, int nbytes) final {
        if(CanWrite() && NativeMillis() % 2 == 0){  //Introduce Noise
            auto s = io.Position();
            for (auto a: receivers){
                io.Seek(s);
                a->store_buffer.WriteByte(nbytes);   //Introduce Noise
                a->store_buffer.ReadFrom(io, nbytes);
                a->store_buffer.WriteByte(nbytes);   //Introduce Noise
            }
            return None;
        }else return DontDispose;
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

struct TestStableConnection : public StableConnection{
    TestStableConnection* c;

    TestStableConnection(){}

    SocketReturn WriteToSocket(PacketInfo& pi, IOBuffer& io, int nbytes) final {
        c->ReceiveBytes(io.Interpret(), nbytes);
        return None;
    }

    void onPacketReceived(PacketInfo& _, IOBuffer &io) final{
        switch(_.Type){
            case 4:;
                println("SC: Tx:%i Rx:%i CTx:%i", WriteBufferSize(), ReadBufferSize(), c->WriteBufferSize());
            break;
        }
    }

    void onPacketCorrupted(PacketInfo& _) final {}
    void ReadFromSocket() final {}
};

TestStableConnection* c3 = new TestStableConnection();
TestStableConnection* c4 = new TestStableConnection();


void test_connection() {
    c0->receivers = {c1, c2};
    c1->receivers = {c0, c2};
    c2->receivers = {c0, c1};
    c0->Start();
    c1->Start();
    c2->Start();
    c0->setSyncInterval(5000);

    c3->c = c4;
    c4->c = c3;

    c0->Send(1, 1, 2.5f);
    c1->Send(0, 2, 8L);

    auto l = make_static_lambda(void, (IOBuffer & io), io.PrintfEnd("\tFrom C[0]\t\n", 123));
    c0->SendData(2, 3, l);

    c0->Send(1, 1, 2.5f);
    define_local_lambda(internal_timer_lam, [], void, (IOBuffer& io), io.PrintfEnd("\tHello From Timer!\t"));

    define_local_lambda(timer_lam, [&], void, (Timer & t), {
        c0->Send(2, 1, 2.5f);
        c1->Send(0, 2, 8L);
        c0->SendData(2, 3, internal_timer_lam);
        c3->Send(4);
        for(auto c : {c0, c1, c2}){
            println("C[%i]: Rx:%i Tx:%i", c->ID, c->ReadBufferSize(), c->WriteBufferSize());
        }
    });

    auto t = new Timer(true, 200, timer_lam);
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

void test_timer(){
    Time<uint8_t> t;

    auto td = t.createDecay(255);

    for(int i = 0; i < 5; i++){
        assert(!t.hasDecayed(td), "Time Decayed To Early!");
        println("Waiting For Timer Decay!");
        auto b = Clock.Millis();
        while(!t.hasDecayed(td)){}
        println("Timer Decayed In:%i", Clock.Millis() - b);
        td = t.createDecay(i % 2 == 0 ? 100 : 255);
    }
}

int main() {
    int local_var = 7;

    if (!InitializeIO())
        printf("Wrong Endian Type!");

    println("%s", "Initializing Test Suite!");
    test_timer();
    test_io();
    create_timer(local_var);
    test_async();
    test_connection();
}
