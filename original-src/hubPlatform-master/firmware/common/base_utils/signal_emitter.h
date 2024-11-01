#pragma once

class SignalEmitter final {
public:
    SignalEmitter( SignalEmitter&& other ) = delete;
    SignalEmitter& operator=( SignalEmitter&& other ) = delete;
    SignalEmitter( const SignalEmitter& other ) = delete;
    SignalEmitter& operator=( const SignalEmitter& other ) = delete;

private:
    SignalEmitter();
    ~SignalEmitter();

public:
    static void emitUserSignal();
    static void emitIntSignal();
    static void emitTermSignal();
};
