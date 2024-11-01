#pragma once

class Domain {
public:
    virtual ~Domain() = default;
    virtual float normalize( int x ) = 0;

protected:
    virtual float normalize( int x, int max );
};