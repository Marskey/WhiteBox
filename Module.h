#pragma once
class IModule
{
public:
    IModule() {}
    virtual ~IModule() {}

    virtual void onDataReady() {}
};
